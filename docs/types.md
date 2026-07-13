# Data Types

JTS GO is dynamically typed. You do not declare a variable's type — it is determined automatically at runtime based on the value you assign.

## Overview

| Type | Description | Example |
|------|-------------|---------|
| **number** | Integers and floating-point numbers | `42`, `3.14`, `-7` |
| **string** | Text enclosed in double quotes | `"hello"`, `""` |
| **boolean** | True or false | `true`, `false` |
| **nil** | Absence of a value | `nil` |

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

print(a + b)     # 13
print(a - b)     # 7
print(a * b)     # 30
print(a / b)     # 3.33333
print(a % b)     # 1
```

### Operator Precedence

Multiplication and division are evaluated before addition and subtraction, as in standard math.

```jts
print(2 + 3 * 4)       # 14  (not 20)
print((2 + 3) * 4)     # 20
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
print(len("JTS"))       # 3
print(len(""))          # 0
print(len("hello"))     # 5
```

### String Concatenation

The `+` operator joins two strings together.

```jts
first = "Hello"
second = "World"
result = first + " " + second
print(result)    # Hello World
```

### Auto-Conversion (String + Number)

When you concatenate a string with a number using `+`, the number is automatically converted to its string representation.

```jts
age = 25
print("Age: " + age)          # Age: 25

price = 19.99
print("Price: $" + price)     # Price: $19.99

count = 0
print("Items: " + count)      # Items: 0
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
print(is_adult)    # true
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
    print("x is between 5 and 20")
end
```

## Nil

`nil` represents the absence of a value. It is similar to `null` in other languages.

```jts
result = nil
print(result)       # nil
print(type(result)) # nil
```

Nil is falsy — it evaluates to `false` in conditions.

```jts
value = nil
if value
    print("This will NOT print")
else
    print("value is nil")
end
```

## Checking Types

Use the `type()` function to check the type of any value at runtime.

```jts
print(type(42))          # number
print(type(3.14))        # number
print(type("hello"))     # string
print(type(true))        # boolean
print(type(nil))         # nil
```

`type()` returns a string that you can use in comparisons:

```jts
value = "hello"
if type(value) == "string"
    print("value is a string")
end
```

## Truthiness

In JTS GO, values are considered "truthy" or "falsy" when used in conditions:

| Value | Truthiness |
|-------|------------|
| `true` | Truthy |
| `false` | Falsy |
| `nil` | Falsy |
| `0` | Falsy |
| Any other number | Truthy |
| `""` (empty string) | Falsy |
| Any non-empty string | Truthy |

```jts
if 0
    print("This will NOT print — 0 is falsy")
end

if 1
    print("This WILL print — 1 is truthy")
end

if ""
    print("This will NOT print — empty string is falsy")
end

if "hello"
    print("This WILL print — non-empty string is truthy")
end
```

## Summary

| Type | Values | Example |
|------|--------|---------|
| number | Integers, floats | `42`, `3.14`, `-7` |
| string | Text in double quotes | `"hello"` |
| boolean | `true`, `false` | `true` |
| nil | `nil` | `nil` |

## Next Steps

- [Control Flow](control-flow.md) — Use types in conditions and loops
- [Functions](functions.md) — Pass types as arguments and return values
- [Built-in Functions](builtins.md) — Learn about print, input, len, and type
