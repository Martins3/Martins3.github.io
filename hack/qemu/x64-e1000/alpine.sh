#!/bin/bash
set -eux

DEBUG_QEMU=false
DEBUG_KERNEL=false
RUN_GDB=false
while getopts "dsg" opt; do
	case $opt in
	d) DEBUG_QEMU=true ;;
	s) DEBUG_KERNEL=true ;;
	g) RUN_GDB=true ;;
	*) exit 0 ;;
	esac
done

sure() {
	read -r -p "$1 " yn
	case $yn in
	[Yy]*) return ;;
	[Nn]*) exit ;;
	*) echo "Please answer yes or no." ;;
	esac
}

abs_loc=/home/maritns3/core/vn/hack/qemu/x64-e1000
iso=${abs_loc}/alpine-standard-3.13.5-x86_64.iso
disk_img=${abs_loc}/alpine.qcow2
kernel=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
qemu=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64
ext4_img1=${abs_loc}/img1.ext4
share_dir=${abs_loc}/share

if [ ! -f "$iso" ]; then
	echo "${iso} not found! Download it from official website"
	exit 0
fi

if [ ! -f "$ext4_img1" ]; then
	sure "create ext4_img"
	dd if=/dev/null of=${ext4_img1} bs=1M seek=100
	mkfs.ext4 -F ${ext4_img1}
	exit 0
fi

if [ ! -f "${disk_img}" ]; then
	sure "create image"
	qemu-img create -f qcow2 ${disk_img} 1T
	qemu-system-x86_64 \
		-cdrom "$iso" \
		-hda ${disk_img} \
		-enable-kvm \
		-m 2G \
		-smp 2

	exit 0
fi

if [ $DEBUG_QEMU = true ]; then
	echo "debug qemu"
	gdb --args ${qemu} \
		-drive "file=${disk_img},format=qcow2" \
		-m 8G \
		-enable-kvm \
		-kernel ${kernel} \
		-append "root=/dev/sda3 nokaslr" \
		-smp 2 \
		-cpu host \
		-monitor stdio \
		-vga virtio \
		-chardev file,path=/tmp/seabios.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios -bios /home/maritns3/core/seabios/out/bios.bin \
		-device nvme,drive=nvme0,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme0

	exit 0
fi

if [ $DEBUG_KERNEL = true ]; then
	echo "start kernel stopped"
	${qemu} \
		-drive "file=${disk_img},format=qcow2" \
		-m 8G \
		-enable-kvm \
		-kernel ${kernel} \
		-append "root=/dev/sda3 nokaslr" \
		-smp 2 \
		-vga virtio \
		-cpu host \
		-device nvme,drive=nvme0,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme0 \
		-S -s

	exit 0
fi

if [ $RUN_GDB = true ]; then
	echo "debug kernel"
	cd /home/maritns3/core/ubuntu-linux/
	gdb vmlinux -ex "target remote :1234" -ex "hbreak start_kernel" -ex "continue"
	exit 0
fi

${qemu} \
	-kernel ${kernel} \
	-drive "file=${disk_img},format=qcow2" \
	-append "root=/dev/sda3 nokaslr" \
	-m 6G \
	-enable-kvm \
	-smp 2 \
	-vga virtio \
	-cpu host \
	-monitor stdio \
	-chardev file,path=/tmp/seabios.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios -bios /home/maritns3/core/seabios/out/bios.bin \
	-device nvme,drive=nvme0,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme0 \
	-virtfs local,path="${share_dir}",mount_tag=host0,security_model=mapped,id=host0

# mount -t 9p -o trans=virtio,version=9p2000.L host0 /mnt/9p

# TODO deadbeaf1 ?
