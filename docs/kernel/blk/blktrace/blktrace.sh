#!/usr/bin/env bash
set -E -e -u -o pipefail

disk=sdb
# disk=nvme0n1
dir=$(mktemp -d)
runtime=1000
echo ---------------------------
echo "$dir"
echo ---------------------------
cd "$dir"
cat <<_EOF_ >fio.script
[global]
time_based
runtime=$runtime
ioengine=aio
iodepth=64
direct=1
numjobs=1
bs=4k

[trash]
rw=randread
filename=/dev/$disk
_EOF_

sudo fio fio.script &

# 这个 -a 参数真的有效果
# sudo blktrace -a complete -a issue -a queue /dev/$disk -w $runtime
sudo blktrace /dev/$disk -w $((runtime + 2)) # 让 fio 先结束
sudo blkparse -i $disk -d $disk.blktrace.bin
sudo btt -i $disk.blktrace.bin
