#!/usr/bin/env bash
set -E -e -u -o pipefail

# 具体代码在 lib/dynamic_debug.c 中
# 进一步参考
# https://www.kernel.org/doc/html/latest/admin-guide/dynamic-debug-howto.html
#
# 内核支持很多范围选择 : func file module format class line
# 其他用法
# echo "module kvm +p" > /sys/kernel/debug/dynamic_debug/control

control=/proc/dynamic_debug/control
# 似乎两个文件的效果是一样的
# 不过，有的环境中没有 proc 下的文件
if [[ ! -f $control ]]; then
	control=/sys/kernel/debug/dynamic_debug/control
fi

if [[ ! -e $control ]]; then
	echo "CONFIG_DYNAMIC_DEBUG is needed"
fi
mkdir -p /tmp/martins3
# 为什么多此一举，因为 fzf $control 如果在 subshell 中，无法输入密码
sudo cat $control | tee /tmp/martins3/dynamic

trap finish EXIT
function finish {
	echo "enable console log"
	sudo dmesg -E
}

choose=$(fzf </tmp/martins3/dynamic)
# choose='arch/x86/kvm/x86.c:529 [kvm]kvm_do_msr_access =_ "kvm: kvm [%i]: unhandled %s: 0x%x data 0x%llx\n"'
file_num=$(echo "$choose" | cut -d' ' -f1)
file=$(echo "$file_num" | cut -d':' -f1)
num=$(echo "$file_num" | cut -d':' -f2)

echo "disable console log"
sudo dmesg -D
echo "file $file line $num +p" | sudo tee $control
