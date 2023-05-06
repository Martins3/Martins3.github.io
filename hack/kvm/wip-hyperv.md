# HyperV
[Documentation](https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/about/)
[Qemu doc for hyperv](https://github.com/qemu/qemu/blob/master/docs/hyperv.txt)


https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/nested-virtualization

- [ ] 还有自己的 layout https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/datatypes/hv_vmx_enlightened_vmcs
    - [ ] /home/maritns3/core/linux/arch/x86/include/asm/hyperv-tlfs.h:struct hv_enlightened_vmcs

- [ ] 这是 intel 的属性啊，为什么需要搭上 hyperv

## Why kvm need hyperv
https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf

- [ ] 好的，现在为什么需要 hyperv 的道理我大概知道了，但是怎么实现啊 ?

Emulating hardware Interfaces can be slow
- Invent virtualization-friendly
  - (paravirtualized) interfaces!
    - Add support to guest OSes
      - ... but what about proprietary OSes?
        - We can try writing device drivers for such OSes
          - ... but some core features (interrupt handling, timekeeping,...) are not devices
            - Emulate an already supported (proprietary) hypervisor interfaces solving the exact same issues!

## https://kernel.love/hyperv-enlightenment.html
分析的很好，看一个例子就可以立刻理解.

https://www.qemu.org/docs/master/system/i386/hyperv.html

## 启动 windows 的时候会调用到这里: kvm_get_hv_cpuid

# hyperv 到底是如何影响的?

## kvm_hv_set_msr_common


# docs/kernel/cpuinfo/material/window.txt
将参数修改为，那么 CPUID 会发生修改：
arg_cpu_model="-cpu host,hv_relaxed,hv_vpindex,hv_time"
```txt
< CPUID 40000000:00 = 40000001 4b4d564b 564b4d56 0000004d | ...@KVMKVMKVM...
< CPUID 40000001:00 = 01007afb 00000000 00000000 00000000 | .z..............
---
> CPUID 40000000:00 = 40000005 7263694d 666f736f 76482074 | ...@Microsoft Hv
> CPUID 40000001:00 = 31237648 00000000 00000000 00000000 | Hv#1............
> CPUID 40000002:00 = 00003839 000a0000 00000000 00000000 | 98..............
> CPUID 40000003:00 = 00000262 00000000 00000000 00000008 | b...............
> CPUID 40000004:00 = 00000020 ffffffff 00000000 00000000 |  ...............
> CPUID 40000005:00 = ffffffff 00000040 00000000 00000000 | ....@...........
```

## 原来 cpuid leaf 40000000 是提供给 hypervisor 提供信息的

进一步参考:
- https://www.kernel.org/doc/Documentation/virtual/kvm/cpuid.txt
- https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery

## QEMU
```c
static struct {
    const char *desc;
    struct {
        uint32_t func;
        int reg;
        uint32_t bits;
    } flags[2];
    uint64_t dependencies;
} kvm_hyperv_properties[] = {
```
