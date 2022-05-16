#!/bin/bash

QEMU=/home/maritns3/core/kvmqemu/build/x86_64-softmmu/qemu-system-x86_64
KERNEL=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
IMG=/home/maritns3/core/vn/docs/qemu/sh/img/yocto.ext4

${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${IMG},if=virtio,format=raw --append "root=/dev/vda console=ttyS0" -nographic
