#!/usr/bin/env bash
set -E -e -u -o pipefail

# ipvlan demo: 验证多个 ipvlan 设备之间的通信
# ipvlan 类似于 macvlan，但所有虚拟接口共享同一个 MAC 地址

PROG_NAME="$(basename "$0")"
PHYSICAL_DEV="${PHYSICAL_DEV:-ens5}"
IP_PREFIX="${IP_PREFIX:-192.168.200}"

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
	log "清理 ipvlan 设备..."
	ip netns del ipvlan-ns1 2>/dev/null || true
	ip netns del ipvlan-ns2 2>/dev/null || true
}

function setup() {
	log "=== 创建两个 ipvlan 设备 ==="

	# 检查物理设备是否存在
	if ! ip link show "$PHYSICAL_DEV" &>/dev/null; then
		echo "Error: 物理设备 $PHYSICAL_DEV 不存在"
		echo "可用设备: $(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | grep -v lo | tr '\n' ' ')"
		exit 1
	fi

	log "物理设备: $PHYSICAL_DEV"

	# 创建两个网络命名空间
	ip netns add ipvlan-ns1 2>/dev/null || true
	ip netns add ipvlan-ns2 2>/dev/null || true

	# 创建两个 ipvlan 设备 (mode l2)
	ip link add ipvlan0 link "$PHYSICAL_DEV" type ipvlan mode l2
	ip link add ipvlan1 link "$PHYSICAL_DEV" type ipvlan mode l2

	# 将 ipvlan 设备分别移入命名空间
	ip link set ipvlan0 netns ipvlan-ns1
	ip link set ipvlan1 netns ipvlan-ns2

	# 配置 ipvlan0
	ip netns exec ipvlan-ns1 ip addr add "${IP_PREFIX}.2/24" dev ipvlan0
	ip netns exec ipvlan-ns1 ip link set ipvlan0 up
	ip netns exec ipvlan-ns1 ip link set lo up

	# 配置 ipvlan1
	ip netns exec ipvlan-ns2 ip addr add "${IP_PREFIX}.3/24" dev ipvlan1
	ip netns exec ipvlan-ns2 ip link set ipvlan1 up
	ip netns exec ipvlan-ns2 ip link set lo up

	log "ipvlan 设备创建完成"
}

function show_info() {
	log "=== ipvlan 设备信息 ==="

	echo ""
	echo "[物理设备 $PHYSICAL_DEV]"
	echo "  MAC: $(cat "/sys/class/net/$PHYSICAL_DEV/address")"

	echo ""
	echo "[ipvlan0 @ ipvlan-ns1]"
	echo "  IP:  ${IP_PREFIX}.2"
	echo "  MAC: $(ip netns exec ipvlan-ns1 cat /sys/class/net/ipvlan0/address 2>/dev/null || echo 'N/A')"

	echo ""
	echo "[ipvlan1 @ ipvlan-ns2]"
	echo "  IP:  ${IP_PREFIX}.3"
	echo "  MAC: $(ip netns exec ipvlan-ns2 cat /sys/class/net/ipvlan1/address 2>/dev/null || echo 'N/A')"

	echo ""
	echo "注意: 两个 ipvlan 设备共享物理设备的 MAC 地址"
}

function test_connectivity() {
	log "=== 连通性测试 ==="

	# 在物理设备上添加临时 IP 用于测试
	ip addr add "${IP_PREFIX}.1/24" dev "$PHYSICAL_DEV" 2>/dev/null || true

	echo ""
	echo "1. 测试 ipvlan0 到主机的连通性..."
	if ip netns exec ipvlan-ns1 ping -c 3 -W 2 "${IP_PREFIX}.1" &>/dev/null; then
		echo "   [OK] ping ${IP_PREFIX}.1 成功"
	else
		echo "   [FAIL] ping ${IP_PREFIX}.1 失败"
	fi

	echo ""
	echo "2. 测试 ipvlan1 到主机的连通性..."
	if ip netns exec ipvlan-ns2 ping -c 3 -W 2 "${IP_PREFIX}.1" &>/dev/null; then
		echo "   [OK] ping ${IP_PREFIX}.1 成功"
	else
		echo "   [FAIL] ping ${IP_PREFIX}.1 失败"
	fi

	echo ""
	echo "3. 测试两个 ipvlan 之间的通信 (关键测试)..."
	if ip netns exec ipvlan-ns1 ping -c 3 -W 2 "${IP_PREFIX}.3" &>/dev/null; then
		echo "   [OK] ipvlan0 -> ipvlan1 通信成功"
	else
		echo "   [FAIL] ipvlan0 -> ipvlan1 通信失败"
	fi

	if ip netns exec ipvlan-ns2 ping -c 3 -W 2 "${IP_PREFIX}.2" &>/dev/null; then
		echo "   [OK] ipvlan1 -> ipvlan0 通信成功"
	else
		echo "   [FAIL] ipvlan1 -> ipvlan0 通信失败"
	fi

	# 清理临时 IP
	ip addr del "${IP_PREFIX}.1/24" dev "$PHYSICAL_DEV" 2>/dev/null || true
}

function usage() {
	cat <<EOF
用法: sudo $PROG_NAME [命令]

命令:
  setup     创建两个 ipvlan 设备
  info      显示 ipvlan 信息
  test      测试连通性
  cleanup   清理所有 ipvlan 设备
  all       执行完整测试 (默认)

选项:
  PHYSICAL_DEV=eth0       指定物理网卡 (默认: ens5)
  IP_PREFIX=192.168.200   IP 地址前缀

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
