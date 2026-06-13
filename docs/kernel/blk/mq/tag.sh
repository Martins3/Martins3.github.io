#!/usr/bin/env bash
set -E -e -u -o pipefail

function nr_requests() {
	readarray -t array < <(find /sys/ -name nr_requests 2>/dev/null)
	for i in "${array[@]}"; do
		printf "%s %d\n" "$i" "$(cat "$i")"
	done
}

# TODO queue_depth 和 nr_requests 是一个东西吗?
# nvme 和 scsi 都实现了自己的 queue_depth 的
function queue_depth() {
	readarray -t array < <(find /sys -name queue_depth 2>/dev/null)
	for i in "${array[@]}"; do
		printf "%s %d\n" "$i" "$(cat "$i")"
	done

}

function tag_set() {
	shopt -s extglob nullglob

	for f in /sys/kernel/debug/block/*/hctx0/tags; do
		echo "$(dirname "$f") "
		grep -E "tags|depth" "$f"
	done

	for f in /sys/kernel/debug/block/*/hctx0/sched_tags; do
		echo "$(dirname "$f") "
		grep -E "tags|depth" "$f"
	done
}

function show_scsi() {
	cat /sys/class/scsi_host/host*/can_queue # megaraid 是 5089 / 其他都是 32
	# 和 debugfs 的 tag_set 的 queue_depth=7500 完全对应的
	cat /sys/class/scsi_host/host*/cmd_per_lun # megarid 是 256 ，其他都是 0
}

function show_nvme() {
	# 这个就是最大的硬件队列数量
	cat /sys/class/nvme/*/queue_count
	# 哦，原来很多设备都是可以配置的
	echo "module:"
	cat /sys/module/nvme/parameters/io_queue_depth
	cat /sys/module/loop/parameters/hw_queue_depth
	cat /sys/module/dm_mod/parameters/dm_mq_queue_depth
	cat /sys/module/virtio_blk/parameters/queue_depth
}

# 显然第一个问题就是，queue 的 depth 比 tag_set 更大
# /sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:17/0:0:17:0/block/sdd/queue/nr_requests 256
# queue_depth=1505
#
# 如何理解 dm 的 nr_requests
# /sys/devices/virtual/block/dm-1/queue/nr_requests
#
# 13900k queue_depth 对于 nvme 似乎没有意义
#
# /sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/queue_depth 32
# /sys/devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/queue_depth 32
# /sys/devices/pci0000:00/0000:00:1a.0/0000:03:00.0/nvme/nvme1/nvme1n1/queue_depth 0
# /sys/devices/pci0000:00/0000:00:1d.0/0000:06:00.0/nvme/nvme0/nvme0n1/queue_depth 0
# /sys/module/virtio_blk/parameters/queue_depth 0

# nr_requests
# tag_set
# queue_depth
show_nvme
