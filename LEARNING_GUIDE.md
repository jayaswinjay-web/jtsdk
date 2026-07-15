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
13. [Object-Oriented Programming](#object-oriented-programming)
14. [ML/AI Functions](#mlai-functions)
15. [Web Development](#web-development)
16. [Built-in Functions](#built-in-functions)
17. [Example Programs](#example-programs)
18. [Next Steps](#next-steps)

---

## Introduction

Welcome to JTS GO! This guide will teach you everything you need to know to start programming in JTS GO.

**What is JTS GO?**

JTS GO is a programming language designed for beginners. It combines:

- **Python-like syntax** — easy to read and write
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

You should see the version number (e.g., `1.1.0`).

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
print("Hello, World!")
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
nothing = nil          # Null/Empty
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

**Unassigned variables (default to nil):**
```jts
int count
string message
float temperature
bool is_ready
list items

print(count)    # nil
print(message)  # nil
```

### Data Types

JTS GO has these data types:

| Type | Description | Example |
|------|-------------|---------|
| **string** | Text | `"hello"`, `"JTS GO"` |
| **number** | Numbers (integers and decimals) | `42`, `3.14` |
| **bool** | True or false | `true`, `false` |
| **nil** | Nothing/empty | `nil` |

### Using Variables

```jts
name = "Alice"
age = 30
height = 5.6
is_student = true

print(name)        # Alice
print(age)         # 30
print(height)      # 5.6
print(is_student)  # true
```

### Reassigning Variables

```jts
x = 10
print(x)    # 10

x = 20
print(x)    # 20
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

print(a + b)    # 13
print(a - b)    # 7
print(a * b)    # 30
print(a / b)    # 3.33333
print(a % b)    # 1
```

### Order of Operations

```jts
result = 2 + 3 * 4    # 14 (multiplication first)
result = (2 + 3) * 4  # 20 (parentheses first)
```

### Compound Assignment

```jts
x = 10
x = x + 5    # x is now 15
x = x * 2    # x is now 30
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
print(message)    # Hello World
```

### String Length

```jts
text = "JTS"
print(len(text))    # 3
```

### String Type

```jts
name = "Alice"
print(type(name))    # string
```

### String Examples

```jts
# Building a sentence
first_name = "John"
last_name = "Doe"
full_name = first_name + " " + last_name
print(full_name)    # John Doe

# Repeated text
line = "-" * 20
print(line)    # --------------------

# Empty string
empty = ""
print(len(empty))    # 0
```

---

## Input and Output

### Output with print()

The `print()` function displays text on the screen:

```jts
print("Hello, World!")
print(42)
print(true)
print(nil)
```

### Multiple Values

```jts
print("Name:", "Alice")
print("Age:", 30)
```

### Input with input()

The `input()` function reads user input:

```jts
name = input("What is your name? ")
print("Hello, " + name + "!")
```

### Smart Input

JTS GO automatically detects the type of input:

```jts
# If user enters "25", age will be a number
age = input("Enter your age: ")
print(type(age))    # number

# If user enters "Alice", name will be a string
name = input("Enter your name: ")
print(type(name))    # string

# If user enters "true", flag will be a boolean
flag = input("Enter true or false: ")
print(type(flag))    # bool
```

---

## Conditionals

Conditionals let your program make decisions.

### If Statement

```jts
age = 18

if age >= 18
    print("You are an adult")
end
```

### If/Else Statement

```jts
age = 15

if age >= 18
    print("You are an adult")
else
    print("You are a minor")
end
```

### If/Else If/Else Statement

```jts
score = 85

if score >= 90
    print("Grade: A")
else if score >= 80
    print("Grade: B")
else if score >= 70
    print("Grade: C")
else if score >= 60
    print("Grade: D")
else
    print("Grade: F")
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
    print("Approved")
end

# OR - at least one condition must be true
if age < 18 or income < 30000
    print("Not eligible")
end
```

---

## Loops

Loops repeat code multiple times.

### While Loop

```jts
count = 0

while count < 5
    print(count)
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
for i in 0 to 5
    print(i)
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
for i in 10 to 0
    print(i)
end
```

### Nested Loops

```jts
for i in 0 to 3
    for j in 0 to 3
        print(i + "," + j)
    end
end
```

### Loop Examples

**Sum of numbers:**
```jts
sum = 0
for i in 1 to 10
    sum = sum + i
end
print(sum)    # 55
```

**Counting occurrences:**
```jts
count = 0
for i in 0 to 10
    if i % 2 == 0
        count = count + 1
    end
end
print(count)    # 6 (0, 2, 4, 6, 8, 10)
```

---

## Functions

Functions are reusable blocks of code.

### Defining a Function

```jts
func greet()
    print("Hello, World!")
end

# Call the function
greet()
```

### Function with Parameters

```jts
func greet(name)
    print("Hello, " + name + "!")
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
print(result)    # 7
```

### Multiple Parameters

```jts
func calculate_area(length, width)
    return length * width
end

area = calculate_area(5, 3)
print(area)    # 15
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
print(result)    # 25 (9 + 16)
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

print(factorial(5))    # 120
print(factorial(10))   # 3628800
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

for i in 0 to 10
    print(fib(i))
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

print(fruits[0])    # apple (first element)
print(fruits[1])    # banana (second element)
print(fruits[2])    # cherry (third element)
```

**Note:** Lists start at index 0, not 1.

### Modifying Elements

```jts
fruits = ["apple", "banana", "cherry"]

fruits[1] = "blueberry"
print(fruits)    # [apple, blueberry, cherry]
```

### Adding Elements

```jts
numbers = [1, 2, 3]

append(numbers, 4)
print(numbers)    # [1, 2, 3, 4]

append(numbers, 5)
print(numbers)    # [1, 2, 3, 4, 5]
```

### List Length

```jts
numbers = [1, 2, 3, 4, 5]
print(len(numbers))    # 5
```

### List Type

```jts
numbers = [1, 2, 3]
print(type(numbers))    # list
```

### Looping Through Lists

```jts
fruits = ["apple", "banana", "cherry"]

for i in 0 to len(fruits) - 1
    print(fruits[i])
end
```

### List Examples

**Find the maximum:**
```jts
func find_max(list)
    max = list[0]
    for i in 1 to len(list) - 1
        if list[i] > max
            max = list[i]
        end
    end
    return max
end

numbers = [3, 7, 2, 9, 4]
print(find_max(numbers))    # 9
```

**Sum of list:**
```jts
func sum_list(list)
    total = 0
    for i in 0 to len(list) - 1
        total = total + list[i]
    end
    return total
end

numbers = [1, 2, 3, 4, 5]
print(sum_list(numbers))    # 15
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
        print(self.name + " makes a sound")
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
print(a.name)   # Dog
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
print(calc.get())   # 8
```

### Inheritance

```jts
class Animal
    func init(self, name)
        self.name = name
    end

    func speak(self)
        print(self.name + " makes a sound")
    end
end

class Dog extends Animal
    func bark(self)
        print(self.name + " barks!")
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

## ML/AI Functions

JTS GO includes built-in ML/AI functions for numerical computing.

### Tensors

```jts
t = tensor([1, 2, 3, 4, 5])
print(t)       # [1, 2, 3, 4, 5]
print(len(t))  # 5
```

### Matrices

```jts
m1 = matrix([[1, 2], [3, 4]])
m2 = matrix([[5, 6], [7, 8]])
result = matmul(m1, m2)
print(result)   # [[19, 22] [43, 50]]
```

### Activation Functions

```jts
print(sigmoid(0))    # 0.5
print(sigmoid(1))    # 0.731...
print(relu(-5))      # 0
print(relu(5))       # 5
```

### Loss Functions

```jts
predicted = [1.0, 2.0, 3.0]
actual = [1.1, 2.2, 3.1]
print(mse(predicted, actual))   # 0.02
```

### Math Functions

```jts
print(sqrt(16))              # 4
print(math("sin", 3.14159))  # ~0
print(math("cos", 0))        # 1
print(math("floor", 3.7))    # 3
print(math("ceil", 3.2))     # 4
print(math("abs", -42))      # 42
print(math("log", 2.71828))  # ~1
print(math("exp", 1))        # ~2.718
```

---

## Web Development

JTS GO includes HTTP server support for web development.

### Creating a Server

```jts
server = http_server(8080)
http_start(server)
```

### Making HTTP Requests

```jts
response = http_request("https://api.example.com/data")
print(response)   # [200, "OK"]
```

---

## Built-in Functions

JTS GO comes with these built-in functions:

### print(value)

Displays a value on the screen:

```jts
print("Hello")
print(42)
print(true)
print(nil)
print([1, 2, 3])
```

### input(prompt)

Reads user input:

```jts
name = input("Enter your name: ")
age = input("Enter your age: ")
```

### len(value)

Returns the length of a string or list:

```jts
print(len("hello"))        # 5
print(len([1, 2, 3]))     # 3
print(len([]))             # 0
```

### type(value)

Returns the type of a value:

```jts
print(type("hello"))    # string
print(type(42))         # number
print(type(true))       # bool
print(type(nil))        # nil
print(type([1, 2]))     # list
```

### append(list, value)

Adds an element to a list:

```jts
nums = [1, 2, 3]
append(nums, 4)
print(nums)    # [1, 2, 3, 4]
```

### number(string)

Converts a string to a number:

```jts
num = number("42")
print(type(num))    # number
print(num + 8)      # 50
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

num1 = input("Enter first number: ")
num2 = input("Enter second number: ")

print("Sum: " + add(num1, num2))
print("Difference: " + subtract(num1, num2))
print("Product: " + multiply(num1, num2))
print("Quotient: " + divide(num1, num2))
```

### 2. FizzBuzz

```jts
for i in 1 to 100
    if i % 15 == 0
        print("FizzBuzz")
    else if i % 3 == 0
        print("Fizz")
    else if i % 5 == 0
        print("Buzz")
    else
        print(i)
    end
end
```

### 3. Guessing Game

```jts
secret = 42
guess = 0

while guess != secret
    guess = input("Guess the number: ")
    if guess < secret
        print("Too low!")
    else if guess > secret
        print("Too high!")
    else
        print("Congratulations! You got it!")
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

num = input("Enter a number: ")
result = factorial(num)
print("Factorial: " + result)
```

### 5. List Statistics

```jts
func average(list)
    total = 0
    for i in 0 to len(list) - 1
        total = total + list[i]
    end
    return total / len(list)
end

func find_max(list)
    max = list[0]
    for i in 1 to len(list) - 1
        if list[i] > max
            max = list[i]
        end
    end
    return max
end

func find_min(list)
    min = list[0]
    for i in 1 to len(list) - 1
        if list[i] < min
            min = list[i]
        end
    end
    return min
end

numbers = [23, 45, 12, 67, 89, 34, 56]

print("List: " + numbers)
print("Average: " + average(numbers))
print("Maximum: " + find_max(numbers))
print("Minimum: " + find_min(numbers))
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

# Unassigned variables (default to nil)
int count
string text

# Strings
greeting = "Hello, " + name

# Numbers
result = 5 + 3 * 2

# Conditionals
if x > 10
    print("Big")
else
    print("Small")
end

# Loops
for i in 0 to 10
    print(i)
end

while x > 0
    x = x - 1
end

# Functions
func add(a, b)
    return a + b
end

# Lists
nums = [1, 2, 3]
append(nums, 4)

# Input/Output
name = input("Name: ")
print("Hello, " + name)
```

### Built-in Functions

| Function | Description |
|----------|-------------|
| `print(value)` | Display output |
| `input(prompt)` | Read input |
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
  Made with passion by <b>Jayaswin Jay</b><br>
  JTS GO v2.0.0 — 2026
</p>