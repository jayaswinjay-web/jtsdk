# Control Flow

Control flow lets you make decisions and repeat actions. JTS GO provides `if/elif/else` for branching, `while` for conditional loops, `for` for counting loops, and `break`/`continue` for loop control. All blocks end with `end`.

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

say(x == 10)    # true
say(x != 5)     # true
say(x > 20)     # false
say(x < 20)     # true
say(x >= 10)    # true
say(x <= 9)     # false
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
    say("Entry allowed")
end

# At least one must be true
if age < 13 or age > 65
    say("Discount applies")
end

# Invert a condition
is_raining = false
if not is_raining
    say("No umbrella needed")
end
```

## Truthiness

When a value is used in a condition, it is evaluated as truthy or falsy:

- **Falsy**: `false`, `void`, `0`, `""` (empty string)
- **Truthy**: `true`, any non-zero number, any non-empty string

```jts
# These are all falsy
if false
    say("won't say")
end

if void
    say("won't say")
end

if 0
    say("won't say")
end

if ""
    say("won't say")
end

# These are all truthy
if true
    say("this prints")
end

if 1
    say("this prints")
end

if "hello"
    say("this prints")
end
```

## If / Else If / Else

Use `if` to execute code based on a condition.

### Basic If

```jts
temperature = 75

if temperature > 80
    say("It's hot outside!")
end
```

### If / Else

Add an `else` branch for when the condition is false.

```jts
temperature = 75

if temperature > 80
    say("It's hot outside!")
else
    say("It's nice outside!")
end
```

### If / Elif / Else (Chained)

Chain multiple conditions with `elif`.

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

The conditions are evaluated top to bottom. The first true branch runs, and the rest are skipped.

### Nested If

`if` blocks can be placed inside other `if` blocks.

```jts
age = 25
has_ticket = true

if age >= 18
    if has_ticket
        say("Entry allowed")
    else
        say("Need a ticket")
    end
else
    say("Must be 18 or older")
end
```

## While Loop

A `while` loop repeats a block as long as its condition is true.

### Basic While

```jts
count = 0

while count < 5
    say(count)
    count = count + 1
end

# Output: 0, 1, 2, 3, 4
```

### Countdown

```jts
n = 5

while n > 0
    say(n)
    n = n - 1
end

say("Liftoff!")
# Output: 5, 4, 3, 2, 1, Liftoff!
```

### Important

Always make sure the condition eventually becomes `false`. Otherwise you get an infinite loop:

```jts
# WARNING: This runs forever — don't do this
# while true
#     say("stuck!")
# end
```

## For Loop

A `for` loop iterates over a range of numbers. The syntax is:

```
for VARIABLE of START to END
    ...
end
```

- `START` is the first value (inclusive)
- `END` is the stopping point (**exclusive** — the loop does not include this value)
- `VARIABLE` is the loop counter, available inside the block

### Counting Up

```jts
for i of 0 to 5
    say(i)
end

# Output: 0, 1, 2, 3, 4
```

### Counting from 1

```jts
for i of 1 to 6
    say(i)
end

# Output: 1, 2, 3, 4, 5
```

### Multiplication Table

```jts
for i of 1 to 11
    say("5 x " + i + " = " + (5 * i))
end
```

### Summing a Range

```jts
total = 0
for i of 1 to 101
    total = total + i
end

say("Sum of 1 to 100: " + total)
# Output: Sum of 1 to 100: 5050
```

## Combining Control Flow

You can mix and nest all of these constructs freely.

```jts
# Find even numbers and categorize them
for i of 1 to 21
    if i % 2 == 0
        if i <= 10
            say(i + " is a small even number")
        else
            say(i + " is a large even number")
        end
    end
end
```

## Break and Continue

Use `break` to exit a loop early, and `continue` to skip to the next iteration.

### Break

```jts
for i of 0 to 10
    if i == 5
        break
    end
    say(i)
end
# Output: 0, 1, 2, 3, 4
```

### Continue

```jts
for i of 0 to 6
    if i == 3
        continue
    end
    say(i)
end
# Output: 0, 1, 2, 4, 5
```

### Break and Continue in While Loops

```jts
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

## Summary

| Construct | Syntax | Ends With |
|-----------|--------|-----------|
| If | `if CONDITION` | `end` |
| Elif | `elif CONDITION` | `end` |
| Else | `else` | `end` |
| While | `while CONDITION` | `end` |
| For | `for VAR of START to END` | `end` |
| Break | `break` | — |
| Continue | `continue` | — |

## Next Steps

- [Functions](functions.md) — Define reusable blocks of code
- [Built-in Functions](builtins.md) — say, ask, len, type
- [Examples](../examples/) — See complete programs
