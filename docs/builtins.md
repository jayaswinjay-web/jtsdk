# Built-in Functions

JTS GO comes with a set of built-in functions that are always available. You do not need to import anything.

## print()

Outputs a value to the console followed by a newline.

### Syntax

```
print(VALUE)
```

### Examples

```jts
# Print a string
print("Hello, World!")

# Print a number
print(42)
print(3.14)

# Print a boolean
print(true)
print(false)

# Print nil
print(nil)

# Print the result of an expression
print(10 + 5)          # 15
print("AB" + "CD")     # ABCD

# Print the result of a function call
print(len("JTS"))      # 3
```

### Building Messages

`print()` is most often used with string concatenation to display formatted messages:

```jts
name = "Alice"
age = 30
print("Name: " + name)
print("Age: " + age)
print(name + " is " + age + " years old")
```

## input()

Reads a line of text from the user's keyboard. Returns the input as a string.

### Syntax

```
input(PROMPT)
```

- `PROMPT` is a string displayed to the user before waiting for input.

### Examples

```jts
# Simple greeting
name = input("Enter your name: ")
print("Hello, " + name + "!")
```

```
Enter your name: Alice
Hello, Alice!
```

```jts
# Reading a number (comes in as a string)
age_str = input("How old are you? ")
print("You said you are " + age_str + " years old")
```

```jts
# Multiple inputs
first = input("First name: ")
last = input("Last name: ")
print("Full name: " + first + " " + last)
```

### Note

`input()` always returns a string. If you need a number, use it in a numeric expression:

```jts
age_str = input("Enter age: ")
# Use the string in a numeric context to convert
age = age_str + 0    # This keeps it as a string in JTS GO
print(type(age))      # string
```

In JTS GO, input is always a string. Use `type()` to inspect it and work with it as needed.

## len()

Returns the length (number of characters) of a string.

### Syntax

```
len(STRING)
```

### Examples

```jts
print(len("JTS"))          # 3
print(len("Hello"))        # 5
print(len(""))             # 0
print(len("Hello World"))  # 11
```

### Practical Uses

```jts
# Validate input
name = input("Enter your name: ")
if len(name) == 0
    print("You didn't enter anything!")
else if len(name) > 20
    print("Name is too long!")
else
    print("Hello, " + name + "!")
end
```

```jts
# Count characters
message = "JTS GO"
print("The message has " + len(message) + " characters")
# The message has 6 characters
```

```jts
# Use len() in a loop
text = "Hello"
i = 0
while i < len(text)
    print(text[i])    # Note: string indexing may not be available
    i = i + 1
end
```

## type()

Returns the type of a value as a string. This is useful for checking what kind of data you are working with.

### Syntax

```
type(VALUE)
```

### Return Values

| Value | type() returns |
|-------|----------------|
| `42` | `"number"` |
| `3.14` | `"number"` |
| `"hello"` | `"string"` |
| `true` | `"boolean"` |
| `false` | `"boolean"` |
| `nil` | `"nil"` |

### Examples

```jts
print(type(42))          # number
print(type(3.14))        # number
print(type("hello"))     # string
print(type(true))        # boolean
print(type(nil))         # nil
```

### Using type() in Conditions

Since `type()` returns a string, you can compare it:

```jts
value = "hello"

if type(value) == "string"
    print("It's a string!")
end

if type(42) == "number"
    print("42 is a number")
end
```

### Checking Multiple Types

```jts
func describe(value)
    t = type(value)
    if t == "string"
        print("String of length " + len(value))
    else if t == "number"
        print("A number")
    else if t == "boolean"
        if value
            print("Boolean: true")
        else
            print("Boolean: false")
        end
    else
        print("nil")
    end
end

describe("hello")    # String of length 5
describe(42)         # A number
describe(true)       # Boolean: true
describe(nil)        # nil
```

## Combining Built-in Functions

The real power comes from combining these functions together.

```jts
# Interactive calculator
a_str = input("Enter first number: ")
b_str = input("Enter second number: ")

# Note: input() returns strings. Arithmetic works because
# JTS GO handles number-string interactions.
print("You entered: " + a_str + " and " + b_str)
print("Type of first input: " + type(a_str))
```

```jts
# Validate and process a name
name = input("Enter your name: ")

if type(name) == "string"
    if len(name) > 0
        print("Welcome, " + name + "!")
        print("Your name has " + len(name) + " characters.")
    else
        print("Please enter a name.")
    end
end
```

```jts
# Quick type-checking utility
func print_type_info(value)
    print("Value: " + value)
    print("Type: " + type(value))
end

print_type_info("hello")
print_type_info(42)
print_type_info(true)
```

## Summary

| Function | Purpose | Returns |
|----------|---------|---------|
| `print(value)` | Output to console | Nothing (nil) |
| `input(prompt)` | Read user input | String |
| `len(string)` | Count characters | Number |
| `type(value)` | Check data type | String |

## Next Steps

- [Syntax Overview](syntax.md) — Review the language basics
- [Data Types](types.md) — Understand all available types
- [Functions](functions.md) — Define your own functions
