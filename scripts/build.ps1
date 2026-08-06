# Build JTS GO for Windows (win32).
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot -Parent)

$cc = Get-Command gcc -ErrorAction Stop

$srcs = @(
  "src/main.c",
  "src/compiler/compiler.c",
  "src/compiler/scanner.c",
  "src/core/chunk.c",
  "src/core/memory.c",
  "src/core/object.c",
  "src/core/table.c",
  "src/core/value.c",
  "src/io/fileio.c",
  "src/vm/debug.c",
  "src/vm/native.c",
  "src/vm/vm.c"
)

New-Item -ItemType Directory -Force -Path "bin/win32" | Out-Null
& $cc.Source -O2 -std=c11 -Wall -Wextra -Isrc -o "bin/win32/jts.exe" $srcs -lm -lws2_32
Copy-Item "bin/win32/jts.exe" "bin/win32/jtsc.exe" -Force
Copy-Item "bin/win32/jts.exe" "bin/win32/jtsvm.exe" -Force
Write-Host "Built bin/win32/{jts,jtsc,jtsvm}.exe"
