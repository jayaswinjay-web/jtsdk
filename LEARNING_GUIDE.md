# JTS GO Learning Guide

<p align="center">
  <b>The Complete Guide to Learning JTS GO</b><br>
  From zero to confident programmer
</p>

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Your First Program](#your-first-program)
4. [How JTS GO Works](#how-jts-go-works)
5. [Variables and Data Types](#variables-and-data-types)
6. [Arithmetic Operations](#arithmetic-operations)
7. [Strings](#strings)
8. [Input and Output](#input-and-output)
9. [Conditionals](#conditionals)
10. [Loops](#loops)
11. [Functions](#functions)
12. [Lists](#lists)
13. [Dictionaries](#dictionaries)
14. [Object-Oriented Programming](#object-oriented-programming)
15. [Error Handling (Try/Catch)](#error-handling-trycatch)
16. [String Methods](#string-methods)
17. [File I/O](#file-io)
18. [ML/AI Functions](#mlai-functions)
19. [Web Development](#web-development)
20. [Built-in Functions](#built-in-functions)
21. [Example Programs](#example-programs)
22. [Next Steps](#next-steps)

---

## Introduction

Welcome to JTS GO! This guide will teach you everything you need to know to start programming in JTS GO.

**What is JTS GO?**

JTS GO is a programming language designed for beginners. It combines:

- **Clean, readable syntax** — easy to read and write
- **Dynamic typing** — no need to declare variable types
- **Bytecode compilation** — fast execution
- **Simple toolchain** — one command to run your code

**Who is this guide for?**

This guide is for anyone who wants to learn programming, including:

- Complete beginners with no coding experience
- Developers who want to learn a new language
- Students learning computer science basics

---

## Installation

### Step 1: Install Node.js

JTS GO runs on npm, which comes with Node.js. Download and install Node.js from:

**https://nodejs.org**

### Step 2: Install JTS GO

Open your terminal (Command Prompt on Windows, Terminal on Mac/Linux) and run:

```bash
npm install -g @jaytechsolutions/jts-go
```

### Step 3: Verify Installation

```bash
jts --version
```

You should see the version number (e.g., `0.9.0-beta`).

### Updating JTS GO

To update to the latest version:

```bash
jts --update
```

---

## Your First Program

Let's create your first JTS GO program.

### Step 1: Create a File

Create a new file called `hello.jts` with this content:

```jts
say("Hello, World!")
```

### Step 2: Run the Program

```bash
jts hello.jts
```

### Step 3: See the Output

```
Hello, World!
```

Congratulations! You've just written your first program!

---

## How JTS GO Works

When you run a JTS GO program, two things happen:

1. **Compilation** — Your code is translated into bytecode
2. **Execution** — A virtual machine runs the bytecode

This is similar to how Java works, but much simpler.

### The Toolchain

| Command | What It Does |
|---------|--------------|
| `jts file.jts` | Compile and run in one step |
| `jtsc file.jts` | Compile to bytecode only |
| `jtsvm file.jbc` | Run compiled bytecode |

---

## Variables and Data Types

Variables store values. In JTS GO, you can declare variables in two ways:

### Creating Variables

**Dynamic typing (no keyword needed):**
```jts
name = "JTS GO"        # String
age = 25               # Number
is_student = true      # Boolean
nothing = void          # Null/Empty
```

**Type-annotated declarations:**
```jts
int age = 25           # Integer
string name = "Alice"  # String
float pi = 3.14        # Float
bool active = true     # Boolean
list nums = [1, 2, 3]  # List
var x = 42             # Same as dynamic
```

**Unassigned variables (default to void):**
```jts
int count
string message
float temperature
bool is_ready
list items

say(count)    # void
say(message)  # void
```

### Data Types

JTS GO has these data types:

| Type | Description | Example |
|------|-------------|---------|
| **string** | Text | `"hello"`, `"JTS GO"` |
| **number** | Numbers (integers and decimals) | `42`, `3.14` |
| **bool** | True or false | `true`, `false` |
| **void** | Nothing/empty | `void` |

### Using Variables

```jts
name = "Alice"
age = 30
height = 5.6
is_student = true

say(name)        # Alice
say(age)         # 30
say(height)      # 5.6
say(is_student)  # true
```

### Reassigning Variables

```jts
x = 10
say(x)    # 10

x = 20
say(x)    # 20
```

### Variable Naming Rules

- Must start with a letter or underscore
- Can contain letters, numbers, and underscores
- Case-sensitive (`name` and `Name` are different)

```jts
# Good variable names
my_name = "Alice"
user_age = 25
_private = "ok"
count1 = 1

# Bad variable names (will cause errors)
# 1name = "error"    # Can't start with number
# my-name = "error"  # Can't use hyphens
```

---

## Arithmetic Operations

JTS GO supports all basic math operations.

### Basic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `5 + 3` → `8` |
| `-` | Subtraction | `5 - 3` → `2` |
| `*` | Multiplication | `5 * 3` → `15` |
| `/` | Division | `5 / 3` → `1.66667` |
| `%` | Modulo (remainder) | `5 % 3` → `2` |

### Examples

```jts
a = 10
b = 3

say(a + b)    # 13
say(a - b)    # 7
say(a * b)    # 30
say(a / b)    # 3.33333
say(a % b)    # 1
```

### Order of Operations

```jts
result = 2 + 3 * 4    # 14 (multiplication first)
result = (2 + 3) * 4  # 20 (parentheses first)
```

### Compound Assignment

```jts
x = 10
x += 5      # x is now 15 (shorthand for x = x + 5)
x -= 3      # x is now 12 (shorthand for x = x - 3)
x *= 2      # x is now 24 (shorthand for x = x * 2)
```

---

## Strings

Strings are sequences of characters (text).

### Creating Strings

```jts
greeting = "Hello"
name = "World"
```

### String Concatenation

Use `+` to combine strings:

```jts
first = "Hello"
second = "World"
message = first + " " + second
say(message)    # Hello World
```

### String Length

```jts
text = "JTS"
say(len(text))    # 3
```

### String Type

```jts
name = "Alice"
say(type(name))    # string
```

### String Examples

```jts
# Building a sentence
first_name = "John"
last_name = "Doe"
full_name = first_name + " " + last_name
say(full_name)    # John Doe

# Repeated text
line = "-" * 20
say(line)    # --------------------

# Empty string
empty = ""
say(len(empty))    # 0
```

---

## Input and Output

### Output with say()

The `say()` function displays text on the screen:

```jts
say("Hello, World!")
say(42)
say(true)
say(void)
```

### Multiple Values

```jts
say("Name:", "Alice")
say("Age:", 30)
```

### Input with ask()

The `ask()` function reads user input:

```jts
name = ask("What is your name? ")
say("Hello, " + name + "!")
```

### Smart Input

JTS GO automatically detects the type of input:

```jts
# If user enters "25", age will be a number
age = ask("Enter your age: ")
say(type(age))    # number

# If user enters "Alice", name will be a string
name = ask("Enter your name: ")
say(type(name))    # string

# If user enters "true", flag will be a boolean
flag = ask("Enter true or false: ")
say(type(flag))    # bool
```

---

## Conditionals

Conditionals let your program make decisions.

### If Statement

```jts
age = 18

if age >= 18
    say("You are an adult")
end
```

### If/Else Statement

```jts
age = 15

if age >= 18
    say("You are an adult")
else
    say("You are a minor")
end
```

### If/Elif/Else Statement

```jts
score = 85

if score >= 90
    say("Grade: A")
elif score >= 80
    say("Grade: B")
elif score >= 70
    say("Grade: C")
elif score >= 60
    say("Grade: D")
else
    say("Grade: F")
end
```

### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal to | `5 == 5` → `true` |
| `!=` | Not equal to | `5 != 3` → `true` |
| `>` | Greater than | `5 > 3` → `true` |
| `<` | Less than | `5 < 3` → `false` |
| `>=` | Greater or equal | `5 >= 5` → `true` |
| `<=` | Less or equal | `5 <= 3` → `false` |

### Logical Operators

```jts
age = 25
income = 50000

# AND - both conditions must be true
if age >= 18 and income >= 30000
    say("Approved")
end

# OR - at least one condition must be true
if age < 18 or income < 30000
    say("Not eligible")
end
```

---

## Loops

Loops repeat code multiple times.

### While Loop

```jts
count = 0

while count < 5
    say(count)
    count = count + 1
end
```

Output:
```
0
1
2
3
4
```

### For Loop

```jts
for i of 0 to 5
    say(i)
end
```

Output:
```
0
1
2
3
4
5
```

### Counting Backwards

```jts
for i of 10 to 0
    say(i)
end
```

### Nested Loops

```jts
for i of 0 to 3
    for j of 0 to 3
        say(i + "," + j)
    end
end
```

### Loop Examples

**Sum of numbers:**
```jts
sum = 0
for i of 1 to 10
    sum = sum + i
end
say(sum)    # 55
```

**Counting occurrences:**
```jts
count = 0
for i of 0 to 10
    if i % 2 == 0
        count = count + 1
    end
end
say(count)    # 6 (0, 2, 4, 6, 8, 10)
```

### Break and Continue

Use `break` to exit a loop early and `continue` to skip to the next iteration.

```jts
# Break: stop at 5
for i of 0 to 10
    if i == 5
        break
    end
    say(i)
end
# Output: 0, 1, 2, 3, 4
```

```jts
# Continue: skip 3
for i of 0 to 6
    if i == 3
        continue
    end
    say(i)
end
# Output: 0, 1, 2, 4, 5
```

```jts
# While loop with break and continue
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

---

## Functions

Functions are reusable blocks of code.

### Defining a Function

```jts
func greet()
    say("Hello, World!")
end

# Call the function
greet()
```

### Function with Parameters

```jts
func greet(name)
    say("Hello, " + name + "!")
end

greet("Alice")    # Hello, Alice!
greet("Bob")      # Hello, Bob!
```

### Function with Return Value

```jts
func add(a, b)
    return a + b
end

result = add(3, 4)
say(result)    # 7
```

### Multiple Parameters

```jts
func calculate_area(length, width)
    return length * width
end

area = calculate_area(5, 3)
say(area)    # 15
```

### Functions Calling Functions

```jts
func square(x)
    return x * x
end

func sum_of_squares(a, b)
    return square(a) + square(b)
end

result = sum_of_squares(3, 4)
say(result)    # 25 (9 + 16)
```

### Recursion

Functions can call themselves:

```jts
func factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end

say(factorial(5))    # 120
say(factorial(10))   # 3628800
```

**How recursion works:**
- `factorial(5)` = 5 × `factorial(4)`
- `factorial(4)` = 4 × `factorial(3)`
- `factorial(3)` = 3 × `factorial(2)`
- `factorial(2)` = 2 × `factorial(1)`
- `factorial(1)` = 1 (base case)

### Fibonacci Sequence

```jts
func fib(n)
    if n <= 1
        return n
    end
    return fib(n - 1) + fib(n - 2)
end

for i of 0 to 10
    say(fib(i))
end
```

Output:
```
0
1
1
2
3
5
8
13
21
34
55
```

---

## Lists

Lists store multiple values in a single variable.

### Creating Lists

```jts
# List of numbers
numbers = [1, 2, 3, 4, 5]

# List of strings
fruits = ["apple", "banana", "cherry"]

# Mixed types
mixed = ["hello", 42, true]

# Empty list
empty = []
```

### Accessing Elements

```jts
fruits = ["apple", "banana", "cherry"]

say(fruits[0])    # apple (first element)
say(fruits[1])    # banana (second element)
say(fruits[2])    # cherry (third element)
```

**Note:** Lists start at index 0, not 1.

### Modifying Elements

```jts
fruits = ["apple", "banana", "cherry"]

fruits[1] = "blueberry"
say(fruits)    # [apple, blueberry, cherry]
```

### Adding Elements

```jts
numbers = [1, 2, 3]

append(numbers, 4)
say(numbers)    # [1, 2, 3, 4]

append(numbers, 5)
say(numbers)    # [1, 2, 3, 4, 5]
```

### List Length

```jts
numbers = [1, 2, 3, 4, 5]
say(len(numbers))    # 5
```

### List Type

```jts
numbers = [1, 2, 3]
say(type(numbers))    # list
```

### Looping Through Lists

```jts
fruits = ["apple", "banana", "cherry"]

for i of 0 to len(fruits) - 1
    say(fruits[i])
end
```

### List Examples

**Find the maximum:**
```jts
func find_max(list)
    max = list[0]
    for i of 1 to len(list) - 1
        if list[i] > max
            max = list[i]
        end
    end
    return max
end

numbers = [3, 7, 2, 9, 4]
say(find_max(numbers))    # 9
```

**Sum of list:**
```jts
func sum_list(list)
    total = 0
    for i of 0 to len(list) - 1
        total = total + list[i]
    end
    return total
end

numbers = [1, 2, 3, 4, 5]
say(sum_list(numbers))    # 15
```

---

## Object-Oriented Programming

JTS GO supports classes with methods, `self` references, field access, and inheritance.

### Defining a Class

```jts
class Animal
    func init(self, name)
        self.name = name
    end

    func speak(self)
        say(self.name + " makes a sound")
    end
end
```

- `class ... end` defines a class
- `init(self, ...)` is the constructor (called automatically with `new`)
- `self` refers to the current instance
- Fields are created by assigning to `self.fieldname`

### Creating Instances

```jts
a = new Animal("Dog")
a.speak()       # Dog makes a sound
say(a.name)   # Dog
```

### Methods

```jts
class Calculator
    func init(self)
        self.result = 0
    end

    func add(self, x)
        self.result = self.result + x
        return self
    end

    func get(self)
        return self.result
    end
end

calc = new Calculator()
calc.add(5).add(3)
say(calc.get())   # 8
```

### Inheritance

```jts
class Animal
    func init(self, name)
        self.name = name
    end

    func speak(self)
        say(self.name + " makes a sound")
    end
end

class Dog extends Animal
    func bark(self)
        say(self.name + " barks!")
    end
end

d = new Dog("Rex")
d.speak()   # Rex makes a sound (inherited method)
d.bark()    # Rex barks! (own method)
```

- `extends` inherits all methods from the parent class
- The child class can define its own methods
- Inherited methods work on child instances

---

## Dictionaries

Dictionaries store key-value pairs. Keys must be strings.

### Creating Dictionaries

```jts
# Empty dictionary
empty = {}

# With values
person = {"name": "Alice", "age": 30, "active": true}

# Accessing values
say(person["name"])    # Alice
say(person["age"])     # 30
```

### Modifying Dictionaries

```jts
d = {"name": "JTS"}
d["version"] = "2.0"
d["name"] = "JTS GO"
say(d)    # {name: JTS GO, version: 2.0}
```

### Dictionary Examples

```jts
# Counting with a dictionary
word = "hello world"
count = {}
for i of 0 to len(word) - 1
    ch = word[i]
    if ch of count
        count[ch] = count[ch] + 1
    else
        count[ch] = 1
    end
end
```

---

## Error Handling (Try/Catch)

Use `try`/`catch` to handle errors without crashing your program.

### Basic Try/Catch

```jts
try
    throw "Something went wrong!"
catch e
    say("Caught: " + e)
end
# Output: Caught: Something went wrong!
```

### How It Works

1. Code inside `try` runs normally
2. If `throw` is executed, execution jumps to `catch`
3. The error value is stored in the catch variable (`e`)
4. Execution continues after the `end`

### Throwing Values

You can throw any value (string, number, etc.):

```jts
try
    throw 42
catch e
    say("Error code: " + e)
end
# Output: Error code: 42
```

### Try/Catch Without Error Variable

```jts
try
    throw "oops"
catch
    say("Something failed!")
end
```

---

## String Methods

JTS GO provides built-in methods for string manipulation. Call them on any string variable using dot notation.

### upper() and lower()

```jts
s = "hello"
say(s.upper())    # HELLO

t = "WORLD"
say(t.lower())    # world
```

### trim()

Removes leading and trailing whitespace:

```jts
s = "  hello  "
say(s.trim())     # hello
```

### contains()

Check if a string contains a substring:

```jts
s = "hello world"
say(s.contains("world"))    # true
say(s.contains("xyz"))      # false
```

### replace()

Replace occurrences of a substring:

```jts
s = "hello world"
say(s.replace("world", "JTS"))    # hello JTS
```

### substring()

Extract a portion of the string:

```jts
s = "hello"
say(s.substring(0, 3))    # hel
say(s.substring(1, 4))    # ell
```

### starts_with() and ends_with()

```jts
s = "hello world"
say(s.starts_with("hello"))    # true
say(s.ends_with("world"))      # true
```

---

## File I/O

JTS GO can read from and write to files.

### Writing to a File

```jts
write_file("output.txt", "Hello from JTS!")
```

### Reading from a File

```jts
content = read_file("output.txt")
say(content)    # Hello from JTS!
```

### File I/O Example

```jts
# Write data
write_file("data.txt", "Line 1\nLine 2\nLine 3")

# Read it back
content = read_file("data.txt")
say(content)

# Build and save a report
name = "JTS GO"
version = "0.9.0-beta"
report = name + " version " + version
write_file("report.txt", report)
say("Report saved!")
```

### String Methods on File Content

```jts
content = read_file("data.txt")
if content.contains("error")
    say("File contains errors!")
end
say(len(content) + " characters read")
```

---

## ML/AI Functions

JTS GO includes built-in ML/AI functions for numerical computing.

### Tensors

```jts
t = tensor([1, 2, 3, 4, 5])
say(t)       # [1, 2, 3, 4, 5]
say(len(t))  # 5
```

### Matrices

```jts
m1 = matrix([[1, 2], [3, 4]])
m2 = matrix([[5, 6], [7, 8]])
result = matmul(m1, m2)
say(result)   # [[19, 22] [43, 50]]
```

### Activation Functions

```jts
say(sigmoid(0))    # 0.5
say(sigmoid(1))    # 0.731...
say(relu(-5))      # 0
say(relu(5))       # 5
```

### Loss Functions

```jts
predicted = [1.0, 2.0, 3.0]
actual = [1.1, 2.2, 3.1]
say(mse(predicted, actual))   # 0.02
```

### Math Functions

```jts
say(sqrt(16))              # 4
say(math("sin", 3.14159))  # ~0
say(math("cos", 0))        # 1
say(math("floor", 3.7))    # 3
say(math("ceil", 3.2))     # 4
say(math("abs", -42))      # 42
say(math("log", 2.71828))  # ~1
say(math("exp", 1))        # ~2.718
```

---

## Web Development

JTS GO includes HTTP server support for web development.

### Creating a Server

```jts
srv = http_server(8080)
http_route(srv, "GET", "/", "<h1>Home</h1>")
http_route(srv, "GET", "/api/data", '{"ok": true}')
http_start(srv)
```
> **Note:** `server` is a reserved keyword — use a different variable name (e.g. `srv`).

### Making HTTP Requests

```jts
response = http_request("https://api.example.com/data")
say(response)   # [200, "OK"]
```

---

## Built-in Functions

JTS GO comes with these built-in functions:

### say(value)

Displays a value on the screen:

```jts
say("Hello")
say(42)
say(true)
say(void)
say([1, 2, 3])
```

### ask(prompt)

Reads user input:

```jts
name = ask("Enter your name: ")
age = ask("Enter your age: ")
```

### len(value)

Returns the length of a string or list:

```jts
say(len("hello"))        # 5
say(len([1, 2, 3]))     # 3
say(len([]))             # 0
```

### type(value)

Returns the type of a value:

```jts
say(type("hello"))    # string
say(type(42))         # number
say(type(true))       # bool
say(type(void))        # void
say(type([1, 2]))     # list
```

### append(list, value)

Adds an element to a list:

```jts
nums = [1, 2, 3]
append(nums, 4)
say(nums)    # [1, 2, 3, 4]
```

### number(string)

Converts a string to a number:

```jts
num = number("42")
say(type(num))    # number
say(num + 8)      # 50
```

### str(value) / string(value)

Converts any value to a string:

```jts
say(str(123))           # 123
say(str(true))          # true
say(str(void))           # void
say(str([1, 2, 3]))     # [1, 2, 3]
```

### int(value)

Converts a value to an integer, truncating decimals:

```jts
say(int("42"))          # 42
say(int(3.99))          # 3
```

### float(value)

Alias for `number()`, converts string to float:

```jts
say(float("3.5"))       # 3.5
```

### bool(value)

Converts a value to boolean (0, void, "" are falsy):

```jts
say(bool("x"))          # true
say(bool(""))           # false
say(bool(0))            # false
```

### list(value)

Converts a string, set, tensor, or list to a new list:

```jts
say(list("abc"))        # [a, b, c]
say(list({1, 2, 3}))    # [1, 2, 3]
```

---

## Example Programs

### 1. Calculator

```jts
func add(a, b)
    return a + b
end

func subtract(a, b)
    return a - b
end

func multiply(a, b)
    return a * b
end

func divide(a, b)
    return a / b
end

num1 = ask("Enter first number: ")
num2 = ask("Enter second number: ")

say("Sum: " + add(num1, num2))
say("Difference: " + subtract(num1, num2))
say("Product: " + multiply(num1, num2))
say("Quotient: " + divide(num1, num2))
```

### 2. FizzBuzz

```jts
for i of 1 to 100
    if i % 15 == 0
        say("FizzBuzz")
    elif i % 3 == 0
        say("Fizz")
    elif i % 5 == 0
        say("Buzz")
    else
        say(i)
    end
end
```

### 3. Guessing Game

```jts
secret = 42
guess = 0

while guess != secret
    guess = ask("Guess the number: ")
    if guess < secret
        say("Too low!")
    elif guess > secret
        say("Too high!")
    else
        say("Congratulations! You got it!")
    end
end
```

### 4. Factorial Calculator

```jts
func factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end

num = ask("Enter a number: ")
result = factorial(num)
say("Factorial: " + result)
```

### 5. List Statistics

```jts
func average(list)
    total = 0
    for i of 0 to len(list) - 1
        total = total + list[i]
    end
    return total / len(list)
end

func find_max(list)
    max = list[0]
    for i of 1 to len(list) - 1
        if list[i] > max
            max = list[i]
        end
    end
    return max
end

func find_min(list)
    min = list[0]
    for i of 1 to len(list) - 1
        if list[i] < min
            min = list[i]
        end
    end
    return min
end

numbers = [23, 45, 12, 67, 89, 34, 56]

say("List: " + numbers)
say("Average: " + average(numbers))
say("Maximum: " + find_max(numbers))
say("Minimum: " + find_min(numbers))
```

---

## Next Steps

Now that you know the basics of JTS GO, here are some suggestions:

### Practice Problems

1. **Temperature Converter** — Convert between Celsius and Fahrenheit
2. **Palindrome Checker** — Check if a word reads the same backwards
3. **Prime Number Checker** — Determine if a number is prime
4. **Simple Banking System** — Deposit, withdraw, and check balance
5. **Quiz Game** — Ask questions and track the score

### Build Projects

1. **Todo List Manager** — Add, remove, and list tasks
2. **Number Guessing Game** — Random number with hints
3. **Simple Calculator** — Full calculator with history
4. **Text Adventure** — Interactive story game
5. **Student Grade Tracker** — Calculate and display grades

### Learn More

- Read the [Example Programs](examples/) in the repository
- Check the [GitHub Repository](https://github.com/jayaswinjay-web/jtsdk) for updates
- Join the community and share your projects

---

## Quick Reference

### Syntax

```jts
# Comments use #

# Variables (dynamic)
x = 10
name = "Alice"

# Variables (type-annotated)
int age = 25
string message = "Hello"
float pi = 3.14
bool flag = true
list nums = [1, 2, 3]
var y = 42

# Unassigned variables (default to void)
int count
string text

# Strings
greeting = "Hello, " + name

# Numbers
result = 5 + 3 * 2

# Conditionals
if x > 10
    say("Big")
elif x > 5
    say("Medium")
else
    say("Small")
end

# Loops
for i of 0 to 10
    say(i)
end

while x > 0
    x = x - 1
end

# Break and Continue
for i of 0 to 10
    if i == 3
        continue
    end
    if i == 7
        break
    end
    say(i)
end

# Functions
func add(a, b)
    return a + b
end

# Lists
nums = [1, 2, 3]
append(nums, 4)
nums.sort()

# Dictionaries
d = {"name": "JTS", "version": "2.0"}
say(d["name"])

# Compound Assignment
x = 10
x += 5
x -= 3

# Try/Catch
try
    throw "error"
catch e
    say(e)
end

# Input/Output
name = ask("Name: ")
say("Hello, " + name)

# File I/O
write_file("out.txt", "data")
content = read_file("out.txt")

# String Methods
s = "hello"
say(s.upper())
say(s.contains("ell"))
```

### Built-in Functions

| Function | Description |
|----------|-------------|
| `say(value)` | Display output |
| `ask(prompt)` | Read input |
| `len(value)` | Get length |
| `type(value)` | Get type |
| `append(list, value)` | Add to list |
| `number(string)` | Convert to number |
| `str(value)` | Convert to string |
| `math(func, x)` | Math functions (sin, cos, tan, sqrt, abs, log, exp, pow, floor, ceil, round) |
| `tensor(data)` | Create a tensor |
| `matrix(data)` | Create a matrix |
| `matmul(a, b)` | Matrix multiplication |
| `sigmoid(x)` | Sigmoid activation |
| `relu(x)` | ReLU activation |
| `mse(predicted, actual)` | Mean squared error |
| `http_server(port)` | Create HTTP server |
| `http_start(server)` | Start HTTP server |
| `http_request(url)` | Make HTTP request |
| `read_file(path)` | Read file contents |
| `write_file(path, data)` | Write to file |

### String Methods

| Method | Description |
|--------|-------------|
| `s.upper()` | Convert to uppercase |
| `s.lower()` | Convert to lowercase |
| `s.trim()` | Remove whitespace |
| `s.contains(sub)` | Check substring |
| `s.replace(old, new)` | Replace text |
| `s.substring(start, end)` | Extract substring |
| `s.starts_with(prefix)` | Check prefix |
| `s.ends_with(suffix)` | Check suffix |

### List Methods

| Method | Description |
|--------|-------------|
| `lst.sort()` | Sort list |
| `lst.append(value)` | Add element |
| `lst.remove(value)` | Remove element |
| `lst.pop()` | Remove last element |

### Operators

| Operator | Description |
|----------|-------------|
| `+` | Add/Concatenate |
| `-` | Subtract |
| `*` | Multiply |
| `/` | Divide |
| `%` | Modulo |
| `==` | Equal |
| `!=` | Not equal |
| `>` | Greater than |
| `<` | Less than |
| `>=` | Greater or equal |
| `<=` | Less or equal |
| `and` | Logical AND |
| `or` | Logical OR |

---

<p align="center">
  Made with passion by <b>JayTech Solutions</b><br>
  JTS GO v0.9.0-beta — 2026
</p>