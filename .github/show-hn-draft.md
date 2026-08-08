# Show HN Launch Draft

## Post title (pick one)

- `Show HN: JTS GO – a programming language that ships its own IDE`
- `Show HN: I built a programming language where "Hello World" is one install + one command`
- `Show HN: JTS GO – a beginner language with built-in ML and web servers, zero setup`

## Post body

A programming language that's actually easy to *run*, not just easy to read.

JTS GO is designed for absolute beginners. One installer gets you the language AND a full Windows IDE with syntax highlighting, an interactive REPL, built-in tutorials, and F5-to-run. No PATH setup, no pip, no config.

```jts
say("Hello, World!")
```

```bash
jts hello.jts
```

A few things I think are neat:

- **Zero-boilerplate syntax** — indentation-based, blocks close with `end`, reads like English
- **Sets and bitwise ops built in** — `{1,2,3}`, `|` `&` `^` `<<` `>>`
- **ML/AI in the standard library** — tensors, matrices, `sigmoid`, `relu`, `mse`. No `pip install`
- **A 3-line web server**:

```jts
srv = http_server(8080)
http_route(srv, "GET", "/", "<h1>Home</h1>")
http_start(srv)
```

- **Compiled to bytecode** and run on a small VM (`jtsc` → `.jbc` → `jtsvm`)
- **Windows 7+** compatible; the native installer embeds everything in a single `.exe`

The language core is C, the IDE is C#/WPF + AvalonEdit, and there's an F# core shared between them.

I built this because I teach people to code, and the single biggest barrier for total beginners is *setup* — not syntax. Everything here is one download, one command.

Source: https://github.com/jayaswinjay-web/jtsdk
Docs & tutorials: LEARNING_GUIDE.md, 12 built-in tutorials
npm: `npm install -g @jaytechsolutions/jts-go`

Happy to answer questions about the VM, the toolchain, or the design.

## First comment (post it immediately after submitting — this is important)

Author here.

Quick backstory: I'm a solo developer, and I kept watching beginners quit after spending an evening fighting Python/Node installs. I wanted a language where the *first program* is the point of friction, not the setup.

Design goals I'd love feedback on:

1. **Friction budget** — the entire toolchain is `jts`, `jtsc`, `jtsvm`. One binary does everything.
2. **Beginner-first error messages** — the compiler tries to explain *why*, not just *what*.
3. **Everything in one installer** — language, IDE, tutorials, examples, a `jts.bat` shim. Uninstall leaves nothing behind.

What I'm wrestling with:
- **Naming** — "JTS GO" vs the existing "Go" language. I'm aware of the collision and considering a rename.
- **WASM playground** — I'd love to run this in the browser with no install. It's on the roadmap.
- **AI assist** — I'm considering adding an AI completion/chat layer to the IDE as the "2026 hook."

If anyone wants to try it, `npm install -g @jaytechsolutions/jts-go` gets you running in under a minute. The IDE ships with the [releases](https://github.com/jayaswinjay-web/jtsdk/releases).

Thanks for reading.
