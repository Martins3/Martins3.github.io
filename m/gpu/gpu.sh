#!/usr/bin/env bash
set -E -e -u -o pipefail

set -x
cd "$(dirname "$0")"
rm -f /lib/modules/"$(uname -r)"/gpu.ko
cp gpu.ko /lib/modules/"$(uname -r)"/
depmod
sudo modprobe gpu

## 各种有趣的 sysfs 测试
ls -la /sys/bus/pci/drivers/gpu

ls -la /sys/class/gpu_pcie

# 先需要安装几个模块
# sudo modprobe fb
# sudo modprobe cfbfillrect
# sudo modprobe cfbcopyarea
# sudo modprobe cfbimgblt
