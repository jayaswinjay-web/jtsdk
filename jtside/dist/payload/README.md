# JTS Development Kit v2.1.12

<p align="center">
  <b>JTS GO — The Easiest Programming Language to Learn</b><br>
  Bytecode compilation. Sets. OOP. Web Dev. ML/AI. Zero boilerplate.
</p>

---

## What is JTS GO?

JTS GO is a programming language designed for **absolute beginners**. If you've never written code before, this is where you start. Built by **JayTech Solutions**.

- **Clean, readable syntax** — indentation-based blocks that close with `end`
- **Object-Oriented Programming** — classes, methods, inheritance
- **Sets & bitwise operators** — `{1, 2, 3}`, `|`/`&`/`^`/`<<`/`>>`, plus `is`, `del`, and `assert`
- **Closures** — nested functions that capture variables
- **Scrolls (packages)** — `bring` reusable library scrolls
- **Dynamic typing** — no need to declare variable types
- **Built-in ML/AI** — tensors, matrices, activation functions
- **Web Development** — HTTP server support
- **Simple toolchain** — one command to run your code
- **Compiled to bytecode** — fast execution via a virtual machine

## Quick Start

### Install via npm

```bash
npm install -g @jaytechsolutions/jts-go
```

Then run any `.jts` file:

```bash
jts hello.jts
```

### Update

```bash
jts --update
```

## Hello, World!

```jts
say("Hello, World!")
```

Save as `hello.jts` and run:

```bash
jts hello.jts
```

## Language Features

### Variables
```jts
# Dynamic typing
name = "JTS GO"
version = 2.0
is_awesome = true
nothing = void

# Type-annotated declarations
int age = 25
string greeting = "Hello"
float pi = 3.14
bool active = true
list numbers = [1, 2, 3]
var x = 42

say(name)
```

### Compound Assignment
```jts
x = 10
x += 5      # x is now 15
x -= 3      # x is now 12
x *= 2      # x is now 24
```

### Control Flow
```jts
# If/Else
score = 85

if score >= 90
    say("Grade: A")
elif score >= 80
    say("Grade: B")
elif score >= 70
    say("Grade: C")
else
    say("Grade: F")
end
```

### Break and Continue
```jts
# Skip 3, stop at 7
i = 0
while i < 10
    i = i + 1
    if i == 3
        continue
    end
    if i == 7
        break
    end
    say(i)
end
# Output: 1, 2, 4, 5, 6
```

### Functions
```jts
func greet(name)
    say("Hello, " + name + "!")
end

func add(a, b)
    return a + b
end

say(add(3, 4))   # 7
```

### Lists
```jts
nums = [3, 1, 2]
say(nums)         # [3, 1, 2]

nums.sort()
say(nums)         # [1, 2, 3]

nums.append(4)
say(nums)         # [1, 2, 3, 4]

nums.remove(3)
say(nums)         # [1, 2, 4]

nums.pop()
say(nums)         # [1, 2]
```

### Dictionaries
```jts
d = {"name": "JTS", "version": "2.0"}
say(d)            # {name: JTS, version: 2.0}
say(d["name"])    # JTS
```

### String Methods
```jts
s = "hello world"
say(s.upper())            # HELLO WORLD
say(s.lower())            # hello world
say(s.trim())             # hello world
say(s.contains("world"))  # true
say(s.replace("world", "JTS"))  # hello JTS
say(s.substring(0, 5))    # hello
say(s.starts_with("hello"))  # true
say(s.ends_with("world"))    # true
```

### Object-Oriented Programming
```jts
class Animal
    func init(self, name)
        self.name = name
    end

    func speak(self)
        say(self.name + " makes a sound")
    end
end

# Create an instance
a = new Animal("Dog")
a.speak()           # Dog makes a sound
say(a.name)       # Dog

# Inheritance
class Dog extends Animal
    func bark(self)
        say(self.name + " barks!")
    end
end

d = new Dog("Rex")
d.speak()           # Rex makes a sound (inherited)
d.bark()            # Rex barks! (own method)
```

### Try/Catch/Throw
```jts
try
    throw "Something went wrong!"
catch e
    say("Caught: " + e)
end
# Output: Caught: Something went wrong!
```

### File I/O
```jts
write_file("output.txt", "Hello from JTS!")
content = read_file("output.txt")
say(content)
```

### ML/AI Functions
```jts
# Tensors
t = tensor([1, 2, 3, 4, 5])
say(t)

# Matrices
m1 = matrix([[1, 2], [3, 4]])
m2 = matrix([[5, 6], [7, 8]])
result = matmul(m1, m2)
say(result)

# Activation functions
say(sigmoid(0))    # 0.5
say(relu(-5))      # 0
say(relu(5))       # 5

# Loss functions
say(mse([1, 2, 3], [1.1, 2.2, 3.1]))
```

### Web Development
```jts
# Create and start an HTTP server
srv = http_server(8080)
http_route(srv, "GET", "/", "<h1>Home</h1>")
http_route(srv, "GET", "/api/data", '{"ok": true}')
http_start(srv)
```
> **Note:** `server` is a reserved keyword — use a different variable name (e.g. `srv`).

### Math Functions
```jts
say(sqrt(16))        # 4
say(math("sin", 3.14159))  # ~0
say(math("floor", 3.7))    # 3
say(math("abs", -42))      # 42
```

### String Conversion
```jts
say(str(42))          # "42"
say(str(true))        # "true"
say(str(void))         # "void"
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `say(value)` | Output a value to the console |
| `ask(prompt)` | Read user input (auto-detects type) |
| `len(value)` | Get length of a string, list, tensor, or matrix |
| `type(value)` | Get the type of a value |
| `append(list, value)` | Add an element to a list |
| `number(string)` | Convert a string to a number |
| `str(value)` | Convert a value to a string |
| `math(func, x)` | Math functions (sin, cos, tan, sqrt, abs, log, exp, pow, floor, ceil, round) |
| `sqrt(x)` | Square root |
| `tensor(data)` | Create a tensor from a list |
| `matrix(data)` | Create a matrix from nested lists |
| `matmul(a, b)` | Matrix multiplication |
| `sigmoid(x)` | Sigmoid activation: 1 / (1 + e^(-x)) |
| `relu(x)` | ReLU activation: max(0, x) |
| `mse(predicted, actual)` | Mean squared error loss |
| `http_server(port)` | Create an HTTP server |
| `http_route(server, method, path, body)` | Register a route on the server |
| `http_start(server)` | Start the HTTP server |
| `http_request(url)` | Make an HTTP request |
| `read_file(path)` | Read a file's contents as a string |
| `write_file(path, data)` | Write data to a file |

## String Methods (call on any string)

| Method | Description |
|--------|-------------|
| `s.upper()` | Convert to uppercase |
| `s.lower()` | Convert to lowercase |
| `s.trim()` | Remove leading/trailing whitespace |
| `s.contains(sub)` | Check if string contains substring |
| `s.replace(old, new)` | Replace occurrences |
| `s.substring(start, end)` | Extract substring |
| `s.starts_with(prefix)` | Check if starts with prefix |
| `s.ends_with(suffix)` | Check if ends with suffix |

## List Methods (call on any list)

| Method | Description |
|--------|-------------|
| `lst.sort()` | Sort the list in place |
| `lst.remove(value)` | Remove first occurrence of value |
| `lst.pop()` | Remove and return the last element |
| `lst.append(value)` | Add element to the end |

## Toolchain

| Command | Purpose |
|---------|---------|
| `jts file.jts` | Compile and run a JTS GO program |
| `jtsc file.jts` | Compile to bytecode only (.jbc) |
| `jtsvm file.jbc` | Run a compiled bytecode file |
| `jts --update` | Update JTS GO to latest version |

## Documentation

Read the full language guide: [JTS GO Learning Guide](LEARNING_GUIDE.md)

## License

Proprietary Software — Copyright (c) 2025–2026 JayTechSolutions. All Rights Reserved.

**You may:** Install, use, and run JTS GO for personal or commercial purposes.

**You may NOT:** Copy, modify, reverse-engineer, redistribute, or develop competing languages from the source code.

See [PROPRIETARY_LICENSE](PROPRIETARY_LICENSE) for full terms.

For permissions beyond this license, contact: jayaswinjay.web@gmail.com

---

<p align="center">
  Made with passion by <b>JayTech Solutions</b><br>
  JTS GO v2.1.12 — 2026
</p>
