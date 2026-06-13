#!/usr/bin/env bash

# 参考 https://github.com/qemu/qemu/blob/master/docs/pcie.txt
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

QEMU=$(jq -r ".qemu" <"$configuration")
KERNEL=$(jq -r ".kernel" <"$configuration")
IMG="$(jq -r ".workstation" <"$configuration")/yocto.img"

ext4_img1=/home/maritns3/core/vn/docs/qemu/sh/img1.ext4
arg_bridge="-device pci-bridge,id=mybridge,chassis_nr=1"
arg_nvme="-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1 -drive file=${ext4_img1},format=raw,if=none,id=nvme1"

${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${IMG},if=virtio,format=raw --append "root=/dev/vda console=ttyS0" -nographic \
  ${arg_bridge} ${arg_nvme}
