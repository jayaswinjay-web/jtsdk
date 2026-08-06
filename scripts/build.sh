#!/bin/sh
# Build JTS GO for the current POSIX platform (linux / darwin).
set -e
cd "$(dirname "$0")/.."

CC="${CC:-cc}"
SRCS="src/main.c src/compiler/compiler.c src/compiler/scanner.c src/core/chunk.c src/core/memory.c src/core/object.c src/core/table.c src/core/value.c src/io/fileio.c src/vm/debug.c src/vm/native.c src/vm/vm.c"

case "$(uname -s)" in
  Linux)  PLAT=linux ;;
  Darwin) PLAT=darwin ;;
  *) echo "build.sh: unsupported platform: $(uname -s)" >&2; exit 1 ;;
esac

mkdir -p "bin/$PLAT"
"$CC" -O2 -std=c11 -Wall -Wextra -Isrc -o "bin/$PLAT/jts" $SRCS -lm
cp "bin/$PLAT/jts" "bin/$PLAT/jtsc"
cp "bin/$PLAT/jts" "bin/$PLAT/jtsvm"
chmod 755 "bin/$PLAT/jts" "bin/$PLAT/jtsc" "bin/$PLAT/jtsvm"
echo "Built bin/$PLAT/{jts,jtsc,jtsvm}"
