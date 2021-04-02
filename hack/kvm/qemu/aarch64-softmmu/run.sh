#!/bin/bash

set -x
PROJECT_DIR=/home/maritns3/core/vn/hack/kvm/qemu/aarch64-softmmu

img=$PROJECT_DIR/busybox-1.33.0/rootfs.cpio.gz
kernel=/home/maritns3/core/arm64-linux/arch/arm64/boot/Image

if [ ! -f "$img" ]; then
	$PROJECT_DIR/cpio.sh
fi

if [ ! -f "$kernel" ]; then
	echo "arm64 kernel not found"
fi

qemu-system-aarch64 \
	-machine virt,virtualization=true,gic-version=3 \
	-nographic \
	-m size=1024M \
	-cpu cortex-a57 \
	-smp 2 \
	-kernel $kernel \
	-initrd $img \
	--append "console=ttyAMA0 rdinit=/linuxrc"

# qemu-system-aarch64 -machine virt -cpu cortex-a57 -smp 8 -nographic -m 4096 -kernel arch/arm64/boot/Image --append "earlyprintk console=ttyAMA0 root=/dev/nfs nfsroot=10.0.2.2:/opt/_install rw ip=dhcp init=/linuxrc"
