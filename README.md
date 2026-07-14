# JTS Development Kit v1.2.0

<p align="center">
  <b>JTS GO — The Easiest Programming Language to Learn</b><br>
  Python-like syntax. Bytecode compilation. Zero boilerplate.
</p>

---

## What is JTS GO?

JTS GO is a new programming language designed for **absolute beginners**. If you've never written code before, this is where you start.

- **Python-like syntax** — readable, indentation-based
- **Dynamic typing** — no need to declare variable types
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

Output:
```
Hello, World!
```

## Language Features

### Variables
```jts
# Dynamic typing (no keyword needed)
name = "JTS GO"
version = 1.2
is_awesome = true
nothing = nil

# Type-annotated declarations
int age = 25
string greeting = "Hello"
float pi = 3.14
bool active = true
list numbers = [1, 2, 3]
var x = 42

# Unassigned variables (default to nil)
int count
string message
list items

print(name)
print(age)
print(pi)
print(count)    # nil
```

### Arithmetic
```jts
a = 10
b = 3

print(a + b)    # 13
print(a - b)    # 7
print(a * b)    # 30
print(a / b)    # 3.33333
print(a % b)    # 1
```

### Strings
```jts
first = "Hello"
second = "World"
print(first + " " + second)   # Hello World
print(len("JTS"))             # 3
print(type("hello"))          # string
```

### If/Else
```jts
score = 85

if score >= 90
    print("Grade: A")
else if score >= 80
    print("Grade: B")
else if score >= 70
    print("Grade: C")
else
    print("Grade: F")
end
```

### While Loops
```jts
count = 0
while count < 5
    print(count)
    count = count + 1
end
```

### For Loops
```jts
for i in 0 to 10
    print(i)
end
```

### Functions
```jts
func greet(name)
    print("Hello, " + name + "!")
end

greet("Alice")

func add(a, b)
    return a + b
end

result = add(3, 4)
print(result)   # 7
```

### Recursion
```jts
func factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end

print(factorial(5))   # 120
```

### Lists
```jts
# Create a list
nums = [1, 2, 3, 4, 5]

# Access elements
print(nums[0])    # 1
print(nums[2])    # 3

# Modify elements
nums[0] = 99

# Append
append(nums, 6)

# Length
print(len(nums))  # 6

# Type
print(type(nums)) # list

# Mixed types
mixed = ["hello", 42, true]
print(mixed)       # [hello, 42, true]

# Empty list
empty = []
```

### Input
```jts
# Smart input - auto-detects types
age = input("Enter your age: ")
print(type(age))  # number (if you entered a number)

name = input("Enter your name: ")
print(type(name))  # string

# Number conversion
num = number("42")
print(type(num))  # number
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `print(value)` | Output a value to the console |
| `input(prompt)` | Read user input (auto-detects type) |
| `len(value)` | Get length of a string or list |
| `type(value)` | Get the type of a value |
| `append(list, value)` | Add an element to a list |
| `number(string)` | Convert a string to a number |

## Example Programs

| Program | Description |
|---------|-------------|
| [hello.jts](examples/hello.jts) | Your first program |
| [variables.jts](examples/variables.jts) | Data types and variables |
| [arithmetic.jts](examples/arithmetic.jts) | Math operations |
| [conditionals.jts](examples/conditionals.jts) | If/else statements |
| [loops.jts](examples/loops.jts) | While and for loops |
| [functions.jts](examples/functions.jts) | Functions and parameters |
| [factorial.jts](examples/factorial.jts) | Recursive factorial |
| [fibonacci.jts](examples/fibonacci.jts) | Recursive Fibonacci |
| [lists.jts](examples/lists.jts) | Lists and arrays |

## Documentation

Read the full language guide: [JTS GO Learning Guide](LEARNING_GUIDE.md)

## Toolchain

| Command | Purpose |
|---------|---------|
| `jts file.jts` | Compile and run a JTS GO program |
| `jtsc file.jts` | Compile to bytecode only (.jbc) |
| `jtsvm file.jbc` | Run a compiled bytecode file |
| `jts --update` | Update JTS GO to latest version |

## License

Licensed under the [Apache License 2.0](LICENSE).

---

<p align="center">
  Made with passion by <b>Jayaswin Jay</b><br>
  JTS GO v1.2.0 — 2026
</p>