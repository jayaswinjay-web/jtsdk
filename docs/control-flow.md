# Control Flow

Control flow lets you make decisions and repeat actions. JTS GO provides `if/else if/else` for branching, `while` for conditional loops, and `for` for counting loops. All blocks end with `end`.

## Comparison Operators

Use these to compare values. Each returns a boolean (`true` or `false`).

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `==` | Equal to | `5 == 5` | `true` |
| `!=` | Not equal to | `5 != 3` | `true` |
| `>` | Greater than | `5 > 3` | `true` |
| `<` | Less than | `5 < 3` | `false` |
| `>=` | Greater than or equal to | `5 >= 5` | `true` |
| `<=` | Less than or equal to | `5 <= 3` | `false` |

```jts
x = 10

print(x == 10)    # true
print(x != 5)     # true
print(x > 20)     # false
print(x < 20)     # true
print(x >= 10)    # true
print(x <= 9)     # false
```

## Logical Operators

Combine multiple conditions with logical operators.

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `and` | True if **both** sides are true | `true and false` | `false` |
| `or` | True if **at least one** side is true | `true or false` | `true` |
| `not` | Inverts a boolean value | `not true` | `false` |

```jts
age = 25
has_id = true

# Both conditions must be true
if age >= 18 and has_id
    print("Entry allowed")
end

# At least one must be true
if age < 13 or age > 65
    print("Discount applies")
end

# Invert a condition
is_raining = false
if not is_raining
    print("No umbrella needed")
end
```

## Truthiness

When a value is used in a condition, it is evaluated as truthy or falsy:

- **Falsy**: `false`, `nil`, `0`, `""` (empty string)
- **Truthy**: `true`, any non-zero number, any non-empty string

```jts
# These are all falsy
if false
    print("won't print")
end

if nil
    print("won't print")
end

if 0
    print("won't print")
end

if ""
    print("won't print")
end

# These are all truthy
if true
    print("this prints")
end

if 1
    print("this prints")
end

if "hello"
    print("this prints")
end
```

## If / Else If / Else

Use `if` to execute code based on a condition.

### Basic If

```jts
temperature = 75

if temperature > 80
    print("It's hot outside!")
end
```

### If / Else

Add an `else` branch for when the condition is false.

```jts
temperature = 75

if temperature > 80
    print("It's hot outside!")
else
    print("It's nice outside!")
end
```

### If / Else If / Else (Chained)

Chain multiple conditions with `else if`.

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

The conditions are evaluated top to bottom. The first true branch runs, and the rest are skipped.

### Nested If

`if` blocks can be placed inside other `if` blocks.

```jts
age = 25
has_ticket = true

if age >= 18
    if has_ticket
        print("Entry allowed")
    else
        print("Need a ticket")
    end
else
    print("Must be 18 or older")
end
```

## While Loop

A `while` loop repeats a block as long as its condition is true.

### Basic While

```jts
count = 0

while count < 5
    print(count)
    count = count + 1
end

# Output: 0, 1, 2, 3, 4
```

### Countdown

```jts
n = 5

while n > 0
    print(n)
    n = n - 1
end

print("Liftoff!")
# Output: 5, 4, 3, 2, 1, Liftoff!
```

### Important

Always make sure the condition eventually becomes `false`. Otherwise you get an infinite loop:

```jts
# WARNING: This runs forever — don't do this
# while true
#     print("stuck!")
# end
```

## For Loop

A `for` loop iterates over a range of numbers. The syntax is:

```
for VARIABLE in START to END
    ...
end
```

- `START` is the first value (inclusive)
- `END` is the stopping point (**exclusive** — the loop does not include this value)
- `VARIABLE` is the loop counter, available inside the block

### Counting Up

```jts
for i in 0 to 5
    print(i)
end

# Output: 0, 1, 2, 3, 4
```

### Counting from 1

```jts
for i in 1 to 6
    print(i)
end

# Output: 1, 2, 3, 4, 5
```

### Multiplication Table

```jts
for i in 1 to 11
    print("5 x " + i + " = " + (5 * i))
end
```

### Summing a Range

```jts
total = 0
for i in 1 to 101
    total = total + i
end

print("Sum of 1 to 100: " + total)
# Output: Sum of 1 to 100: 5050
```

## Combining Control Flow

You can mix and nest all of these constructs freely.

```jts
# Find even numbers and categorize them
for i in 1 to 21
    if i % 2 == 0
        if i <= 10
            print(i + " is a small even number")
        else
            print(i + " is a large even number")
        end
    end
end
```

## Summary

| Construct | Syntax | Ends With |
|-----------|--------|-----------|
| If | `if CONDITION` | `end` |
| Else If | `else if CONDITION` | `end` |
| Else | `else` | `end` |
| While | `while CONDITION` | `end` |
| For | `for VAR in START to END` | `end` |

## Next Steps

- [Functions](functions.md) — Define reusable blocks of code
- [Built-in Functions](builtins.md) — print, input, len, type
- [Examples](../examples/) — See complete programs
