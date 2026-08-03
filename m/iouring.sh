#!/usr/bin/env bash
set -E -e -u -o pipefail

rm -f ./c/liburing-cmd.out
make -C c
./mod.sh iouring 1
./c/liburing-cmd.out
