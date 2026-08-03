# Builds the native (Windows 7+ compatible) JTS-IDE-Setup.exe.
# Embeds payload.zip as a Win32 RCDATA resource and links with the
# MSVCRT MinGW-w64 toolchain (no .NET, no Node dependency).
param(
    [Parameter(Mandatory = $true)]
    [string]$Payload,

    [Parameter(Mandatory = $false)]
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# MSVCRT MinGW-w64 (Win7-compatible CRT). Prefer the POSIX.MSVCRT package.
$candidates = @(
    "C:\Users\jayas\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.MSVCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64",
    "C:\Users\jayas\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.MSVCRT.Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64"
)
$mingw = $candidates | Where-Object { Test-Path (Join-Path $_ "bin\gcc.exe") } | Select-Object -First 1
if (-not $mingw) {
    $mingw = Read-Host "MinGW-w64 MSVCRT not found. Enter path to its mingw64 root"
}
if (-not (Test-Path (Join-Path $mingw "bin\g++.exe"))) {
    throw "g++ not found under $mingw"
}

$bin = Join-Path $mingw "bin"
$env:PATH = "$bin;$env:PATH"

if (-not (Test-Path $Payload)) { throw "Payload not found: $Payload" }
if (-not $OutDir) { $OutDir = Join-Path $scriptDir "..\..\dist\setup" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$tmp = Join-Path $env:TEMP ("native_installer_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

try {
    # Generate the .rc that embeds the payload zip, the license text, the app
    # icon, the wizard dialog templates, and version info. Use numeric ids that
    # match installer.cpp. We do NOT add an RT_MANIFEST here: MinGW's GCC always
    # links its own default-manifest.o (asInvoker + common controls v6), and a
    # second manifest would conflict at link time.
    $escaped = $Payload.Replace("\", "\\")
    $icon = (Join-Path $scriptDir "..\..\..\assets\installer.ico")
    if (-not (Test-Path $icon)) { throw "Icon not found: $icon" }
    $iconEscaped = $icon.Replace("\", "\\")
    $licenseSrc = (Join-Path $scriptDir "..\..\..\PROPRIETARY_LICENSE")
    $licensePath = Join-Path $tmp "license.txt"
    Copy-Item $licenseSrc $licensePath -Force
    $licenseEscaped = $licensePath.Replace("\", "\\")

    $rc = @"
#include <windows.h>

#define IDI_APP 1
#define IDD_WELCOME 200
#define IDD_LICENSE 201
#define IDD_LOCATION 202
#define IDD_PATH 203
#define IDD_READY 204
#define IDD_PROGRESS 205
#define IDD_DONE 206
#define IDC_ACCEPT 1001
#define IDC_DIR 1002
#define IDC_BROWSE 1003
#define IDC_PATH_YES 1004
#define IDC_PATH_NO 1005
#define IDC_SUMMARY 1006
#define IDC_PROGRESS_BAR 1007
#define IDC_STATUS 1008
#define IDC_LAUNCH 1009
#define IDC_LICENSE_EDIT 1010

IDI_APP ICON "$iconEscaped"
101 RCDATA "$escaped"
102 RCDATA "$licenseEscaped"

IDD_WELCOME DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Welcome to the JTS GO 2.1.0 Setup Wizard"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    ICON IDI_APP, -1, 10, 10, 0, 0
    LTEXT "Welcome to the JTS GO 2.1.0 Setup Wizard", -1, 60, 12, 240, 16, SS_NOPREFIX
    LTEXT "JTS GO is a modern programming language with a bytecode virtual machine, the jtsc compiler, and a built-in IDE.", -1, 60, 34, 240, 44, SS_NOPREFIX
    LTEXT "This wizard will install JTS GO and the JTS IDE on your computer. No administrator rights are required.", -1, 60, 82, 240, 36, SS_NOPREFIX
    LTEXT "Click Next to continue.", -1, 60, 160, 240, 16, SS_NOPREFIX
END

IDD_LICENSE DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "License Agreement"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    LTEXT "Please read the following license agreement. You must accept the terms to continue.", -1, 10, 8, 297, 20, SS_NOPREFIX
    CONTROL "", IDC_LICENSE_EDIT, "EDIT", ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP | WS_BORDER, 10, 30, 297, 126
    CONTROL "I accept the terms in the License Agreement", IDC_ACCEPT, "BUTTON", BS_AUTOCHECKBOX | WS_TABSTOP, 10, 164, 297, 14
END

IDD_LOCATION DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Choose Install Location"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    LTEXT "Install JTS GO to the folder below. The JTS IDE and tools will be installed into this folder.", -1, 10, 8, 297, 22, SS_NOPREFIX
    LTEXT "Folder:", -1, 10, 42, 40, 12, SS_NOPREFIX
    EDITTEXT IDC_DIR, 54, 40, 200, 14, WS_TABSTOP | ES_AUTOHSCROLL
    PUSHBUTTON "Browse...", IDC_BROWSE, 260, 39, 50, 16
    LTEXT "JTS GO installs per-user and does not require administrator rights. JTS commands become available after you reopen your terminal.", -1, 10, 66, 297, 40, SS_NOPREFIX
END

IDD_PATH DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Add to PATH"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    LTEXT "Select how the JTS GO commands (jts, jtsc, jtsvm) should be made available on this computer.", -1, 10, 8, 297, 22, SS_NOPREFIX
    CONTROL "Add jts, jtsc and jtsvm to your PATH (recommended)", IDC_PATH_YES, "BUTTON", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 10, 44, 297, 14
    CONTROL "Don't add JTS GO to your PATH", IDC_PATH_NO, "BUTTON", BS_AUTORADIOBUTTON, 10, 64, 297, 14
    LTEXT "Adding to PATH lets you run jts from any terminal. You can change this later by editing your PATH environment variable.", -1, 10, 96, 297, 44, SS_NOPREFIX
END

IDD_READY DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Ready to Install"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    LTEXT "The installer is ready to install JTS GO 2.1.0 on your computer. Review the details below, then click Next.", -1, 10, 8, 297, 26, SS_NOPREFIX
    CONTROL "", IDC_SUMMARY, "EDIT", ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_BORDER, 10, 42, 297, 118
END

IDD_PROGRESS DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Installing JTS GO"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    LTEXT "Please wait while JTS GO and the JTS IDE are being installed...", -1, 10, 16, 297, 20, SS_NOPREFIX
    CONTROL "", IDC_PROGRESS_BAR, "msctls_progress32", WS_BORDER | PBS_SMOOTH, 10, 56, 297, 12
    CONTROL "", IDC_STATUS, "STATIC", SS_LEFT | SS_NOPREFIX, 10, 78, 297, 14
END

IDD_DONE DIALOGEX 0, 0, 317, 193
STYLE DS_SETFONT | DS_CONTROL | WS_CHILD
CAPTION "Completing the JTS GO Setup Wizard"
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    ICON IDI_APP, -1, 10, 10, 0, 0
    LTEXT "Setup Completed", -1, 60, 12, 240, 16, SS_NOPREFIX
    LTEXT "JTS GO 2.1.0 was installed successfully. Commands are ready to use in new terminals.", -1, 60, 34, 240, 44, SS_NOPREFIX
    CONTROL "Launch JTS IDE", IDC_LAUNCH, "BUTTON", BS_AUTOCHECKBOX | WS_TABSTOP, 60, 100, 240, 14
    LTEXT "Click Finish to exit the wizard.", -1, 60, 160, 240, 16, SS_NOPREFIX
END

1 VERSIONINFO
FILEVERSION 2,1,0,0
PRODUCTVERSION 2,1,0,0
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904b0"
    BEGIN
      VALUE "CompanyName", "JayTech Solutions"
      VALUE "FileDescription", "JTS GO installer"
      VALUE "FileVersion", "2.1.0"
      VALUE "InternalName", "JTS-IDE-Setup"
      VALUE "LegalCopyright", "Copyright (c) JayTech Solutions"
      VALUE "OriginalFilename", "JTS-IDE-Setup.exe"
      VALUE "ProductName", "JTS GO"
      VALUE "ProductVersion", "2.1.0"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x409, 1200
  END
END
"@
    $rcPath = Join-Path $tmp "payload.rc"
    Set-Content -Path $rcPath -Value $rc -Encoding ASCII

    Write-Host "Compiling resources..." -ForegroundColor Cyan
    & "$bin\windres.exe" $rcPath -O coff -o (Join-Path $tmp "payload_res.o")
    if ($LASTEXITCODE -ne 0) { throw "windres failed" }

    Write-Host "Compiling installer..." -ForegroundColor Cyan
    $sources = @(
        (Join-Path $scriptDir "installer.cpp"),
        (Join-Path $scriptDir "miniz\miniz.c"),
        (Join-Path $scriptDir "miniz\miniz_tinfl.c"),
        (Join-Path $scriptDir "miniz\miniz_tdef.c"),
        (Join-Path $scriptDir "miniz\miniz_zip.c")
    )
    $outExe = Join-Path $OutDir "JTS-IDE-Setup.exe"
    & "$bin\g++.exe" -municode -mwindows -std=c++17 -O2 -s -static `
        -D_WIN32_WINNT=0x0601 `
        -I (Join-Path $scriptDir "miniz") `
        $sources (Join-Path $tmp "payload_res.o") `
        -o $outExe `
        -lcomctl32 -lole32 -lshell32 -lshlwapi -luser32 -ladvapi32 -luuid -lurlmon
    if ($LASTEXITCODE -ne 0) { throw "g++ failed" }

    $sizeMB = [math]::Round((Get-Item $outExe).Length / 1MB, 1)
    Write-Host ""
    Write-Host "Done. $outExe ($sizeMB MB)" -ForegroundColor Green
}
finally {
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
