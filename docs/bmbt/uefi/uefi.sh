#!/bin/bash

WORK_DIR=/home/maritns3/core/vn/docs/bmbt/uefi
DISK_IMG=${WORK_DIR}/uefi.img
PART_IMG=${WORK_DIR}/part.img
if [[ ! -f /usr/share/OVMF/OVMF_CODE.fd ]]; then
	echo "ovmf not found"
	echo "sudo apt install ovmf"
fi

if [[ $# -ne 1 ]]; then
	echo "need extractly one "
	exit 1
fi
EFI="$1"

if [[ ! -f ${EFI} ]]; then
	echo "${EFI} not found"
	exit 1
fi

dd if=/dev/zero of=${DISK_IMG} bs=512 count=93750
parted ${DISK_IMG} -s -a minimal mklabel gpt
parted ${DISK_IMG} -s -a minimal mkpart EFI FAT16 2048s 93716s
parted ${DISK_IMG} -s -a minimal toggle 1 boot

dd if=/dev/zero of=${PART_IMG} bs=512 count=91669
mformat -i ${PART_IMG} -h 32 -t 32 -n 64 -c 1

mcopy -i ${PART_IMG} "${EFI}" ::

dd if=${PART_IMG} of=${DISK_IMG} bs=512 count=91669 seek=2048 conv=notrunc

# ref: https://blog.hartwork.org/posts/get-qemu-to-boot-efi/
qemu-system-x86_64 \
	-enable-kvm -cpu host -m 2G \
	-bios /usr/share/OVMF/OVMF_CODE.fd \
	-drive file=${DISK_IMG},format=raw -net none
