#!/usr/bin/env bash

set -E -e -u -o pipefail
shopt -s inherit_errexit
for i in "$@"; do
	echo "$i"
done
cd "$(dirname "$0")"

pvcreate /dev/nvme0n1
vgextend /dev/MyVG01 /dev/nvme0n1

vgcreate LVMvgTEST /dev/sdb /dev/sdc
