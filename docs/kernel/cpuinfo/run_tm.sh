#!/usr/bin/env bash

set -E -e -u -o pipefail
cd "$(dirname "$0")"

gcc -std=c11 -g -o tm_example.out tm_example.c -pthread -fgnu-tm
./tm_example.out
