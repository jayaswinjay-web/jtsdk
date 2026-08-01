const { spawn } = require('child_process');
const path = require('path');

const adapter = spawn('node', [path.join(__dirname, 'debugAdapter.js')], {
    stdio: ['pipe', 'pipe', 'pipe']
});

let buffer = '';
let seq = 1;
let started = false;

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

let inputSent = false;
function handleMessage(msg) {
    if (msg.type === 'response') {
        console.log('RESP', msg.command, msg.success, msg.message || '');
        if (msg.command === 'disconnect') setTimeout(() => process.exit(0), 300);
    } else if (msg.type === 'event') {
        console.log('EVENT', msg.event, msg.body ? JSON.stringify(msg.body).slice(0, 120) : '');
        if (msg.event === 'initialized' && !started) {
            started = true;
            sendRequest('configurationDone', {});
        }
        if (msg.event === 'terminated') {
            setTimeout(() => process.exit(0), 100);
        }
        if (msg.event === 'output' && !inputSent) {
            inputSent = true;
        }
    }
}

setTimeout(() => {
    sendRequest('initialize', { clientID: 'test', adapterID: 'jts' });
    sendRequest('launch', {
        program: 'D:\\jts programing language\\tests\\debug_input.jts',
        jtsPath: 'D:\\jts programing language\\jts.exe'
    });
}, 100);

setTimeout(() => {
    console.log('>> sending input "World" via evaluate');
    sendRequest('evaluate', { expression: 'World', context: 'repl' });
}, 3000);

setTimeout(() => { console.log('TIMEOUT'); process.exit(1); }, 15000);
