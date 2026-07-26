#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR=$(readlink -m "$(dirname "$0")")
cd "$PROGDIR"/..

# 不可以把 $img 放到 nfs 上，
# 不然 mount 的总是一个 read only 的
simplefs_img=$HOME/simplefs.img
simplefs_path=/mnt/simplefs
mb_size=12028

# 设计目标，自己上手来调试问题
# 构建环境的时候，自动将所有的东西全部都拉齐，也就是
# 1. image mount kernel module
#
# 清理环境的时候，umount ，rmmod ，但是不去清理 image ，因为 image 是受到影响的
function cleanup_env() {
	if sudo lsof "$simplefs_path"; then
		echo "kill them and try again"
		return 1
	fi

	if grep simplefs /proc/mounts; then
		sudo umount "$simplefs_path"
	fi

	if [[ -d /sys/module/simplefs ]]; then
		sudo rmmod simplefs
	fi
}

function setup_env() {
	set -x
	if [[ ! -f $simplefs_img ]]; then
		dd if=/dev/zero of="$simplefs_img" bs=1M count=$mb_size
		./mkfs.simplefs.out "$simplefs_img"
	fi

	if [[ ! -d /sys/module/simplefs ]]; then
		sudo insmod simplefs.ko
	fi

	if ! grep simplefs /proc/mounts; then
		sudo mount -o loop,rw -t simplefs "$simplefs_img" "$simplefs_path"
		sudo chown martins3 "$simplefs_path"
		ls "$simplefs_path"
	fi
}

function setup_reference() {
	fs=ext2
	fs=ext4
	# fs=xfs
	fs_img=$HOME/test_${fs}.img
	fs_path=/mnt/${fs}
	sudo mkdir -p $fs_path
	if mount | grep "$fs_path"; then
		echo "reference env done"
		return
	fi
	if [[ ! -f $fs_img ]]; then
		dd if=/dev/zero of="$fs_img" bs=1M count=300
	fi
	case $fs in
	ext*)
		mkfs.${fs} -F "$fs_img"
		;;
	xfs)
		mkfs.xfs "$fs_img"
		;;
	esac
	sudo mount -o loop,rw "$fs_img" $fs_path
	sudo chown martins3 $fs_path
}

[ $# -gt 0 ] && {
	cleanup_env
	exit
}

setup_reference
setup_env
