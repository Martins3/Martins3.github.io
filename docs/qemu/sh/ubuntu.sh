#!/bin/bash

# 参考
# https://github.com/cirosantilli/linux-cheat/blob/master/ubuntu-18.04.1-desktop-amd64.sh
set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".qemu" <"$configuration")
KERNEL=$(jq -r ".kernel" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
iso="$workstation/ubuntu.iso"
img="$workstation/ubuntu.img"
snap_img="$workstation/ubuntu_bak.img"

if [[ ! -f ${iso} ]]; then
  wget https://releases.ubuntu.com/22.04/ubuntu-22.04-desktop-amd64.iso -O "${iso}"
fi

# Go through installer manually.
if [ ! -f "$img" ]; then
  qemu-img create -f qcow2 "$img" 100G
  qemu-system-x86_64 -cdrom "$iso" -hda "$img" -enable-kvm -m 2G -smp 2 -cpu host
  qemu-img create -b "$img" -f qcow2 "$snap_img"
  exit 0
fi

"$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8
# "$QEMU" -hda "${img}" -enable-kvm -kernel "${KERNEL}" -cpu host -m 8G -smp 8 -append "root=/dev/sda" -soundhw -vga virtio
