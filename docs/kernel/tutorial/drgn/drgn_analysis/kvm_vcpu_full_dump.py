#!/usr/bin/env drgn
"""
Dump 指定 KVM vCPU 结构体的所有字段

使用方法:
    ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_full_dump.py <vcpu_addr>

示例:
    ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_full_dump.py 0xffff888109ab2300
"""

import sys
from drgn import cast, Object

# 获取命令行参数
if len(sys.argv) < 2:
    print("用法: ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_full_dump.py <vcpu_addr>")
    print("示例: ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_full_dump.py 0xffff888109ab2300")
    sys.exit(1)

vcpu_addr_str = sys.argv[1]
try:
    vcpu_addr = int(vcpu_addr_str, 16) if vcpu_addr_str.startswith('0x') else int(vcpu_addr_str)
except ValueError:
    print(f"错误: 无效的地址格式: {vcpu_addr_str}")
    sys.exit(1)

print(f"=== KVM vCPU 结构体完整 Dump ===")
print(f"地址: {vcpu_addr:#x}\n")

# 获取 vCPU 结构体
vcpu = cast("struct kvm_vcpu *", Object(prog, "void *", value=vcpu_addr))

# 打印基本信息
print(f"vcpu_id: {vcpu.vcpu_id.value_()}")
print(f"cpu (pCPU): {vcpu.cpu.value_()}")
print(f"kvm: {vcpu.kvm.value_():#x}")
print(f"run: {vcpu.run.value_():#x}")
print("")

# 打印完整结构体
print("=" * 60)
print("完整结构体内容:")
print("=" * 60)
print(vcpu)
