#!/usr/bin/env bash
set -E -e -u -o pipefail

# GRE demo: 验证两台机器通过 GRE 隧道通信
# GRE: Generic Routing Encapsulation (IP 协议号 47)
# 将原始 IP 包封装在新的 IP 包中，实现隧道通信

PROG_NAME="$(basename "$0")"

# 模拟两台机器的网络
# Host A: 10.0.1.0/24 (通过 veth-pair 模拟物理连接)
# Host B: 10.0.2.0/24
# GRE 隧道: 192.168.100.0/24

function log() {
	echo "[$(date '+%H:%M:%S')] $*"
}

function check_root() {
	if [[ $EUID -ne 0 ]]; then
		echo "Error: 需要 root 权限运行"
		exit 1
	fi
}

function cleanup() {
	log "清理 GRE 隧道..."
	ip netns del host-a 2>/dev/null || true
	ip netns del host-b 2>/dev/null || true
	ip link del veth-a 2>/dev/null || true
}

function setup() {
	log "=== 创建 GRE 隧道环境 ==="

	# 创建两个命名空间模拟两台机器
	ip netns add host-a
	ip netns add host-b

	# 创建 veth-pair 模拟两台机器的物理连接
	ip link add veth-a type veth peer name veth-b
	ip link set veth-a netns host-a
	ip link set veth-b netns host-b

	# 配置 Underlay 网络（公网 IP）
	ip netns exec host-a ip addr add 10.0.12.1/24 dev veth-a
	ip netns exec host-a ip link set veth-a up
	ip netns exec host-a ip link set lo up

	ip netns exec host-b ip addr add 10.0.12.2/24 dev veth-b
	ip netns exec host-b ip link set veth-b up
	ip netns exec host-b ip link set lo up

	log "Underlay 网络配置完成"
}

function setup_gre() {
	log "=== 创建 GRE 隧道 ==="

	# 在 host-a 上创建 GRE 隧道
	# mode gre: 标准 GRE 协议
	# local/remote: 隧道的两端 Underlay IP
	ip netns exec host-a ip tunnel add tun0 mode gre local 10.0.12.1 remote 10.0.12.2 ttl 255
	ip netns exec host-a ip addr add 192.168.100.1/24 dev tun0
	ip netns exec host-a ip link set tun0 up

	# 在 host-b 上创建 GRE 隧道
	ip netns exec host-b ip tunnel add tun0 mode gre local 10.0.12.2 remote 10.0.12.1 ttl 255
	ip netns exec host-b ip addr add 192.168.100.2/24 dev tun0
	ip netns exec host-b ip link set tun0 up

	log "GRE 隧道创建完成"
}

function show_info() {
	log "=== GRE 隧道信息 ==="

	echo ""
	echo "[Host A - Underlay]"
	ip netns exec host-a ip -br addr show veth-a

	echo ""
	echo "[Host A - GRE 隧道]"
	ip netns exec host-a ip -br addr show tun0
	echo "  隧道模式: $(ip netns exec host-a cat /sys/class/net/gre0/type 2>/dev/null || echo 'gre')"

	echo ""
	echo "[Host B - Underlay]"
	ip netns exec host-b ip -br addr show veth-b

	echo ""
	echo "[Host B - GRE 隧道]"
	ip netns exec host-b ip -br addr show tun0

	echo ""
	echo "网络拓扑:"
	echo "  [Host A: 10.0.12.1] ===== GRE Tunnel ===== [Host B: 10.0.12.2]"
	echo "         [tun0: 192.168.100.1]             [tun0: 192.168.100.2]"
}

function test_connectivity() {
	log "=== 连通性测试 ==="

	echo ""
	echo "1. 测试 Underlay 网络连通性..."
	if ip netns exec host-a ping -c 3 -W 2 10.0.12.2 &>/dev/null; then
		echo "   [OK] Host A -> Host B (Underlay)"
	else
		echo "   [FAIL] Underlay 不通"
	fi

	echo ""
	echo "2. 测试 GRE 隧道连通性 (关键测试)..."
	if ip netns exec host-a ping -c 3 -W 2 192.168.100.2 &>/dev/null; then
		echo "   [OK] Host A -> Host B (GRE 隧道)"
	else
		echo "   [FAIL] GRE 隧道不通"
	fi

	if ip netns exec host-b ping -c 3 -W 2 192.168.100.1 &>/dev/null; then
		echo "   [OK] Host B -> Host A (GRE 隧道)"
	else
		echo "   [FAIL] GRE 隧道不通"
	fi
}

function show_packet() {
	echo ""
	echo "GRE 封装结构:"
	echo "  ┌─────────────────────────────────────────┐"
	echo "  │  外层 IP 头 (Underlay)                   │"
	echo "  │    Src: 10.0.12.1  Dst: 10.0.12.2       │"
	echo "  │    Protocol: 47 (GRE)                   │"
	echo "  ├─────────────────────────────────────────┤"
	echo "  │  GRE 头                                  │"
	echo "  │    Flags: 0x0000  Protocol: 0x0800      │"
	echo "  ├─────────────────────────────────────────┤"
	echo "  │  内层 IP 头 (Overlay/Payload)            │"
	echo "  │    Src: 192.168.100.1  Dst: 192.168.100.2│"
	echo "  │    Protocol: ICMP (ping)                │"
	echo "  ├─────────────────────────────────────────┤"
	echo "  │  数据 (ICMP Echo Request)                │"
	echo "  └─────────────────────────────────────────┘"
}

function usage() {
	cat <<EOF
用法: sudo $PROG_NAME [命令]

命令:
  setup     创建 GRE 隧道环境
  info      显示隧道信息
  test      测试连通性
  packet    显示 GRE 包结构
  cleanup   清理环境
  all       执行完整测试 (默认)

GRE 特点:
  - 最简单的 IP-in-IP 隧道
  - IP 协议号 47
  - 无加密，无状态检查
  - 可被 NAT 穿透 (但需配置)

EOF
}

function main() {
	case "${1:-all}" in
		setup)
			check_root
			setup
			setup_gre
			show_info
			;;
		info)
			show_info
			;;
		test)
			check_root
			test_connectivity
			;;
		packet)
			show_packet
			;;
		cleanup)
			check_root
			cleanup
			;;
		all)
			check_root
			cleanup
			setup
			setup_gre
			show_info
			test_connectivity
			show_packet
			log "运行 'sudo $PROG_NAME cleanup' 清理环境"
			;;
		help | -h | --help)
			usage
			;;
		*)
			echo "未知命令: $1"
			usage
			exit 1
			;;
	esac
}

main "$@"
