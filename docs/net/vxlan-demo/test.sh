#!/usr/bin/env bash
set -E -e -u -o pipefail

# VXLAN 实验测试脚本

# sudo 封装
SUDO() {
	echo 'a' | sudo -S "$@"
}

log_info() {
	echo "[INFO] $1"
}

log_pass() {
	echo "[PASS] $1"
}

log_fail() {
	echo "[FAIL] $1"
}

log_section() {
	echo ""
	echo "=============================================="
	echo "  $1"
	echo "=============================================="
}

# 检查 namespace 是否存在
check_namespaces() {
	log_section "检查 Network Namespace"

	for ns in ns-host1 ns-host2 ns-container1 ns-container2; do
		if SUDO ip netns list | grep -q "^$ns"; then
			log_pass "Namespace $ns 存在"
		else
			log_fail "Namespace $ns 不存在，请先运行 setup.sh"
			exit 1
		fi
	done
}

# 检查 underlay 连通性
check_underlay() {
	log_section "测试 Underlay 网络 (VTEP 之间)"

	if SUDO ip netns exec ns-host1 ping -c 2 -W 2 172.16.0.2 >/dev/null 2>&1; then
		log_pass "ns-host1 -> ns-host2 (172.16.0.2) 可达"
	else
		log_fail "ns-host1 -> ns-host2 不可达"
	fi

	if SUDO ip netns exec ns-host2 ping -c 2 -W 2 172.16.0.1 >/dev/null 2>&1; then
		log_pass "ns-host2 -> ns-host1 (172.16.0.1) 可达"
	else
		log_fail "ns-host2 -> ns-host1 不可达"
	fi
}

# 检查 VXLAN 接口
check_vxlan() {
	log_section "检查 VXLAN 接口"

	for host in ns-host1 ns-host2; do
		if SUDO ip netns exec $host ip link show vxlan0 >/dev/null 2>&1; then
			log_pass "$host: vxlan0 接口存在"
			SUDO ip netns exec $host ip -d link show vxlan0 | head -5
			echo ""
		else
			log_fail "$host: vxlan0 接口不存在"
		fi
	done
}

# 测试 overlay 网络连通性
check_overlay() {
	log_section "测试 Overlay 网络 (通过 VXLAN 隧道)"

	log_info "从 ns-container1 (10.200.1.10) ping ns-container2 (10.200.1.20)"

	if SUDO ip netns exec ns-container1 ping -c 3 -W 3 10.200.1.20 >/dev/null 2>&1; then
		log_pass "VXLAN 隧道连通性正常!"
		log_info "延迟测试:"
		SUDO ip netns exec ns-container1 ping -c 3 10.200.1.20 | tail -1
	else
		log_fail "VXLAN 隧道连通性失败"

		log_info "诊断信息:"
		echo "--- ns-container1 路由表 ---"
		SUDO ip netns exec ns-container1 ip route
		echo ""
		echo "--- ns-container2 路由表 ---"
		SUDO ip netns exec ns-container2 ip route
	fi
}

# 显示 FDB 表
show_fdb() {
	log_section "FDB 转发表 (MAC 地址学习)"

	log_info "ns-host1 FDB:"
	SUDO ip netns exec ns-host1 bridge fdb show 2>/dev/null || echo "(空)"

	echo ""
	log_info "ns-host2 FDB:"
	SUDO ip netns exec ns-host2 bridge fdb show 2>/dev/null || echo "(空)"
}

# 显示桥接信息
show_bridge() {
	log_section "Bridge 配置"

	log_info "ns-host1 br0:"
	SUDO ip netns exec ns-host1 bridge link show master br0 2>/dev/null || true

	echo ""
	log_info "ns-host2 br0:"
	SUDO ip netns exec ns-host2 bridge link show master br0 2>/dev/null || true
}

# 显示所有接口信息
show_interfaces() {
	log_section "所有接口信息"

	for ns in ns-host1 ns-host2 ns-container1 ns-container2; do
		echo "--- $ns ---"
		SUDO ip netns exec $ns ip addr show | grep -E "^[0-9]|inet " | head -10
		echo ""
	done
}

main() {
	check_namespaces
	check_underlay
	check_vxlan
	check_overlay
	show_fdb
	show_bridge
	show_interfaces

	log_section "测试完成"
	log_info "使用 bash capture.sh 可以抓包分析 VXLAN 封装"
}

main "$@"
