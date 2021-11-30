#!/bin/bash

WORK_DIR=/home/maritns3/core/vn/docs/bmbt/uefi
DISK_IMG=${WORK_DIR}/uefi.img
LOOP_DEV=/dev/loop1234
EFI=${WORK_DIR}/hello.efi

# https://wiki.osdev.org/UEFI#Creating_disk_images
if [[ ! -d ${WORK_DIR} ]]; then
	echo "invalid WORK_DIR=${WORK_DIR}"
	exit 1
fi

dd if=/dev/zero of=${DISK_IMG} bs=512 count=93750
# https://askubuntu.com/questions/338857/automatically-enter-input-in-command-line
printf "o\nn\nw\ny\n" | gdisk ${DISK_IMG}


# https://unix.stackexchange.com/questions/81241/mount-could-not-find-any-free-loop-device
if [[ ! -e ${LOOP_DEV} ]]; then
  # FIXME loopdev 不太了解，应该看看 man loop(4)
	sudo mknod -m640 ${LOOP_DEV} b 7 1234
else
	echo "loop100 already created"
	exit 1
fi

# FIXME osdev 中解释了这两个数值的来源，但是本宝宝没看懂
losetup --offset 1048576 --sizelimit 46934528 ${LOOP_DEV} ${DISK_IMG}
mkdosfs -F 32 ${LOOP_DEV}
mount ${LOOP_DEV} /mnt
cp ${EFI} /mnt/
umount /mnt
losetup -d ${LOOP_DEV}
