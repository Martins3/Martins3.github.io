#!/usr/bin/env bash
set -E -e -u -o pipefail

# 收集一些有趣的脚本
function list_iommu_group() {
	for g in $(find /sys/kernel/iommu_groups/* -maxdepth 0 -type d | sort -V); do
		echo "IOMMU Group ${g##*/}:"
		for d in "$g"/devices/*; do
			bdf=${d##*/}
			if [[ $bdf == *:* ]]; then
				echo -e "\t$(lspci -nns "$bdf")"
			else
				# mdev 的设备
				echo "$bdf"
			fi
		done
	done
}

function list_iommu_group2() {
	for d in $(find /sys/kernel/iommu_groups/ -type l | sort -n -k5 -t/); do
		n=${d#*/iommu_groups/*}
		n=${n%%/*}
		printf 'IOMMU Group %s ' "$n"
		lspci -nns "${d##*/}"
	done
}

# usb 控制器最好不要 vfio 直通，但是可以通过 QEMU 的 usb redirect 机制来直通
function check_usb() {
	for usb_ctrl in /sys/bus/pci/devices/*/usb*; do
		pci_path=${usb_ctrl%/*}
		iommu_group=$(readlink "$pci_path"/iommu_group)
		echo "Bus $(cat "$usb_ctrl"/busnum) --> ${pci_path##*/} (IOMMU group ${iommu_group##*/})"
		lsusb -s "${usb_ctrl#*/usb}":
		echo
	done

}

# 检查到底为什么 sata 控制器不能正常使用
function check_pcie_resetable() {
	while IFS= read -r -d '' iommu_group; do
		# echo "IOMMU group $(basename "$iommu_group")"
		for device in "$iommu_group"/devices/*; do
			[[ -e $device ]] || continue
			device=${device##*/}
			if [[ -e "$iommu_group"/devices/"$device"/reset ]]; then
				:
			else
				echo -n " not RESET able"
				lspci -nns "$device"
			fi
		done
	done < <(find /sys/kernel/iommu_groups/ -maxdepth 1 -mindepth 1 -type d -print0)
}

# list_iommu_group
# list_iommu_group2
# check_usb
check_pcie_resetable
