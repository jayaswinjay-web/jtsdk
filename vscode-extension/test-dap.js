const { spawn } = require('child_process');
const path = require('path');

const adapter = spawn('node', [path.join(__dirname, 'debugAdapter.js')], {
    stdio: ['pipe', 'pipe', 'pipe']
});

let buffer = '';
let seq = 1;
let bpsSent = false;
let localsRef = null;
let globalsRef = null;

function sendRequest(command, args) {
    const msg = { seq: seq++, type: 'request', command, arguments: args || {} };
    const json = JSON.stringify(msg);
    adapter.stdin.write(`Content-Length: ${Buffer.byteLength(json)}\r\n\r\n${json}`);
}

adapter.stdout.on('data', (data) => {
    buffer += data.toString();
    let idx;
    while ((idx = buffer.indexOf('\r\n\r\n')) !== -1) {
        const headerEnd = buffer.indexOf('\r\n\r\n') + 4;
        const header = buffer.slice(0, buffer.indexOf('\r\n\r\n'));
        const m = /Content-Length:\s*(\d+)/i.exec(header);
        if (!m) { buffer = buffer.slice(headerEnd); continue; }
        const len = parseInt(m[1], 10);
        if (buffer.length < headerEnd + len) return;
        const json = buffer.slice(headerEnd, headerEnd + len);
        buffer = buffer.slice(headerEnd + len);
        try { handleMessage(JSON.parse(json)); } catch (e) {}
    }
});

adapter.stderr.on('data', (d) => console.log('[adapter stderr]', d.toString()));

function handleMessage(msg) {
    if (msg.type === 'response') {
        console.log('RESP', msg.command, msg.success, msg.message || '');
        if (msg.command === 'scopes' && msg.body) {
            localsRef = msg.body.scopes[0].variablesReference;
            globalsRef = msg.body.scopes[1].variablesReference;
            setTimeout(() => sendRequest('variables', { variablesReference: localsRef }), 100);
            setTimeout(() => sendRequest('variables', { variablesReference: globalsRef }), 200);
        }
        if (msg.command === 'variables') {
            console.log('  VARS:', JSON.stringify(msg.body.variables));
        }
        if (msg.command === 'disconnect') {
            setTimeout(() => process.exit(0), 300);
        }
    } else if (msg.type === 'event') {
        console.log('EVENT', msg.event, JSON.stringify(msg.body || {}));
        if (msg.event === 'initialized' && !bpsSent) {
            bpsSent = true;
            sendRequest('setBreakpoints', {
                source: { name: 'debug_func.jts', path: 'D:\\jts programing language\\tests\\debug_func.jts' },
                breakpoints: [{ line: 2 }]
            });
        }
        if (msg.event === 'stopped') {
            sendRequest('stackTrace', { threadId: 1 });
            setTimeout(() => sendRequest('scopes', { frameId: 0 }), 200);
            setTimeout(() => sendRequest('continue', {}), 1500);
        }
        if (msg.event === 'terminated') {
            setTimeout(() => process.exit(0), 100);
        }
    }
}

setTimeout(() => {
    sendRequest('initialize', { clientID: 'test', adapterID: 'jts' });
    sendRequest('launch', {
        program: 'D:\\jts programing language\\tests\\debug_func.jts',
        jtsPath: 'D:\\jts programing language\\jts.exe'
    });
    setTimeout(() => sendRequest('configurationDone', {}), 500);
}, 100);

setTimeout(() => { console.log('TIMEOUT'); process.exit(1); }, 15000);
