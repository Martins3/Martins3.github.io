#!/bin/bash
set -eux

# 当前目录
abs_loc=/home/maritns3/core/vn/hack/qemu/x64-e1000
iso=${abs_loc}/alpine-standard-3.13.5-x86_64.iso
disk_img=${abs_loc}/alpine.qcow2
ext4_img1=${abs_loc}/img1.ext4
ext4_img2=${abs_loc}/img2.ext4
share_dir=${abs_loc}/share

kernel=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
qemu=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64
seabios=/home/maritns3/core/seabios/out/bios.bin
# qemu=/home/maritns3/core/kvmqemu/build-4.2/x86_64-softmmu/qemu-system-x86_64

debug_qemu=
debug_kernel=
LAUNCH_GDB=false

arg_img="-drive \"file=${disk_img},format=qcow2\""
arg_mem="-m 6G -smp 2,maxcpus=3 -vga virtio"
arg_kernel="--kernel ${kernel} -append \"root=/dev/sda3 nokaslr\""
arg_seabios="-chardev file,path=/tmp/seabios.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios -bios ${seabios}"
arg_nvme="-device nvme,drive=nvme1,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme1"
arg_nvme2="-device nvme,drive=nvme2,serial=foo -drive file=${ext4_img2},format=raw,if=none,id=nvme2"
arg_share_dir="-virtfs local,path=${share_dir},mount_tag=host0,security_model=mapped,id=host0"
arg_accel="-enable-kvm -cpu host"
arg_monitor="-monitor stdio"
# arg_tmp="-device host-x86_64-cpu,socket-id=0,core-id=0,thread-id=0"
arg_tmp=""

# huxueshi:blockdev_init ide0-hd0
# huxueshi:blockdev_init nvme0
# huxueshi:blockdev_init ide1-cd0
# huxueshi:blockdev_init floppy0
# huxueshi:blockdev_init sd0

while getopts "dkgt" opt; do
	case $opt in
	d) debug_qemu="gdb --args" ;;
	k) debug_kernel="-S -s" ;;
	g) LAUNCH_GDB=true ;;
	t) arg_accel="--accel tcg,thread=single" ;;
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

if [ ! -f "$iso" ]; then
	echo "${iso} not found! Download it from official website"
	exit 0
fi

if [ ! -f "$ext4_img1" ]; then
	sure "create ${ext4_img1}"
	dd if=/dev/null of=${ext4_img1} bs=1M seek=100
	mkfs.ext4 -F ${ext4_img1}
	exit 0
fi

if [ ! -f "$ext4_img2" ]; then
	sure "create ${ext4_img1}"
	dd if=/dev/null of=${ext4_img2} bs=1M seek=100
	mkfs.ext4 -F ${ext4_img2}
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

if [ $LAUNCH_GDB = true ]; then
	echo "debug kernel"
	cd /home/maritns3/core/ubuntu-linux/
	gdb vmlinux -ex "target remote :1234" -ex "hbreak start_kernel" -ex "continue"
	exit 0
fi

cmd="${debug_qemu} ${qemu} ${debug_kernel} ${arg_img} ${arg_mem} ${arg_kernel} ${arg_seabios} ${arg_nvme} ${arg_nvme2} ${arg_share_dir} ${arg_accel} ${arg_monitor} ${arg_tmp}"
echo "$cmd"
eval "$cmd"

# mount -t 9p -o trans=virtio,version=9p2000.L host0 /mnt/9p
# 内核参数 : pci=nomsi

# TODO deadbeaf1 ?
