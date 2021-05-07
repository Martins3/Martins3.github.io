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
abs_loc=/home/maritns3/core/vn/hack/qemu/ubuntu
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
	qemu-img create -f qcow2 "$disk_img" 20G
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
if [ $# -eq 1 ]; then
  echo ":"
	qemu-system-x86_64 \
		-drive "file=${disk_img},format=qcow2" \
		-enable-kvm \
		-m 8G \
		-smp 8 \
		-soundhw hda \
		-vga virtio \
    -virtfs local,path=/home/maritns3/core/vn/hack/qemu/ubuntu,mount_tag=host0,security_model=mapped,id=host0
else
	qemu-system-x86_64 \
		-hda ${disk_img} \
		-enable-kvm \
    -append "root=/dev/sda3" \
    -kernel /home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage \
    -cpu host \
		-m 8G \
		-smp 8 \

    # -kernel ${kernel} \
    # -kernel /home/maritns3/core/vn/hack/qemu/ubuntu/vmlinuz \
fi
