#!/bin/bash

set -eux

sure() {
	read -r -p "$1 " yn
	case $yn in
	[Yy]*) return ;;
	[Nn]*) exit ;;
	*) echo "Please answer yes or no." ;;
	esac
}

abs_loc=/home/maritns3/core/vn/hack/qemu/x64-e1000
iso=${abs_loc}/alpine-standard-3.13.5-x86_64.iso
disk_img=${abs_loc}/alpine.qcow2
kernel=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage

if [ ! -f "$iso" ]; then
	echo "${iso} not found!"
	exit 0
fi

if [ ! -f "${disk_img}" ]; then
	sure "create image"
	qemu-img create -f qcow2 ${disk_img} 1T
	qemu-system-x86_64 \
		-cdrom "$iso" \
		-hda ${disk_img} \
		-enable-kvm \
		-m 2G \
		-smp 2 \
		;
	exit 0
fi

qemu-system-x86_64 \
	-drive "file=${disk_img},format=qcow2" \
	-m 8G \
	-enable-kvm \
	-kernel ${kernel} \
	-append "root=/dev/sda3" \
	-smp 8 \
	-soundhw hda \
	-vga virtio
# qemu-system-x86_64 -m 512 -enable-kvm -nic user -hda ${disk_img}
