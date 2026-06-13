#!/usr/bin/env bash

set -E -e -u -o pipefail

cd "$(dirname "$0")"
gcc -E pageflags-expand.c > output.c
