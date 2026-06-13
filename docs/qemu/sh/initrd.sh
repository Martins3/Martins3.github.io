#!/usr/bin/env bash
set -eu

# 参考 https://docs.kernel.org/filesystems/ramfs-rootfs-initramfs.html
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json
qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
QEMU=${qemu_dir}/build/x86_64-softmmu/qemu-system-x86_64
KERNEL=${kernel_dir}/arch/x86/boot/bzImage
workstation="$(jq -r ".workstation" <"$configuration")"
# gcc -m32 -static hello.c -o hello.out
img="$workstation/test.cpio.gz"
src="$abs_loc/hello.c"
out="$workstation/hello.out"
gcc -static "$src" -o "$out"
cd "$workstation" || exit 1
echo hello.out | cpio -o -H newc | gzip > "$img"

# 可以直接使用本系统中 initrd
# img=/boot/initrd.img-$(uname -r)

cmd="$QEMU -enable-kvm -kernel $KERNEL -initrd $img -append 'nokaslr earlyprink=serial root=/dev/ram rdinit=/hello.out'"
echo "$cmd"
eval "$cmd"
