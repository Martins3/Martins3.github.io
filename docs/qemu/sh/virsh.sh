#!/usr/bin/env bash

abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

# ----------------------- 配置区 -----------------------------------------------
# kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
# qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation="$(jq -r ".workstation" <"$configuration")"

distribution=Rocky-9.1-x86_64-dvd
os_variant=rocky9

iso=${workstation}/${distribution}.iso
img=${workstation}/${distribution}.qcow2

if [[ ! -f $iso ]]; then
  wget https://mirror.nju.edu.cn/rocky/9.1/isos/x86_64/Rocky-9.1-x86_64-dvd.iso -O $iso
fi

virt-install \
  -n $distribution \
  --description "$distribution" \
  --os-variant=$os_variant \
  --ram=2048 \
  --vcpus=2 \
  --disk path=$img,bus=virtio,size=100 \
  --graphics none \
  --cdrom $iso \
  --network bridge:br0
