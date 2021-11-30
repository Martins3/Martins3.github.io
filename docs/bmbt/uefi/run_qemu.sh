#!/bin/bash
OVMF_CODE_FD=/usr/share/OVMF/OVMF_CODE.fd
OVMF_VARS_FD=/usr/share/OVMF/OVMF_VARS.fd

# qemu-system-x86_64 -enable-kvm -cpu host\
# -drive if=pflash,format=raw,unit=0,file=${OVMF_CODE_FD},readonly=on \
# -drive if=pflash,format=raw,unit=1,file=${OVMF_VARS_FD} \
# -drive file=uefi.img,if=ide \
# -net none

# qemu-system-x86_64 -cpu qemu64 -bios /usr/share/OVMF/OVMF_CODE.fd -drive file=non_root_uefi.img,if=ide -net none

# sudo qemu-system-x86_64 -cpu qemu64 -drive if=pflash,format=raw,unit=0,file=/usr/share/OVMF/OVMF_CODE.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=/usr/share/OVMF/OVMF_VARS.fd -drive format=raw,file=non_root_uefi.img,if=ide -net none

IMG=~/Downloads/memtest86-usb.img
IMG=/home/maritns3/core/vn/docs/bmbt/uefi/uefi.img
IMG=/home/maritns3/core/vn/docs/bmbt/uefi/non_root_uefi.img

# 这些 QEMU 配置应该都是可以的，但是 vars 和 code 是什么关系不知道
# https://blog.hartwork.org/posts/get-qemu-to-boot-efi/
qemu-system-x86_64 \
	-enable-kvm -cpu host -m 2G \
	-bios /usr/share/OVMF/OVMF_CODE.fd \
	-drive file=${IMG},format=raw -net none
