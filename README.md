<p align="center">
  <img src="assets/logo-512.png" alt="JTS GO" width="140"/>
</p>

<h1 align="center">JTS GO</h1>

<p align="center">
  <strong>A programming language that gets out of your way.</strong><br>
  One installer. One command. From <code>say("Hello")</code> to ML models and web servers — no <code>pip</code>, no config, no <code>PATH</code> hell.
</p>

<p align="center">
  <a href="https://github.com/jayaswinjay-web/jtsdk/releases"><img alt="Windows Installer" src="https://img.shields.io/badge/Windows-Installer-0078d4?logo=windows&logoColor=white"></a>
  <a href="https://www.npmjs.com/package/@jaytechsolutions/jts-go"><img alt="npm" src="https://img.shields.io/npm/v/@jaytechsolutions/jts-go?logo=npm&logoColor=white"></a>
  <a href="https://jayaswinjay-web.github.io/jtsdk/"><img alt="Try Online" src="https://img.shields.io/badge/Try%20Online-jtsdk.io-2ea44f?logo=github&logoColor=white"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-Apache%202.0-blue"></a>
  <a href="https://github.com/jayaswinjay-web/jtsdk/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/jayaswinjay-web/jtsdk?style=social"></a>
</p>

---

## Why this exists

You want to learn to code. You download Python. Then you need `pip`, `venv`, `PATH`, an editor, extensions, a linter. Two hours later you've written zero code.

**JTS GO flips that.** One `.exe` installs:
- The language (bytecode VM, ~2MB)
- A **dark-themed Windows IDE** with syntax highlighting, REPL, debugger-style output
- 12 built-in tutorials
- Standard library with **ML (tensors, matrices, sigmoid/relu/mse)**, **web server**, **file I/O**, **sets**, **OOP**

You open the IDE, press **F5**, your program runs. That's the whole experience.

---

## 30-second demo

```jts
# hello.jts
say("Hello, World!")
```

```bash
jts hello.jts
# Hello, World!
```

**That's it.** No project init. No `main` function. No import soup.

---

## The IDE (it's good)

<table>
<tr>
<td width="50%">

**Built for beginners, not demos**
- F5 → run, F6 → build bytecode
- Embedded console output
- Interactive REPL tab
- Error list with line numbers (double-click to jump)
- Open-folder workspace, multi-tabs
- Tutorials panel built in

</td>
<td width="50%">

![IDE](assets/logo-cover.png)

</td>
</tr>
</table>

*One installer drops `Jts.Ide.exe` next to `jts.exe`. They know about each other.*

---

## What you can actually build

### Web server in 3 lines
```jts
srv = http_server(8080)
http_route(srv, "GET", "/", "<h1>It works</h1>")
http_start(srv)
```

### ML without `pip install`
```jts
t = tensor([1, 2, 3, 4, 5])
m = matrix([[1, 2], [3, 4]])
say(matmul(m, m))
say(sigmoid(0))   # 0.5
say(relu(-1))     # 0
say(mse([1,2,3], [1.1,2.1,2.9]))
```

### Sets, dictionaries, OOP — all built in
```jts
scores = {90, 85, 88}        # a set
user = {"name": "Ada", "age": 36}

class Agent
    func init(self, name)
        self.name = name
    end
    func greet(self)
        say("Hi, I'm " + self.name)
    end
end

a = new Agent("JTS")
a.greet()
```

---

## Install

### Windows (recommended — includes IDE)
[**Download the installer**](https://github.com/jayaswinjay-web/jtsdk/releases) → run → done.

### Anywhere via npm
```bash
npm install -g @jaytechsolutions/jts-go
jts hello.jts
```

### Update
```bash
jts --update
```

---

## Language at a glance

| Feature | Syntax |
|---------|--------|
| **Print / Input** | `say(x)` · `ask("prompt")` |
| **Variables** | `name = "JTS"` (dynamic) · `int x = 5` (optional types) |
| **Blocks** | Indentation + `end` (no braces, no bracket matching) |
| **Conditionals** | `if / elif / else / end` |
| **Loops** | `for i of 0 to 10 / end` · `while cond / end` |
| **Functions** | `func name(args) / return x / end` |
| **Data** | Lists `[1,2]`, Sets `{1,2}`, Dicts `{"k": "v"}` |
| **OOP** | `class Name / func / end` · `extends` for inheritance |
| **Packages** | `bring "scroll-name"` |

---

## Toolchain

| Command | What it does |
|---------|--------------|
| `jts file.jts` | Compile + run instantly |
| `jtsc file.jts` | Compile to bytecode (`.jbc`) |
| `jtsvm file.jbc` | Run precompiled bytecode |
| `jts --update` | Self-update to latest |

---

## Project structure

```
jtsdk/
├── bin/              # jts.exe, jtsc.exe, jtsvm.exe (Windows)
├── scrolls/          # Standard library packages
├── tutorials/        # 12 beginner tutorials (.jts)
├── book/             # Full HTML documentation
├── assets/           # Logo, icons
├── src/              # C VM, C# IDE, F# core, C++ installer
├── LICENSE           # Apache 2.0
└── README.md         # You are here
```

---

## Docs & Community

- **Learning Guide** → [`LEARNING_GUIDE.md`](LEARNING_GUIDE.md)
- **Language Spec** → [`LANGUAGE_SPEC.md`](LANGUAGE_SPEC.md)
- **Tutorials** → [`book/`](book/) (12 lessons, HTML)
- **Discussions** → [GitHub Discussions](https://github.com/jayaswinjay-web/jtsdk/discussions) *(enable in Settings → Features)*
- **Issues** → [GitHub Issues](https://github.com/jayaswinjay-web/jtsdk/issues)

---

## License

Apache 2.0 — use it, modify it, ship it.  
Copyright (c) 2025–2026 JayTechSolutions.

---

<p align="center">
  <strong>JTS GO v0.9.3</strong> — Made for the person writing their first line of code.
</p>

<p align="center">
  <a href="https://github.com/jayaswinjay-web/jtsdk/releases">Download for Windows</a> •
  <a href="https://jayaswinjay-web.github.io/jtsdk/">Try the site</a> •
  <a href="https://www.npmjs.com/package/@jaytechsolutions/jts-go">npm package</a>
</p>