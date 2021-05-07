#!/bin/bash
# https://github.com/cirosantilli/linux-cheat/blob/4c8ee243e0121f9bbd37f0ab85294d74fb6f3aec/ubuntu-18.04.1-desktop-amd64.sh

# qemu-img create -f qcow2 ${img}  30G
# qemu-system-x86_64 -hda ${img} -boot d -cdrom ${iso} -m 512 -enable-kvm

# Tested on: Ubuntu 18.10.
# https://askubuntu.com/questions/884534/how-to-run-ubuntu-16-04-desktop-on-qemu/1046792#1046792

set -eux

sure() {
	read -r -p "$1 " yn
	case $yn in
	[Yy]*) return ;;
	[Nn]*) exit ;;
	*) echo "Please answer yes or no." ;;
	esac
}

# Parameters.
id=ubuntu
abs_loc=/home/maritns3/core/module-example/3-qemu/virt
disk_img="${abs_loc}/${id}.img.qcow2"
disk_img_snapshot="${abs_loc}/${id}.snapshot.qcow2"
version="21.04"
iso_name=ubuntu-${version}-desktop-amd64.iso
iso=${abs_loc}/${iso_name}
kernel=/home/maritns3/core/linux/arch/x86/boot/bzImage

# Get image.
if [ ! -f "$iso" ]; then
	sure "Do you want to download ${iso}"
	echo "ubuntu image not found"
	wget "http://releases.ubuntu.com/${version}/${iso_name}"
	exit 0
fi

# Go through installer manually.
if [ ! -f "$disk_img" ]; then
  sure "Do you want to create ${disk_img}"
	qemu-img create -f qcow2 "$disk_img" 1T
	qemu-system-x86_64 \
		-cdrom "$iso" \
		-drive "file=${disk_img},format=qcow2" \
		-enable-kvm \
		-m 2G \
		-smp 2 \
		;
	exit 0
fi

# Snapshot the installation.
if [ ! -f "$disk_img_snapshot" ]; then
  sure "Do you want to backup ${disk_img} to ${disk_img_snapshot}"
	qemu-img \
		create \
		-b "$disk_img" \
		-f qcow2 \
		"$disk_img_snapshot" \
		;
	exit 0
fi

# Run the installed image.
qemu-system-x86_64 \
  -drive "file=${disk_img},format=qcow2" \
  -enable-kvm \
  -m 8G \
  -smp 8 \
  -soundhw hda \
  -vga virtio
