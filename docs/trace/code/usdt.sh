#!/usr/bin/env bash

function usage() {
  echo "Usage :   [options] [--]

    Options:
    -h|help       Display this message"
}

cmd=""
while getopts "hc:" opt; do
  case $opt in
  c) cmd=${OPTARG} ;;
  h)
    usage
    exit 0
    ;;
  *)
    echo -e "\n  Option does not exist : OPTARG\n"
    usage
    exit 1
    ;;
  esac # --- end of case ---
done
shift $((OPTIND - 1))

case "$cmd" in
1) bpftrace -l 'usdt:./program.out' ;;
esac
