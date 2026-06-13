#!/usr/bin/env bash

set -E -e -u -o pipefail

function numa_status() {
	numastat -m | grep -E "Node|HugePages_"
}

function set_node_nr() {
	node=$1
	num=$2

	path=/sys/devices/system/node/node"$node"/hugepages/
	path+=hugepages-2048kB/nr_hugepages

	echo "$num" | sudo tee "$path"
	numa_status
}

function set_global_nr() {
	node_path=/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
	echo "$1" | sudo tee $node_path
	numa_status
}

function touch_hugepages() {
	gcc mm-hugetlb.c -o touch.out
	sudo numactl --membind="$1" ./touch.out &
	sleep 1
}

function bug() {
	# clear
	set_node_nr 0 100
	set_node_nr 1 100
	set_node_nr 0 0
	set_node_nr 1 0

	echo "make it"
	set_node_nr 0 100
	touch_hugepages 0
	set_node_nr 0 0

	echo "------------"
	set_node_nr 1 100
	# set_node_nr 1 0

	# 这是一个系统的问题
	set_global_nr 0
	# set_global_nr 0
	pkill touch.out
}

function bug2() {
	set_global_nr 0

	set_node_nr 0 100
	touch_hugepages 0
	set_node_nr 0 0

	echo ""
	cat /proc/sys/vm/nr_hugepages
	cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
	echo ""
	grep "HugePages_" /proc/meminfo
}

bug2
