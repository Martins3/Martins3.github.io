#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
pushd "$SCRIPT_DIR" >/dev/null
# shellcheck source=collei/scripts/collei-lib.sh
source ./collei-lib.sh
popd >/dev/null

linux_bridge="br9527"
ovs_bridge="br-in"

# https://stackoverflow.com/a/14541533
# 为了让无论从 collei-action.sh 调用，也会获取一个唯一的 id

# 返回值 @tap_name @mac_addr
function get_if() {
	local vif_counter
	vif_counter=$(cat "$vm_dir/$which_qemu"/vif_counter)

	if [[ $vif_counter -eq 10 ]]; then
		error "too many nic"
	fi

	if [[ -z $which_qemu ]]; then
		error "which one ?"
	fi

	local mac_level
	local mac_guest
	local mac_idx
	mac_level=$(printf "%02x\n" "$host_level")
	mac_guest=$(printf "%02x\n" "$guest_id")
	mac_idx=$(printf "%02x\n" "$vif_counter")

	tap_name=vif_${which_qemu}_${guest_id}_${vif_counter}
	mac_addr="52:54:00:$mac_level:$mac_guest:$mac_idx"
	log "$mac_addr"

	vif_counter=$((vif_counter + 1))
	echo $vif_counter >"$vm_dir/$which_qemu"/vif_counter
}

# 如果是新建的 tap 设备，那么就 attach 到 switch 上
new_tap=false
function tap_create() {
	new_tap=false
	if ! ip link show dev "$tap_name" >/dev/null 2>&1; then
		sudo ip tuntap add mode tap user "$USER" dev "$tap_name"
		new_tap=true
	fi
	if ! ip link show "$tap_name" | grep -q ',UP'; then
		sudo ip link set "$tap_name" up
	fi
}

# 提供整个机器唯一的 name 和本虚拟机中唯一的 num
function create_ovs_tap() {
	get_if
	tap_create "$tap_name"
	if [[ $new_tap != true ]]; then
		return
	fi
	sudo ovs-vsctl list-ports br-in | tee /tmp/martins3/ovs-ports >/dev/null
	if ! grep "$tap_name" /tmp/martins3/ovs-ports; then
		sudo ovs-vsctl add-port $ovs_bridge "$tap_name"
		# sudo ovs-vsctl add-port "$ovs_bridge" "$tap_name" tag=100
		# sudo ovs-vsctl set port $tap_name tag=100
	fi
}

function create_linux_bridge_tap() {
	get_if
	tap_create "$tap_name"
	if [[ $new_tap != true ]]; then
		return
	fi
	sudo ip link set dev "$tap_name" master $linux_bridge
}

function create_switch_tap() {
	case $network_switch in
		ovs) create_ovs_tap ;;
		bridge) create_linux_bridge_tap ;;
	esac
}

function create_orphan_tap() {
	get_if
	tap_name=${tap_name//vif/tap}
	tap_create "$tap_name"
	subnet=172.213.0
	if ! ip addr show "$tap_name" | grep "172\."; then
		sudo ip address add $subnet."${guest_id}"/24 dev "$tap_name"
		sudo ip link set "$tap_name" up
	fi

	# 这个 dns 根本没有生效过，dns 也是一个有趣的问题
	# sudo dnsmasq --port=0 --no-resolv --no-hosts --bind-interfaces \
	# 	--interface "$device" -F $subnet.2,$subnet.20 --listen-address $subnet.1 \
	# 	-x /tmp/dnsmasq-"$device".pid -l /tmp/dnsmasq-"$device".leases || true

}

function init_switch() {
	case $network_switch in
		ovs)
			if ! ip link show dev $ovs_bridge &>/dev/null; then
				sudo ovs-vsctl add-br $ovs_bridge
				sudo ip link set $ovs_bridge up
				echo "attach a nic to $ovs_bridge:"
				echo "sudo ovs-vsctl add-port $ovs_bridge ens5"
				error "abort"
			fi

			if ! ip addr show $ovs_bridge | grep "10\.0"; then
				sudo ip address add "$ovs_br_ip"/16 dev $ovs_bridge
				sudo ip link set $ovs_bridge up
			fi
			# 重建的方法
			# sudo ip link delete vif14.0
			# sudo ovs-vsctl del-br $ovs_bridge
			# sudo ovs-vsctl add-br $ovs_bridge
			# sudo ip address add 10.0.0.2/16 dev $ovs_bridge
			# sudo ifconfig $ovs_bridge up
			;;

		bridge)
			# host setup
			if ! ip link show dev $linux_bridge &>/dev/null; then
				sudo ip link add $linux_bridge type bridge
				echo "sudo ip link set enp7s0 master br0"
			fi

			if ! ifconfig $linux_bridge | grep 'inet addr:' >/dev/null; then
				sudo ip address add "$ovs_br_ip"/16 dev $linux_bridge
				sudo ip link set dev $linux_bridge up
			fi
			;;
		*)
			error "unknown switch"
			;;
	esac
}
