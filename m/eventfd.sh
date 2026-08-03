#!/usr/bin/env bash
set -E -e -u -o pipefail

# 参考
# https://stackoverflow.com/questions/13607730/writing-to-eventfd-from-kernel-module : 在内核模块中间可以直接让等待的等待 eventfd 的 select 返回
#
# 这个可以看看:
# https://news.ycombinator.com/item?id=11792178
gcc eventfd-user.c -o eventfd.out
rmmod martins3 || true
insmod martins3.ko

# 首先运行 eventfd.out ，他会一直等待，知道 echo 到 eventfd
echo "echo 0 > /sys/kernel/hacking/eventfd"
./eventfd.out
