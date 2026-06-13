#!/usr/bin/env drgn
"""
Dump KVM 内核 vCPU 结构体

使用方法:
    ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_dump.py
"""

import drgn
from drgn import Object, cast
from drgn.helpers.linux import for_each_task

print("=== KVM vCPU 结构体 Dump ===\n")

def find_kvm_vcpus(prog):
    """通过进程的 fd 查找 KVM vCPU"""
    vcpus = {}
    
    for task in for_each_task(prog):
        comm = task.comm.string_().decode('utf-8', errors='replace')
        pid = task.pid.value_()
        
        # 只检查 QEMU 进程
        if 'qemu' not in comm.lower():
            continue
        
        # 获取文件的 fd
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
                
                # 尝试读取作为 kvm_vcpu
                vcpu = cast("struct kvm_vcpu *", private_data)
                vcpu_id = vcpu.vcpu_id.value_()
                vcpu_addr = vcpu.value_()
                
                # 只保留有效的 vCPU (vcpu_id 在 0-127 范围内)
                if 0 <= vcpu_id < 128:
                    if vcpu_addr not in vcpus:
                        vcpus[vcpu_addr] = {
                            'pid': pid,
                            'comm': comm,
                            'vcpu': vcpu,
                            'vcpu_addr': vcpu_addr,
                            'vcpu_id': vcpu_id,
                        }
            except:
                pass
    
    return list(vcpus.values())

try:
    vcpus = find_kvm_vcpus(prog)
    
    print(f"找到 {len(vcpus)} 个唯一 KVM vCPU 结构体\n")
    
    # 按 vcpu_id 排序
    vcpus.sort(key=lambda x: x['vcpu_id'])
    
    # 输出 vCPU 信息
    for vcpu_info in vcpus:
        vcpu = vcpu_info['vcpu']
        vcpu_id = vcpu_info['vcpu_id']
        pid = vcpu_info['pid']
        
        print(f"vCPU {vcpu_id}:")
        print(f"  kvm_vcpu struct: {vcpu_info['vcpu_addr']:#x}")
        print(f"  kvm struct: {vcpu.kvm.value_():#x}")
        print(f"  QEMU PID: {pid}")
        print(f"  cpu (pCPU): {vcpu.cpu.value_()}")
        print(f"  requests: {vcpu.requests.value_():#x}")
        print(f"  guest_debug: {vcpu.guest_debug.value_()}")
        print(f"  run: {vcpu.run.value_():#x}")
        print(f"  mmio_needed: {vcpu.mmio_needed.value_()}")
        print(f"  mmio_is_write: {vcpu.mmio_is_write.value_()}")
        print(f"  preempted: {vcpu.preempted.value_()}")
        print(f"  ready: {vcpu.ready.value_()}")
        
        # x86 架构特定字段
        try:
            arch = vcpu.arch
            print(f"\n  x86 arch 字段:")
            print(f"    cr0: {arch.cr0.value_():#x}")
            print(f"    cr2: {arch.cr2.value_():#x}")
            print(f"    cr3: {arch.cr3.value_():#x}")
            print(f"    cr4: {arch.cr4.value_():#x}")
            print(f"    efer: {arch.efer.value_():#x}")
            print(f"    apic_base: {arch.apic_base.value_():#x}")
            print(f"    mp_state: {arch.mp_state.value_()}")
            print(f"    tsc_offset: {arch.tsc_offset.value_()}")
        except Exception as e:
            print(f"  (无法读取 x86 arch 字段: {e})")
        
        # mmu 相关
        try:
            mmu = arch.mmu
            print(f"\n  MMU 字段:")
            print(f"    root_hpa: {mmu.root_hpa.value_():#x}")
            print(f"    root_level: {mmu.root_level.value_()}")
            print(f"    shadow_root_level: {mmu.shadow_root_level.value_()}")
            print(f"    ept_ad: {mmu.ept_ad.value_()}")
        except:
            pass
        
        print("")

except Exception as e:
    print(f"错误: {e}")
    import traceback
    traceback.print_exc()

print("=== 分析完成 ===")
