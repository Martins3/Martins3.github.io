#!/usr/bin/env bash
set -E -e -u -o pipefail

readonly ITER_PIN="/sys/fs/bpf/iter-example-${BASHPID}"

function cleanup() {
	sudo rm -f "${ITER_PIN}"
}

trap cleanup EXIT

function uds() {
	make iter_uds
	sudo bpftool iter pin .output/iter_uds.bpf.o "${ITER_PIN}"
	sudo cat "${ITER_PIN}"
}

function slub() {
	make iter_slub
	sudo bpftool iter pin .output/iter_slub.bpf.o "${ITER_PIN}"
	sudo cat "${ITER_PIN}"
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

uds
