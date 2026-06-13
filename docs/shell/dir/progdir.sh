#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR1=$(readlink -m "$(dirname "\$0")")
PROGDIR2=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
