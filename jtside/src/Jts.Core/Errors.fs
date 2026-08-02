namespace Jts.Core

/// Parses the error lines produced by the JTS GO toolchain.
///
/// Compiler:  [line 12] Error at 'foo': message
///            [line 12] Error at end: message
///            [line 12] Error: message
/// Runtime:   message
///            [line 12] in script
///            [line 12] in fact()
module Errors =

    open System.Text.RegularExpressions

    type JtsError =
        { Line: int
          Message: string
          IsRuntime: bool }

    let private compilePattern =
        Regex(
            "^\[line (\d+)\] Error(?: at (?:'[^']*'|end))?: (.*)$",
            RegexOptions.Compiled)

    let private runtimePattern =
        Regex(
            "^\[line (\d+)\] in (.*)$",
            RegexOptions.Compiled)

    /// Parse a single compiler output line into an error.
    let tryParse (line: string) : JtsError option =
        let m = compilePattern.Match line
        if m.Success then
            Some
                { Line = int m.Groups.[1].Value
                  Message = m.Groups.[2].Value
                  IsRuntime = false }
        else
            None

    /// Parse a toolchain output line into an error. For runtime errors the
    /// traceback puts the message on the line just before "[line N] in script".
    let tryParseWithPrevious (previousLine: string) (line: string) : JtsError option =
        let m = compilePattern.Match line
        if m.Success then
            Some
                { Line = int m.Groups.[1].Value
                  Message = m.Groups.[2].Value
                  IsRuntime = false }
        else
            let r = runtimePattern.Match line
            if r.Success then
                Some
                    { Line = int r.Groups.[1].Value
                      Message = previousLine.Trim()
                      IsRuntime = true }
            else
                None
