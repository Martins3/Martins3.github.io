#!/usr/bin/env bash

set -E -e -u -o pipefail
# https://www.gnu.org/software/bash/manual/bash.html#The-Set-Builtin
# -E : any trap on ERR is inherited
# -u : 必须给变量赋值
PROGNAME=$(basename "$0")
PROGDIR=$(readlink -m "$(dirname "$0")")
for i in "$@"; do
  echo "$i"
done
cd "$(dirname "$0")"
set -x

dir=/nix/store/4hihy0f905bvv6kyb4qmyvhz0a2dbxxs-nixos-disk-image

if [[ ! -d nixos.qcow2 ]]; then
  install -m644 $dir/nixos.qcow2  nixos.qcow2
fi

qemu-system-x86_64 -S -s \
  -kernel /home/martins3/core/linux/arch/x86/boot/bzImage \
  -hda nixos.qcow2 \
  -append "root=/dev/sda console=ttyS0" \
  -enable-kvm
