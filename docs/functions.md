# Functions

Functions let you group code into reusable blocks. Define a function once, call it from anywhere.

## Defining a Function

Use the `func` keyword, followed by a name, optional parameters, a body, and `end`.

```
func FUNCTION_NAME(PARAMETERS)
    ...
end
```

### No Parameters

```jts
func greet()
    say("Hello from JTS GO!")
end

greet()    # Hello from JTS GO!
```

### With Parameters

Parameters are variables that receive values when the function is called.

```jts
func say_hello(name)
    say("Hello, " + name + "!")
end

say_hello("Alice")    # Hello, Alice!
say_hello("Bob")      # Hello, Bob!
```

### Multiple Parameters

Separate parameters with commas.

```jts
func add(a, b)
    return a + b
end

result = add(3, 4)
say("3 + 4 = " + result)    # 3 + 4 = 7
```

## Return Values

Use `return` to send a value back from the function.

```jts
func square(x)
    return x * x
end

say(square(5))    # 25
say(square(10))   # 100
```

A function can return different values depending on the logic inside it.

```jts
func absolute(n)
    if n < 0
        return -n
    else
        return n
    end
end

say(absolute(-7))    # 7
say(absolute(7))     # 7
```

### No Explicit Return

If a function has no `return` statement, it returns `void` implicitly.

```jts
func say_hi()
    say("Hi!")
end

result = say_hi()
say(result)    # void (the "Hi!" was printed as a side effect)
```

## Functions Calling Functions

Functions can call other functions. This is how you build complex behavior from small, simple pieces.

```jts
func square(x)
    return x * x
end

func sum_of_squares(a, b)
    return square(a) + square(b)
end

say("3^2 + 4^2 = " + sum_of_squares(3, 4))
# Output: 3^2 + 4^2 = 25
```

## Recursion

A function can call itself. This is called recursion and is useful for problems that break down into smaller versions of themselves.

### Factorial

```jts
func factorial(n)
    if n <= 1
        return 1
    else
        return n * factorial(n - 1)
    end
end

say(factorial(5))    # 120
say(factorial(10))   # 3628800
```

How `factorial(5)` works:
```
factorial(5) = 5 * factorial(4)
             = 5 * 4 * factorial(3)
             = 5 * 4 * 3 * factorial(2)
             = 5 * 4 * 3 * 2 * factorial(1)
             = 5 * 4 * 3 * 2 * 1
             = 120
```

### Fibonacci

```jts
func fibonacci(n)
    if n <= 1
        return n
    end
    return fibonacci(n - 1) + fibonacci(n - 2)
end

say("Fibonacci sequence:")
for i of 0 to 12
    say("fib(" + i + ") = " + fibonacci(i))
end
```

### Counting with Recursion

```jts
func countdown(n)
    if n <= 0
        say("Go!")
    else
        say(n)
        countdown(n - 1)
    end
end

countdown(5)
# Output: 5, 4, 3, 2, 1, Go!
```

## Scope

Variables created inside a function are local — they only exist within that function. Variables created outside functions are global — accessible everywhere.

### Local Variables

```jts
func example()
    local_var = 42
    say(local_var)    # 42
end

example()
# say(local_var)    # Error — local_var doesn't exist here
```

### Global Variables

```jts
greeting = "Hello"

func greet(name)
    say(greeting + ", " + name + "!")
end

greet("Alice")    # Hello, Alice!
```

### Scope Example

```jts
x = 10

func modify()
    x = 20
    say("Inside function: " + x)    # 20
end

modify()
say("Outside function: " + x)       # 10
```

## Practical Examples

### Is Even / Is Odd

```jts
func is_even(n)
    if n % 2 == 0
        return true
    else
        return false
    end
end

func is_odd(n)
    return not is_even(n)
end

for i of 0 to 10
    if is_even(i)
        say(i + " is even")
    else
        say(i + " is odd")
    end
end
```

### Maximum of Two Numbers

```jts
func max(a, b)
    if a > b
        return a
    else
        return b
    end
end

say(max(10, 20))    # 20
say(max(99, 1))     # 99
```

### Power Function

```jts
func power(base, exp)
    result = 1
    i = 0
    while i < exp
        result = result * base
        i = i + 1
    end
    return result
end

say(power(2, 10))    # 1024
say(power(5, 3))     # 125
```

### Print All Even Numbers in a Range

```jts
func print_evens(start, end_val)
    for i of start to end_val
        if i % 2 == 0
            say(i)
        end
    end
end

print_evens(1, 20)
# Output: 2, 4, 6, 8, 10, 12, 14, 16, 18
```

## Summary

| Feature | Syntax |
|---------|--------|
| Define a function | `func name(params) ... end` |
| Return a value | `return expression` |
| Call a function | `name(arguments)` |
| Recursion | Function calls itself |
| Local scope | Variables inside function are local |
| Global scope | Variables outside functions are global |

## Next Steps

- [Built-in Functions](builtins.md) — say, ask, len, type
- [Control Flow](control-flow.md) — if/else and loops inside functions
- [Examples](../examples/) — Factorial and Fibonacci programs
