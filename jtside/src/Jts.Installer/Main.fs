namespace Jts.Installer

open System

/// Command-line entry point for the setup program.
module Program =

    [<EntryPoint>]
    let main argv =
        let flags = argv |> Array.map (fun a -> a.ToLowerInvariant())
        let has f = flags |> Array.contains f
        try
            if has "--uninstall" then
                Installer.uninstall (has "--silent")
                0
            elif has "--version" then
                printfn "%s %s" Jts.Core.LanguageInfo.Name Jts.Core.LanguageInfo.Version
                0
            elif has "--help" || has "-h" || has "-?" then
                printfn "%s %s Installer" Jts.Core.LanguageInfo.Name Jts.Core.LanguageInfo.Version
                printfn ""
                printfn "Usage:"
                printfn "  JTS-IDE-Setup                Install %s + JTS IDE" Jts.Core.LanguageInfo.Name
                printfn "  JTS-IDE-Setup --install       Install (same as no argument)"
                printfn "  JTS-IDE-Setup --uninstall     Remove the language and the IDE"
                printfn "  JTS-IDE-Setup --uninstall --silent"
                printfn "                                Remove without any prompts"
                printfn "  JTS-IDE-Setup --version       Show the version"
                0
            else
                Installer.install ()
                0
        with ex ->
            printfn "Error: %s" ex.Message
            1
