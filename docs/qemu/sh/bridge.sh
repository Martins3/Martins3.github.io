#!/bin/bash

QEMU=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64
KERNEL=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
IMG=/home/maritns3/core/vn/docs/qemu/sh/img/yocto.ext4

ext4_img1=/home/maritns3/core/vn/docs/qemu/sh/img1.ext4
arg_bridge="-device pci-bridge,id=mybridge,chassis_nr=1"
arg_nvme="-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1 -drive file=${ext4_img1},format=raw,if=none,id=nvme1"

${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${IMG},if=virtio,format=raw --append "root=/dev/vda console=ttyS0" -nographic \
  ${arg_bridge} ${arg_nvme}
