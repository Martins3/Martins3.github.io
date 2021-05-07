#!/bin/bash
set -eux

kernel=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
qemu=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64

gcc -static -o init test.c
disk_img=test.cpio.gz
echo init | cpio -o -H newc | gzip > ${disk_img}
${qemu} -kernel ${kernel} -initrd ${disk_img}
