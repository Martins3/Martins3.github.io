#!/usr/bin/env bash
set -E -e -u -o pipefail
shopt -s extglob nullglob
PROGDIR=$(readlink -m "$(dirname "$0")")
set -x

disks=(
	/dev/nvme0n1
	/dev/nvme1n1
)

function get_unused_partion() {
	:
}

function test_sysfs_proc() {
	echo 1000 | sudo tee /proc/sys/dev/raid/speed_limit_max
	cat /proc/mdstat

	sudo mdadm -v --detail --scan /dev/md1
	sudo mdadm --detail /dev/md0
	sudo mdadm --examine /dev/nvme0n1p1
}

# 一共三种模式:
# 1. resync : 默认模式，crash 的时候使用一个盘作为基准，来同步另外的一个盘
# 2. bitmap :
# 	--bitmap=internal: 将位图存储在阵列成员盘的元数据区域，这是最常见的用法。
# 	--bitmap=/path/to/file: 也可以将位图存储在一个外部文件中。
#
# journal 模式 raid1 并不支持:
# 需要一个额外的设备（或在成员盘上划分一块区域）作为“日志设备”（journal device）。
# 所有对RAID阵列的写操作，会先被写入到这个日志设备中。
# 只有当写操作成功记录在日志中后，数据才会被写入到实际的RAID成员盘上。
# sudo mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/nvme0n1p1 /dev/nvme1n1p1 --consistency-policy=journal --write-journal=/dev/nvme1n1p4
# mdadm: --write-journal is only supported for RAID level 4/5/6.

function consistency_policy() {
	sudo mdadm --grow /dev/md1 --bitmap=internal
	# sudo mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/sdb1 /dev/sdc1 --consistency-policy=journal --write-journal=/dev/sdd1
}

function size() {
	# 空间可以改大, 前提是 device 有空间，
	# 也可以改小
	sudo mdadm --grow /dev/md1 --size=2G
}

function get_md_disks() {
	sudo mdadm -vDs "$1" | awk -F= '/^[ ]+devices/ {print $2}' | tr , '\n'
	# https://serverfault.com/questions/650151/how-do-i-list-which-drives-are-part-of-each-raid-array}
	# 显然可以通过 vfs 实现找这个东西
}

function destroy_md() {
	echo "???"
	for md in /dev/md[0-9]*; do
		echo "$md"
		if mount | grep "$md"; then
			sudo umount "$md"
		fi
		readarray -t array < <(get_md_disks "$md")
		printf "%s\n" "${array[@]}"
		sudo mdadm --stop "$md"
		# 这个是必需的，不然 raid 有自动探测
		set -x
		for d in "${array[@]}"; do
			sudo mdadm --zero-superblock "$d"
		done
		set +x
	done
}

function stop_md() {
	for md in /dev/md[0-9]*; do
		set -x
		if mount | grep "$md"; then
			sudo umount "$md"
		fi
		sudo mdadm --stop "$md"
	done
}

function reload_kmod() {
	# make M=drivers/md -j128
	# pr_info("%pg \n", rdev->bdev);

	disks=()
	for md in /dev/md[0-9]*; do
		readarray -t array < <(get_md_disks "$md")
		printf "%s\n" "${array[@]}"
		disks+=("${array[@]}")
	done
	stop_md

	cd /lib/modules/"$(uname -r)"/
	mkdir -p extra && cd extra
	scp -r martins3@10.0.0.2:/home/martins3/data/kernel/linux-build/drivers/md/raid1.ko .
	scp -r martins3@10.0.0.2:/home/martins3/data/kernel/linux-build/drivers/md/md-mod.ko .
	depmod

	sudo modprobe -r raid1
	sudo modprobe -r md_mod

	# 不能使用 modprobe ，似乎有时候无法加载
	sudo insmod md-mod.ko
	sudo insmod raid1.ko
	for d in "${disks[@]}"; do
		echo "$d"
		sudo mdadm --incremental "${d}"
		# /sys/devices/virtual/block/md1/md/dev-nvme0n1p1
	done

	# sudo mdadm --manage "$md" --add /dev/nvme1n1p1
	# sudo mdadm --wait "$md"
	# sudo mdadm --manage "$md" --fail /dev/nvme1n1p1
	# sudo mdadm --manage "$md" --remove /dev/nvme1n1p1
}

function device_num() {
	local md=""
	for m in /dev/md[0-9]*; do
		md=$m
		break
	done
	if [[ $md == "" ]]; then
		return
	fi
	p=$(get_parttion "${disks[0]}" 3)

	# 1. 需要先 add disk ，才可以添加 grow
	sudo mdadm --detail "$md"
	sudo mdadm --manage "$md" --add "$p"
	sudo mdadm --detail "$md"
	sudo mdadm --grow --raid-devices=3 "$md"
	sudo mdadm --detail "$md"

	# 2. 一般操作还是先 fail ，然后 remove
	#  hot remove failed for /dev/nvme1n1p1: Device or resource busy
	#  原因是
	# state_store -> remove_and_add_spares 中会检查 rdev_removeable ，如果不是先 fail ，后续基本都是失败
	sudo mdadm --manage "$md" --fail "$p"
	sudo mdadm --detail "$md"
	sudo mdadm --manage "$md" --remove "$p"
	sudo mdadm --detail "$md"

	# 3. raid1 对于阵列大小没有 2 个预设
	# 	1. 一个 partition 已经组成了 raid 想不到还是可以直接写，
	# 	2. 如果配置 --raid-devices=3 ，那么 raid 认为有 3 个盘才是正常的
	# 	所以我们需要先 --add ，然后配置 --raid-devices=3 ，不然会被认为进入到少盘的状态

	#     Raid Devices  : 3 # 整列大小
	#     Total Devices : 4 # 所有的设备，包括 failed 但是没有被移除的，spare 的
	#    Active Devices : 1 #
	#   Working Devices : 1 # 包括 Active Devices 和 Spare Devices
	#    Failed Devices : 3 # 被标记 failed 的
	#     Spare Devices : 0 # 暂时没有使用的

}

function do_io() {
	md=""
	for d in /dev/md[0-9]*; do
		md="$d"
		break
	done
	if [[ $md == "" ]]; then
		exit 1
	fi

	if ! mount | grep "$md"; then
		if ! mount "$md" /mnt; then
			sudo mkfs.ext4 "$md"
			sudo mount "$md" /mnt
			sudo chown "$USER" /mnt
		fi
	fi

	sudo fio "$PROGDIR"/raid.ini
}

function get_parttion() {
	dev=$1
	num=$2
	if [[ $dev == *nvme* ]]; then
		echo "${dev}p$num"
	else
		echo "${dev}$num"
	fi
}

function create_md() {
	local num=$1
	p1=$(get_parttion "${disks[0]}" "$num")
	p2=$(get_parttion "${disks[1]}" "$num")
	# sudo mdadm --zero-superblock "$p1"
	# sudo mdadm --zero-superblock "$p2"
	sudo mdadm --create --failfast --verbose /dev/md"$num" \
		--bitmap=internal --level=1 --raid-devices=2 "$p1" "$p2"
	# --name=martins3 这个可以控制 /dev/md/ 下的名称
	# 相关的参数
	# --level=mirror --force --raid-devices="/dev/sda1 /dev/sdb1"
}

function create_paritions_disk() {
	dev=$1
	# 注意，这里的 0% 不能修改为 0
	# https://askubuntu.com/questions/507274/how-to-create-two-partitions-with-exactly-the-same-size
	sudo parted "$dev" -- mklabel gpt
	sudo parted "$dev" -- mkpart primary 0% 25%
	sudo parted "$dev" -- mkpart primary 25% 50%
	sudo parted "$dev" -- mkpart primary 50% 75%
	sudo parted "$dev" -- mkpart primary 75% 100%
}

function create_paritions() {
	for d in "${disks[@]}"; do
		create_paritions_disk "$d"
	done

}

function misc() {
	# 应该自动获取的
	set +x
	a=(
		/dev/nvme0n1p1
		/dev/nvme0n1p2
		/dev/nvme1n1p1
		/dev/nvme1n1p2
	)
	for d in "${a[@]}"; do
		sudo mdadm --examine "$d" | grep Ev
	done

	for md in /dev/md[0-9]*; do
		mdadm --detail /dev/md0
	done
}

function how() {
	# 实现细节是:
	#
	# echo md1234 > /sys/module/md_mod/parameters/new_array
	# ioctl(mdfd, ADD_NEW_DISK, &info->disk);
	#
	# 注意，需要此时 $A 和 $B 中已经包含了 raid superblock
	#
	# 才意识到，创建磁盘 raid 不是一个原子性的，而是首先创建一个整列出来，
	# 然后再逐个加入磁盘
	local tmp_md=md1234
	local m
	local n
	echo $tmp_md | sudo tee /sys/module/md_mod/parameters/new_array
	m=$(get_parttion "${disks[0]}" 3)
	n=$(get_parttion "${disks[0]}" 4)
	sudo mdadm --assemble $tmp_md "$m" "$n"
}

function setup_md() {
	# destroy_md
	create_paritions
	create_md 1
	# create_md 2
}

action=""
# action=consistency_policy

all_actions=(
	setup_md
	device_num
	consistency_policy
	do_io
	reload_kmod
)

function check_bare_metal() {
	if [[ $(uname -m) == aarch64 ]]; then
		# shellcheck disable=2010
		if ls /sys/devices/platform/ | grep -i qemu; then
			return
		fi
	fi

	if [[ $(uname -m) == x86_64 ]]; then
		if grep hypervisor /proc/cpuinfo >/dev/null; then
			return
		fi
	fi

	echo "Are you sure run it in bare metal environment"
	exit 1
}

if [[ $action == "" ]]; then
	action=$(printf "%s\n" "${all_actions[@]}" | fzf)
fi
$action
