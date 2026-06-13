#!/usr/bin/env bash
set -E -e -u -o pipefail

# VXLAN 抓包分析脚本

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
	echo -e "${GREEN}[INFO]${NC} $1"
}

log_tip() {
	echo -e "${BLUE}[TIP]${NC} $1"
}

# 检查 root 权限
check_root() {
	if [[ $EUID -ne 0 ]]; then
		echo "此脚本需要 root 权限运行"
		exit 1
	fi
}

# 显示帮助信息
show_help() {
	echo "VXLAN 抓包分析脚本"
	echo ""
	echo "用法: $0 [选项]"
	echo ""
	echo "选项:"
	echo "  live      实时显示 VXLAN 包 (默认)"
	echo "  save      保存到 pcap 文件用于 Wireshark 分析"
	echo "  detail    显示详细的包解析"
	echo ""
	echo "示例:"
	echo "  sudo $0 live      # 实时查看"
	echo "  sudo $0 save      # 保存到 vxlan_capture.pcap"
	echo "  sudo $0 detail    # 详细解析"
}

# 实时抓包
capture_live() {
	log_info "在 ns-host1 上实时抓取 VXLAN 包..."
	log_info "按 Ctrl+C 停止"
	echo ""

	# 先触发一些流量
	log_info "触发测试流量..."
	ip netns exec ns-container1 ping -c 1 -W 1 10.200.1.20 >/dev/null 2>&1 || true

	sleep 1

	log_tip "你将看到类似如下的 VXLAN 包结构:"
	log_tip "  172.16.0.1.4789 > 172.16.0.2.4789: VXLAN, flags [I] (0x08), vni 100"
	log_tip "  IP 10.200.1.10 > 10.200.1.20: ICMP ..."
	echo ""

	# 实时抓包
	timeout 10 ip netns exec ns-host1 tcpdump -i veth1 -n -l udp port 4789 2>/dev/null || true
}

# 保存到文件
capture_save() {
	local pcap_file="vxlan_capture.pcap"

	log_info "抓取 VXLAN 包并保存到 $pcap_file..."

	# 在后台抓包
	ip netns exec ns-host1 tcpdump -i veth1 -w "$pcap_file" -n udp port 4789 2>/dev/null &
	local pid=$!

	sleep 1

	# 触发流量
	log_info "触发测试流量..."
	ip netns exec ns-container1 ping -c 5 -i 0.2 10.200.1.20 >/dev/null 2>&1 || true

	sleep 2

	# 停止抓包
	kill $pid 2>/dev/null || true
	wait $pid 2>/dev/null || true

	if [[ -f $pcap_file ]]; then
		local size=$(du -h "$pcap_file" | cut -f1)
		log_info "抓包完成: $pcap_file (大小: $size)"
		log_tip "可以使用 Wireshark 打开分析: wireshark $pcap_file"
		log_tip "在 Wireshark 中: Analyze -> Decode As -> VXLAN"
	fi
}

# 详细解析
capture_detail() {
	log_info "详细解析 VXLAN 包结构..."
	echo ""

	# 在后台抓包
	ip netns exec ns-host1 tcpdump -i veth1 -w /tmp/vxlan_temp.pcap -n udp port 4789 2>/dev/null &
	local pid=$!

	sleep 1

	# 触发流量
	ip netns exec ns-container1 ping -c 3 -i 0.3 10.200.1.20 >/dev/null 2>&1 || true

	sleep 2

	kill $pid 2>/dev/null || true
	wait $pid 2>/dev/null || true

	# 解析 pcap
	if [[ -f /tmp/vxlan_temp.pcap ]]; then
		echo "=== VXLAN 包结构解析 ==="
		echo ""
		tcpdump -r /tmp/vxlan_temp.pcap -n -X -vvv 2>/dev/null | head -60 || {
			echo "使用简单模式显示:"
			tcpdump -r /tmp/vxlan_temp.pcap -n 2>/dev/null || true
		}
		rm -f /tmp/vxlan_temp.pcap
	fi

	echo ""
	echo "=== VXLAN 封装结构说明 ==="
	echo ""
	echo "[Outer Ethernet]"
	echo "    |"
	echo "[Outer IP] Src: 172.16.0.1, Dst: 172.16.0.2 (Underlay 网络)"
	echo "    |"
	echo "[Outer UDP] Src Port: random, Dst Port: 4789"
	echo "    |"
	echo "[VXLAN Header] Flags: 0x08, VNI: 100"
	echo "    |"
	echo "[Inner Ethernet] (原始二层帧)"
	echo "    |"
	echo "[Inner IP] Src: 10.200.1.10, Dst: 10.200.1.20 (Overlay 网络)"
	echo "    |"
	echo "[Inner ICMP/其他 Payload]"
}

# 主函数
main() {
	check_root

	case "${1:-live}" in
		live)
			capture_live
			;;
		save)
			capture_save
			;;
		detail)
			capture_detail
			;;
		help | -h | --help)
			show_help
			;;
		*)
			echo "未知选项: $1"
			show_help
			exit 1
			;;
	esac
}

main "$@"
