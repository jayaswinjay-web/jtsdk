# Built-in Functions

JTS GO comes with a set of built-in functions that are always available. You do not need to import anything.

## say()

Outputs a value to the console followed by a newline.

### Syntax

```
say(VALUE)
```

### Examples

```jts
# Print a string
say("Hello, World!")

# Print a number
say(42)
say(3.14)

# Print a boolean
say(true)
say(false)

# Print void
say(void)

# Print the result of an expression
say(10 + 5)          # 15
say("AB" + "CD")     # ABCD

# Print the result of a function call
say(len("JTS"))      # 3
```

### Building Messages

`say()` is most often used with string concatenation to display formatted messages:

```jts
name = "Alice"
age = 30
say("Name: " + name)
say("Age: " + age)
say(name + " is " + age + " years old")
```

## ask()

Reads a line of text from the user's keyboard. Returns the input as a string.

### Syntax

```
ask(PROMPT)
```

- `PROMPT` is a string displayed to the user before waiting for input.

### Examples

```jts
# Simple greeting
name = ask("Enter your name: ")
say("Hello, " + name + "!")
```

```
Enter your name: Alice
Hello, Alice!
```

```jts
# Reading a number (comes of as a string)
age_str = ask("How old are you? ")
say("You said you are " + age_str + " years old")
```

```jts
# Multiple inputs
first = ask("First name: ")
last = ask("Last name: ")
say("Full name: " + first + " " + last)
```

### Note

`ask()` always returns a string. If you need a number, use it in a numeric expression:

```jts
age_str = ask("Enter age: ")
# Use the string of a numeric context to convert
age = age_str + 0    # This keeps it as a string of JTS GO
say(type(age))      # string
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
say(len("JTS"))          # 3
say(len("Hello"))        # 5
say(len(""))             # 0
say(len("Hello World"))  # 11
```

### Practical Uses

```jts
# Validate ask
name = ask("Enter your name: ")
if len(name) == 0
    say("You didn't enter anything!")
elif len(name) > 20
    say("Name is too long!")
else
    say("Hello, " + name + "!")
end
```

```jts
# Count characters
message = "JTS GO"
say("The message has " + len(message) + " characters")
# The message has 6 characters
```

```jts
# Use len() of a loop
text = "Hello"
i = 0
while i < len(text)
    say(text[i])    # Note: string indexing may not be available
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
| `void` | `"void"` |

### Examples

```jts
say(type(42))          # number
say(type(3.14))        # number
say(type("hello"))     # string
say(type(true))        # boolean
say(type(void))         # void
```

### Using type() in Conditions

Since `type()` returns a string, you can compare it:

```jts
value = "hello"

if type(value) == "string"
    say("It's a string!")
end

if type(42) == "number"
    say("42 is a number")
end
```

### Checking Multiple Types

```jts
func describe(value)
    t = type(value)
    if t == "string"
        say("String of length " + len(value))
    elif t == "number"
        say("A number")
    elif t == "boolean"
        if value
            say("Boolean: true")
        else
            say("Boolean: false")
        end
    else
        say("void")
    end
end

describe("hello")    # String of length 5
describe(42)         # A number
describe(true)       # Boolean: true
describe(void)        # void
```

## Combining Built-in Functions

The real power comes from combining these functions together.

```jts
# Interactive calculator
a_str = ask("Enter first number: ")
b_str = ask("Enter second number: ")

# Note: ask() returns strings. Arithmetic works because
# JTS GO handles number-string interactions.
say("You entered: " + a_str + " and " + b_str)
say("Type of first ask: " + type(a_str))
```

```jts
# Validate and process a name
name = ask("Enter your name: ")

if type(name) == "string"
    if len(name) > 0
        say("Welcome, " + name + "!")
        say("Your name has " + len(name) + " characters.")
    else
        say("Please enter a name.")
    end
end
```

```jts
# Quick type-checking utility
func print_type_info(value)
    say("Value: " + value)
    say("Type: " + type(value))
end

print_type_info("hello")
print_type_info(42)
print_type_info(true)
```

## Summary

| Function | Purpose | Returns |
|----------|---------|---------|
| `say(value)` | Output to console | Nothing (void) |
| `ask(prompt)` | Read user input | String |
| `len(value)` | Count characters (string) or elements (list/dict/set) | Number |
| `type(value)` | Check data type | String |
| `str(value)` / `string(value)` | Convert any value to string | String |
| `number(value)` / `float(value)` | Convert string to number | Number |
| `int(value)` | Convert to integer (truncates) | Number |
| `bool(value)` | Convert to boolean | Boolean |
| `list(value)` | Convert string/set/tensor to list | List |
| `read_file(path)` | Read file contents | String |
| `write_file(path, data)` | Write data to file | Nothing (void) |

## Conversion Functions

These functions convert values between different types.

### str(value) / string(value)

Converts any value to its string representation.

```jts
say(str(123))           # 123
say(str(true))          # true
say(str(void))           # void
say(str([1, 2, 3]))     # [1, 2, 3]
```

### number(value) / float(value)

Converts a string to a floating-point number.

```jts
say(number("42"))       # 42
say(number("3.5"))      # 3.5
say(float("3.5"))       # 3.5
```

### int(value)

Converts a value to an integer, truncating any fractional part.

```jts
say(int("42"))          # 42
say(int(3.99))          # 3
say(int("3.7"))         # 3
```

### bool(value)

Converts a value to a boolean following truthiness rules.

```jts
say(bool("x"))          # true
say(bool(""))           # false
say(bool(0))            # false
say(bool(1))            # true
```

### list(value)

Converts a string, set, tensor, or list to a new list.

```jts
say(list("abc"))        # [a, b, c]
say(list({1, 2, 3}))    # [1, 2, 3]
say(list([1, 2]))       # [1, 2] (copy)
```

## String Methods

String methods are called directly on string variables using dot notation.

### upper()
```jts
s = "hello"
say(s.upper())    # HELLO
```

### lower()
```jts
s = "HELLO"
say(s.lower())    # hello
```

### trim()
```jts
s = "  hello  "
say(s.trim())     # hello
```

### lstrip()
```jts
s = "  hello  "
say(s.lstrip())   # hello
```

### rstrip()
```jts
s = "  hello  "
say(s.rstrip())   # hello
```

### contains(substring)
```jts
s = "hello world"
say(s.contains("world"))    # true
say(s.contains("xyz"))      # false
```

### replace(old, new)
```jts
s = "hello world"
say(s.replace("world", "JTS"))    # hello JTS
```

### substring(start, end)
```jts
s = "hello"
say(s.substring(0, 3))    # hel
say(s.substring(1, 4))    # ell
```

### starts_with(prefix)
```jts
s = "hello world"
say(s.starts_with("hello"))    # true
say(s.starts_with("world"))    # false
```

### ends_with(suffix)
```jts
s = "hello world"
say(s.ends_with("world"))    # true
say(s.ends_with("hello"))    # false
```

### ljust(width)
```jts
s = "hi"
say(s.ljust(5))     # hi
```

### rjust(width)
```jts
s = "hi"
say(s.rjust(5))     # hi
```
