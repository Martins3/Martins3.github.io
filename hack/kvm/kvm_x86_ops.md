## 问题
1. 两个架构是处理 msr 不同吗?

## 逐个分析
### [ ] cache_reg

为什么这几个必须走特殊通道 ?
```c
struct kvm_vcpu_arch {
	/*
	 * rip and regs accesses must go through
	 * kvm_{register,rip}_{read,write} functions.
	 */
	unsigned long regs[NR_VCPU_REGS];
	u32 regs_avail;
	u32 regs_dirty;
```

- handle_invpcid
  - kvm_register_read_raw
    - kvm_register_is_available
    - 如果 available，直接返回 `vcpu->arch.regs[reg]`
