# JTS GO Language Support for Visual Studio Code

Provides syntax highlighting, code completion, snippets, and language support for JTS GO programming language.

## Features

- Syntax highlighting for all JTS GO keywords, operators, and built-in functions
- Code completion for keywords, built-in functions, and methods
- Code snippets for common constructs (if/else, loops, functions, classes, try/catch)
- Bracket matching and auto-closing pairs
- Comment toggling with `#`
- Folding support for blocks

## File Association

Automatically activates for `.jts` files.

## Snippets

- `if` - If statement
- `ifelse` - If-elif-else statement
- `while` - While loop
- `for` - For loop
- `func` - Function definition
- `class` - Class definition
- `classextends` - Class with inheritance
- `trycatch` - Try-catch block
- `print` - Print statement
- `input` - Input statement
- `import` - Import statement
- `list` - List literal
- `dict` - Dictionary literal
- `httpserver` - HTTP server setup

## Installation

### From VSIX

1. Download the `.vsix` file
2. Open VS Code
3. Press `Ctrl+Shift+P` and type "Install from VSIX"
4. Select the downloaded file

### From Source

1. Clone this repository
2. Run `npm install` in this directory
3. Press `F5` to launch Extension Development Host

## License

MIT

## Repository

https://github.com/jayaswinjay-web/jtsdk