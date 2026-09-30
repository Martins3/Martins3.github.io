#!/usr/bin/env bash
set -E -e -u -o pipefail
cd "$(dirname "$0")"

# 只能从 virtme guest 中以 root 执行；这里不调用物理机的 sudo/insmod。
if [[ $EUID != 0 ]] || ! grep -q 'virtme_hostname=' /proc/cmdline; then
	echo '请在 virtme guest 中以 root 运行此脚本'
	exit 1
fi

#  L1 内的 kvm_intel 与 peach 是两个独立的管理者。硬件允许切换多个 VMCS，但它们需要协调 VMXON/VMXOFF、当前 VMCS 和 CPU 状态；目前 peach 没有这种协调机
#   制。例如 KVM 已经执行过 VMXON，peach 再执行一次就会失败；peach 也不能擅自 VMXOFF，否则会破坏 KVM 的状态。
if lsof /dev/kvm; then
	echo 'kvm 和 pearch 应该是互相干扰的'
	exit 1
fi
if [[ -e /dev/peach ]] || [[ -d /sys/module/peach ]]; then
	echo 'peach 设备或模块已存在，请先确认没有其他测试正在运行'
	exit 1
fi

function cleanup() {
	rm -f /dev/peach
	rmmod peach
}

insmod ./peach.ko
trap cleanup EXIT
mknod -m 600 /dev/peach c 511 0
./peach-user.out
dmesg | tail -n 12
