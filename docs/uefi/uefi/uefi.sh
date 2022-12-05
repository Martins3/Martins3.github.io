#!/usr/bin/env bash
set -eu

# --------------- 选项配置 ---------------------------
WORK_DIR=/home/maritns3/core/vn/docs/bmbt/uefi
EDK2_DIR=/home/maritns3/core/ld/edk2-workstation/edk2
PEINFO=/home/maritns3/core/ld/edk2-workstation/peinfo/peinfo
QEMU=/home/maritns3/core/kvmqemu/build/x86_64-softmmu/qemu-system-x86_64
# 将需要拷贝到 fs0: 中的代码放到此处
EFI_APPLICATIONS=(
	/home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/bmbt.efi
  /home/maritns3/core/ubuntu-linux/arch/x86_64/boot/bzImage
)
KERNEL=/home/maritns3/core/ubuntu-linux/arch/x86_64/boot/bzImage
# CDROM=/home/maritns3/arch/nixos-gnome-21.11.334247.573095944e7-x86_64-linux.iso
CDROM=/home/maritns3/arch/nixos-minimal-21.11.334247.573095944e7-x86_64-linux.iso
# ----------------------------------------------------

VirtualDrive=${WORK_DIR}/VirtualDrive
DISK_IMG=${WORK_DIR}/uefi.img
PART_IMG=${WORK_DIR}/part.img

OVMFBASE=${EDK2_DIR}/Build/OvmfX64/DEBUG_GCC5
SEARCHPATHS="${OVMFBASE}/X64 ${EDK2_DIR}/Build/AppPkg/DEBUG_GCC5/X64"

OVMF_CODE=${OVMFBASE}/FV/OVMF_CODE.fd
OVMF_VARS=${OVMFBASE}/FV/OVMF_VARS.fd
OVMF_LOG=/tmp/ovmf.log
GDB_SCRIPT=/tmp/gdbscript.ovmf

show_help() {
	echo "------ 配置参数 ---------"
	echo "qemu=${QEMU}"
	echo "OVMF=${OVMFBASE}"
	echo "workdir=${WORK_DIR}"
	echo "edk2=${EDK2_DIR}"
	echo "peinfo=${PEINFO}"
	echo "-------------------------"
	echo ""
	echo "-h 展示本消息"
	echo "-s 调试 OVMF"
	echo "-g 生成 gdb script"
	echo "-x 使用图形界面"
	echo "-b 让 debugcon 的输出到标准输出上，同时使用图形界面"
	echo "-c 使用 cdrom，UEFI 将会自动执行 BOOTX64 而不是 shell"
	exit 0
}

gen_symbol_offsets() {
  if [[ ! -f ${PEINFO} ]];then
    echo "${PEINFO} not found, install https://github.com/retrage/peinfo"
  fi


	if [[ ! -f ${OVMF_LOG} ]]; then
		echo "${OVMF_LOG} not found, run 'uefi.sh' firstly to generate the log"
    exit 1
	fi

	if [[ ! -s ${OVMF_LOG} ]]; then
		echo "${OVMF_LOG} is empty, run 'uefi.sh' firstly to generate the log"
    exit 1
	fi

  rm -f $GDB_SCRIPT
  touch $GDB_SCRIPT
	grep Loading <${OVMF_LOG} | grep -i efi | while read LINE; do
		BASE="$(echo ${LINE} | cut -d " " -f4)"
		NAME="$(echo ${LINE} | cut -d " " -f6 | tr -d "[:cntrl:]")"
		EFIFILE="$(find ${SEARCHPATHS} -name ${NAME} -maxdepth 1 -type f)"
		ADDR="$(${PEINFO} ${EFIFILE} |
			grep -A 5 text | grep VirtualAddress | cut -d " " -f2)"
		TEXT="$(python -c "print(hex(${BASE} + ${ADDR}))")"
		SYMS="$(echo ${NAME} | sed -e "s/\.efi/\.debug/g")"
		SYMFILE="$(find ${SEARCHPATHS} -name ${SYMS} -maxdepth 1 -type f)"

    echo "------------------"
    echo "${SYMFILE}"
    echo "${TEXT}"
    echo "------------------"

		echo "add-symbol-file ${SYMFILE} ${TEXT}" | tee -a $GDB_SCRIPT
	done
	exit 0
}

run_gdb() {
	if [[ ! -f ${GDB_SCRIPT} ]]; then
		echo "${GDB_SCRIPT} not found, run 'uefi.sh -g' firstly"
	fi

	gdb -ex "source ${GDB_SCRIPT}" -ex "target remote :1234"
	exit 0
}

USE_GDB=""
USE_GRAPHIC="-nographic"
USE_CDROM=""
DEBUG_BACKEND="file:${OVMF_LOG}"

while getopts "shgdxbc" opt; do
	case $opt in
	s) USE_GDB="-S -s" ;;
	h) show_help ;;
	g) gen_symbol_offsets ;;
	d) run_gdb ;;
	x) USE_GRAPHIC="" ;;
	b)
    DEBUG_BACKEND="stdio"
    USE_GRAPHIC=""
    ;;
  c) USE_CDROM="-cdrom ${CDROM}";;
	*) exit 0 ;;
	esac
done

if [[ ! -f ${OVMF_CODE} ]]; then
	echo "ovmf not found"
	echo "sudo apt install ovmf"
fi

if [[ ! -f ${DISK_IMG} ]]; then
	dd if=/dev/zero of=${DISK_IMG} bs=512 count=93750
	parted ${DISK_IMG} -s -a minimal mklabel gpt
	parted ${DISK_IMG} -s -a minimal mkpart EFI FAT16 2048s 93716s
	parted ${DISK_IMG} -s -a minimal toggle 1 boot
fi

dd if=/dev/zero of=${PART_IMG} bs=512 count=91669
mformat -i ${PART_IMG} -h 32 -t 32 -n 64 -c 1

for VAR in "${EFI_APPLICATIONS[@]}"; do
	mcopy -i ${PART_IMG} "${VAR}" ::
done

dd if=${PART_IMG} of=${DISK_IMG} bs=512 count=91669 seek=2048 conv=notrunc

${QEMU} \
	-machine q35,smm=on,accel=kvm \
  ${USE_GRAPHIC} \
	-cpu host -m 2G  -drive file=${DISK_IMG},format=raw -net none \
	-debugcon ${DEBUG_BACKEND} -global isa-debugcon.iobase=0x402 \
	-global driver=cfi.pflash01,property=secure,value=on \
	-drive if=pflash,format=raw,readonly=on,file=${OVMF_CODE} \
	-drive file=fat:rw:${VirtualDrive},format=raw,media=disk \
	-drive if=pflash,format=raw,file=${OVMF_VARS} ${USE_GDB} ${USE_CDROM}
  # -kernel ${KERNEL} \
