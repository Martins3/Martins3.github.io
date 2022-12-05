#!/usr/bin/env bash

function exits() {
  if ! [ -x "$(command -v $1)" ]; then
    echo "Error: $1 is not installed." >&2
    exit 1
  fi
}

exits python3
exits dot
exits gprof2dot

cmd=""
while getopts "c:" opt; do
  case $opt in
  c) cmd=${OPTARG} ;;
  *)
    echo -e "\n  Option does not exist : OPTARG\n"
    exit 1
    ;;
  esac # --- end of case ---
done
shift $((OPTIND - 1))

echo "cmd=$cmd"
WORKDIR=/home/maritns3/core/vn/tmp/gprof2dot
mkdir -p $WORKDIR
cd $WORKDIR || exit 1
img=$WORKDIR/output.png

perf record -g -- $cmd
perf script | c++filt | gprof2dot -f perf | dot -Tpng -o  $img
microsoft-edge-dev "${img}"
