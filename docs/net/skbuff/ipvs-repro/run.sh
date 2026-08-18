#!/usr/bin/env bash
# 复现 v15: netem delay 放大 RTT 窗口 + 高频瞬时 unreachable
# crash 窗口 = 加 unreachable 后 RS 还能发 dup ACK 的过渡期 ≈ 1 RTT
# 本地 veth RTT 微秒级窗口极窄, 用 netem delay 放大 RTT,
# 让 fast retransmit 中间段有更长时间命中 err_unreach
set -E -e -u -o pipefail
cd "$(dirname "$0")"

RS=10.200.0.2
NS=repro
ROUNDS=20

# 幂等清理: 每轮开始前和脚本退出时还原环境, 命令失败属于正常情况
function cleanup() {
	set +e
	pkill -9 -f "repro.py client"
	pkill -9 -f "repro.py server"
	python3 repro.py flush
	tc qdisc del dev veth-h root
	ip link del veth-h
	ip netns del "$NS"
	ip link del dummy-vip
	ip route del unreachable "$RS/32"
	set -e
}
trap cleanup EXIT

modprobe ip_vs ip_vs_rr

for round in $(seq "$ROUNDS"); do
	echo "======== round $round/$ROUNDS ========"
	cleanup

	ip netns add "$NS"
	ip link add veth-h type veth peer name veth-p
	ip link set veth-p netns "$NS"
	ip addr add 10.200.0.1/24 dev veth-h
	ip link set veth-h up
	ip netns exec "$NS" ip addr add "$RS/24" dev veth-p
	ip netns exec "$NS" ip link set veth-p up
	ip netns exec "$NS" ip link set lo up
	ip netns exec "$NS" ip route add default via 10.200.0.1
	ip link add dummy-vip type dummy
	ip link set dummy-vip up
	ip addr add 10.99.0.100/32 dev dummy-vip
	python3 repro.py setup

	ip netns exec "$NS" python3 repro.py server &
	sleep 1
	tc qdisc add dev veth-h root netem delay 100ms loss 25%
	python3 repro.py client &
	sleep 8
	ping -c 1 -W 1 "$RS" | grep -oE 'time=[0-9.]+ ms' || true

	for i in $(seq 600); do
		ip route add unreachable "$RS/32" 2>/dev/null || true
		sleep 0.02
		ip route del unreachable "$RS/32" 2>/dev/null || true
		sleep 0.02
		if ((i % 100 == 0)); then
			echo "unreachable 切换 $i/600"
		fi
	done

	dmesg | grep -iE 'panic|oops|BUG:|unable to handle|general protection' | tail -5 || true
	echo "--- round $round alive: $(uname -r) ---"
done
