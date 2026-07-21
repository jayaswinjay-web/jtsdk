const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

class JTSDebugSession {
    constructor() {
        this.runtime = null;
        this_vmProcess = null;
        this._pendingResponses = new Map();
        this._seqCounter = 1;
        this._breakpoints = new Map();
        this._breakpointId = 1;
        this._outputBuffer = '';
        this._stderrBuffer = '';
    }

    sendResponse(response) {
        const msg = JSON.stringify(response) + '\n';
        process.stdout.write(msg);
    }

    sendEvent(event) {
        const msg = JSON.stringify(event) + '\n';
        process.stdout.write(msg);
    }

    sendError(response, message) {
        response.success = false;
        response.message = message;
        this.sendResponse(response);
    }

    handleRequest(request) {
        const { command, arguments: args } = request;

        switch (command) {
            case 'initialize':
                this.handleInitialize(request);
                break;
            case 'launch':
                this.handleLaunch(request, args);
                break;
            case 'setBreakpoints':
                this.handleSetBreakpoints(request, args);
                break;
            case 'threads':
                this.handleThreads(request);
                break;
            case 'stackTrace':
                this.handleStackTrace(request, args);
                break;
            case 'scopes':
                this.handleScopes(request, args);
                break;
            case 'variables':
                this.handleVariables(request, args);
                break;
            case 'continue':
                this.handleContinue(request);
                break;
            case 'next':
                this.handleNext(request);
                break;
            case 'stepIn':
                this.handleStepIn(request);
                break;
            case 'stepOut':
                this.handleStepOut(request);
                break;
            case 'pause':
                this.handlePause(request);
                break;
            case 'configurationDone':
                this.sendResponse({ seq: this._seqCounter++, request_seq: request.seq, success: true, command: 'configurationDone' });
                break;
            case 'disconnect':
                this.handleDisconnect(request);
                break;
            default:
                this.sendError({ seq: this._seqCounter++, request_seq: request.seq, command }, `Unknown command: ${command}`);
                break;
        }
    }

    handleInitialize(request) {
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'initialize',
            body: {
                supportsConfigurationDoneRequest: true,
                supportsStepInTargetsRequest: false,
                supportsStepBack: false,
                supportsRestartRequest: false,
                supportsSetVariable: false,
                supportsEvaluateForHovers: true,
                exceptionBreakpointFilters: [],
                supportedChecksumAlgorithms: [],
            }
        });
    }

    handleLaunch(request, args) {
        const program = args.program;
        if (!program) {
            this.sendError({ seq: this._seqCounter++, request_seq: request.seq, command: 'launch' }, 'No program specified');
            return;
        }

        const jtsPath = args.jtsPath || 'jts';
        const cwd = args.cwd || path.dirname(program);

        this._vmProcess = spawn(jtsPath, ['--debug', program], {
            cwd,
            stdio: ['pipe', 'pipe', 'pipe']
        });

        this._vmProcess.stderr.on('data', (data) => {
            this._stderrBuffer += data.toString();
            const lines = this._stderrBuffer.split('\n');
            this._stderrBuffer = lines.pop();
            for (const line of lines) {
                if (line.trim()) {
                    try {
                        const event = JSON.parse(line.trim());
                        this.handleVMEvent(event);
                    } catch (e) {
                        // ignore non-JSON stderr output
                    }
                }
            }
        });

        this._vmProcess.on('exit', (code) => {
            this.sendEvent({
                seq: this._seqCounter++,
                event: 'terminated',
                body: { restart: false }
            });
        });

        this._vmProcess.on('error', (err) => {
            this.sendEvent({
                seq: this._seqCounter++,
                event: 'output',
                body: { category: 'stderr', output: `JTS VM error: ${err.message}\n` }
            });
        });

        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'launch',
            body: {}
        });

        this.sendEvent({
            seq: this._seqCounter++,
            event: 'initialized',
            body: {}
        });
    }

    handleVMEvent(event) {
        switch (event.event) {
            case 'started':
                break;
            case 'stopped':
                this.sendEvent({
                    seq: this._seqCounter++,
                    event: 'stopped',
                    body: {
                        reason: event.reason || 'step',
                        threadId: 1,
                        line: event.line || 0,
                        source: this._currentSource()
                    }
                });
                break;
            case 'error':
                this.sendEvent({
                    seq: this._seqCounter++,
                    event: 'output',
                    body: { category: 'stderr', output: event.message + '\n' }
                });
                break;
        }
    }

    _currentSource() {
        return {
            name: 'script.jts',
            path: this._lastProgram || 'script.jts',
            sourceReference: 0
        };
    }

    handleSetBreakpoints(request, args) {
        const source = args.source;
        const breakpoints = args.breakpoints || [];
        const lines = breakpoints.map(bp => bp.line);

        const cmd = JSON.stringify({ command: 'setBreakpoints', lines }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }

        const verified = breakpoints.map(bp => ({
            id: this._breakpointId++,
            verified: true,
            line: bp.line,
            source: source
        }));

        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'setBreakpoints',
            body: { breakpoints: verified }
        });
    }

    handleThreads(request) {
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'threads',
            body: {
                threads: [{
                    id: 1,
                    name: 'Main Thread'
                }]
            }
        });
    }

    handleStackTrace(request, args) {
        const cmd = JSON.stringify({ command: 'scopes' }) + '\n';
        this._pendingResponses.set('scopes_' + request.seq, request);
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);

            const onData = (data) => {
                this._stderrBuffer += data.toString();
                const lines = this._stderrBuffer.split('\n');
                this._stderrBuffer = lines.pop();
                for (const line of lines) {
                    if (line.trim()) {
                        try {
                            const resp = JSON.parse(line.trim());
                            if (resp.event === 'scopes') {
                                this._vmProcess.stdout.removeListener('data', onData);
                                const frames = (resp.frames || []).map((f, i) => ({
                                    id: i,
                                    name: f.name,
                                    line: f.line || 0,
                                    column: 0,
                                    source: this._currentSource()
                                }));
                                this.sendResponse({
                                    seq: this._seqCounter++,
                                    request_seq: request.seq,
                                    success: true,
                                    command: 'stackTrace',
                                    body: { stackFrames: frames, totalFrames: frames.length }
                                });
                                return;
                            }
                        } catch (e) {}
                    }
                }
            };
            this._vmProcess.stdout.on('data', onData);
            return;
        }

        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'stackTrace',
            body: { stackFrames: [], totalFrames: 0 }
        });
    }

    handleScopes(request, args) {
        const frameId = args.frameId || 0;
        const scopes = [
            { name: 'Locals', variablesReference: frameId + 1, expensive: false },
            { name: 'Globals', variablesReference: 9999, expensive: false }
        ];
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'scopes',
            body: { scopes }
        });
    }

    handleVariables(request, args) {
        const variablesReference = args.variablesReference;
        const isGlobal = variablesReference === 9999;
        const frameIdx = isGlobal ? -1 : variablesReference - 1;
        const cmd = isGlobal
            ? JSON.stringify({ command: 'globals' }) + '\n'
            : JSON.stringify({ command: 'variables', frame: frameIdx }) + '\n';

        const onData = (data) => {
            this._stderrBuffer += data.toString();
            const lines = this._stderrBuffer.split('\n');
            this._stderrBuffer = lines.pop();
            for (const line of lines) {
                if (line.trim()) {
                    try {
                        const resp = JSON.parse(line.trim());
                        if (resp.event === 'variables' || resp.event === 'globals') {
                            this._vmProcess.stdout.removeListener('data', onData);
                            const vars = (resp.variables || []).map(v => ({
                                name: v.name,
                                value: v.value,
                                type: 'string',
                                variablesReference: 0
                            }));
                            this.sendResponse({
                                seq: this._seqCounter++,
                                request_seq: request.seq,
                                success: true,
                                command: 'variables',
                                body: { variables: vars }
                            });
                            return;
                        }
                    } catch (e) {}
                }
            }
        };

        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
            this._vmProcess.stdout.on('data', onData);
        } else {
            this.sendResponse({
                seq: this._seqCounter++,
                request_seq: request.seq,
                success: true,
                command: 'variables',
                body: { variables: [] }
            });
        }
    }

    handleContinue(request) {
        const cmd = JSON.stringify({ command: 'continue' }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'continue',
            body: { allThreadsContinued: true }
        });
    }

    handleNext(request) {
        const cmd = JSON.stringify({ command: 'stepOver' }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'next',
            body: {}
        });
    }

    handleStepIn(request) {
        const cmd = JSON.stringify({ command: 'stepIn' }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'stepIn',
            body: {}
        });
    }

    handleStepOut(request) {
        const cmd = JSON.stringify({ command: 'stepOut' }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'stepOut',
            body: {}
        });
    }

    handlePause(request) {
        const cmd = JSON.stringify({ command: 'stop' }) + '\n';
        if (this._vmProcess && this._vmProcess.stdin.writable) {
            this._vmProcess.stdin.write(cmd);
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'pause',
            body: {}
        });
    }

    handleDisconnect(request) {
        if (this._vmProcess) {
            const cmd = JSON.stringify({ command: 'stop' }) + '\n';
            if (this._vmProcess.stdin.writable) {
                this._vmProcess.stdin.write(cmd);
            }
            this._vmProcess.kill();
            this._vmProcess = null;
        }
        this.sendResponse({
            seq: this._seqCounter++,
            request_seq: request.seq,
            success: true,
            command: 'disconnect',
            body: {}
        });
    }
}

const session = new JTSDebugSession();
let inputBuffer = '';

process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => {
    inputBuffer += chunk;
    const lines = inputBuffer.split('\n');
    inputBuffer = lines.pop();
    for (const line of lines) {
        if (line.trim()) {
            try {
                const request = JSON.parse(line.trim());
                session.handleRequest(request);
            } catch (e) {
                // ignore parse errors
            }
        }
    }
});

process.stdin.on('end', () => {
    process.exit(0);
});
