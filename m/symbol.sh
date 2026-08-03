#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
# 一个 symbol 同时出现在两个文件中不支持。使用这个 symbol.sh 的情况都是极少的，
# 就不用考虑了吧。
declare -A a
a=(
	["get_arm64_ftr_reg"]="arch/aarch64/sysreg.c"
	["read_sanitised_ftr_reg"]="arch/aarch64/sysreg.c"
)

# 使用 ！来遍历 key
for s in "${!a[@]}"; do
	f=${a[$s]}
	echo "$s -> $f"
	set -x
	addr=$(sudo cat /proc/kallsyms | grep -E " $s$" | awk '{print $1}')
	echo "$s -- $addr"
	sed -i "s/$s)0xfff.*;/$s)0x$addr;/" "$f"
	set +x
done
