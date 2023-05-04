- ArchCPU::ucode_rev 是做什么的?

## 初始化的流程最终如何决定 cpuid 的样子的
- kvm_cpu_instance_init
  - kvm_cpu_max_instance_init
    - host_cpu_max_instance_init : 使用 host 中的
    - lmce_supported
    - kvm_arch_get_supported_cpuid

## x86_cpu_expand_features

- [ ] 为什么叫做 expand ？
- [ ] filter 是什么含义?

## KVM_CPUID_FLAG_SIGNIFCANT_INDEX : 这个是什么含义?

## clearcpuid 到底是如何实现的?
又可以禁用 avx2，同时不会影响 cpuflags 的，也许尝试 disable 其他的 flags 试试

## plans
1. 理解 cpuid -r 1 的内容
2. 整理关键的 cpu flags
3. QEMU 和 kvm 如何模拟
4. msr
5. 从 cpuid 到 cpuinfo 的实现
