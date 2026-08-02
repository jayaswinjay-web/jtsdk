param(
    [switch]$Run
)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdk  = Join-Path $HOME ".dotnet"
$env:PATH     = "$sdk;$env:PATH"
$env:DOTNET_ROOT = $sdk

$repo     = Split-Path -Parent $root
$payload  = Join-Path $root "dist\payload"
$dist     = Join-Path $root "dist"

Write-Host "== JTS IDE build ==" -ForegroundColor Cyan
Write-Host "SDK: $(& dotnet --version)" -ForegroundColor Gray

# 1. Build everything
dotnet build (Join-Path $root "JtsIde.sln") -c Release
if ($LASTEXITCODE -ne 0) { throw "Solution build failed" }

# 2. Publish the IDE (self-contained, win-x64)
$ideOut = Join-Path $payload "ide"
if (Test-Path $ideOut) { Remove-Item $ideOut -Recurse -Force }
Write-Host "Publishing IDE..." -ForegroundColor Cyan
dotnet publish (Join-Path $root "src\Jts.Ide") -c Release -r win-x64 --self-contained true -o $ideOut
if ($LASTEXITCODE -ne 0) { throw "IDE publish failed" }

# 3. Assemble the payload: language binaries + stdlib + examples + IDE
if (Test-Path (Join-Path $payload "bin"))    { Remove-Item (Join-Path $payload "bin") -Recurse -Force }
if (Test-Path (Join-Path $payload "scrolls")){ Remove-Item (Join-Path $payload "scrolls") -Recurse -Force }
if (Test-Path (Join-Path $payload "examples")){ Remove-Item (Join-Path $payload "examples") -Recurse -Force }

New-Item -ItemType Directory -Force -Path (Join-Path $payload "bin") | Out-Null
Copy-Item (Join-Path $repo "bin\win32\jts.exe")  (Join-Path $payload "bin") -Force
Copy-Item (Join-Path $repo "bin\win32\jtsc.exe") (Join-Path $payload "bin") -Force
Copy-Item (Join-Path $repo "bin\win32\jtsvm.exe")(Join-Path $payload "bin") -Force
Copy-Item (Join-Path $repo "scrolls")  (Join-Path $payload "scrolls") -Recurse -Force
Copy-Item (Join-Path $repo "examples") (Join-Path $payload "examples") -Recurse -Force
Copy-Item (Join-Path $root "payload-files\jts.bat") (Join-Path $payload "jts.bat") -Force
Copy-Item (Join-Path $repo "README.md") (Join-Path $payload "README.md") -Force

# 4. Zip the payload (embedded into setup.exe)
Write-Host "Zipping payload..." -ForegroundColor Cyan
$zip = Join-Path $dist "payload.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $payload "*") -DestinationPath $zip -CompressionLevel Optimal -Force

# 5. Publish the installer (single-file, self-contained, payload embedded)
$setupOut = Join-Path $dist "setup"
if (Test-Path $setupOut) { Remove-Item $setupOut -Recurse -Force }
Write-Host "Publishing installer..." -ForegroundColor Cyan
dotnet publish (Join-Path $root "src\Jts.Installer") -c Release -r win-x64 --self-contained true `
    -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o $setupOut
if ($LASTEXITCODE -ne 0) { throw "Installer publish failed" }

Write-Host ""
Write-Host "Done." -ForegroundColor Green
Write-Host "  IDE       : $ideOut\Jts.Ide.exe"
Write-Host "  Setup     : $setupOut\JTS-IDE-Setup.exe"

if ($Run) {
    & (Join-Path $ideOut "Jts.Ide.exe")
}
