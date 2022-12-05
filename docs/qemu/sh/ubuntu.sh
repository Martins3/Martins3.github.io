#!/usr/bin/env bash

# 参考
# https://github.com/cirosantilli/linux-cheat/blob/master/ubuntu-18.04.1-desktop-amd64.sh
set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".qemu" <"$configuration")
QEMU=qemu-system-x86_64
KERNEL=$(jq -r ".kernel" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
workstation=/home/maritns3/hack/vm/

iso="$workstation/ubuntu-20.04-server.iso"
img="$workstation/ubuntu-20.04-server.img"
snap_img="$workstation/ubuntu_bak-20.04-server.img"

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
