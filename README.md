# JTS Development Kit v2.0.2

<p align="center">
  <b>JTS GO — The Easiest Programming Language to Learn</b><br>
  Python-like syntax. Bytecode compilation. OOP. Web Dev. ML/AI. Zero boilerplate.
</p>

---

## What is JTS GO?

JTS GO is a programming language designed for **absolute beginners**. If you've never written code before, this is where you start.

- **Python-like syntax** — readable, indentation-based
- **Object-Oriented Programming** — classes, methods, inheritance
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
print("Hello, World!")
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
nothing = nil

# Type-annotated declarations
int age = 25
string greeting = "Hello"
float pi = 3.14
bool active = true
list numbers = [1, 2, 3]
var x = 42

print(name)
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
    print("Grade: A")
elif score >= 80
    print("Grade: B")
elif score >= 70
    print("Grade: C")
else
    print("Grade: F")
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
    print(i)
end
# Output: 1, 2, 4, 5, 6
```

### Functions
```jts
func greet(name)
    print("Hello, " + name + "!")
end

func add(a, b)
    return a + b
end

print(add(3, 4))   # 7
```

### Lists
```jts
nums = [3, 1, 2]
print(nums)         # [3, 1, 2]

nums.sort()
print(nums)         # [1, 2, 3]

nums.append(4)
print(nums)         # [1, 2, 3, 4]

nums.remove(3)
print(nums)         # [1, 2, 4]

nums.pop()
print(nums)         # [1, 2]
```

### Dictionaries
```jts
d = {"name": "JTS", "version": "2.0"}
print(d)            # {name: JTS, version: 2.0}
print(d["name"])    # JTS
```

### String Methods
```jts
s = "hello world"
print(s.upper())            # HELLO WORLD
print(s.lower())            # hello world
print(s.trim())             # hello world
print(s.contains("world"))  # true
print(s.replace("world", "JTS"))  # hello JTS
print(s.substring(0, 5))    # hello
print(s.starts_with("hello"))  # true
print(s.ends_with("world"))    # true
```

### Object-Oriented Programming
```jts
class Animal
    func init(self, name)
        self.name = name
    end

    func speak(self)
        print(self.name + " makes a sound")
    end
end

# Create an instance
a = new Animal("Dog")
a.speak()           # Dog makes a sound
print(a.name)       # Dog

# Inheritance
class Dog extends Animal
    func bark(self)
        print(self.name + " barks!")
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
    print("Caught: " + e)
end
# Output: Caught: Something went wrong!
```

### File I/O
```jts
write_file("output.txt", "Hello from JTS!")
content = read_file("output.txt")
print(content)
```

### ML/AI Functions
```jts
# Tensors
t = tensor([1, 2, 3, 4, 5])
print(t)

# Matrices
m1 = matrix([[1, 2], [3, 4]])
m2 = matrix([[5, 6], [7, 8]])
result = matmul(m1, m2)
print(result)

# Activation functions
print(sigmoid(0))    # 0.5
print(relu(-5))      # 0
print(relu(5))       # 5

# Loss functions
print(mse([1, 2, 3], [1.1, 2.2, 3.1]))
```

### Web Development
```jts
# Create and start an HTTP server
server = http_server(8080)
http_start(server)
```

### Math Functions
```jts
print(sqrt(16))        # 4
print(math("sin", 3.14159))  # ~0
print(math("floor", 3.7))    # 3
print(math("abs", -42))      # 42
```

### String Conversion
```jts
print(str(42))          # "42"
print(str(true))        # "true"
print(str(nil))         # "nil"
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `print(value)` | Output a value to the console |
| `input(prompt)` | Read user input (auto-detects type) |
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
  Made with passion by <b>Jayaswin Jay</b><br>
  JTS GO v2.0.2 — 2026
</p>
