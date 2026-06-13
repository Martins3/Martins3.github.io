#!/usr/bin/env drgn
"""
交互式选择 vCPU 并 Dump 完整结构体

使用方法:
    ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_select_dump.py
"""

from drgn import cast, Object
from drgn.helpers.linux import for_each_task

def find_kvm_vcpus(prog):
    """查找所有有效的 KVM vCPU"""
    vcpus = {}
    
    for task in for_each_task(prog):
        comm = task.comm.string_().decode('utf-8', errors='replace')
        if 'qemu' not in comm.lower():
            continue
        
        files = task.files
        if files.value_() == 0:
            continue
        
        fdt = files.fdt
        max_fds = fdt.max_fds.value_()
        fd = fdt.fd
        
        for i in range(min(max_fds, 256)):
            file = fd[i]
            if file.value_() == 0:
                continue
            
            try:
                private_data = file.private_data
                if private_data.value_() == 0:
                    continue
                
                vcpu = cast("struct kvm_vcpu *", private_data)
                vcpu_id = vcpu.vcpu_id.value_()
                vcpu_addr = vcpu.value_()
                
                if 0 <= vcpu_id < 128 and vcpu_addr not in vcpus:
                    vcpus[vcpu_addr] = {
                        'vcpu_id': vcpu_id,
                        'vcpu': vcpu,
                        'addr': vcpu_addr,
                    }
            except:
                pass
    
    return sorted(vcpus.values(), key=lambda x: x['vcpu_id'])

print("=== KVM vCPU 选择器 ===\n")

vcpus = find_kvm_vcpus(prog)

if not vcpus:
    print("未找到 vCPU")
    exit(1)

print(f"找到 {len(vcpus)} 个 vCPU:\n")

for i, info in enumerate(vcpus):
    vcpu = info['vcpu']
    print(f"[{i}] vCPU {info['vcpu_id']:2d} @ {info['addr']:#x} | pCPU: {vcpu.cpu.value_():2d} | requests: {vcpu.requests.value_():#x}")

# 选择要 dump 的 vCPU（默认选择第一个）
selection = 0
print(f"\n显示 vCPU {vcpus[selection]['vcpu_id']} 的完整结构体...")
print("=" * 70)

vcpu = vcpus[selection]['vcpu']
print(vcpu)
