# Reddit Launch Drafts

## r/programming

### Title

I built a programming language for absolute beginners where the toolchain is one install and one command

### Post

For the last year I've been building JTS GO — a language aimed at people who've never written code before.

The core idea: for total beginners, **setup is the biggest barrier, not syntax**. So the entire language ships as a single installer that also includes a full Windows IDE (syntax highlighting, interactive REPL, tutorials, F5-to-run). No PATH, no pip, no config file.

The language itself is a bytecode-compiled, dynamically-typed language with:

- Indentation-based blocks that close with `end`
- Sets and bitwise operators
- Closures and OOP (classes, inheritance)
- Standard-library ML/AI: tensors, matrices, `sigmoid`, `relu`, `mse`
- A 3-line HTTP server
- `jts` (run), `jtsc` (compile to `.jbc` bytecode), `jtsvm` (execute)

The toolchain is one tiny command:

```bash
jts hello.jts
```

Technical bits: core VM/compiler in C, IDE in C#/WPF + AvalonEdit, shared F# core, native C++ installer with embedded payload (Windows 7+).

Source: https://github.com/jayaswinjay-web/jtsdk

I'm genuinely interested in feedback on the VM design, the beginner-first error messages, and whether a "dead simple" language like this has a place today. Thanks for any thoughts.

## r/ProgrammingLanguages

### Title

A beginner-first language where the killer feature is the installer and IDE, not the syntax

### Post

Most PL discussions here focus on type systems, ownership, and performance. I'm coming at it from a different angle: my goal is **absolute beginners**, so my biggest design constraint is friction.

JTS GO is a bytecode-compiled language (small C VM) with:

- Dynamic typing with optional annotations
- Sets, closures, OOP, dictionaries
- Built-in ML/AI (tensors, matrices, activation/loss functions)
- HTTP server + client in the standard library
- 3-command toolchain: `jts`, `jtsc`, `jtsvm`

Syntax highlights:
- Indentation-based blocks closed with `end`
- `say()`, `ask()`, `bring` for packages

Design decisions I'd like review:

1. **`end` instead of braces** — matches Python indentation but gives an explicit terminator so the parser doesn't rely on indentation. Trade-offs?
2. **Everything in one binary** — the runner, compiler, and VM are a single exe (~what the Node community moved *away* from). Deliberate choice for simplicity.
3. **Beginner-first errors** — messages explain *why*, not just *what*.

Known weakness I'm already aware of: the name collides with Google's Go. Rename is on the table.

The IDE (C#/WPF) ships in the same installer and is where most beginners will live: https://github.com/jayaswinjay-web/jtsdk

Curious what the PL crowd thinks of a language where the differentiator is delivery, not semantics.
