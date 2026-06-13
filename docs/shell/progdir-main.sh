#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"

# shellcheck source=docs/shell/progdir-main.sh
source ./dir/progdir.sh

echo "$PROGDIR1"
echo "$PROGDIR2"

# 输出为:
# /home/martins3/data/vn/docs/shell
# /home/martins3/data/vn/docs/shell/dir
# 所以 PROGDIR2 写法是正确的
