#!/bin/bash
set -eu

# 关键参考: https://retrage.github.io/2019/12/05/debugging-ovmf-en.html
WORK_DIR=/home/maritns3/core/vn/docs/bmbt/uefi
DISK_IMG=${WORK_DIR}/uefi.img
PART_IMG=${WORK_DIR}/part.img
EDK2_PATH=/home/maritns3/core/ld/edk2-workstation/edk2

OVMFBASE=${EDK2_PATH}/Build/OvmfX64/DEBUG_GCC5
SEARCHPATHS="${OVMFBASE}/X64 ${EDK2_PATH}/Build/AppPkg/DEBUG_GCC5/X64/"

OVMF_CODE=${OVMFBASE}/FV/OVMF_CODE.fd
OVMF_VARS=${OVMFBASE}/FV/OVMF_VARS.fd
QEMU=/home/maritns3/core/kvmqemu/build/x86_64-softmmu/qemu-system-x86_64
PEINFO=/home/maritns3/core/ld/edk2-workstation/peinfo/peinfo
OVMF_LOG=/tmp/ovmf.log
GDB_SCRIPT=/tmp/gdbscript.ovmf

show_help() {
	echo "------ 配置参数 ---------"
	echo "qemu=${QEMU}"
	echo "OVMF=${OVMFBASE}"
	echo "workdir=${WORK_DIR}"
	echo "-------------------------"
	echo ""
	echo "-h 展示本消息"
	echo "-s 调试 OVMF"
	echo "-g 生成 gdb script"
	exit 0
}

gen_symbol_offsets() {
	if [[ ! -f ${OVMF_LOG} ]]; then
		echo "${OVMF_LOG} not found, run 'uefi.sh' firstly to generate the log"
	fi

	echo "" >$GDB_SCRIPT
	grep Loading <${OVMF_LOG} | grep -i efi | while read LINE; do
		BASE="$(echo ${LINE} | cut -d " " -f4)"
		NAME="$(echo ${LINE} | cut -d " " -f6 | tr -d "[:cntrl:]")"
		EFIFILE="$(find ${SEARCHPATHS} -name ${NAME} -maxdepth 1 -type f)"
		ADDR="$(${PEINFO} ${EFIFILE} |
			grep -A 5 text | grep VirtualAddress | cut -d " " -f2)"
		TEXT="$(python -c "print(hex(${BASE} + ${ADDR}))")"
		SYMS="$(echo ${NAME} | sed -e "s/\.efi/\.debug/g")"
		SYMFILE="$(find ${SEARCHPATHS} -name ${SYMS} -maxdepth 1 -type f)"
		echo "add-symbol-file ${SYMFILE} ${TEXT}" | tee -a $GDB_SCRIPT
	done
	exit 0
}

run_gdb() {
	if [[ ! -f ${GDB_SCRIPT} ]]; then
		echo "${GDB_SCRIPT} not found, run 'uefi.sh -g' firstly"
	fi

	gdb -ex "source ${GDB_SCRIPT}.bak" -ex "target remote :1234" -ex "hb DxeMain"
	exit 0
}

USE_GDB=""

while getopts "shgd" opt; do
	case $opt in
	s) USE_GDB="-S -s" ;;
	h) show_help ;;
	g) gen_symbol_offsets ;;
	d) run_gdb ;;
	*) exit 0 ;;
	esac
done

if [[ ! -f ${OVMF_CODE} ]]; then
	echo "ovmf not found"
	echo "sudo apt install ovmf"
fi

res=(
  /home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/Main.efi
  /home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/Hello.efi
  /home/maritns3/core/ld/edk2-workstation/edk2/Build/Shell/DEBUG_GCC5/X64/AcpiViewApp.efi
  /home/maritns3/core/ld/edk2-workstation/edk2/Build/Shell/DEBUG_GCC5/X64/dp.efi
	/home/maritns3/core/ld/edk2-workstation/edk2/AppPkg/Applications/Lua/scripts/Hello.lua
  /home/maritns3/core/ubuntu-linux/arch/x86_64/boot/bzImage
)

if [[ ! -f ${DISK_IMG} ]]; then
	dd if=/dev/zero of=${DISK_IMG} bs=512 count=93750
	parted ${DISK_IMG} -s -a minimal mklabel gpt
	parted ${DISK_IMG} -s -a minimal mkpart EFI FAT16 2048s 93716s
	parted ${DISK_IMG} -s -a minimal toggle 1 boot
fi

dd if=/dev/zero of=${PART_IMG} bs=512 count=91669
mformat -i ${PART_IMG} -h 32 -t 32 -n 64 -c 1

for VAR in "${res[@]}"; do
	mcopy -i ${PART_IMG} "${VAR}" ::
done

dd if=${PART_IMG} of=${DISK_IMG} bs=512 count=91669 seek=2048 conv=notrunc

# ref: https://blog.hartwork.org/posts/get-qemu-to-boot-efi/
${QEMU} \
	-enable-kvm -cpu host -m 2G -nographic -drive file=${DISK_IMG},format=raw -net none \
	-debugcon file:${OVMF_LOG} -global isa-debugcon.iobase=0x402 \
	-drive if=pflash,format=raw,readonly=on,file=${OVMF_CODE} \
	-drive if=pflash,format=raw,file=${OVMF_VARS} ${USE_GDB}