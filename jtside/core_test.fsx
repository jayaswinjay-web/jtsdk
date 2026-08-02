#r "src/Jts.Core/bin/Release/net8.0/Jts.Core.dll"

open Jts.Core

printfn "== tokenize =="
let line = "print(primes & odds)   # {5, 7}"
for t in Tokenizer.tokenize line do
    printfn "  %-12A [%d..%d] '%s'" t.Kind t.Start t.Length (line.Substring(t.Start, t.Length))

printfn "== keywords =="
for t in Tokenizer.tokenize "func fib(n)" do
    printfn "  %A '%s'" t.Kind (line.Substring(t.Start, t.Length) |> fun _ -> "")
let ks = Tokenizer.tokenize "func if else end for while" |> List.map (fun t -> t.Kind) |> List.distinct
printfn "  keyword tokens: %A" ks

printfn "== errors =="
printfn "  %A" (Errors.tryParse "[line 12] Error at 'foo': Expected expression")
printfn "  %A" (Errors.tryParse "[line 12] Error: Expected expression")
printfn "  %A" (Errors.tryParse "  2 + 2   (not an error)")
printfn "  %A" (Errors.tryParse "[line 5] Error at end: Unexpected end of file")
printfn "  %A" (Errors.tryParseWithPrevious "Undefined variable 'x'." "[line 22] in script")
printfn "  %A" (Errors.tryParseWithPrevious "Expected 1 arguments but got 2." "[line 3] in fact()")
printfn "  %A" (Errors.tryParseWithPrevious "hello" "  normal line  ")
