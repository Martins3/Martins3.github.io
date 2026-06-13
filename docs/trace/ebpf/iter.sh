#!/usr/bin/env bash
set -E -e -u -o pipefail

function uds() {
	sudo bpftool iter pin .output/iter_uds.bpf.o /sys/fs/bpf/my_route
	cat /sys/fs/bpf/my_route
}

function slub() {
	sudo bpftool iter pin .output/iter_slub.bpf.o /sys/fs/bpf/slub
	cat /sys/fs/bpf/slub
	# 输出内容为:
	# ext4_inode_cache: 1096
	# ext4_allocation_context: 152
	# ext4_prealloc_space: 112
	# ext4_system_zone: 40
	# bio_post_read_ctx: 48
	# extent_status: 40
	# jbd2_journal_handle: 56
	# ...
}
