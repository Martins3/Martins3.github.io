#!/bin/bash

QEMU=/home/maritns3/core/xqm/build/x86_64-softmmu/qemu-system-x86_64
KERNEL=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
# QEMU=qemu-system-x86_64

gdb --args \
$QEMU -kernel ${KERNEL} \
  -enable-kvm \
	-drive file=/home/maritns3/core/vn/hack/qemu/mini-img/core-image-minimal-qemux86-64.ext4,if=virtio,format=raw \
	-device virtio-serial-pci -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
	--append "root=/dev/vda loglevel=15 console=ttyS0" \
  -nographic
