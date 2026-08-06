# Getting Started with JTS GO

## System Requirements

- **Operating System**: Windows 10+, macOS 10.15+, or Linux (Ubuntu 18.04+)
- **Node.js**: v14.0 or higher (for npm installation)
- **Disk Space**: ~50 MB
- **RAM**: 256 MB minimum

## Installation

### Option 1: Install via npm (Recommended)

```bash
npm install -g @jaytechsolutions/jts-go
```

This installs three commands globally:

| Command | Purpose |
|---------|---------|
| `jts file.jts` | Compile and run a JTS GO program in one step |
| `jtsc file.jts` | Compile to bytecode only (produces `.jbc` file) |
| `jtsvm file.jbc` | Run a pre-compiled bytecode file |

Verify the installation:

```bash
jts --version
```

### Option 2: Download Manually

1. Go to the [Releases page](https://github.com/jayaswinjay-web/jtsdk/releases)
2. Download the latest release for your operating system
3. Extract the archive to a folder (e.g., `C:\jtsdk` or `/usr/local/jtsdk`)
4. Add the extracted folder to your system PATH

**Windows:**
```powershell
# Add to PATH permanently (run of PowerShell as Admin)
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
[Environment]::SetEnvironmentVariable("Path", "$currentPath;C:\jtsdk\bin", "User")
```

**macOS / Linux:**
```bash
# Add to ~/.bashrc or ~/.zshrc
export PATH="$PATH:/usr/local/jtsdk/bin"
source ~/.bashrc
```

## Your First Program

### Step 1: Create a file

Create a new file called `hello.jts` with this content:

```jts
# Hello World - The simplest JTS GO program
say("Hello, World!")
say("Welcome to JTS GO!")
```

### Step 2: Run it

```bash
jts hello.jts
```

### Output

```
Hello, World!
Welcome to JTS GO!
```

That's it. You just wrote and ran your first JTS GO program.

## Project Structure

When you install the JTS SDK, you get this layout:

```
jtsdk/
├── bin/              # CLI executables (jts, jtsc, jtsvm)
├── docs/             # Language documentation
│   ├── getting-started.md
│   ├── syntax.md
│   ├── types.md
│   ├── control-flow.md
│   ├── functions.md
│   └── builtins.md
├── examples/         # Example programs
│   ├── hello.jts
│   ├── variables.jts
│   ├── arithmetic.jts
│   ├── conditionals.jts
│   ├── loops.jts
│   ├── functions.jts
│   ├── factorial.jts
│   └── fibonacci.jts
├── LICENSE
└── README.md
```

## Toolchain

JTS GO uses a compile-then-run model. Your `.jts` source code is compiled to bytecode (`.jbc`), which runs on the JTS VM.

```
.jts source  →  jtsc  →  .jbc bytecode  →  jtsvm  →  output
```

For convenience, the `jts` command does both steps in one go:

```bash
jts hello.jts
```

If you want to compile separately (for example, to distribute bytecode without source):

```bash
# Step 1: Compile
jtsc hello.jts    # produces hello.jbc

# Step 2: Run
jtsvm hello.jbc
```

## Next Steps

- [Syntax Overview](syntax.md) — Learn the basics of JTS GO syntax
- [Data Types](types.md) — Understand numbers, strings, booleans, and void
- [Control Flow](control-flow.md) — if/else, while loops, for loops
- [Functions](functions.md) — Define and call functions
- [Built-in Functions](builtins.md) — say, ask, len, type
- [Examples](../examples/) — Browse working example programs
