# Data Types

JTS GO is dynamically typed. You do not declare a variable's type — it is determined automatically at runtime based on the value you assign.

## Overview

| Type | Description | Example |
|------|-------------|---------|
| **number** | Integers and floating-point numbers | `42`, `3.14`, `-7` |
| **string** | Text enclosed in double quotes | `"hello"`, `""` |
| **boolean** | True or false | `true`, `false` |
| **void** | Absence of a value | `void` |
| **list** | Ordered collection of values | `[1, 2, 3]`, `["a", "b"]` |
| **dict** | Key-value pairs | `{"name": "JTS", "version": "2.0"}` |

## Numbers

JTS GO has a single number type that covers both integers and floating-point values.

```jts
# Integers
count = 42
negative = -10
zero = 0

# Floating-point
price = 19.99
pi = 3.14159
```

### Arithmetic Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `+` | Addition | `10 + 3` | `13` |
| `-` | Subtraction | `10 - 3` | `7` |
| `*` | Multiplication | `10 * 3` | `30` |
| `/` | Division | `10 / 3` | `3.33333` |
| `%` | Modulo (remainder) | `10 % 3` | `1` |
| `-` (unary) | Negation | `-5` | `-5` |

```jts
a = 10
b = 3

say(a + b)     # 13
say(a - b)     # 7
say(a * b)     # 30
say(a / b)     # 3.33333
say(a % b)     # 1
```

### Operator Precedence

Multiplication and division are evaluated before addition and subtraction, as in standard math.

```jts
say(2 + 3 * 4)       # 14  (not 20)
say((2 + 3) * 4)     # 20
```

## Strings

Strings are sequences of characters enclosed in double quotes.

```jts
name = "JTS GO"
empty = ""
greeting = "Hello, World!"
```

### String Length

Use `len()` to get the number of characters in a string.

```jts
say(len("JTS"))       # 3
say(len(""))          # 0
say(len("hello"))     # 5
```

### String Concatenation

The `+` operator joins two strings together.

```jts
first = "Hello"
second = "World"
result = first + " " + second
say(result)    # Hello World
```

### Auto-Conversion (String + Number)

When you concatenate a string with a number using `+`, the number is automatically converted to its string representation.

```jts
age = 25
say("Age: " + age)          # Age: 25

price = 19.99
say("Price: $" + price)     # Price: $19.99

count = 0
say("Items: " + count)      # Items: 0
```

This makes it easy to build messages without explicit conversion.

## Booleans

Booleans represent truth values: `true` or `false`.

```jts
is_active = true
is_deleted = false
```

Booleans are commonly used with comparison and logical operators, and in if/else conditions.

```jts
age = 25
is_adult = age >= 18
say(is_adult)    # true
```

### Comparison Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `==` | Equal to | `5 == 5` | `true` |
| `!=` | Not equal to | `5 != 3` | `true` |
| `>` | Greater than | `5 > 3` | `true` |
| `<` | Less than | `5 < 3` | `false` |
| `>=` | Greater than or equal to | `5 >= 5` | `true` |
| `<=` | Less than or equal to | `5 <= 3` | `false` |

### Logical Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `and` | Both conditions must be true | `true and false` | `false` |
| `or` | At least one condition must be true | `true or false` | `true` |
| `not` | Inverts a boolean value | `not true` | `false` |

```jts
x = 10
if x > 5 and x < 20
    say("x is between 5 and 20")
end
```

## Nil

`void` represents the absence of a value. It is similar to `null` in other languages.

```jts
result = void
say(result)       # void
say(type(result)) # void
```

Nil is falsy — it evaluates to `false` in conditions.

```jts
value = void
if value
    say("This will NOT say")
else
    say("value is void")
end
```

## Checking Types

Use the `type()` function to check the type of any value at runtime.

```jts
say(type(42))          # number
say(type(3.14))        # number
say(type("hello"))     # string
say(type(true))        # boolean
say(type(void))         # void
```

`type()` returns a string that you can use in comparisons:

```jts
value = "hello"
if type(value) == "string"
    say("value is a string")
end
```

## Type Conversion Functions

JTS GO provides functions to convert between types. These are useful when you need explicit control over how values are converted.

### str(value) / string(value)

Converts any value to its string representation.

```jts
say(str(123))           # "123"
say(str(true))          # "true"
say(str(void))           # "void"
say(str([1, 2, 3]))     # "[1, 2, 3]"
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

Converts a value to a boolean following truthiness rules (0, void, and empty string are falsy; everything else is truthy).

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

## Truthiness

In JTS GO, values are considered "truthy" or "falsy" when used in conditions:

| Value | Truthiness |
|-------|------------|
| `true` | Truthy |
| `false` | Falsy |
| `void` | Falsy |
| `0` | Falsy |
| Any other number | Truthy |
| `""` (empty string) | Falsy |
| Any non-empty string | Truthy |

```jts
if 0
    say("This will NOT say — 0 is falsy")
end

if 1
    say("This WILL say — 1 is truthy")
end

if ""
    say("This will NOT say — empty string is falsy")
end

if "hello"
    say("This WILL say — non-empty string is truthy")
end
```

## Lists

Lists store multiple values in an ordered collection.

```jts
numbers = [1, 2, 3, 4, 5]
fruits = ["apple", "banana", "cherry"]
mixed = ["hello", 42, true]
empty = []
```

### Accessing Elements
```jts
fruits = ["apple", "banana", "cherry"]
say(fruits[0])    # apple (first element, index starts at 0)
say(fruits[2])    # cherry
```

### List Methods
```jts
nums = [3, 1, 2]
nums.sort()             # [1, 2, 3]
nums.append(4)          # [1, 2, 3, 4]
nums.remove(3)          # [1, 2, 4]
nums.pop()              # [1, 2]
```

## Dictionaries

Dictionaries store key-value pairs. Keys must be strings.

```jts
d = {"name": "JTS", "version": "2.0"}
say(d)            # {name: JTS, version: 2.0}
say(d["name"])    # JTS
```

### Creating Dictionaries
```jts
# Empty dictionary
empty = {}

# With values
person = {"name": "Alice", "age": 30, "active": true}
say(person["name"])    # Alice
```

## Summary

| Type | Values | Example |
|------|--------|---------|
| number | Integers, floats | `42`, `3.14`, `-7` |
| string | Text in double quotes | `"hello"` |
| boolean | `true`, `false` | `true` |
| void | `void` | `void` |
| list | Ordered collection | `[1, 2, 3]` |
| dict | Key-value pairs | `{"a": 1}` |

## Next Steps

- [Control Flow](control-flow.md) — Use types in conditions and loops
- [Functions](functions.md) — Pass types as arguments and return values
- [Built-in Functions](builtins.md) — Learn about say, ask, len, and type
