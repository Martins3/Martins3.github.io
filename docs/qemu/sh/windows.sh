#!/usr/bin/env bash

set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".QEMU" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
iso="$workstation/windows.iso"
img="$workstation/windows.img"

if [[ ! -f ${iso} ]]; then
  echo "Download the ISO from
https://www.microsoft.com/en-us/software-download/windows10ISO
and rename it as $iso"
fi

"$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8 hda

if [ ! -f "$img" ]; then
  qemu-img create -f qcow2 "$img" 100G
  qemu-system-x86_64 -cdrom "$iso" -hda "$img" -enable-kvm -m 2G -smp 8 -cpu host
  exit 0
fi

"$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8
