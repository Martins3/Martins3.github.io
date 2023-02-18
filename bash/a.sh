#!/usr/bin/env bash

# 修改下 template 的使用方法
set -E -e -u -o pipefail
PROGNAME=$(basename "$0")
PROGDIR=$(readlink -m "$(dirname "$0")")
# $* 和 $@ 有什么区别
ARGS=($*)

echo $PROGDIR
echo $PROGNAME
for i in "${ARGS[@]}"; do
  echo "$i"
done

function b() {
  cat g
  cat g
  cat g
  cat g
  cat g
}

# @todo 这是什么意思
function a() {
  cat g || true

  a=$(b)
}

a
