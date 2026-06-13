#!/usr/bin/env bash
set -E -e -u -o pipefail

PROG_NAME="$(basename "$0")"

NS_LEFT="${NS_LEFT:-tc-left}"
NS_RIGHT="${NS_RIGHT:-tc-right}"
DEV_LEFT="${DEV_LEFT:-veth-tc-left}"
DEV_RIGHT="${DEV_RIGHT:-veth-tc-right}"

LEFT_IP_CIDR="${LEFT_IP_CIDR:-10.88.0.1/24}"
RIGHT_IP_CIDR="${RIGHT_IP_CIDR:-10.88.0.2/24}"
LEFT_IP="${LEFT_IP_CIDR%/*}"
RIGHT_IP="${RIGHT_IP_CIDR%/*}"

PING_COUNT="${PING_COUNT:-6}"
PING_INTERVAL="${PING_INTERVAL:-0.2}"
NETEM_DELAY="${NETEM_DELAY:-80ms}"
NETEM_LOSS="${NETEM_LOSS:-30%}"
TBF_RATE="${TBF_RATE:-1mbit}"
TBF_BURST="${TBF_BURST:-32kbit}"
TBF_LATENCY="${TBF_LATENCY:-400ms}"
IPERF_PORT="${IPERF_PORT:-5201}"

IP_BIN="${IP_BIN:-$(command -v ip || true)}"
TC_BIN="${TC_BIN:-$(command -v tc || true)}"
PING_BIN="${PING_BIN:-$(command -v ping || true)}"
IPERF3_BIN="${IPERF3_BIN:-$(command -v iperf3 || true)}"

function log() {
	echo "[$(date '+%H:%M:%S')] $*"
}

function die() {
	echo "Error: $*" >&2
	exit 1
}

function check_root() {
	if [[ $EUID -ne 0 ]]; then
		die "请用 root 运行。若 tc 来自 nix-shell，请使用: sudo env PATH=\"\$PATH\" ./$PROG_NAME all"
	fi
}

function require_cmd() {
	local name=$1
	local path=$2

	if [[ -z $path ]]; then
		die "缺少命令 $name。可以先进入: nix-shell -p iproute2 iputils iperf3"
	fi
}

function ip_cmd() {
	"$IP_BIN" "$@"
}

function tc_cmd() {
	"$TC_BIN" "$@"
}

function require_base_cmds() {
	require_cmd ip "$IP_BIN"
	require_cmd tc "$TC_BIN"
	require_cmd ping "$PING_BIN"
}

function cleanup() {
	log "清理 namespace 和 veth ..."
	ip_cmd netns del "$NS_LEFT" 2>/dev/null || true
	ip_cmd netns del "$NS_RIGHT" 2>/dev/null || true
}

function setup_env() {
	log "创建测试拓扑: $NS_LEFT <-> $NS_RIGHT"
	cleanup

	ip_cmd netns add "$NS_LEFT"
	ip_cmd netns add "$NS_RIGHT"

	ip_cmd link add "$DEV_LEFT" type veth peer name "$DEV_RIGHT"
	ip_cmd link set "$DEV_LEFT" netns "$NS_LEFT"
	ip_cmd link set "$DEV_RIGHT" netns "$NS_RIGHT"

	ip_cmd -n "$NS_LEFT" addr add "$LEFT_IP_CIDR" dev "$DEV_LEFT"
	ip_cmd -n "$NS_RIGHT" addr add "$RIGHT_IP_CIDR" dev "$DEV_RIGHT"

	ip_cmd -n "$NS_LEFT" link set lo up
	ip_cmd -n "$NS_RIGHT" link set lo up
	ip_cmd -n "$NS_LEFT" link set "$DEV_LEFT" up
	ip_cmd -n "$NS_RIGHT" link set "$DEV_RIGHT" up

	show_topology
}

function ensure_env_exists() {
	if ! ip_cmd netns list | awk '{print $1}' | grep -Fx "$NS_LEFT" >/dev/null; then
		die "未找到 namespace $NS_LEFT，请先运行 $PROG_NAME setup"
	fi

	if ! ip_cmd netns list | awk '{print $1}' | grep -Fx "$NS_RIGHT" >/dev/null; then
		die "未找到 namespace $NS_RIGHT，请先运行 $PROG_NAME setup"
	fi
}

function show_topology() {
	echo
	echo "[topology]"
	echo "  $NS_LEFT:  $DEV_LEFT  $LEFT_IP"
	echo "  $NS_RIGHT: $DEV_RIGHT  $RIGHT_IP"
	echo

	echo "[ip -br addr]"
	ip_cmd -n "$NS_LEFT" -br addr show
	ip_cmd -n "$NS_RIGHT" -br addr show
}

function reset_tc() {
	tc_cmd -n "$NS_LEFT" qdisc del dev "$DEV_LEFT" root 2>/dev/null || true
	tc_cmd -n "$NS_RIGHT" qdisc del dev "$DEV_RIGHT" root 2>/dev/null || true
	tc_cmd -n "$NS_LEFT" qdisc del dev "$DEV_LEFT" clsact 2>/dev/null || true
	tc_cmd -n "$NS_RIGHT" qdisc del dev "$DEV_RIGHT" clsact 2>/dev/null || true
}

function show_tc_state() {
	echo
	echo "[qdisc]"
	tc_cmd -n "$NS_LEFT" -s qdisc show dev "$DEV_LEFT"
	tc_cmd -n "$NS_RIGHT" -s qdisc show dev "$DEV_RIGHT"
	echo
	echo "[filter]"
	tc_cmd -n "$NS_LEFT" filter show dev "$DEV_LEFT" ingress 2>/dev/null || true
	tc_cmd -n "$NS_RIGHT" filter show dev "$DEV_RIGHT" ingress 2>/dev/null || true
}

function baseline() {
	log "baseline: 不加任何 tc 规则，确认 veth 连通"
	reset_tc
	show_tc_state
	ip_cmd netns exec "$NS_LEFT" \
		"$PING_BIN" -n -c "$PING_COUNT" -i "$PING_INTERVAL" "$RIGHT_IP"
}

function delay_test() {
	log "delay: 在双向 egress 上挂 netem delay $NETEM_DELAY"
	reset_tc
	tc_cmd -n "$NS_LEFT" qdisc add dev "$DEV_LEFT" root netem delay "$NETEM_DELAY"
	tc_cmd -n "$NS_RIGHT" qdisc add dev "$DEV_RIGHT" root netem delay "$NETEM_DELAY"
	show_tc_state
	ip_cmd netns exec "$NS_LEFT" \
		"$PING_BIN" -n -c "$PING_COUNT" -i "$PING_INTERVAL" "$RIGHT_IP"
}

function loss_test() {
	log "loss: 在 $NS_LEFT egress 上挂 netem loss $NETEM_LOSS"
	reset_tc
	tc_cmd -n "$NS_LEFT" qdisc add dev "$DEV_LEFT" root netem loss "$NETEM_LOSS"
	show_tc_state
	ip_cmd netns exec "$NS_LEFT" \
		"$PING_BIN" -n -c "$PING_COUNT" -i "$PING_INTERVAL" "$RIGHT_IP"
}

function rate_test() {
	local server_pid=""

	log "rate: 在 $NS_LEFT egress 上挂 tbf rate $TBF_RATE"
	reset_tc
	tc_cmd -n "$NS_LEFT" qdisc add dev "$DEV_LEFT" root tbf \
		rate "$TBF_RATE" burst "$TBF_BURST" latency "$TBF_LATENCY"
	show_tc_state

	if [[ -z $IPERF3_BIN ]]; then
		echo "[SKIP] 未找到 iperf3，跳过吞吐测试"
		echo "       可使用 nix-shell -p iproute2 iputils iperf3"
		return 0
	fi

	ip_cmd netns exec "$NS_RIGHT" \
		"$IPERF3_BIN" -s -1 -p "$IPERF_PORT" >/tmp/tc-demo-iperf3.log 2>&1 &
	server_pid=$!
	sleep 1

	ip_cmd netns exec "$NS_LEFT" \
		"$IPERF3_BIN" -c "$RIGHT_IP" -p "$IPERF_PORT" -t 5

	wait "$server_pid"
}

function filter_test() {
	log "filter: 在 $NS_RIGHT ingress 上挂 clsact + u32 + drop icmp"
	reset_tc
	tc_cmd -n "$NS_RIGHT" qdisc add dev "$DEV_RIGHT" clsact
	tc_cmd -n "$NS_RIGHT" filter add dev "$DEV_RIGHT" ingress protocol ip pref 1 u32 \
		match ip protocol 1 0xff action drop
	show_tc_state

	set +e
	ip_cmd netns exec "$NS_LEFT" \
		"$PING_BIN" -n -c 3 -i "$PING_INTERVAL" -W 1 "$RIGHT_IP"
	local rc=$?
	set -e

	if [[ $rc -eq 0 ]]; then
		die "filter 实验失败: ICMP 本应被 drop"
	fi

	echo "[OK] ICMP 已被 ingress filter 丢弃"
}

function print_cmd_examples() {
	cat <<EOF
常用命令:
  tc -n $NS_LEFT qdisc show dev $DEV_LEFT
  tc -n $NS_RIGHT filter show dev $DEV_RIGHT ingress
  ip netns exec $NS_LEFT ping -n -c 4 $RIGHT_IP

手动实验:
  tc -n $NS_LEFT qdisc add dev $DEV_LEFT root netem delay 100ms
  tc -n $NS_LEFT qdisc change dev $DEV_LEFT root netem delay 20ms loss 10%
  tc -n $NS_LEFT qdisc del dev $DEV_LEFT root
  tc -n $NS_RIGHT qdisc add dev $DEV_RIGHT clsact
  tc -n $NS_RIGHT filter add dev $DEV_RIGHT ingress protocol ip pref 1 u32 \\
    match ip protocol 1 0xff action drop
EOF
}

function usage() {
	cat <<EOF
用法: sudo env PATH="\$PATH" ./$PROG_NAME [命令]

命令:
  setup      创建 netns + veth 测试拓扑
  show       显示拓扑和当前 tc 状态
  baseline   连通性基线测试
  delay      netem 延迟实验
  loss       netem 丢包实验
  rate       tbf 限速实验
  filter     clsact + u32 过滤实验
  cleanup    清理测试环境
  cmd        显示常用 tc 命令
  all        从 setup 开始依次运行全部实验

环境变量:
  NETEM_DELAY=$NETEM_DELAY
  NETEM_LOSS=$NETEM_LOSS
  TBF_RATE=$TBF_RATE
  TBF_BURST=$TBF_BURST
  TBF_LATENCY=$TBF_LATENCY

示例:
  nix-shell -p iproute2 iputils --run 'printf "a\\n" | sudo -S env PATH="\$PATH" ./net/tc-demo.sh all'
  nix-shell -p iproute2 iputils iperf3 --run 'printf "a\\n" | sudo -S env PATH="\$PATH" ./net/tc-demo.sh rate'
EOF
}

function main() {
	require_base_cmds

	case "${1:-all}" in
		setup)
			check_root
			setup_env
			;;
		show)
			check_root
			ensure_env_exists
			show_topology
			show_tc_state
			;;
		baseline)
			check_root
			setup_env
			baseline
			;;
		delay)
			check_root
			setup_env
			delay_test
			;;
		loss)
			check_root
			setup_env
			loss_test
			;;
		rate)
			check_root
			setup_env
			rate_test
			;;
		filter)
			check_root
			setup_env
			filter_test
			;;
		cleanup)
			check_root
			cleanup
			;;
		cmd)
			print_cmd_examples
			;;
		all)
			check_root
			setup_env
			baseline
			delay_test
			loss_test
			rate_test
			filter_test
			log "实验完成，可运行 'sudo env PATH=\"\$PATH\" ./$PROG_NAME cleanup' 清理"
			;;
		help | -h | --help)
			usage
			;;
		*)
			usage
			exit 1
			;;
	esac
}

main "$@"
