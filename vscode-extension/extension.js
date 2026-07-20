const vscode = require('vscode');

function activate(context) {
    console.log('JTS GO extension is now active');

    // Register completion provider for JTS
    const completionProvider = vscode.languages.registerCompletionItemProvider('jts', {
        provideCompletionItems(document, position) {
            const items = [];

            // Keywords
            const keywords = [
                'if', 'elif', 'else', 'while', 'for', 'in', 'to', 'break', 'continue',
                'return', 'func', 'class', 'extends', 'new', 'self', 'super',
                'try', 'catch', 'throw', 'import', 'end', 'and', 'or', 'not',
                'true', 'false', 'nil', 'int', 'float', 'string', 'bool', 'list', 'var'
            ];

            keywords.forEach(keyword => {
                const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
                items.push(item);
            });

            // Built-in functions
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

            // String methods
            const stringMethods = [
                'upper', 'lower', 'trim', 'split', 'contains', 'replace',
                'substring', 'starts_with', 'ends_with'
            ];

            stringMethods.forEach(method => {
                const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                item.detail = 'String method';
                items.push(item);
            });

            // List methods
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