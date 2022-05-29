#!/bin/bash
set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".QEMU" <"$configuration")
KERNEL=$(jq -r ".KERNEL" <"$configuration")
IMG=$(jq -r ".IMG" <"$configuration")

#!/bin/bash
# https://github.com/cirosantilli/linux-cheat/blob/4c8ee243e0121f9bbd37f0ab85294d74fb6f3aec/ubuntu-18.04.1-desktop-amd64.sh

# qemu-img create -f qcow2 ${img}  30G
# qemu-system-x86_64 -hda ${img} -boot d -cdrom ${iso} -m 512 -enable-kvm

# Tested on: Ubuntu 18.10.
# https://askubuntu.com/questions/884534/how-to-run-ubuntu-16-04-desktop-on-qemu/1046792#1046792

set -eux

sure() {
  read -r -p "$1 " yn
  case $yn in
  [Yy]*) return ;;
  [Nn]*) exit ;;
  *) echo "Please answer yes or no." ;;
  esac
}

# Parameters.
id=ubuntu
abs_loc=/home/maritns3/core/vn/hack/qemu/ubuntu
disk_img="${abs_loc}/${id}.img.qcow2"
disk_img_snapshot="${abs_loc}/${id}.snapshot.qcow2"
iso=/home/maritns3/arch/nixos-gnome-21.11.334247.573095944e7-x86_64-linux.iso
kernel=/home/maritns3/core/ubuntu-linux/arch/x86_64/boot/bzImage

# Go through installer manually.
if [ ! -f "$disk_img" ]; then
  sure "Do you want to create ${disk_img}"
  qemu-img create -f qcow2 "$disk_img" 20G
  qemu-system-x86_64 -cdrom "$iso" -hda "$disk_img" -enable-kvm -m 2G -smp 2 -cpu host
  qemu-img create -b "$disk_img" -f qcow2 "$disk_img_snapshot"
  exit 0
fi

# Run the installed image.
if [ $# -eq 1 ]; then
  echo "::::::::::::::::::::::::::::::::::"
  qemu-system-x86_64 -hda "${disk_img}" -enable-kvm \
    -m 8G -smp 8 -soundhw hda -vga virtio \
    -virtfs local,path=/home/maritns3/core/vn/hack/qemu/ubuntu,mount_tag=host0,security_model=mapped,id=host0
else
  qemu-system-x86_64 -hda ${disk_img} -enable-kvm -kernel ${kernel} -cpu host -m 8G -smp 8 \
    -append "root=/dev/sda"
fi
