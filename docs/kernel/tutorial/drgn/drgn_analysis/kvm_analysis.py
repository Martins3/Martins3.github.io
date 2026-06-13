#!/usr/bin/env drgn
"""
KVM 分析脚本 - 使用 DRGN 分析 KVM 内核状态

使用方法:
    drgn -s /path/to/vmlinux -c /proc/kcore -i kvm_analysis.py
"""

import drgn
from drgn import Object, cast, container_of
from drgn.helpers.linux import for_each_task, list_for_each_entry

print("=== KVM 分析开始 ===")

# 查找 kvm_vm 结构（如果有公开的链表）
# 注意：kvm_vm 结构通常是 per-file 的，没有全局链表
# 需要通过 /dev/kvm 的 fd 或者其他方式查找

# 列出所有任务，查找可能持有 KVM fd 的进程
print("\n1. 查找可能的 QEMU/KVM 进程:")
for task in for_each_task(prog):
    comm = task.comm.string_().decode('utf-8', errors='replace')
    if 'qemu' in comm.lower() or 'kvm' in comm.lower():
        print(f"  PID {task.pid.value_()}: {comm}")

# 查找 kvm 模块的符号
print("\n2. KVM 模块符号:")
try:
    kvm = prog.module("kvm")
    print(f"  KVM 模块地址: {kvm.address:#x}")
except LookupError as e:
    print(f"  无法找到 kvm 模块: {e}")

# 尝试获取 kvm 相关函数
print("\n3. KVM 关键函数地址:")
kvm_funcs = [
    "kvm_create_vm",
    "kvm_destroy_vm", 
    "kvm_vcpu_ioctl",
    "kvm_vm_ioctl",
    "kvm_mmu_load",
    "kvm_mmu_unload",
]

for func_name in kvm_funcs:
    try:
        func = prog.symbol(func_name)
        print(f"  {func_name}: {func.address:#x}")
    except LookupError:
        print(f"  {func_name}: not found")

print("\n=== KVM 分析完成 ===")
