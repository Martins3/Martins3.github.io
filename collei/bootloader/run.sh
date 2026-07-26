#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
cd "$SCRIPT_DIR/../bootloader"
nasm -f bin boot"$1".asm -o boot.bin

qboot=/home/martins3/core/qboot/build/bios.bin
if [[ ! -f $qboot ]]; then
	cd ~/core
	if [[ ! -d qboot ]]; then
		git clone https://github.com/bonzini/qboot.git
	fi
	cd qboot
	meson setup --reconfigure build
	cd build
	ninja
fi
arg_bios=(-bios "$qboot") # 这个就不支持了
arg_bios=()
qemu-system-x86_64 "${arg_bios[@]}" -hda boot.bin
