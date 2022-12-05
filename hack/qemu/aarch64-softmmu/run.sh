#!/usr/bin/env bash

set -x
PROJECT_DIR=/home/maritns3/core/vn/hack/kvm/qemu/aarch64-softmmu
# QEMU=qemu-system-aarch64
QEMU=/home/maritns3/core/qemu/build/aarch64-softmmu/qemu-system-aarch64

img=$PROJECT_DIR/busybox-1.33.0/rootfs.cpio.gz
kernel=/home/maritns3/core/arm64-linux/arch/arm64/boot/Image

if [ ! -f "$img" ]; then
  $PROJECT_DIR/cpio.sh
fi

if [ ! -f "$kernel" ]; then
  echo "arm64 kernel not found"
fi

$QEMU \
  -machine virt,virtualization=true,gic-version=3 \
  -nographic \
  -m size=1024M \
  -cpu cortex-a57 \
  -smp 2 \
  -kernel $kernel \
  -initrd $img \
  --append "console=ttyAMA0 rdinit=/linuxrc" \
  -d in_asm,out_asm -D log.txt

# TODO 使用 network 启动
# qemu-system-aarch64 -machine virt -cpu cortex-a57 -smp 8 -nographic -m 4096 -kernel arch/arm64/boot/Image --append "earlyprintk console=ttyAMA0 root=/dev/nfs nfsroot=10.0.2.2:/opt/_install rw ip=dhcp init=/linuxrc"
