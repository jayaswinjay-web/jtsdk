const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

// ---- DAP framed protocol helpers (for VS Code <-> this adapter) ----
function writeDAP(msg) {
    const json = JSON.stringify(msg);
    const header = `Content-Length: ${Buffer.byteLength(json)}\r\n\r\n`;
    process.stdout.write(header + json);
}

function sendResponse(request, success, body, command, message) {
    const resp = {
        seq: 0,
        type: 'response',
        request_seq: request.seq,
        success: success,
        command: command || request.command,
    };
    if (message) resp.message = message;
    if (body) resp.body = body;
    writeDAP(resp);
}

function sendEvent(event, body) {
    const ev = {
        seq: 0,
        type: 'event',
        event: event,
    };
    if (body) ev.body = body;
    writeDAP(ev);
}

// ---- Read DAP messages from VS Code (framed protocol) ----
let dapBuffer = Buffer.alloc(0);
let dapInitializing = false;

function parseDAPMessages() {
    while (true) {
        const headerEnd = dapBuffer.indexOf('\r\n\r\n');
        if (headerEnd === -1) return;

        const header = dapBuffer.slice(0, headerEnd).toString('utf8');
        const match = /Content-Length:\s*(\d+)/i.exec(header);
        if (!match) {
            // malformed; drop the header
            dapBuffer = dapBuffer.slice(headerEnd + 4);
            continue;
        }
        const len = parseInt(match[1], 10);
        const bodyStart = headerEnd + 4;
        if (dapBuffer.length < bodyStart + len) return;

        const json = dapBuffer.slice(bodyStart, bodyStart + len).toString('utf8');
        dapBuffer = dapBuffer.slice(bodyStart + len);

        let request;
        try {
            request = JSON.parse(json);
        } catch (e) {
            continue;
        }
        handleRequest(request);
    }
}

process.stdin.on('data', (chunk) => {
    dapBuffer = Buffer.concat([dapBuffer, chunk]);
    parseDAPMessages();
});

process.stdin.on('end', () => {
    // Don't force-exit while a VM is still running; wait for it to terminate.
    if (!vmProcess) process.exit(0);
});

// ---- Debug session state ----
let vmProcess = null;
let vmStdoutBuf = '';
let vmStderrBuf = '';
let pendingSourcePath = null;
let pendingVariablesReference = 0; // next ref to assign
let variablesReferenceToFrame = new Map(); // ref -> frame index
let globalsReference = 0;
let nextVariablesReference = 1000; // refs for non-frame scopes
let pendingBreakpointsForFile = new Map(); // path -> { lines, source, request }

// ---- Send a JSON command to VM (newline-delimited) ----
function sendVMCommand(cmdObj) {
    if (vmProcess && vmProcess.stdin.writable) {
        vmProcess.stdin.write(JSON.stringify(cmdObj) + '\n');
    }
}

// ---- Handle VM events (stderr) and responses (stdout) ----
function handleVMStderrData(data) {
    vmStderrBuf += data.toString();
    const lines = vmStderrBuf.split('\n');
    vmStderrBuf = lines.pop();
    for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        try {
            const evt = JSON.parse(trimmed);
            handleVMEvent(evt);
        } catch (e) {
            // not JSON; emit as output
            sendEvent('output', { category: 'stderr', output: line + '\n' });
        }
    }
}

function handleVMStdoutData(data) {
    vmStdoutBuf += data.toString();
    const lines = vmStdoutBuf.split('\n');
    vmStdoutBuf = lines.pop();
    for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        let evt;
        try {
            evt = JSON.parse(trimmed);
        } catch (e) {
            /* Not a protocol message; this is program output (print, etc.) */
            sendEvent('output', { category: 'stdout', output: line + '\n' });
            continue;
        }
        handleVMResponse(evt);
    }
}

function handleVMEvent(evt) {
    if (evt.event === 'started') {
        // program loaded; if we have pending breakpoints, they were already sent in launch
        sendEvent('initialized');
    } else if (evt.event === 'stopped') {
        sendEvent('stopped', {
            reason: evt.reason || 'breakpoint',
            threadId: 1,
            preserveFocusHint: false,
            allThreadsStopped: true,
        });
    } else if (evt.event === 'error') {
        sendEvent('output', { category: 'stderr', output: (evt.message || 'error') + '\n' });
    }
}

// ---- Map a VM response to a pending DAP request ----
let pendingScopesRequests = new Map(); // key -> request  (key = 'stack_<reqseq>')
let pendingVariablesRequests = new Map(); // key -> request (key = 'vars_<reqseq>_<ref>')
let pendingStackFrames = null; // last computed stack frame list

function handleVMResponse(resp) {
    if (resp.event === 'scopes') {
        // Convert to stack trace response for any pending stackTrace request
        const req = pendingStackTraceRequest;
        pendingStackTraceRequest = null;
        if (req) {
            const frames = (resp.frames || []).map((f, i) => ({
                id: i,
                name: f.name || '<script>',
                line: f.line || 0,
                column: 1,
                source: { name: path.basename(pendingSourcePath || 'script.jts'), path: pendingSourcePath || 'script.jts', sourceReference: 0 },
            }));
            sendResponse(req, true, { stackFrames: frames, totalFrames: frames.length }, 'stackTrace');
        }
    } else if (resp.event === 'variables' || resp.event === 'globals') {
        const ref = resp.event === 'globals' ? globalsReference : currentVariablesRef;
        const req = pendingVariablesRequests.get(ref);
        pendingVariablesRequests.delete(ref);
        if (req) {
            const vars = (resp.variables || []).map(v => ({
                name: v.name,
                value: v.value || '',
                type: typeof v.value,
                variablesReference: 0,
                presentationHint: {},
            }));
            sendResponse(req, true, { variables: vars }, 'variables');
        }
    }
}

// ---- DAP request handlers ----
let pendingStackTraceRequest = null;
let currentVariablesRef = 0;

function handleRequest(request) {
    const args = request.arguments || {};
    switch (request.command) {
        case 'initialize':
            sendResponse(request, true, {
                supportsConfigurationDoneRequest: true,
                supportsEvaluateForHovers: false,
                supportsStepInTargetsRequest: false,
                supportsSetVariable: false,
                supportsTerminateRequest: true,
                supportsRestartRequest: false,
                supportsStepBack: false,
                supportsCancelRequest: false,
                exceptionBreakpointFilters: [],
                supportedChecksumAlgorithms: [],
            }, 'initialize');
            break;

        case 'launch':
            handleLaunch(request, args);
            break;

        case 'setBreakpoints':
            handleSetBreakpoints(request, args);
            break;

        case 'configurationDone':
            sendResponse(request, true, {}, 'configurationDone');
            // tell VM to continue
            sendVMCommand({ command: 'continue' });
            break;

        case 'threads':
            sendResponse(request, true, { threads: [{ id: 1, name: 'Main Thread' }] }, 'threads');
            break;

        case 'stackTrace':
            pendingStackTraceRequest = request;
            sendVMCommand({ command: 'scopes' });
            break;

        case 'scopes':
            handleScopes(request, args);
            break;

        case 'variables':
            handleVariables(request, args);
            break;

        case 'continue':
            sendVMCommand({ command: 'continue' });
            sendResponse(request, true, { allThreadsContinued: true }, 'continue');
            break;

        case 'next':
            sendVMCommand({ command: 'stepOver' });
            sendResponse(request, true, {}, 'next');
            break;

        case 'stepIn':
            sendVMCommand({ command: 'stepIn' });
            sendResponse(request, true, {}, 'stepIn');
            break;

        case 'stepOut':
            sendVMCommand({ command: 'stepOut' });
            sendResponse(request, true, {}, 'stepOut');
            break;

        case 'pause':
            sendVMCommand({ command: 'stop' });
            sendResponse(request, true, {}, 'pause');
            break;

        case 'evaluate':
            handleEvaluate(request, args);
            break;

        case 'disconnect':
            sendVMCommand({ command: 'stop' });
            if (vmProcess) { try { vmProcess.kill(); } catch (e) {} }
            sendResponse(request, true, {}, 'disconnect');
            setTimeout(() => process.exit(0), 100);
            break;

        default:
            sendResponse(request, false, null, request.command, `Unknown command: ${request.command}`);
    }
}

function handleLaunch(request, args) {
    const program = args.program;
    if (!program) {
        sendResponse(request, false, null, 'launch', 'No program specified');
        return;
    }
    pendingSourcePath = program;
    const jtsPath = args.jtsPath || 'jts';
    const cwd = args.cwd || path.dirname(program);

    try {
        vmProcess = spawn(jtsPath, ['--debug', program], {
            cwd,
            stdio: ['pipe', 'pipe', 'pipe'],
            env: process.env,
        });
    } catch (e) {
        sendResponse(request, false, null, 'launch', `Failed to launch: ${e.message}`);
        return;
    }

    vmProcess.stdout.on('data', handleVMStdoutData);
    vmProcess.stderr.on('data', handleVMStderrData);
    vmProcess.on('exit', (code) => {
        sendEvent('terminated', { restart: false });
    });
    vmProcess.on('error', (err) => {
        sendEvent('output', { category: 'stderr', output: `JTS VM error: ${err.message}\n` });
        sendEvent('terminated', { restart: false });
    });

    // Respond success to launch; 'initialized' event is sent when VM emits 'started'
    sendResponse(request, true, {}, 'launch');
}

function handleSetBreakpoints(request, args) {
    const sourcePath = args.source && args.source.path;
    const bps = args.breakpoints || [];
    const lines = bps.map(b => b.line);

    if (sourcePath) {
        // We only support breakpoints for the active program file
        // If VM isn't ready yet, store and send later
        sendVMCommand({ command: 'setBreakpoints', lines: lines });
    }

    const verified = bps.map((b, i) => ({
        id: i + 1,
        verified: true,
        line: b.line,
        source: args.source,
    }));

    sendResponse(request, true, { breakpoints: verified }, 'setBreakpoints');
}

function handleEvaluate(request, args) {
    const expression = args.expression || '';
    const context = args.context || '';
    const frameId = args.frameId;

    if (context === 'repl') {
        /* Debug Console input while the program is running (e.g. for input()).
           Forward it to the VM's stdin as program input. */
        if (vmProcess && vmProcess.stdin.writable) {
            vmProcess.stdin.write(expression + '\n');
        }
        sendResponse(request, true, { result: expression, variablesReference: 0 }, 'evaluate');
        return;
    }

    if (vmProcess && vmProcess.stdin.writable) {
        sendVMCommand({ command: 'scopes' });
    }
    sendResponse(request, true, { result: expression, variablesReference: 0 }, 'evaluate');
}

function handleScopes(request, args) {
    const frameId = args.frameId || 0;
    // Create two scopes: Locals (frame idx = frameId) and Globals
    const localsRef = nextVariablesReference++;
    variablesReferenceToFrame.set(localsRef, frameId);
    if (!globalsReference) globalsReference = nextVariablesReference++;

    const scopes = [
        { name: 'Locals', variablesReference: localsRef, expensive: false, presentationHint: 'locals' },
        { name: 'Globals', variablesReference: globalsReference, expensive: false, presentationHint: 'globals' },
    ];
    sendResponse(request, true, { scopes }, 'scopes');
}

function handleVariables(request, args) {
    const ref = args.variablesReference;
    if (ref === globalsReference) {
        pendingVariablesRequests.set(ref, request);
        sendVMCommand({ command: 'globals' });
    } else {
        const frameIdx = variablesReferenceToFrame.get(ref);
        pendingVariablesRequests.set(ref, request);
        currentVariablesRef = ref;
        sendVMCommand({ command: 'variables', frame: frameIdx || 0 });
    }
}