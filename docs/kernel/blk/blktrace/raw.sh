#!/usr/bin/env bash
set -E -e -u -o pipefail

disk=${1:-}
if [[ -z $disk || ! -d /sys/block/"$disk" ]]; then
	echo "usage : ./raw.sh sda"
	exit 1
fi

trap finish EXIT

function finish {
	echo 0 | sudo tee /sys/block/"$disk"/trace/enable
}

# 那么这里的 blk 的作用是什么?
echo blk | sudo tee /sys/kernel/tracing/current_tracer
echo 1 | sudo tee /sys/block/"$disk"/trace/enable
sudo cat /sys/kernel/tracing/trace_pipe
