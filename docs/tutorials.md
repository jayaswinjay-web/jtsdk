# Tutorials

A 12-part hands-on ladder that teaches JTS GO from zero to machine learning and web servers.

Each tutorial is a small `.jts` program you can run yourself. `say` prints to the console, so just run the file and read the output.

```
jts tutorials/01_hello.jts
```

Some tutorials use standard-library scrolls (`bring math`, `bring json`, ...). They resolve from any working directory.

| # | Tutorial | Topics | File |
|---|----------|--------|------|
| 1 | Hello, World! | `say`, your first program | [01_hello.jts](../tutorials/01_hello.jts) |
| 2 | Variables and Math | variables, arithmetic, `str()` | [02_variables_math.jts](../tutorials/02_variables_math.jts) |
| 3 | Conditionals | `if` / `elif` / `else`, comparisons, `of` | [03_conditionals.jts](../tutorials/03_conditionals.jts) |
| 4 | Loops | `for` / `while`, `break`, `continue` | [04_loops.jts](../tutorials/04_loops.jts) |
| 5 | Strings | string methods, `bring strings` | [05_strings.jts](../tutorials/05_strings.jts) |
| 6 | Functions | `func`, parameters, `return`, recursion | [06_functions.jts](../tutorials/06_functions.jts) |
| 7 | Lists | indexing, methods, `bring lists` | [07_lists.jts](../tutorials/07_lists.jts) |
| 8 | Dictionaries and Sets | `dict`, `set`, `bring sets` | [08_dicts_sets.jts](../tutorials/08_dicts_sets.jts) |
| 9 | Classes and Inheritance | `class`, `init`, `self`, `extends`, `super` | [09_classes.jts](../tutorials/09_classes.jts) |
| 10 | Files and JSON | `read_file` / `write_file`, `bring json` / `bring fs` | [10_files_json.jts](../tutorials/10_files_json.jts) |
| 11 | Errors | `throw`, `try` / `catch` / `finally` | [11_errors.jts](../tutorials/11_errors.jts) |
| 12 | Machine Learning and Web | `sigmoid`, `mse`, `matrix`, `http_server` | [12_ml_web.jts](../tutorials/12_ml_web.jts) |

## Suggested Reading

- [Getting Started](getting-started.md) — install JTS GO
- [Syntax](syntax.md) — language rules
- [Built-in Functions](builtins.md) — `say`, `ask`, `len`, `type`
- [Data Types](types.md) — numbers, strings, lists, dicts
- [Control Flow](control-flow.md) — conditionals and loops
- [Functions](functions.md) — functions in depth

Each tutorial ends with a **Challenge** — try to solve it before moving to the next one.
