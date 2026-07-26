#!/usr/bin/env bash
set -E -e -u -o pipefail
shopt -s nullglob

iommu_group=""
vendor_device=""

function is_valid_name() {
	readarray -t array < <(ls /sys/bus/pci/devices)
	for i in "${array[@]}"; do
		if [[ $1 == "$i" ]]; then
			return 0
		fi
	done
	echo "invalid bdf=[$1], choose bdf from:"
	printf "%s\n" "${array[@]}"
	return 1
}

# TODO 似乎这个方法搞的太复杂了
function get_iommu_group() {
	# /sys/kernel/iommu_groups/20/devices:
	#  0000:07:00.0
	#
	# /sys/kernel/iommu_groups/21/devices:
	#  83b8f4f2-509f-382f-3c1e-e6bfe0fa1001
	#
	# $device_name can be
	# 1. 0000:07:00.0
	# 2. 83b8f4f2-509f-382f-3c1e-e6bfe0fa1001
	device_name=$1
	iommu_group=""
	for g in $(find /sys/kernel/iommu_groups/* -maxdepth 0 -type d | sort -V); do
		for d in "$g"/devices/*; do
			local device=${d##*/}
			if [[ $device == "$device_name" ]]; then
				iommu_group="${g##*/}"
				echo "----> $iommu_group"
			fi
		done

		if [[ -n $iommu_group ]]; then
			break
		fi
	done
}

function get_vendor_device() {
	local vendor device
	vendor=$(cat /sys/bus/pci/devices/"$bdf"/vendor)
	device=$(cat /sys/bus/pci/devices/"$bdf"/device)
	vendor_device="$vendor $device"
}

function is_virtio_iommu() {
	# TODO 有待深究这个问题
	if exa -la /sys/class/iommu/ | grep virtio; then
		echo 1 | sudo tee /sys/module/vfio_iommu_type1/parameters/allow_unsafe_interrupts
	fi
}

# 自动解决
# 需要在 ubuntu 中同样解决这个问题
# [  459.870767] vfio_pin_pages_remote: RLIMIT_MEMLOCK (67108864) exceeded
# [  459.878182] vfio_pin_pages_remote: RLIMIT_MEMLOCK (67108864) exceeded
function remove_rlimit() {
	# ulimit -a 检查
	if ulimit -l | grep unlimited >/dev/null; then
		return
	fi
	# 这个修改是给 systemd 服务用的
	# echo "/etc/systemd/system.conf 修改 DefaultLimitMEMLOCK=unlimited"
	#
	# 注意，如果是 pueue 启动，肯那么需要重启 pueue 才可以
	echo "/etc/security/limits.conf 中添加"
	echo "
* hard memlock unlimited
* soft memlock unlimited
	"
	echo "然后退出 session ，也就是 ssh 要离开"
	error "remove rlimit and retry"
}

function change_owner() {
	set -x
	local group=$1
	change_file_owner /dev/vfio/"$group"
	change_file_owner /dev/iommu

	# 处理 iommufd 的问题
	# 也许有问题，/dev/vfio/devices/* 下的设备都是如何关联的
	if [[ -e /dev/iommu ]]; then
		for file in /dev/vfio/devices/*; do
			change_file_owner "$file"
		done
	fi
	set +x
}

# 这个 bind 操作真的很奇怪了。
function bind_to_vfio() {
	id="$1"
	get_iommu_group "$id"
	if [[ -z $iommu_group ]]; then
		echo "failed to get iommu group of $id"
		exit 0
	fi

	# 存在 /dev/vfio 的判断是不充分的，例如这两个设备是公用一个 /dev/vfio/ 的
	# 需要同时绑定
	# 01:00.0 VGA compatible controller: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] (rev a1)
	# 01:00.1 Audio device: NVIDIA Corporation GP106 High Definition Audio Controller (rev a1)

	change_owner "$iommu_group"
	if [[ -e /sys/bus/pci/drivers/vfio-pci/$id ]]; then
		return 0
	fi

	is_virtio_iommu
	remove_rlimit
	return 1
}

function check_iommu() {
	if ! find /sys/class/iommu -mindepth 1 -maxdepth 1 | read -r; then
		info 'sudo grubby --update-kernel=ALL --args="intel_iommu=on iommu=pt"'
		error "iommu is not enabled"
	fi

}

function load_moudles() {
	if lsmod | grep vfio_pci; then
		return
	fi
	echo "vfio vfio-pci" | sudo tee /etc/modules-load.d/vfio.conf
	sudo modprobe vfio-pci
}

function pci_bind_to_vfio() {
	bdf="$1"
	check_iommu
	load_moudles
	is_valid_name "$bdf"
	if bind_to_vfio "$bdf"; then
		return
	fi

	if [[ -f /sys/bus/pci/devices/"$bdf"/driver/unbind ]]; then
		echo "$bdf" | sudo tee /sys/bus/pci/devices/"$bdf"/driver/unbind
	fi
	get_vendor_device "$bdf"
	set -x
	# echo 8086 15f3 | sudo tee /sys/bus/pci/drivers/vfio-pci/remove_id
	if ! echo "$vendor_device" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id; then
		echo "$vendor_device" | sudo tee /sys/bus/pci/drivers/vfio-pci/remove_id
		echo "$vendor_device" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
		# /dev/vfio/20 的 20 就是这个设备所属的 iommu_group
	fi

	get_iommu_group "$bdf"
	change_owner "$iommu_group"
}

function mdev_bind_to_vfio() {
	id="$1"
	if bind_to_vfio "$1"; then
		return
	fi
}

# # 参考 libvirt 的方法，使用 driver 自动匹配
# echo 8086 15f3 | sudo tee /sys/bus/pci/drivers/vfio-pci/remove_id
# echo 0000:06:00.0 | sudo tee /sys/bus/pci/devices/0000:06:00.0/driver/unbind
# echo 0000:06:00.0 | sudo tee /sys/bus/pci/drivers_probe
#
# # 这是一种解决办法，重新 rescan ，太暴力了
# echo 1 | sudo tee /sys/bus/pci/devices/0000:06:00.0/remove
# echo 1 | sudo tee /sys/bus/pci/rescan
function unbind_from_vfio() {
	bdf="$1"
	get_vendor_device "$bdf"
	if echo "$vendor_device" | sudo tee /sys/bus/pci/drivers/vfio-pci/remove_id; then
		echo "TODO 仔细想想，这里有问题"
		# 如果我现在有 nvme 多个盘，只是想要 bind 一个到 vfio 或者解绑
		# 结果现在的操作对象全部都是 vendor_device 也就是上来就搞了所有的盘
	fi
	echo "$bdf" | sudo tee /sys/bus/pci/devices/"$bdf"/driver/unbind
	echo "$bdf" | sudo tee /sys/bus/pci/drivers_probe
}

function split_vf() {
	local d=$1
	local total
	local vf

	set -x
	cd /sys/class/net/"$d"/device
	total=$(cat sriov_totalvfs)
	vf=$(cat sriov_numvfs)
	if [[ $vf == 0 ]]; then
		echo "$total" | sudo tee sriov_numvfs

		vf=$(cat sriov_numvfs)
		if [[ $vf != "$total" ]]; then
			error "split vf failed, get $vf , expect $total"
		fi
	fi
}

vf_target_pci=
function get_vf() {
	echo "$1"
	vf_target_pci=
	for virtfn in /sys/class/net/"$1"/device/virtfn*; do
		[[ ! -L $virtfn ]] && continue
		vf_num="${virtfn##*virtfn}"
		# 提取 VF PCI: 如 0000:82:01.2
		target_pci=$(readlink -f "$virtfn" | sed 's|.*/||')
		# 获取该 VF 的 IOMMU group 编号
		iommu_group_link="/sys/bus/pci/devices/$target_pci/iommu_group"
		if [[ ! -L $iommu_group_link ]]; then
			continue
		fi

		group_num=$(basename "$(readlink -f "$iommu_group_link")")
		vfio_dev="/dev/vfio/$group_num"
		if [[ ! -e $vfio_dev ]]; then
			vf_target_pci="$target_pci"
			log "$vf_target_pci"
			return
		fi

		echo "$vfio_dev"
		if ! lsof "$vfio_dev"; then
			vf_target_pci="$target_pci"
			log "$vf_target_pci"
			return
		fi
		echo "VF$vf_num ($target_pci) has been used"
	done
}

# pci_bind_to_vfio 0000:00:03.0
#
# 按道理，现在应该做的事情是，结果现在搞的太复杂了
#
#  c8:00.0 Non-Volatile memory controller [0108]: PETAIO INC PETA8118 NVMe SSD Series [1ee4:1180] (rev 01)
#
# echo "1ee4 1180" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id # 让 vfio-pci 支持这个设备
# echo 0000:c8:00.0 | sudo tee /sys/bus/pci/drivers/nvme/unbind # 原来的解绑
# echo "0000:c8:00.0" | sudo tee /sys/bus/pci/drivers/vfio-pci/bind
#
# 看看 driver_override 这个接口如何使用
