#!/usr/bin/env bash
#
# This script runs qemu and creates a symbolic link named serial.pts
# to the qemu serial console (pts based). Because the qemu pts
# allocation is dynamic, it is preferable to have a stable path to
# avoid visual inspection of the qemu output when connecting to the
# serial console.

set -x
case $ARCH in
    x86-64)
	qemu="qemu-system-x86_64"
	;;
    arm)
	qemu="qemu-system-aarch64"
	;;
esac

# 通过 unix domain 的方式来监控
# 从而可以输入命令 info chardev 了
# 从而知道创建的到底是 /dev/pts/? 来将信息导出来
# 这个 pty 就是靠参数 -chardev pty 创建的
# - [ ] 至于为什么是 pty，现在 pty 没有什么理解
echo info chardev | nc -U -l qemu.mon | grep -E --line-buffered -o "/dev/pts/[0-9]*" | xargs -I PTS ln -fs PTS serial.pts &

$qemu "$@" -monitor unix:qemu.mon
rm qemu.mon 
rm serial.pts
