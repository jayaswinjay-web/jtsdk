<div align="center">

<img src="../assets/logo-512.png" alt="JTS GO Logo" width="120"/>

# JTS GO — VS Code Extension

**Syntax highlighting · snippets · & debugging for the JTS GO language in VS Code.**

[![Version](https://img.shields.io/badge/version-2.0.7-6554E0?logo=visual-studio-code&logoColor=white)](https://github.com/jayaswinjay-web/jtsdk) [![VS Code Engine](https://img.shields.io/badge/VS%20Code-%3E%3D1.74.0-007ACC?logo=visual-studio-code&logoColor=white)](https://code.visualstudio.com/) [![License: MIT](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

</div>

---

## ✨ Features

| Feature | Status |
|---------|--------|
| 🎨 **Syntax Highlighting** | Keywords, operators, built-ins, strings, numbers, comments |
| 🧩 **Code Snippets** | `if`, `while`, `for`, `func`, `class`, `trycatch`, `httpserver` & more |
| 🔗 **Bracket Matching & Auto-Closing** | `end` blocks, `()`, `[]`, `{}` |
| 💬 **Comment Toggling** | `#` single-line comments via `Ctrl+/` |
| 📦 **Code Folding** | Indentation-based block folding (`if`, `func`, `class`, `for`, `while`) |
| 🐛 **Debugger Support** | Launch & debug JTS GO programs with breakpoints via the Debug Adapter Protocol |

---

## 🚀 Installation

### Option 1: From VSIX (offline)

1. Download the latest `.vsix` from [`/vscode-extension`](../vscode-extension) (e.g. `jts-go-2.0.7.vsix`)
2. Open VS Code → `Ctrl+Shift+P` → **Extensions: Install from VSIX...**
3. Select the downloaded file → reload window

### Option 2: From Source

```bash
git clone https://github.com/jayaswinjay-web/jtsdk.git
cd jtsdk/vscode-extension
npm install
# Press F5 in VS Code to launch Extension Development Host
```

### Prerequisite

Make sure the JTS GO toolchain is installed and on your `PATH`:

```bash
npm install -g @jaytechsolutions/jts-go
```

Verify:
```bash
jts --version
```

---

## 🎨 Syntax Highlighting

Full grammar covering:

- **Keywords** — `func`, `class`, `extends`, `if`, `elif`, `else`, `for`, `while`, `return`, `break`, `continue`, `try`, `catch`, `throw`, `end`, `bring`, `new`
- **Built-ins** — `say`, `ask`, `len`, `type`, `str`, `number`, `tensor`, `matrix`, `matmul`, `sigmoid`, `relu`, `mse`, `http_server`, `http_route`, `http_start`, `read_file`, `write_file`
- **Operators** — `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<=`, `>=`, `and`, `or`, `not`, `|`, `&`, `^`, `<<`, `>>`
- **Types** — `int`, `string`, `float`, `bool`, `list`, `dict`, `set`, `var`, `void`
- **Literals** — Strings, numbers (int/float), `\b` booleans, `void`

---

## 🧩 Snippets

| Prefix | Expands To |
|--------|------------|
| `if` | `if ... end` |
| `ifelse` | `if ... elif ... else ... end` |
| `while` | `while ... end` |
| `for` | `for i of 0 to n ... end` |
| `func` | `func name(args) ... end` |
| `class` | `class Name ... end` |
| `classextends` | `class Name extends Base ... end` |
| `trycatch` | `try ... catch e ... end` |
| `tryfinally` | `try ... catch e ... finally ... end` |
| `comprehension` | List comprehension expression |
| `print` | `say(...)` |
| `input` | `ask("...")` |
| `import` | `bring "scroll"` |
| `list` | `[1, 2, 3]` |
| `dict` | `{"key": "value"}` |
| `httpserver` | Full HTTP server template |

---

## 🐛 Debugging

1. Open a `.jts` file in VS Code
2. Go to the Run & Debug panel (`Ctrl+Shift+D`)
3. Click **create a launch.json file** → select **JTS GO**
4. Set breakpoints by clicking in the gutter
5. Press **F5** to launch

The extension generates this `launch.json` automatically:
```json
{
  "type": "jts",
  "request": "launch",
  "name": "Debug JTS GO",
  "program": "${workspaceFolder}/${fileBasename}",
  "cwd": "${workspaceFolder}",
  "jtsPath": "jts"
}
```

> If `jts` is not on your `PATH`, set `"jtsPath"` to the full path of `jts.exe`.

---

## 📸 Showcase

```jts
# Syntax highlighting in action
func greet(name)
    if name != ""
        say("Hello, " + name + "!")
    else
        say("Hello, stranger!")
    end
end

greet("JTS GO")
```

---

## 📋 Configuration

The extension activates automatically for `.jts` files. No settings required.

To override the JTS executable path in `launch.json`:
```json
{
  "jtsPath": "C:/path/to/jts.exe"
}
```

---

## 🤝 Contributing

Found a bug or want a feature? Open an issue at [jtsdk/issues](https://github.com/jayaswinjay-web/jtsdk/issues).

To develop locally:
```bash
cd vscode-extension
npm install
# Open in VS Code → F5 → Extension Development Host
```

---

## 📄 License

MIT — see [LICENSE](LICENSE).

---

<div align="center">

**JTS GO Extension v2.0.7** — built by [JayTech Solutions](https://github.com/jayaswinjay-web)

</div>