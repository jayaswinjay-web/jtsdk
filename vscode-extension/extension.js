const vscode = require('vscode');
const path = require('path');
const { spawn } = require('child_process');

class JTSDebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(session, executable) {
        const adapterPath = path.join(__dirname, 'debugAdapter.js');
        return new vscode.DebugAdapterExecutable('node', [adapterPath], {});
    }
}

function activate(context) {
    console.log('JTS GO extension is now active');

    // Register debug adapter factory
    const debugFactory = new JTSDebugAdapterDescriptorFactory();
    const debugDisp = vscode.debug.registerDebugAdapterDescriptorFactory('jts', debugFactory);
    context.subscriptions.push(debugDisp);

    // Register debug configuration provider
    const debugConfigDisp = vscode.debug.registerDebugConfigurationProvider('jts', {
        provideDebugConfigurations(folder, token) {
            return [{
                type: 'jts',
                request: 'launch',
                name: 'Debug JTS GO',
                program: '${workspaceFolder}/${fileBasename}',
                cwd: '${workspaceFolder}',
                jtsPath: 'jts'
            }];
        },
        resolveDebugConfiguration(folder, config, token) {
            if (!config.type) {
                config.type = 'jts';
            }
            if (!config.request) {
                config.request = 'launch';
            }

            let program = config.program || '';
            const editor = vscode.window.activeTextEditor;

            // ${file} may not resolve if no file is open; fall back to the active editor.
            if (!program ||
                program === '${file}' ||
                program === '${workspaceFolder}/${fileBasename}' ||
                program === '/' ||
                program === '.') {
                if (editor) {
                    program = editor.document.fileName;
                } else if (folder) {
                    program = path.join(folder.uri.fsPath, 'main.jts');
                }
            }

            // Resolve workspace folder prefix if still present.
            if (folder && program.indexOf('${workspaceFolder}') === 0) {
                program = path.join(folder.uri.fsPath, program.substring('${workspaceFolder}'.length));
            }

            config.program = program;

            if (!config.cwd) {
                config.cwd = folder ? folder.uri.fsPath
                                   : (program ? path.dirname(program) : process.cwd());
            }
            if (config.cwd === '${workspaceFolder}' && folder) {
                config.cwd = folder.uri.fsPath;
            }

            const fs = require('fs');
            if (!config.program || !fs.existsSync(config.program)) {
                vscode.window.showErrorMessage(
                    'JTS GO: Program file not found. Open a .jts file and press F5 again.');
                return undefined;
            }
            return config;
        }
    });
    context.subscriptions.push(debugConfigDisp);

    // Register completion provider
    const completionProvider = vscode.languages.registerCompletionItemProvider('jts', {
        provideCompletionItems(document, position) {
            const items = [];

            const keywords = [
                'if', 'elif', 'else', 'while', 'for', 'in', 'to', 'break', 'continue',
                'return', 'func', 'class', 'extends', 'new', 'self', 'super',
                'try', 'catch', 'finally', 'throw', 'import', 'end', 'and', 'or', 'not',
                'true', 'false', 'nil', 'int', 'float', 'string', 'bool', 'list', 'var'
            ];

            keywords.forEach(keyword => {
                const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
                items.push(item);
            });

            const builtins = [
                'print', 'input', 'len', 'type', 'str', 'number', 'math', 'sqrt',
                'append', 'tensor', 'matrix', 'matmul', 'sigmoid', 'relu', 'mse',
                'http_server', 'http_start', 'http_request', 'read_file', 'write_file'
            ];

            builtins.forEach(builtin => {
                const item = new vscode.CompletionItem(builtin, vscode.CompletionItemKind.Function);
                item.detail = 'Built-in function';
                items.push(item);
            });

            const stringMethods = [
                'upper', 'lower', 'trim', 'split', 'contains', 'replace',
                'substring', 'starts_with', 'ends_with'
            ];

            stringMethods.forEach(method => {
                const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                item.detail = 'String method';
                items.push(item);
            });

            const listMethods = ['sort', 'remove', 'pop', 'append'];

            listMethods.forEach(method => {
                const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                item.detail = 'List method';
                items.push(item);
            });

            return items;
        }
    });

    context.subscriptions.push(completionProvider);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};