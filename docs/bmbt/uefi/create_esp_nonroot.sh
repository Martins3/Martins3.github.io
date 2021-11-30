#!/bin/bash

# 很好，这个操作就是正确的操作了
WORK_DIR=/home/maritns3/core/vn/docs/bmbt/uefi
DISK_IMG=${WORK_DIR}/non_root_uefi.img
EFI=${WORK_DIR}/hello.efi
EFI=/home/maritns3/core/ld/edk2-workstation/edk2/Build/Bootloader/DEBUG_GCC5/X64/Bootloader.efi

cp ~/core/ubuntu-linux/arch/x86/boot/bzImage ~/core/ubuntu-linux/arch/x86/boot/bzImage.efi
EFI=~/core/ubuntu-linux/arch/x86/boot/bzImage.efi

if [[ $# -ne 1 ]];then
  echo "need extractly one "
  exit 1
fi
EFI=$1

dd if=/dev/zero of=${DISK_IMG} bs=512 count=93750
parted ${DISK_IMG} -s -a minimal mklabel gpt
parted ${DISK_IMG} -s -a minimal mkpart EFI FAT16 2048s 93716s
parted ${DISK_IMG} -s -a minimal toggle 1 boot

dd if=/dev/zero of=/tmp/part.img bs=512 count=91669
mformat -i /tmp/part.img -h 32 -t 32 -n 64 -c 1

mcopy -i /tmp/part.img "${EFI}" ::

dd if=/tmp/part.img of=${DISK_IMG} bs=512 count=91669 seek=2048 conv=notrunc

${WORK_DIR}/run_qemu.sh
