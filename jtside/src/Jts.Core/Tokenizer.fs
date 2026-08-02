namespace Jts.Core

/// A small, dependency-free scanner for the JTS GO language.
/// Produces color-coded tokens that the IDE renders as syntax highlighting.
module Tokenizer =

    type TokenKind =
        | Keyword
        | String
        | Number
        | Comment
        | Operator
        | Identifier
        | Whitespace
        | Other

    type Token =
        { Start: int
          Length: int
          Kind: TokenKind }

    /// Every reserved word recognised by the scanner (from src/compiler/scanner.c).
    let keywords =
        [ "and"; "append"; "assert"; "bool"; "break"; "bring"; "catch"; "class"; "continue"
          "del"; "elif"; "else"; "end"; "extends"; "false"; "finally"; "float"; "for"
          "func"; "http"; "if"; "import"; "in"; "input"; "int"; "is"; "len"; "list"
          "matrix"; "model"; "new"; "nil"; "not"; "number"; "or"; "predict"; "print"
          "request"; "response"; "return"; "self"; "server"; "set"; "string"; "super"
          "tensor"; "throw"; "to"; "true"; "try"; "type"; "var"; "while"; "yield" ]
        |> Set.ofList

    let private isDigit c = c >= '0' && c <= '9'

    let private isHexDigit c =
        isDigit c || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')

    let private isIdentStart c =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c = '_'

    let private isIdentPart c =
        isIdentStart c || isDigit c

    let private isOpChar c =
        match c with
        | '+' | '-' | '*' | '/' | '%' | '=' | '!' | '<' | '>' | '&' | '|' | '^' | '~'
        | '(' | ')' | '[' | ']' | '{' | '}' | ',' | ';' | '.' | ':' | '?' | '@' -> true
        | _ -> false

    /// Length of a multi-character operator starting at index i, or 1 for single-char.
    let private opLength (source: string) (i: int) : int =
        if i + 2 < source.Length then
            let triple = source.Substring(i, 3)
            if triple = "<<=" || triple = ">>=" then 3
            else
                if i + 1 < source.Length then
                    let pair = source.Substring(i, 2)
                    match pair with
                    | "==" | "!=" | "<=" | ">=" | "&&" | "||" | "<<" | ">>"
                    | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "**" -> 2
                    | _ -> 1
                else 1
        elif i + 1 < source.Length then
            let pair = source.Substring(i, 2)
            match pair with
            | "==" | "!=" | "<=" | ">=" | "&&" | "||" | "<<" | ">>"
            | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "**" -> 2
            | _ -> 1
        else
            1

    /// Tokenize a single line of JTS source. Returns tokens in source order.
    let tokenize (source: string) : Token list =
        let n = source.Length
        let out = ResizeArray<Token>()
        let mutable i = 0
        while i < n do
            let c = source[i]
            if System.Char.IsWhiteSpace c then
                let start = i
                while i < n && System.Char.IsWhiteSpace source[i] do
                    i <- i + 1
                out.Add { Start = start; Length = i - start; Kind = Whitespace }
            elif c = '#' then
                let start = i
                while i < n && source[i] <> '\n' do
                    i <- i + 1
                out.Add { Start = start; Length = i - start; Kind = Comment }
            elif c = '"' then
                let start = i
                i <- i + 1
                let mutable closed = false
                while i < n && not closed do
                    if source[i] = '\\' && i + 1 < n then
                        i <- i + 2
                    elif source[i] = '"' then
                        closed <- true
                        i <- i + 1
                    else
                        i <- i + 1
                out.Add { Start = start; Length = i - start; Kind = String }
            elif isDigit c || (c = '.' && i + 1 < n && isDigit source[i + 1]) then
                let start = i
                // hex literal 0x...
                if c = '0' && i + 1 < n && (source[i + 1] = 'x' || source[i + 1] = 'X') then
                    i <- i + 2
                    while i < n && isHexDigit source[i] do
                        i <- i + 1
                else
                    while i < n && isDigit source[i] do
                        i <- i + 1
                    if i < n && source[i] = '.' && i + 1 < n && isDigit source[i + 1] then
                        i <- i + 1
                        while i < n && isDigit source[i] do
                            i <- i + 1
                out.Add { Start = start; Length = i - start; Kind = Number }
            elif isIdentStart c then
                let start = i
                while i < n && isIdentPart source[i] do
                    i <- i + 1
                let word = source.Substring(start, i - start)
                let kind = if keywords.Contains word then Keyword else Identifier
                out.Add { Start = start; Length = i - start; Kind = kind }
            elif isOpChar c then
                let start = i
                let len = opLength source i
                i <- i + len
                out.Add { Start = start; Length = len; Kind = Operator }
            else
                let start = i
                i <- i + 1
                out.Add { Start = start; Length = 1; Kind = Other }
        List.ofSeq out
