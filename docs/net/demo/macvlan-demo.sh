#!/usr/bin/env bash
set -E -e -u -o pipefail

# macvlan demo: 验证多个 macvlan 设备之间的通信
# macvlan 允许单个物理接口拥有多个 MAC 地址，每个虚拟接口独立

PROG_NAME="$(basename "$0")"
PHYSICAL_DEV="${PHYSICAL_DEV:-ens5}"
IP_PREFIX="${IP_PREFIX:-192.168.100}"

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
	log "清理 macvlan 设备..."
	ip netns del macvlan-ns1 2>/dev/null || true
	ip netns del macvlan-ns2 2>/dev/null || true
}

function setup() {
	log "=== 创建两个 macvlan 设备 ==="

	# 检查物理设备是否存在
	if ! ip link show "$PHYSICAL_DEV" &>/dev/null; then
		echo "Error: 物理设备 $PHYSICAL_DEV 不存在"
		echo "可用设备: $(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | grep -v lo | tr '\n' ' ')"
		exit 1
	fi

	log "物理设备: $PHYSICAL_DEV"

	# 创建两个网络命名空间
	ip netns add macvlan-ns1 2>/dev/null || true
	ip netns add macvlan-ns2 2>/dev/null || true

	# 创建两个 macvlan 设备 (mode bridge 允许它们之间通信)
	ip link add macvlan0 link "$PHYSICAL_DEV" type macvlan mode bridge
	ip link add macvlan1 link "$PHYSICAL_DEV" type macvlan mode bridge

	# 将 macvlan 设备分别移入命名空间
	ip link set macvlan0 netns macvlan-ns1
	ip link set macvlan1 netns macvlan-ns2

	# 配置 macvlan0
	ip netns exec macvlan-ns1 ip addr add "${IP_PREFIX}.2/24" dev macvlan0
	ip netns exec macvlan-ns1 ip link set macvlan0 up
	ip netns exec macvlan-ns1 ip link set lo up

	# 配置 macvlan1
	ip netns exec macvlan-ns2 ip addr add "${IP_PREFIX}.3/24" dev macvlan1
	ip netns exec macvlan-ns2 ip link set macvlan1 up
	ip netns exec macvlan-ns2 ip link set lo up

	log "macvlan 设备创建完成"
}

function show_info() {
	log "=== macvlan 设备信息 ==="

	echo ""
	echo "[物理设备 $PHYSICAL_DEV]"
	echo "  MAC: $(cat "/sys/class/net/$PHYSICAL_DEV/address")"

	echo ""
	echo "[macvlan0 @ macvlan-ns1]"
	echo "  IP:  ${IP_PREFIX}.2"
	echo "  MAC: $(ip netns exec macvlan-ns1 cat /sys/class/net/macvlan0/address 2>/dev/null || echo 'N/A')"

	echo ""
	echo "[macvlan1 @ macvlan-ns2]"
	echo "  IP:  ${IP_PREFIX}.3"
	echo "  MAC: $(ip netns exec macvlan-ns2 cat /sys/class/net/macvlan1/address 2>/dev/null || echo 'N/A')"

	echo ""
	echo "注意: 两个 macvlan 设备有独立的 MAC 地址"
}

function test_connectivity() {
	log "=== 连通性测试 ==="

	# 在物理设备上添加临时 IP 用于测试
	ip addr add "${IP_PREFIX}.1/24" dev "$PHYSICAL_DEV" 2>/dev/null || true

	echo ""
	echo "1. 测试 macvlan0 到主机的连通性..."
	if ip netns exec macvlan-ns1 ping -c 3 -W 2 "${IP_PREFIX}.1" &>/dev/null; then
		echo "   [OK] ping ${IP_PREFIX}.1 成功"
	else
		echo "   [FAIL] ping ${IP_PREFIX}.1 失败"
	fi

	echo ""
	echo "2. 测试 macvlan1 到主机的连通性..."
	if ip netns exec macvlan-ns2 ping -c 3 -W 2 "${IP_PREFIX}.1" &>/dev/null; then
		echo "   [OK] ping ${IP_PREFIX}.1 成功"
	else
		echo "   [FAIL] ping ${IP_PREFIX}.1 失败"
	fi

	echo ""
	echo "3. 测试两个 macvlan 之间的通信 (关键测试)..."
	if ip netns exec macvlan-ns1 ping -c 3 -W 2 "${IP_PREFIX}.3" &>/dev/null; then
		echo "   [OK] macvlan0 -> macvlan1 通信成功"
	else
		echo "   [FAIL] macvlan0 -> macvlan1 通信失败"
	fi

	if ip netns exec macvlan-ns2 ping -c 3 -W 2 "${IP_PREFIX}.2" &>/dev/null; then
		echo "   [OK] macvlan1 -> macvlan0 通信成功"
	else
		echo "   [FAIL] macvlan1 -> macvlan0 通信失败"
	fi

	# 清理临时 IP
	ip addr del "${IP_PREFIX}.1/24" dev "$PHYSICAL_DEV" 2>/dev/null || true
}

function usage() {
	cat <<EOF
用法: sudo $PROG_NAME [命令]

命令:
  setup     创建两个 macvlan 设备
  info      显示 macvlan 信息
  test      测试连通性
  cleanup   清理所有 macvlan 设备
  all       执行完整测试 (默认)

选项:
  PHYSICAL_DEV=eth0       指定物理网卡 (默认: ens5)
  IP_PREFIX=192.168.100   IP 地址前缀

示例:
  sudo $PROG_NAME
  sudo PHYSICAL_DEV=ens33 $PROG_NAME
EOF
}

function main() {
	case "${1:-all}" in
		setup)
			check_root
			setup
			show_info
			;;
		info)
			show_info
			;;
		test)
			check_root
			test_connectivity
			;;
		cleanup)
			check_root
			cleanup
			;;
		all)
			check_root
			cleanup
			setup
			show_info
			test_connectivity
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
