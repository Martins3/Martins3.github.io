#!/usr/bin/env bash
set -E -e -u -o pipefail

kvm="accel=kvm,"
kvm=""
smm="off"
# smm="on"
function a() {
	/home/martins3/core/qemu/build/qemu-system-x86_64 \
		-machine pc,${kvm}smm=${smm} \
		-bios /home/martins3/core/seabios/out/bios.bin \
		-device isa-debugcon,iobase=0x402,chardev=seabios \
		-chardev stdio,id=seabios \
		--display none
	# -s -S
}

# handle_smi cmd=0 smbase=0x00030000
#
# TODO，那些代码是 16bit 的，那些是 32bit 的啊
# # CONFIG_RELOCATE_INIT is not set
# 就可以直接用了

function b() {
	pushd ~/core/seabios/
	gdb \
		-ex " set architecture i386:x86-64                          " \
		-ex " target remote localhost:1234                          " \
		-ex " break init_virtio_scsi                                " \
		out/rom.o
	# -ex " add-symbol-file out/ccode32flat.o 0x0              " \
	# -ex " hbreak smm_setup" \
	# -ex " hbreak set_a20" \
}

# 似乎需要  CONFIG_RELOCATE_INIT 才可以
#
# set architecture i8086
# add-symbol-file rom16offset.o 0

do_gdb=${1-}
if [[ $do_gdb ]]; then
	b
else
	a
fi
