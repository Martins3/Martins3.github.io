## cr0
windows 启动之后
```txt
Samples: 104K of event 'kvm:kvm_cr', 1 Hz, Event count (approx.): 330
  60.53%  cr_write 0 = 0x11
  37.41%  cr_write 0 = 0x10
   2.05%  cr_write 0 = 0x80000011
```

linux 启动之后:
```txt
Samples: 3M of event 'kvm:kvm_cr', 1 Hz, Event count (approx.): 245824
  50.19%  cr_write 0 = 0x11
  49.77%  cr_write 0 = 0x10
   0.01%  cr_write 0 = 0x80050033
   0.00%  cr_write 0 = 0x1
   0.00%  cr_write 0 = 0x80010001
   0.00%  cr_write 0 = 0xc0050033
   0.00%  cr_write 4 = 0x1406a0
   0.00%  cr_write 4 = 0x3406a0
   0.00%  cr_write 4 = 0x340ea0
   0.00%  cr_write 4 = 0x406a0
   0.00%  cr_write 4 = 0x406b0
   0.00%  cr_write 4 = 0x740ea0
   0.00%  cr_write 4 = 0x740ee0
   0.00%  cr_write 4 = 0xa0
```

看似是中断的打开关闭导致的。

## 通用的分析
```sh
sudo perf trace -e kvm:kvm_cr
```

```c
static int cr_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int reg, cr;
	unsigned long val;
	int err;

	if (!static_cpu_has(X86_FEATURE_DECODEASSISTS))
		return emulate_on_interception(vcpu);

	if (unlikely((svm->vmcb->control.exit_info_1 & CR_VALID) == 0))
		return emulate_on_interception(vcpu);

	reg = svm->vmcb->control.exit_info_1 & SVM_EXITINFO_REG_MASK;
	if (svm->vmcb->control.exit_code == SVM_EXIT_CR0_SEL_WRITE)
		cr = SVM_EXIT_WRITE_CR0 - SVM_EXIT_READ_CR0;
	else
		cr = svm->vmcb->control.exit_code - SVM_EXIT_READ_CR0;

	err = 0;
```

最后调用到这里的:
```c
static const struct x86_emulate_ops emulate_ops = {
	.vm_bugged           = emulator_vm_bugged,
	.read_gpr            = emulator_read_gpr,
	.write_gpr           = emulator_write_gpr,
	.read_std            = emulator_read_std,
	.write_std           = emulator_write_std,
	.fetch               = kvm_fetch_guest_virt,
	.read_emulated       = emulator_read_emulated,
	.write_emulated      = emulator_write_emulated,
	.cmpxchg_emulated    = emulator_cmpxchg_emulated,
	.invlpg              = emulator_invlpg,
	.pio_in_emulated     = emulator_pio_in_emulated,
	.pio_out_emulated    = emulator_pio_out_emulated,
	.get_segment         = emulator_get_segment,
	.set_segment         = emulator_set_segment,
	.get_cached_segment_base = emulator_get_cached_segment_base,
	.get_gdt             = emulator_get_gdt,
	.get_idt	     = emulator_get_idt,
	.set_gdt             = emulator_set_gdt,
	.set_idt	     = emulator_set_idt,
	.get_cr              = emulator_get_cr,
	.set_cr              = emulator_set_cr,
	.cpl                 = emulator_get_cpl,
	.get_dr              = emulator_get_dr,
	.set_dr              = emulator_set_dr,
	.set_msr_with_filter = emulator_set_msr_with_filter,
	.get_msr_with_filter = emulator_get_msr_with_filter,
	.get_msr             = emulator_get_msr,
	.check_rdpmc_early   = emulator_check_rdpmc_early,
	.read_pmc            = emulator_read_pmc,
	.halt                = emulator_halt,
	.wbinvd              = emulator_wbinvd,
	.fix_hypercall       = emulator_fix_hypercall,
	.intercept           = emulator_intercept,
	.get_cpuid           = emulator_get_cpuid,
	.guest_has_movbe     = emulator_guest_has_movbe,
	.guest_has_fxsr      = emulator_guest_has_fxsr,
	.guest_has_rdpid     = emulator_guest_has_rdpid,
	.set_nmi_mask        = emulator_set_nmi_mask,
	.is_smm              = emulator_is_smm,
	.is_guest_mode       = emulator_is_guest_mode,
	.leave_smm           = emulator_leave_smm,
	.triple_fault        = emulator_triple_fault,
	.set_xcr             = emulator_set_xcr,
	.get_untagged_addr   = emulator_get_untagged_addr,
};
```

## cr3

ept 打开的时候，看不到 cr3 的使用

分析 arch/x86/kvm/svm/svm.c:cr_interception

```sh
sudo perf trace -e kvm:kvm_cr
```

但是 cr3 看上去从来不会有。

## cr8

amd64/intel-sdm/v3-ch02.md

- CR8.TPL
Task Priority Level (bit 3:0 of CR8) — This sets the threshold value corresponding to the highestpriority interrupt to be blocked.
A value of 0 means all interrupts are enabled. This field is available in 64-bit mode.
A value of 15 means all interrupts will be disabled.

docs/kernel/cpuinfo/material/query-cpu-definitions.json

1385:        "cr8legacy",

141:  clflush clflushopt clwb clzero cmov cmp-legacy core-capability cr8legacy
162:  vmx-cr3-store-noexit vmx-cr8-load-exit vmx-cr8-store-exit vmx-desc-exit

- cr8_write_interception : svm 特有的


```c
static void apic_set_tpr(struct kvm_lapic *apic, u32 tpr)
{
	kvm_lapic_set_reg(apic, APIC_TASKPRI, tpr);
	apic_update_ppr(apic);
}
```
docs/qemu/int.md 中继续分析

```c
int kvm_set_cr8(struct kvm_vcpu *vcpu, unsigned long cr8)
{
	if (cr8 & CR8_RESERVED_BITS)
		return 1;
	if (lapic_in_kernel(vcpu))
		kvm_lapic_set_tpr(vcpu, cr8);
	else
		vcpu->arch.cr8 = cr8;
	return 0;
}
EXPORT_SYMBOL_GPL(kvm_set_cr8);
```

### linux 会使用 CR8 或者 TPR 吗?

setup_local_APIC 中看，linux 应该是不去使用的:
```c
	/*
	 * Set Task Priority to 'accept all except vectors 0-31'.  An APIC
	 * vector in the 16-31 range could be delivered if TPR == 0, but we
	 * would think it's an exception and terrible things will happen.  We
	 * never change this later on.
	 */
	value = apic_read(APIC_TASKPRI);
	value &= ~APIC_TPRI_MASK;
	value |= 0x10;
	apic_write(APIC_TASKPRI, value);
```

### [ ] 那么就可以在 linux 中模拟这个操作了

之前的 EOI 模拟前面加上 tpr 的修改。

### 可以控制 cr8 的 exit 情况吗?

vmx_update_msr_bitmap_x2apic

```c
	/*
	 * TPR reads and writes can be virtualized even if virtual interrupt
	 * delivery is not in use.
	 */
	vmx_set_intercept_for_msr(vcpu, X2APIC_MSR(APIC_TASKPRI), MSR_TYPE_RW,
				  !(mode & MSR_BITMAP_MODE_X2APIC));
```
注释如何理解?

init_vmcs

```c
	if (cpu_has_vmx_tpr_shadow()) {
		vmcs_write64(VIRTUAL_APIC_PAGE_ADDR, 0;ic->regs));
		vmcs_write32(TPR_THRESHOLD, 0);
	}

		if (cpu_need_tpr_shadow(&vmx->vcpu))
			vmcs_write64(VIRTUAL_APIC_PAGE_ADDR,
				     __pa(vmx->vcpu.arch.ap)
```

vmx_exec_control

```c
#ifdef CONFIG_X86_64
	if (exec_control & CPU_BASED_TPR_SHADOW)
		exec_control &= ~(CPU_BASED_CR8_LOAD_EXITING |
				  CPU_BASED_CR8_STORE_EXITING);
	else
		exec_control |= CPU_BASED_CR8_STORE_EXITING |
				CPU_BASED_CR8_LOAD_EXITING;
#endif
```

这里还理解一个事情，是不是如果使用 mmio ，APIC_TASKPRI 的访问无论如何都无法被虚拟化，总是必须 exit ?


<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
