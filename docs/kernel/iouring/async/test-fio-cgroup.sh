#!/usr/bin/env bash
set -E -e -u -o pipefail

# 测试，如果内存不足，需要回收，那么这个回收操作是在哪个 thread 中完成的
readonly CGROUP_NAME="fio-test-100m"
readonly MEMORY_LIMIT="100M"
readonly FIO_CONFIG="/home/martins3/data/vn/docs/kernel/blk/fio/4k-read.ini"

cleanup() {
	sudo cgdelete memory:"$CGROUP_NAME"
}

trap cleanup EXIT

# 创建 cgroup
sudo cgcreate -g memory:"$CGROUP_NAME"
echo "$MEMORY_LIMIT" | sudo tee /sys/fs/cgroup/$CGROUP_NAME/memory.max
echo "0" | sudo tee /sys/fs/cgroup/$CGROUP_NAME/memory.swap.max

echo "cgroup $CGROUP_NAME 创建成功，内存限制: $MEMORY_LIMIT"

# 在 cgroup 中运行 fio
sudo cgexec -g memory:"$CGROUP_NAME" -- fio "$FIO_CONFIG"
