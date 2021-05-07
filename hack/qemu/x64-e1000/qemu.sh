#!/bin/bash

QEMU=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64
# QEMU=qemu-system-x86_64

$QEMU -kernel /home/maritns3/core/linux/arch/x86/boot/bzImage \
  -enable-kvm \
	-drive file=/home/maritns3/core/vn/hack/qemu/mini-img/core-image-minimal-qemux86-64.ext4,if=virtio,format=raw \
	-device virtio-serial-pci -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
	--append "root=/dev/vda loglevel=15 console=ttyS0" \
  -device e1000,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::8081-:80 \
  -monitor stdio
