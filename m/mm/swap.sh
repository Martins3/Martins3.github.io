#!/usr/bin/env bash
set -E -e -u -o pipefail

cgroup_name=swap_test

set -x
PROGDIR=$(readlink -m "$(dirname "$0")")

function setup_cgroup() {
	if ! stat -fc %T /sys/fs/cgroup/ | grep cgroup2fs; then
		echo "only support cgroup v2"
		exit 0
	fi
	if [[ -d /sys/fs/cgroup/$cgroup_name ]]; then
		return 0
	fi

	sudo cgcreate -g memory:$cgroup_name
	sudo cgset -r memory.max=100m $cgroup_name

}

function setup_lru_gen() {
	echo n | sudo tee /sys/kernel/mm/lru_gen/enabled
	cat /sys/kernel/mm/lru_gen/enabled
}

function create_io() {
	# 🧀  free -m
	#                total        used        free      shared  buff/cache   available
	# Mem:            7398        4384          60         817        2953        2093
	# Swap:           8071           4        8067

	available=$(free -m | grep Mem | awk '{print $7}')
	swap_free=$(free -m | grep Swap | awk '{print $4}')
	if [[ $swap_free == 0 ]]; then
		echo "no swap space"
		exit 1
	fi
	size=$((available + swap_free / 2))
	stress-ng --vm-bytes ${size}m --vm-keep --vm 1
}

function create_cgroup_swap_io() {
	# 如果当前目录实际上是一个 nfs ，那么可能会有这个问题
	# aborting: temp-path '.' must be readable and writeable
	pushd /tmp
	sudo cgexec -g memory:$cgroup_name stress-ng --vm-bytes 400m --vm-keep --vm 1
	pushd "$PROGDIR"
	sudo cgexec -g memory:$cgroup_name ./gup.out 0
}

function create_swap_file() {
	local swapfile=/swapfile
	sudo fallocate -l 1G $swapfile
	sudo chmod 600 $swapfile
	sudo mkswap $swapfile
	sudo swapon $swapfile
	sudo swapon --show
}

# 测试 cgroup 的 swap
# FIXME cgroup 在还有那么多 swap 空间的情况下，为什么还是会 oom 啊
function test1() {
	setup_lru_gen
	setup_cgroup
	create_cgroup_swap_io
}

function test2() {
	create_io
}

test1
