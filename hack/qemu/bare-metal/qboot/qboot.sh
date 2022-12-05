#!/usr/bin/env bash
QBOOT=/home/maritns3/core/qboot/build/bios.bin
KERNEL=/home/maritns3/core/linux/arch/x86/boot/bzImage
QEMU=/home/maritns3/core/qemu/build/x86_64-softmmu/qemu-system-x86_64

"$QEMU" \
  -enable-kvm \
  -bios "$QBOOT" \
  -kernel "$KERNEL" \
  -drive file=/home/maritns3/core/vn/hack/kvm/qemu/mini-img/core-image-minimal-qemux86-64.ext4,if=virtio,format=raw \
  -serial mon:stdio -append 'root=/dev/vda console=ttyS0,115200,8n1'
