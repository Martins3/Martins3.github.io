#!/bin/bash

QEMU=/home/maritns3/core/kvmqemu/build/x86_64-softmmu/qemu-system-x86_64
KERNEL=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
IMG=/home/maritns3/core/vn/docs/qemu/sh/img/yocto.ext4

function usage() {
  echo "Usage :   [options] [--]

    Options:
    -h|help       Display this message
    -c|help       Display this message"
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

case $cmd in
1) ${QEMU} ;;
2) ${QEMU} -kernel ${KERNEL} ;;
3) ${QEMU} -kernel ${KERNEL} -enable-kvm ;;
*) ${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${IMG},if=virtio,format=raw --append "root=/dev/vda console=ttyS0" -nographic ;;
esac
