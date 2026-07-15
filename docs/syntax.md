# Syntax Overview

JTS GO uses clean, Python-inspired syntax with indentation-based blocks. This page covers the fundamental syntax rules.

## Comments

Use `#` to write comments. Comments run until the end of the line.

```jts
# This is a comment
name = "JTS"  # This is an inline comment
```

Comments are ignored by the compiler. Use them to explain your code.

## Code Blocks and the `end` Keyword

JTS GO uses indentation to define code blocks, terminated by the `end` keyword. Every block that starts with `if`, `elif`, `while`, `for`, `func`, `class`, `try`, or `catch` must end with `end`.

```jts
if temperature > 80
    print("It's hot!")
end
```

**Rules:**
- Use 4 spaces (recommended) or tabs for indentation — be consistent
- Every opened block (`if`, `while`, `for`, `func`) **must** be closed with `end`
- Blocks can be nested inside other blocks

```jts
func check_age(age)
    if age >= 18
        print("Adult")
    else
        print("Minor")
    end
end
```

## Variable Assignment

Variables are created by assigning a value. No keywords like `var`, `let`, or `int` are needed.

```jts
name = "Alice"         # string
age = 25               # number (integer)
price = 9.99           # number (float)
is_active = true       # boolean
result = nil           # nil
```

**Rules:**
- Variable names can contain letters, digits, and underscores
- Variable names must start with a letter or underscore
- Variable names are case-sensitive (`age` and `Age` are different)
- Assignment uses a single `=` sign

```jts
# Valid variable names
count = 10
_user_name = "admin"
max_score = 100
item2 = "second"

# Invalid variable names (these would cause errors)
# 2item = "bad"       # can't start with a digit
# my-var = "bad"      # can't use hyphens
# my var = "bad"      # can't use spaces
```

## Strings

Strings are written with double quotes.

```jts
greeting = "Hello, World!"
empty = ""
```

### String Concatenation

Use the `+` operator to join strings together.

```jts
first = "Hello"
second = "World"
message = first + " " + second
print(message)    # Hello World
```

When you concatenate a string with a number, the number is automatically converted to a string.

```jts
age = 25
print("I am " + age + " years old")    # I am 25 years old
```

### Parentheses in Expressions

Use parentheses to control the order of operations or to make concatenation clearer.

```jts
a = 10
b = 3
print("a + b = " + (a + b))        # a + b = 13
print("(a + b) * 2 = " + ((a + b) * 2))   # (a + b) * 2 = 26
```

### Compound Assignment

Shorthand operators for modifying variables:

```jts
x = 10
x += 5      # Same as: x = x + 5 → 15
x -= 3      # Same as: x = x - 3 → 12
x *= 2      # Same as: x = x * 2 → 24
```

## Elif (Else If)

Chain multiple conditions with `elif`:

```jts
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

## Dictionaries

Dictionaries store key-value pairs:

```jts
d = {"name": "JTS", "version": "2.0"}
print(d["name"])    # JTS
```

## Try/Catch/Throw

Handle errors gracefully with try/catch:

```jts
try
    throw "Something went wrong!"
catch e
    print("Caught: " + e)
end
```

## Built-in Functions

JTS GO provides several built-in functions you can use right away.

### print()

Outputs a value to the console.

```jts
print("Hello!")
print(42)
print(true)
print(nil)
```

### input()

Reads a line of text from the user. Returns a string.

```jts
name = input("Enter your name: ")
print("Hello, " + name + "!")
```

### len()

Returns the length of a string.

```jts
print(len("JTS"))         # 3
print(len("Hello"))       # 5
print(len(""))            # 0
```

### type()

Returns the type of a value as a string.

```jts
print(type(42))           # number
print(type("hello"))      # string
print(type(true))         # boolean
print(type(nil))          # nil
```

## Putting It All Together

Here is a small program that uses everything covered on this page:

```jts
# A simple greeting program

print("Welcome to JTS GO!")

# Get user input
name = input("What is your name? ")

# Check the length of the name
name_length = len(name)

# Build and print a message
if name_length > 0
    print("Hello, " + name + "!")
    print("Your name has " + name_length + " characters.")
else
    print("You didn't enter a name!")
end
```

## Next Steps

- [Data Types](types.md) — Learn about numbers, strings, booleans, and nil
- [Control Flow](control-flow.md) — if/else, while, and for loops
- [Functions](functions.md) — Define your own functions
- [Built-in Functions](builtins.md) — Full reference for all built-in functions
