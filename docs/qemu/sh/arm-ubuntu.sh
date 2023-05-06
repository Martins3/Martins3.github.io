#!/usr/bin/env bash

set -E -e -u -o pipefail

# https://github.com/cirosantilli/linux-cheat/blob/master/ubuntu-18.04.1-desktop-amd64.sh
set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation="$(jq -r ".workstation" <"$configuration")"

qemu=${qemu_dir}/build/qemu-system-$(uname -m)
kernel=${kernel_dir}/arch/x86/boot/bzImage

distribution=ubuntu-22.04.2-live-server-arm64
iso=${workstation}/iso/${distribution}.iso
img=${workstation}/vm/${distribution}.qcow2

if [ ! -f "$img" ]; then
  qemu-img create -f qcow2 "$img" 20G
  $qemu -cdrom "$iso" -hda "$img" -enable-kvm -m 2G -smp 2 -machine virt
  exit 0
fi

$qemu -hda "${img}" -enable-kvm -m 8G -smp 8 -machine virt
