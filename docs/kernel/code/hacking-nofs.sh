#!/usr/bin/env bash

set -E -e -u -o pipefail
# shopt -s inherit_errexit
# PROGNAME=$(basename "$0")
# PROGDIR=$(readlink -m "$(dirname "$0")")
for i in "$@"; do
	echo "$i"
done
cd "$(dirname "$0")"


cgcreate -g memory:mem
cgset -r memory.max=100m mem

# 测试了一下，都很正常
umount /dev/vdc
# mkfs.xfs -f /dev/vdc
mkfs.ext4  /dev/vdc
mount /dev/vdc /root/mnt
cgexec -g memory:mem  dd if=/dev/random of=/root/mnt/g.dump bs=1M count=10000
