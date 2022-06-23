#!/usr/bin/env bash

set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".qemu" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
iso="$workstation/windows.iso"
img="$workstation/windows.img"

if [[ ! -f ${iso} ]]; then
  echo "Download the ISO from
https://www.microsoft.com/en-us/software-download/windows10ISO
and rename it as $iso"
fi

if [ ! -f "$img" ]; then
  qemu-img create -f qcow2 "$img" 100G
  qemu-system-x86_64 -cdrom "$iso" -hda "$img" -enable-kvm -m 8G -smp 8 -cpu host
  exit 0
fi

# "$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8 -vga virtio -soundhw
"$QEMU" -hda "${img}" -m 8G -smp 8 -machine type=q35,accel=kvm -soundhw hda -cpu host \
  -device vfio-pci,host=01:00.0
# "$QEMU" -drive file=/dev/nvme0n1p2,format=raw -drive file=/dev/nvme1n1p1,format=raw,readonly=on -m 8G -smp 8 -device vfio-pci,host=01:00.0 -machine type=q35,accel=kvm -soundhw hda

# https://superuser.com/questions/1293112/kvm-gpu-passthrough-of-nvidia-dgpu-on-laptop-with-hybrid-graphics-without-propri
# https://github.com/jscinoz/optimus-vfio-docs : 很复杂，根本看不懂
