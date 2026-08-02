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
    # Generate the .rc that embeds the payload zip and version info. Use the
    # numeric id 101 for the payload directly (matches IDR_PAYLOAD in
    # installer.cpp); an undefined name would be embedded as a string and
    # never found at runtime. We do NOT add an RT_MANIFEST here: MinGW's GCC
    # always links its own default-manifest.o (asInvoker + common controls),
    # and a second manifest would conflict at link time.
    $escaped = $Payload.Replace("\", "\\")

    $rc = @"
#include <windows.h>
101 RCDATA "$escaped"
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
    & "$bin\g++.exe" -municode -std=c++17 -O2 -s -static -D_WIN32_WINNT=0x0601 `
        -I (Join-Path $scriptDir "miniz") `
        $sources (Join-Path $tmp "payload_res.o") `
        -o $outExe `
        -lole32 -lshell32 -lshlwapi -luser32 -ladvapi32 -luuid
    if ($LASTEXITCODE -ne 0) { throw "g++ failed" }

    $sizeMB = [math]::Round((Get-Item $outExe).Length / 1MB, 1)
    Write-Host ""
    Write-Host "Done. $outExe ($sizeMB MB)" -ForegroundColor Green
}
finally {
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
