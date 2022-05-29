#!/usr/bin/env bash
set -eu

# 参考 https://docs.kernel.org/filesystems/ramfs-rootfs-initramfs.html
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json
workstation=$(jq -r ".workstation" <"$configuration")
QEMU=$(jq -r ".qemu" <"$configuration")
KERNEL=$(jq -r ".kernel" <"$configuration")
# gcc -m32 -static hello.c -o hello.out
img="$workstation/test.cpio.gz"
src="$abs_loc/hello.c"
gcc -static "$src" -o "$img"
cd "$workstation" || exit 1
echo init | cpio -o -H newc | gzip > "$img"

# 可以直接使用本系统中 initrd
# img=/boot/initrd.img-$(uname -r)

cmd="$QEMU -kernel $KERNEL -initrd $img -nographic -append 'console=ttyS0' -m 4G"
echo "$cmd"
eval "$cmd"
