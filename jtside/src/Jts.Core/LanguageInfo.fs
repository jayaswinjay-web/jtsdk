namespace Jts.Core

/// Shared facts about the language, used by the IDE and the installer.
module LanguageInfo =

    let Version = "2.1.0"
    let Name = "JTS GO"
    let Company = "JayTech Solutions"
    let Repository = "https://github.com/jayaswinjay-web/jtsdk"
    let PackageName = "@jaytechsolutions/jts-go"

    /// Per-user install location (no admin required), like Python's per-user setup.
    let DefaultInstallDir =
        System.IO.Path.Combine(
            System.Environment.GetFolderPath(System.Environment.SpecialFolder.LocalApplicationData),
            "Programs",
            "JTS GO")
