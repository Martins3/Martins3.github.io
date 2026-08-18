#!/usr/bin/env bash
#===============================================================================
# Intel GPU 驱动切换脚本: i915 -> xe
#===============================================================================

set -E -e -u -o pipefail

INTEL_GPU="0000:00:02.0"
FORCE_PROBE="a780"

check_root() {
	if [[ $EUID -ne 0 ]]; then
		echo "ERROR: 需要 root 权限" >&2
		exit 1
	fi
}

main() {
	check_root

	echo "========== 切换到 xe 驱动 =========="

	# 解绑 i915
	echo "[*] 解绑 i915..."
	if [[ -L "/sys/bus/pci/drivers/i915/${INTEL_GPU}" ]]; then
		echo "${INTEL_GPU}" >/sys/bus/pci/drivers/i915/unbind
		sleep 1
	fi

	# 卸载 i915
	echo "[*] 卸载 i915..."
	modprobe -r i915 2>/dev/null || true

	# 加载 xe
	#
	### 关键发现：需要 force_probe
	# Raptor Lake-S (0xA780) 不被 xe 驱动正式支持，需要强制探测：
	# [183989.512652] xe 0000:00:02.0: Your graphics device a780 is not officially supported
	#                 by xe driver in this kernel version. To force Xe probe,
	#                 use xe.force_probe='a780' and i915.force_probe='!a780'
	echo "[*] 加载 xe (force_probe=${FORCE_PROBE})..."
	modprobe xe force_probe="${FORCE_PROBE}"
	sleep 1

	# 绑定 xe
	echo "[*] 绑定 xe..."
	echo "${INTEL_GPU}" >/sys/bus/pci/drivers/xe/bind
	sleep 1

	# 验证
	echo ""
	echo "========== 验证结果 =========="
	lspci -k -s 00:02.0 | grep "Kernel driver"
}

main "$@"
