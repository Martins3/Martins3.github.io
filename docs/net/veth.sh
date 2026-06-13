#!/usr/bin/env bash
set -E -e -u -o pipefail

mkdir -p /tmp/martins3
echo 10 >/tmp/martins3/counter

use_ovs=true

function get_ip() {
	a=$(cat /tmp/martins3/counter)
	a=$((a + 1))
	echo $a >/tmp/martins3/counter
	echo $a
}

function add_ns() {
	ns=$1
	if ip netns list | grep "$ns"; then
		echo "namespace $ns already added"
	else
		sudo ip netns add "$ns"
	fi

}

function add_veth_to_ns() {
	ns=$1
	veth0=$2.0
	veth1=$2.1

	if ip link list "$veth1"; then
		return
	fi

	# 创建两个 veth ，0 在 netns 里面 ，1 在 netns 外面
	sudo ip link add "$veth1" type veth peer name "$veth0" || true

	sudo ip link set "$veth0" netns "$ns" || true

	sudo ip netns exec "$ns" ip link list "$veth0" # 展示 blue 的结果

	ip=10.0.1."$(get_ip)"
	sudo ip netns exec "$ns" ip addr add "$ip"/24 dev "$veth0" || true
	sudo ip netns exec "$ns" ip link set dev "$veth0" up
	# 默认 lo 没有 up ，导致 ping 127.0.0.1 失败
	sudo ip netns exec "$ns" ip link set dev lo up

	sudo ip netns exec "$ns" route add default gw "$ip" "$veth0"

	# TODO 如果给外面的 veth 配置上 ip ，那么整个 bridge 上连接的所有 ip 无法联通
	# sudo ip addr add 10.1.0."$(get_ip)"/24 dev "$veth1" || true
	sudo ip link set dev "$veth1" up
}

function add_br() {
	sudo brctl addbr br0 || true
	sudo ip addr add 10.1.0."$(get_ip)"/24 dev br0 || true
	sudo ip link set br0 up
}

function connect_to_br() {
	veth=$1
	sudo ip link set dev "$veth" master br0
}

function add_ovs() {
	if ! ifconfig br-in | grep "10.0.0"; then
		echo "run alpine.sh to create br-in"
	fi
}

function connect_to_ovs() {
	veth=$1
	sudo ovs-vsctl add-port br-in "$veth"
}

function clean_up_ns() {
	if [[ $use_ovs == true ]]; then
		sudo ovs-vsctl del-port C.1
		sudo ovs-vsctl del-port D.1
	else
		sudo ifconfig br0 down
		sudo brctl delbr br0
	fi

	sudo ip link delete C.1
	sudo ip link delete D.1
}

function create_ns() {

	add_ns C
	add_ns D
	add_veth_to_ns C C
	add_veth_to_ns D D

	if [[ $use_ovs == true ]]; then
		add_ovs
		connect_to_ovs C.1
		connect_to_ovs D.1
	else
		add_br
		connect_to_br C.1
		connect_to_br D.1
	fi
	echo "try to switch to the ns:"
	echo "sudo ip netns exec D bash"
	echo "sudo ip netns exec C bash"
}

function help(){
	echo "-c : create"
	echo "-C : clear"
	exit 1
}
while getopts "hcC" opt; do
	case $opt in
		c) create_ns ;;
		C) clean_up_ns ;;
		h) echo "help" ;;
		*)
			help
			;;
	esac
done
help
