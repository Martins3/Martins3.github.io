#!/usr/bin/env bash
set -E -e -u -o pipefail

# VXLAN 实验环境清理脚本

# sudo 封装
SUDO() {
	echo 'a' | sudo -S "$@"
}

log_info() {
	echo "[INFO] $1"
}

main() {
	log_info "清理 VXLAN 实验环境..."

	# 删除 network namespaces
	for ns in ns-host1 ns-host2 ns-container1 ns-container2; do
		if SUDO ip netns list | grep -q "^$ns"; then
			log_info "删除 namespace: $ns"
			SUDO ip netns del $ns 2>/dev/null || true
		fi
	done

	# 清理简单实验的设备
	SUDO ip link del vxlan-demo 2>/dev/null || true
	SUDO ip link del vxlan-test 2>/dev/null || true

	# 清理 pcap 文件
	rm -f vxlan_capture.pcap

	log_info "环境清理完成"
}

main "$@"
