#!/usr/bin/env bash

# 参考
# https://github.com/cirosantilli/linux-cheat/blob/master/ubuntu-18.04.1-desktop-amd64.sh
set -eu
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".qemu" <"$configuration")
QEMU=/home/martins3/core/qemu/build/qemu-system-x86_64
workstation=$(jq -r ".workstation" <"$configuration")
workstation=/home/maritns3/hack/vm/
kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
KERNEL=${kernel_dir}/arch/x86/boot/bzImage

iso="$workstation/ubuntu-20.04-server.iso"
img="$workstation/ubuntu-20.04-server.img"
img="/home/martins3/hack/vm/centos8.qcow2"
img="/home/martins3/core/packer-kvm/artifacts/qemu/centos9/packer-centos9"
snap_img="$workstation/ubuntu_bak-20.04-server.img"

# if [[ ! -f ${iso} ]]; then
#   wget https://releases.ubuntu.com/22.04/ubuntu-22.04-desktop-amd64.iso -O "${iso}"
# fi

# Go through installer manually.
# if [ ! -f "$img" ]; then
#   qemu-img create -f qcow2 "$img" 100G
#   qemu-system-x86_64 -cdrom "$iso" -hda "$img" -enable-kvm -m 2G -smp 2 -cpu host
#   qemu-img create -b "$img" -f qcow2 "$snap_img"
#   exit 0
# fi

set -x
$QEMU -hda "${img}" -enable-kvm -m 8G -smp 8
# "$QEMU" -hda "${img}" -enable-kvm -kernel "${KERNEL}" -cpu host -m 8G -smp 8 -append "root=/dev/sda" -vga virtio

# 如果是在其他的环境中使用，可以尝试一下这个
# 1. -nographic 的使用需要在 guest 中设置命令行参数 console=ttyS0,9600 earlyprink=serial
cat <<_EOF_
img="/home/martins3/hack/vm/ubuntu-22.04.1-live-server-amd64.qcow2"
qemu-system-x86_64 -hda "${img}" -enable-kvm -m 8G -smp 8  -netdev user,id=net1,hostfwd=tcp::5559-:22 -device e1000e,netdev=net1 -nographic
_EOF_
