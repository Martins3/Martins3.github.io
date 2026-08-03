#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR=$(readlink -m "$(dirname "$0")")
cd "$PROGDIR"/..
make LLVM=1 -j
gdb -quiet "$PROGDIR"/access-once.o -ex "disass test$1" -ex "q"
