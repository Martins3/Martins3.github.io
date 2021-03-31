#!/bin/bash

qemu-system-x86_64 -kernel /home/maritns3/core/linux/arch/x86/boot/bzImage \
  -enable-kvm \
	-drive file=/home/maritns3/core/vn/hack/kvm/qemu/mini-img/core-image-minimal-qemux86-64.ext4,if=virtio,format=raw \
	--append "root=/dev/vda loglevel=15 console=ttyS0" \
