#!/bin/bash

set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".QEMU" <"$configuration")
KERNEL=$(jq -r ".KERNEL" <"$configuration")
IMG=$(jq -r ".YOCTO" <"$configuration")

function usage() {
  echo "Usage :   [options] [--]

    Options:
    -h|help       Display this message
    -c|help       Which cmd to run"
}

which=""
while getopts "hc:" opt; do
  case $opt in
  c) which=${OPTARG} ;;
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

cmd="echo 'no such option'"
case $which in
1) cmd="${QEMU}" ;;
2) cmd="${QEMU} -kernel ${KERNEL}" ;;
3) cmd="${QEMU} -kernel ${KERNEL} -enable-kvm" ;;
*) cmd="${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${IMG},if=virtio,format=raw --append 'root=/dev/vda console=ttyS0' -nographic" ;;
esac

echo "$cmd"
eval "$cmd"
