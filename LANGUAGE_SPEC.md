# JTS GO Language Specification v2.0.2

This document is the complete, formal specification of the JTS GO programming language. It covers every keyword, operator, type, function, and semantic rule. AI language models can use this document to generate correct JTS GO code.

---

## Table of Contents

1. [Lexical Structure](#1-lexical-structure)
2. [Types and Values](#2-types-and-values)
3. [Variables](#3-variables)
4. [Operators](#4-operators)
5. [Control Flow](#5-control-flow)
6. [Functions](#6-functions)
7. [Lists](#7-lists)
8. [Dictionaries](#8-dictionaries)
9. [Strings](#9-strings)
10. [Object-Oriented Programming](#10-object-oriented-programming)
11. [Error Handling](#11-error-handling)
12. [Built-in Functions](#12-built-in-functions)
13. [String Methods](#13-string-methods)
14. [List Methods](#14-list-methods)
15. [File I/O](#15-file-io)
16. [Web Development](#16-web-development)
17. [ML/AI](#17-mlai)
18. [Operator Precedence](#18-operator-precedence)
19. [VM Limits](#19-vm-limits)
20. [Grammar (EBNF)](#20-grammar-ebnf)

---

## 1. Lexical Structure

### 1.1 Comments

```
# This is a comment — extends to end of line
```

### 1.2 Indentation

JTS uses indentation-based blocks (like Python). The scanner tracks an indent stack. Use spaces consistently (4 spaces recommended). Blocks opened with `if`, `elif`, `while`, `for`, `func`, `class`, `try`, `catch` must be closed with `end`.

### 1.3 Keywords

All keywords are lowercase:

```
and         break       catch       class       continue
elif        else        end         extends     false
for         func        if          in          import
input       len         list        new         nil
not         or          print       return      self
super       throw       to          true        try
type        var         while       append      int
float       string      bool        number
```

Reserved but unused in user code: `server`, `request`, `response`, `train`, `model`, `http`, `tensor`, `matrix`.

### 1.4 Literals

| Literal | Example | Type |
|---------|---------|------|
| Number | `42`, `3.14`, `-7`, `0` | `number` (always 64-bit float) |
| String | `"hello"`, `""`, `"hello\nworld"` | `string` |
| Boolean | `true`, `false` | `boolean` |
| Nil | `nil` | `nil` |
| List | `[1, 2, 3]`, `[]` | `list` |
| Dict | `{"key": "val"}` | `dict` |

### 1.5 Identifiers

- Must start with a letter or underscore
- Can contain letters, digits, underscores
- Case-sensitive: `name` ≠ `Name`
- Cannot be a keyword

### 1.6 Operators and Delimiters

```
+    -    *    /    %    =    ==
!=   <    <=   >    >=   +=   -=
(    )    [    ]    {    }    .
,    :    .
```

### 1.7 String Literals

Strings use double quotes. No escape sequences are supported (no `\"`, `\n`, etc. in the scanner — they are treated as literal backslash + character).

---

## 2. Types and Values

### 2.1 Type System

JTS is dynamically typed. Every value has a runtime type.

| Type | `type()` returns | Description |
|------|-----------------|-------------|
| `nil` | `"nil"` | Absence of a value |
| `boolean` | `"boolean"` | `true` or `false` |
| `number` | `"number"` | All numbers are 64-bit IEEE 754 doubles |
| `string` | `"string"` | Sequence of characters |
| `list` | `"list"` | Ordered collection, 0-indexed |
| `dict` | `"dict"` | Key-value pairs (keys are strings) |
| `function` | `"function"` | User-defined or native function |
| `class` | `"class"` | Class definition |
| `instance` | `"instance"` | Instance of a class |
| `tensor` | `"tensor"` | N-dimensional numeric array |
| `matrix` | `"matrix"` | 2D numeric array |
| `http_server` | `"http_server"` | HTTP server object |

### 2.2 Truthiness

A value is **falsy** if:

| Condition | Example |
|-----------|---------|
| Is `nil` | `nil` |
| Is boolean `false` | `false` |
| Is number `0` | `0`, `0.0` |
| Is empty string | `""` |

**All other values are truthy**, including:
- Non-empty strings: `"false"`, `"0"`, `"nil"` are all truthy
- Negative numbers: `-1` is truthy
- Empty lists: `[]` is truthy
- Empty dicts: `{}` is truthy

### 2.3 Type Coercion

Only `+` supports cross-type operations:

| Left | Right | Result |
|------|-------|--------|
| `number` | `number` | `number` (arithmetic) |
| `string` | `string` | `string` (concatenation) |
| `string` | `number` | `string` (number auto-converted to string) |
| `number` | `string` | `string` (number auto-converted to string) |
| other | other | **Runtime error** |

All other operators (`-`, `*`, `/`, `%`, comparisons) require both operands to be numbers.

---

## 3. Variables

### 3.1 Dynamic Declaration

```
name = "Alice"       # string
age = 25              # number
is_active = true      # boolean
result = nil          # nil
```

### 3.2 Type-Annotated Declaration

```
int count = 0
float pi = 3.14
string name = "hello"
bool flag = true
list nums = [1, 2, 3]
var x = 42            # same as dynamic
```

Type annotations are syntactic sugar — they are not enforced at runtime. Unassigned type-annotated variables default to `nil`:

```
int count
print(count)          # nil
```

### 3.3 Assignment

```
x = 10
x = 20                # reassignment
x += 5                # compound: x = x + 5
x -= 3                # compound: x = x - 3
x *= 2                # compound: x = x * 2
```

### 3.4 Scope

- **Global variables**: declared at top level, accessible everywhere
- **Local variables**: declared inside a function/block, only accessible within that scope
- Max 256 local variables per scope
- Max 64 nesting levels

---

## 4. Operators

### 4.1 Arithmetic

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `+` | Addition / concatenation | `5 + 3` | `8` |
| `-` | Subtraction | `5 - 3` | `2` |
| `*` | Multiplication | `5 * 3` | `15` |
| `/` | Division (numbers only) | `10 / 3` | `3.33333` |
| `%` | Modulo (numbers only) | `10 % 3` | `1` |
| `-` (unary) | Negation | `-5` | `-5` |

Division by zero raises a runtime error.

### 4.2 Comparison (numbers only)

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `==` | Equal to | `5 == 5` | `true` |
| `!=` | Not equal to | `5 != 3` | `true` |
| `>` | Greater than | `5 > 3` | `true` |
| `<` | Less than | `5 < 3` | `false` |
| `>=` | Greater or equal | `5 >= 5` | `true` |
| `<=` | Less or equal | `5 <= 3` | `false` |

### 4.3 Logical

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `and` | Logical AND (short-circuit) | `true and false` | `false` |
| `or` | Logical OR (short-circuit) | `true or false` | `true` |
| `not` | Logical NOT (returns boolean) | `not true` | `false` |

### 4.4 Compound Assignment

| Operator | Description | Equivalent |
|----------|-------------|------------|
| `+=` | Add and assign | `x = x + y` |
| `-=` | Subtract and assign | `x = x - y` |
| `*=` | Multiply and assign | `x = x * y` |

---

## 5. Control Flow

### 5.1 If / Elif / Else

```
if condition
    body
elif condition
    body
else
    body
end
```

- `elif` and `else` are optional
- Multiple `elif` blocks allowed
- Conditions evaluated top-to-bottom; first true branch runs

```
score = 85
if score >= 90
    print("A")
elif score >= 80
    print("B")
elif score >= 70
    print("C")
else
    print("F")
end
```

### 5.2 While Loop

```
while condition
    body
end
```

```
i = 0
while i < 5
    print(i)
    i = i + 1
end
```

### 5.3 For Loop

```
for variable in start to end
    body
end
```

- Range is **inclusive** on both ends: `for i in 0 to 5` visits 0, 1, 2, 3, 4, 5
- Increments by 1 each iteration
- Works with negative ranges (counts down): `for i in 10 to 0`

### 5.4 Break and Continue

```
break           # exit the current loop immediately
continue        # skip to the next iteration of the current loop
```

Only valid inside `while` or `for` loops.

### 5.5 Return

```
return              # returns nil
return expression   # returns the expression value
```

---

## 6. Functions

### 6.1 Definition

```
func function_name(param1, param2, ...)
    body
end
```

### 6.2 Calling

```
result = function_name(arg1, arg2)
```

### 6.3 Parameters

- Max 255 parameters
- Max 255 arguments per call
- No default parameter values
- No variadic parameters (except native functions)

### 6.4 Return Values

Use `return` to return a value. If no `return` is present, the function returns `nil`.

```
func add(a, b)
    return a + b
end

func greet(name)
    print("Hello, " + name)
    # implicitly returns nil
end
```

### 6.5 Recursion

Functions can call themselves:

```
func factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end
```

---

## 7. Lists

### 7.1 Creation

```
nums = [1, 2, 3, 4, 5]
fruits = ["apple", "banana", "cherry"]
mixed = ["hello", 42, true]
empty = []
```

### 7.2 Indexing (0-based)

```
fruits = ["apple", "banana", "cherry"]
print(fruits[0])     # apple
print(fruits[1])     # banana
print(fruits[2])     # cherry
```

### 7.3 Index Assignment

```
fruits[1] = "blueberry"
print(fruits)        # [apple, blueberry, cherry]
```

### 7.4 Negative Indexing

Negative indexing is **NOT supported** for lists. Only strings support negative indexing.

### 7.5 Out of Bounds

Accessing an index outside the list raises a runtime error.

### 7.6 List Methods

See [Section 14: List Methods](#14-list-methods).

---

## 8. Dictionaries

### 8.1 Creation

```
# With values
d = {"name": "JTS", "version": "2.0"}

# Empty
empty = {}
```

Keys must be string literals or identifiers (both become strings).

### 8.2 Accessing Values

```
d = {"name": "JTS", "version": "2.0"}
print(d["name"])     # JTS
print(d["version"])  # 2.0
```

Accessing a missing key returns `nil`.

### 8.3 Setting Values

```
d["age"] = 30
d["name"] = "Updated"
```

---

## 9. Strings

### 9.1 Creation

```
s = "hello world"
empty = ""
```

### 9.2 Concatenation

```
first = "Hello"
second = "World"
message = first + " " + second     # "Hello World"
```

Number-to-string auto-conversion with `+`:

```
age = 25
print("Age: " + age)               # "Age: 25"
print(age + " years old")          # "25 years old"
```

### 9.3 Length

```
print(len("hello"))                # 5
print(len(""))                     # 0
```

### 9.4 Indexing

```
s = "hello"
print(s[0])                        # h
print(s[1])                        # e
```

**Negative indexing IS supported for strings:**

```
s = "hello"
print(s[-1])                       # o (last character)
print(s[-2])                       # l (second to last)
```

Out-of-bounds indexing raises a runtime error.

### 9.5 String Methods

See [Section 13: String Methods](#13-string-methods).

---

## 10. Object-Oriented Programming

### 10.1 Class Definition

```
class ClassName
    func init(self, param1, param2)
        self.field1 = param1
        self.field2 = param2
    end

    func method_name(self)
        print(self.field1)
    end
end
```

### 10.2 Creating Instances

```
obj = new ClassName("value1", "value2")
obj.method_name()
print(obj.field1)       # value1
```

### 10.3 Methods

- First parameter is always `self` (refers to the current instance)
- Fields are created by assigning to `self.fieldname` in `init`
- Methods are called on instances: `obj.method(args)`

### 10.4 Inheritance

```
class Parent
    func init(self, name)
        self.name = name
    end

    func speak(self)
        print(self.name + " makes a sound")
    end
end

class Child extends Parent
    func bark(self)
        print(self.name + " barks!")
    end
end

c = new Child("Rex")
c.speak()       # Rex makes a sound (inherited)
c.bark()        # Rex barks! (own method)
```

- `extends` copies all parent methods into the child class
- Child can define its own methods
- Child can override parent methods (redefining with same name)

### 10.5 Super

```
class Child extends Parent
    func speak(self)
        super.speak(self)
        print("and also barks!")
    end
end
```

### 10.6 Constructor (`init`)

The `init` method is called automatically when using `new`. It implicitly returns `self`.

```
class Timer
    func init(self)
        self.count = 0
    end
end

t = new Timer()
print(t.count)     # 0
```

---

## 11. Error Handling

### 11.1 Try/Catch

```
try
    risky_code
catch error_variable
    handle_error(error_variable)
end
```

### 11.2 Throw

```
throw "Something went wrong!"
throw 42
throw nil
```

Any value can be thrown.

### 11.3 Catch Without Variable

```
try
    throw "error"
catch
    print("Something failed!")
end
```

### 11.4 How It Works

1. Code in `try` block runs normally
2. If `throw` is encountered, the error value is pushed onto the stack
3. The VM unwinds the stack to the state at the `try` entry point
4. Execution jumps to the `catch` block
5. The error value is stored in the catch variable (or discarded if no variable)
6. Execution continues after the `end`

If no `try` block is active when `throw` executes, the error is printed to stderr and the program terminates.

---

## 12. Built-in Functions

These functions are always available. No imports needed.

### General

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `print` | `print(value, ...)` | `nil` | Print values space-separated + newline |
| `input` | `input(prompt?)` | `string`/`number`/`boolean`/`nil` | Read from stdin; auto-parses types |
| `len` | `len(obj)` | `number` | Length of string, list, tensor, or matrix |
| `type` | `type(obj)` | `string` | Type name as string |
| `str` | `str(value)` | `string` | Convert to string |
| `number` | `number(value)` | `number` | Convert string to number |

### Math

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `sqrt` | `sqrt(x)` | `number` | Square root |
| `math` | `math(func, x)` | `number` | Math function by name |

`math()` function names: `"sin"`, `"cos"`, `"tan"`, `"sqrt"`, `"abs"`, `"log"`, `"exp"`, `"pow"`, `"floor"`, `"ceil"`, `"round"`

### List Operations

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `append` | `append(list, value)` | `list` | Add element to end |

### ML/AI

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `tensor` | `tensor(data)` | `tensor` | Create tensor from list |
| `matrix` | `matrix(data)` | `matrix` | Create matrix from nested list |
| `matmul` | `matmul(a, b)` | `matrix` | Matrix multiplication |
| `sigmoid` | `sigmoid(x)` | `number` | Sigmoid activation: `1 / (1 + e^(-x))` |
| `relu` | `relu(x)` | `number` | ReLU activation: `max(0, x)` |
| `mse` | `mse(predicted, actual)` | `number` | Mean squared error |

### Web

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `http_server` | `http_server(port)` | `http_server` | Create HTTP server |
| `http_start` | `http_start(server)` | `nil` | Start server (blocking) |
| `http_request` | `http_request(url)` | `list` | GET request; returns `[status, body]` |

### File I/O

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `read_file` | `read_file(path)` | `string` | Read file contents |
| `write_file` | `write_file(path, content)` | `boolean` | Write to file; `true` on success |

### `input()` Auto-Parsing

| User enters | `input()` returns |
|-------------|-------------------|
| `true` | boolean `true` |
| `false` | boolean `false` |
| `nil` | `nil` |
| `42`, `3.14`, `-7` | number |
| `hello`, `123abc` | string |

---

## 13. String Methods

Called on string values using dot notation:

```
result = string_value.method(args)
```

| Method | Signature | Returns | Description |
|--------|-----------|---------|-------------|
| `upper` | `s.upper()` | `string` | Uppercase (ASCII) |
| `lower` | `s.lower()` | `string` | Lowercase (ASCII) |
| `trim` | `s.trim()` | `string` | Remove leading/trailing whitespace |
| `contains` | `s.contains(sub)` | `boolean` | Check if contains substring |
| `replace` | `s.replace(old, new)` | `string` | Replace all occurrences |
| `substring` | `s.substring(start, end)` | `string` | Extract substring (start inclusive, end exclusive) |
| `starts_with` | `s.starts_with(prefix)` | `boolean` | Check prefix |
| `ends_with` | `s.ends_with(suffix)` | `boolean` | Check suffix |

### Examples

```
s = "Hello World"
print(s.upper())                    # HELLO WORLD
print(s.lower())                    # hello world
print(s.trim())                     # Hello World
print(s.contains("World"))          # true
print(s.replace("World", "JTS"))    # Hello JTS
print(s.substring(0, 5))            # Hello
print(s.starts_with("Hello"))       # true
print(s.ends_with("World"))         # true
```

---

## 14. List Methods

Called on list values using dot notation:

| Method | Signature | Returns | Description |
|--------|-----------|---------|-------------|
| `sort` | `lst.sort()` | `list` | Sort numbers in-place (ascending) |
| `append` | `lst.append(value)` | `list` | Add element to end |
| `remove` | `lst.remove(value)` | `any` | Remove first occurrence; shifts elements |
| `pop` | `lst.pop()` | `any` | Remove and return last element |

### Examples

```
nums = [3, 1, 4, 1, 5]
nums.sort()
print(nums)              # [1, 1, 3, 4, 5]

nums.append(9)
print(nums)              # [1, 1, 3, 4, 5, 9]

nums.remove(1)
print(nums)              # [1, 3, 4, 5, 9]

nums.pop()
print(nums)              # [1, 3, 4, 5]
```

---

## 15. File I/O

### Reading Files

```
content = read_file("data.txt")
print(content)
```

Returns the entire file contents as a string. Raises error if file doesn't exist.

### Writing Files

```
write_file("output.txt", "Hello from JTS!")
```

Creates the file if it doesn't exist, overwrites if it does. Returns `true` on success.

### Example

```
# Write data
write_file("log.txt", "Entry 1\nEntry 2")

# Read it back
data = read_file("log.txt")
print(data)

# Process file content
content = read_file("config.txt")
if content.contains("debug")
    print("Debug mode enabled")
end
```

---

## 16. Web Development

### Creating a Server

```
server = http_server(8080)
http_start(server)
```

### Making HTTP Requests

```
response = http_request("https://api.example.com/data")
# response = [status_code, body_string]
print(response[0])     # 200
print(response[1])     # response body
```

---

## 17. ML/AI

### Tensors

```
t = tensor([1, 2, 3, 4, 5])
print(t)            # [1, 2, 3, 4, 5]
print(len(t))       # 5
```

### Matrices

```
m1 = matrix([[1, 2], [3, 4]])
m2 = matrix([[5, 6], [7, 8]])
result = matmul(m1, m2)
print(result)        # [[19, 22] [43, 50]]
```

### Activation Functions

```
print(sigmoid(0))    # 0.5
print(sigmoid(1))    # 0.731059
print(relu(-5))      # 0
print(relu(5))       # 5
```

### Loss Functions

```
predicted = [1.0, 2.0, 3.0]
actual = [1.1, 2.2, 3.1]
print(mse(predicted, actual))   # 0.02
```

---

## 18. Operator Precedence

Lowest to highest:

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| Assignment | `=`, `+=`, `-=` | Right |
| Logical OR | `or` | Left |
| Logical AND | `and` | Left |
| Equality | `==`, `!=` | Left |
| Comparison | `<`, `<=`, `>`, `>=` | Left |
| Term | `+`, `-` | Left |
| Factor | `*`, `/`, `%` | Left |
| Unary | `-`, `not` | Right |
| Call | `(`, `.`, `[` | Left |
| Primary | literals, identifiers | — |

---

## 19. VM Limits

| Limit | Value |
|-------|-------|
| Stack depth | 256 |
| Call frame depth | 64 |
| Local variables per scope | 256 |
| Scope nesting depth | 64 |
| Exception handler nesting | 64 |
| Arguments per function call | 255 |
| Parameters per function | 255 |
| Jump offset | 65535 bytes |

---

## 20. Grammar (EBNF)

```
program         = { statement } ;

statement       = print_stmt
                | if_stmt
                | while_stmt
                | for_stmt
                | return_stmt
                | try_stmt
                | throw_stmt
                | break_stmt
                | continue_stmt
                | func_def
                | class_def
                | import_stmt
                | type_decl
                | expr_stmt ;

print_stmt      = "print" expression NEWLINE ;
if_stmt         = "if" expression NEWLINE block
                  { "elif" expression NEWLINE block }
                  [ "else" NEWLINE block ] "end" ;
while_stmt      = "while" expression NEWLINE block "end" ;
for_stmt        = "for" IDENTIFIER "in" expression "to" expression NEWLINE block "end" ;
return_stmt     = "return" [ expression ] NEWLINE ;
try_stmt        = "try" NEWLINE block
                  [ "catch" [ IDENTIFIER ] NEWLINE block ] "end" ;
throw_stmt      = "throw" expression NEWLINE ;
break_stmt      = "break" NEWLINE ;
continue_stmt   = "continue" NEWLINE ;
import_stmt     = "import" STRING NEWLINE ;
type_decl       = type_keyword IDENTIFIER [ "=" expression ] NEWLINE ;

type_keyword    = "int" | "float" | "string" | "bool" | "list" | "var" ;

func_def        = "func" IDENTIFIER "(" [ param_list ] ")" NEWLINE block "end" ;
class_def       = "class" IDENTIFIER [ "extends" IDENTIFIER ] NEWLINE class_body "end" ;
class_body      = { method_def } ;
method_def      = "func" IDENTIFIER "(" param_list ")" NEWLINE block "end" ;
param_list      = IDENTIFIER { "," IDENTIFIER } ;

expr_stmt       = expression NEWLINE ;
block           = NEWLINE { statement } ;

expression      = assignment ;
assignment      = ( IDENTIFIER | IDENTIFIER "[" expression "]" )
                  ( "=" | "+=" | "-=" | "*=" ) assignment
                | logic_or ;
logic_or        = logic_and { "or" logic_and } ;
logic_and       = equality { "and" equality } ;
equality        = comparison { ( "==" | "!=" ) comparison } ;
comparison      = term { ( "<" | ">" | "<=" | ">=" ) term } ;
term            = factor { ( "+" | "-" ) factor } ;
factor          = unary { ( "*" | "/" | "%" ) unary } ;
unary           = ( "-" | "not" ) unary | call ;
call            = primary { "(" [ arg_list ] ")" | "." IDENTIFIER [ "(" [ arg_list ] ")" ] | "[" expression "]" } ;
primary         = NUMBER | STRING | "true" | "false" | "nil"
                | IDENTIFIER
                | "(" expression ")"
                | list_literal
                | dict_literal
                | new_expr
                | super_expr
                | builtin_call ;

list_literal    = "[" [ expression { "," expression } ] "]" ;
dict_literal    = "{" [ ( STRING | IDENTIFIER ) ":" expression { "," ( STRING | IDENTIFIER ) ":" expression } ] "}" ;
new_expr        = "new" IDENTIFIER "(" [ arg_list ] ")" ;
super_expr      = "super" "." IDENTIFIER "(" [ arg_list ] ")" ;
builtin_call    = ( "len" | "type" | "input" | "append" | "number" | "str"
                  | "print" | "math" | "sqrt" | "tensor" | "matrix" | "matmul"
                  | "sigmoid" | "relu" | "mse" | "read_file" | "write_file"
                  | "http_server" | "http_start" | "http_request" )
                  "(" [ arg_list ] ")" ;
arg_list        = expression { "," expression } ;
```
