namespace Jts.Installer

/// Per-user installer for the JTS GO language + JTS IDE.
/// No admin rights and no Node.js/npm are needed: everything ships in the payload.
module Installer =

    open System
    open System.IO
    open System.IO.Compression
    open System.Reflection
    open Microsoft.Win32

    let private appName = Jts.Core.LanguageInfo.Name
    let private version = Jts.Core.LanguageInfo.Version
    let private company = Jts.Core.LanguageInfo.Company
    let private repository = Jts.Core.LanguageInfo.Repository

    let installDir = Jts.Core.LanguageInfo.DefaultInstallDir
    let binDir = Path.Combine(installDir, "bin")
    let ideDir = Path.Combine(installDir, "ide")

    let private payloadResource = "Jts.Installer.payload.zip"
    let private uninstallKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\JTS GO"
    let private startMenuFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), "JTS GO")
    let private setupExe = Environment.ProcessPath

    // ---- user PATH helpers ----

    let private getUserPath () =
        use key = Registry.CurrentUser.OpenSubKey("Environment", false)
        if isNull key then ""
        else string (key.GetValue("Path", ""))

    let private setUserPath (value: string) =
        use key = Registry.CurrentUser.CreateSubKey("Environment")
        key.SetValue("Path", value, RegistryValueKind.ExpandString)

    let private normalizePath (p: string) =
        p.Trim().TrimEnd('\\').ToUpperInvariant()

    let private pathContains (dir: string) (current: string) =
        current.Split([| ';' |], StringSplitOptions.RemoveEmptyEntries)
        |> Array.exists (fun p -> normalizePath p = normalizePath dir)

    let private addToPath dir =
        let current = getUserPath ()
        if pathContains dir current then
            false
        else
            let sep = if String.IsNullOrEmpty current || current.EndsWith(";", StringComparison.Ordinal) then "" else ";"
            setUserPath (current + sep + dir)
            true

    let private removeFromPath dir =
        let current = getUserPath ()
        let filtered =
            current.Split(';', StringSplitOptions.RemoveEmptyEntries)
            |> Array.filter (fun p -> normalizePath p <> normalizePath dir)
            |> String.concat ";"
        if String.Equals(current, filtered, StringComparison.Ordinal) then
            false
        else
            setUserPath filtered
            true

    // ---- payload extraction ----

    let extractPayload () =
        let asm = Assembly.GetExecutingAssembly()
        use stream = asm.GetManifestResourceStream payloadResource
        if isNull stream then
            failwithf "This installer was built without a payload (%s). Rebuild with build.ps1." payloadResource
        use zip = new ZipArchive(stream, ZipArchiveMode.Read)
        let mutable count = 0
        for entry in zip.Entries do
            if not (entry.FullName.EndsWith("/")) && not (String.IsNullOrWhiteSpace entry.FullName) then
                let target = Path.Combine(installDir, entry.FullName.Replace('/', Path.DirectorySeparatorChar))
                let dir = Path.GetDirectoryName target
                if not (Directory.Exists dir) then
                    Directory.CreateDirectory dir |> ignore
                use inStream = entry.Open()
                use outStream = File.Create target
                inStream.CopyTo outStream
                count <- count + 1
        count

    // ---- registry / shortcuts ----

    /// The setup embeds a copy of itself into the install directory so the
    /// uninstall entry keeps working even after the original setup.exe is gone.
    let private copySetupIntoTarget () =
        if not (isNull setupExe) && File.Exists setupExe then
            let target = Path.Combine(installDir, Path.GetFileName setupExe)
            try
                File.Copy(setupExe, target, true)
                target
            with _ ->
                setupExe
        else
            setupExe

    let private writeUninstallEntry () =
        let uninstaller = copySetupIntoTarget ()
        use key = Registry.CurrentUser.CreateSubKey(uninstallKeyPath)
        key.SetValue("DisplayName", appName + " " + version)
        key.SetValue("DisplayVersion", version)
        key.SetValue("Publisher", company)
        key.SetValue("URLInfoAbout", repository)
        key.SetValue("InstallLocation", installDir)
        key.SetValue("DisplayIcon", Path.Combine(ideDir, "Jts.Ide.exe"))
        key.SetValue("UninstallString", sprintf "\"%s\" --uninstall" uninstaller)
        key.SetValue("QuietUninstallString", sprintf "\"%s\" --uninstall --silent" uninstaller)
        key.SetValue("NoModify", 1, RegistryValueKind.DWord)
        key.SetValue("NoRepair", 1, RegistryValueKind.DWord)
        key.SetValue("EstimatedSize", 0, RegistryValueKind.DWord)

    let private deleteUninstallEntry () =
        Registry.CurrentUser.DeleteSubKeyTree(uninstallKeyPath, false)

    let private createStartMenuShortcut (lnkName: string) (target: string) (arguments: string) =
        Directory.CreateDirectory startMenuFolder |> ignore
        let shellType = Type.GetTypeFromProgID "WScript.Shell"
        let shell = Activator.CreateInstance shellType
        let link =
            shellType.InvokeMember(
                "CreateShortcut",
                BindingFlags.InvokeMethod,
                null,
                shell,
                [| box (Path.Combine(startMenuFolder, lnkName)) |])
        shellType.InvokeMember("TargetPath", BindingFlags.SetProperty, null, link, [| box target |]) |> ignore
        shellType.InvokeMember("Arguments", BindingFlags.SetProperty, null, link, [| box arguments |]) |> ignore
        shellType.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, link, [| box (Path.GetDirectoryName target) |]) |> ignore
        shellType.InvokeMember("IconLocation", BindingFlags.SetProperty, null, link, [| box target |]) |> ignore
        shellType.InvokeMember("Save", BindingFlags.InvokeMethod, null, link, [||]) |> ignore

    let private createShortcuts () =
        createStartMenuShortcut "JTS IDE.lnk" (Path.Combine(ideDir, "Jts.Ide.exe")) ""
        createStartMenuShortcut "JTS GO Prompt.lnk" "cmd.exe" (sprintf "/K \"\"%s\"" (Path.Combine(installDir, "jts.bat")))

    let private deleteShortcuts () =
        if Directory.Exists startMenuFolder then
            Directory.Delete(startMenuFolder, true)

    // ---- public API ----

    let install () =
        let files = extractPayload ()
        let pathAdded = addToPath binDir
        writeUninstallEntry ()
        createShortcuts ()
        printfn ""
        printfn "%s %s installed successfully." appName version
        printfn "  Location : %s" installDir
        printfn "  Files    : %d" files
        printfn "  Commands : jts (run)   jtsc (compile)   jtsvm (bytecode)"
        printfn "  IDE      : JTS IDE (Start Menu > JTS GO)"
        if pathAdded then
            printfn "  PATH     : %s added" binDir
            printfn ""
            printfn "NOTE: Reopen any open terminals so 'jts' becomes available."

    let uninstall (silent: bool) =
        let removed = removeFromPath binDir
        deleteUninstallEntry ()
        deleteShortcuts ()
        if Directory.Exists installDir then
            try
                Directory.Delete(installDir, true)
            with _ -> ()
        if not silent then
            printfn "%s has been uninstalled." appName
            printfn "  Removed PATH entry : %b" removed
            printfn "  Removed location   : %s" installDir
