# 简单浏览下 svm.c 的代码

## 一些硬件机制，简单了解一下
### efer
- efer_trap : https://en.wikipedia.org/wiki/Control_register

功能定义在: arch/x86/include/asm/msr-index.h

例如:
```c
/* EFER bits: */
#define _EFER_NX		11 /* No execute enable */
```
让页面不可执行。

### others
- svm_init_osvw
  - AMD 手册 volume 2 : Appendix E OS-Visible Workarounds
```c
#define X86_FEATURE_OSVW		( 6*32+ 9) /* OS Visible Workaround */
```
- svm_enable_lbrv : ( last branch records) : https://lwn.net/Articles/680985/
- grow_ple_window : https://patchwork.kernel.org/project/kvm/patch/1408637291-18533-5-git-send-email-rkrcmar@redhat.com/
  - https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/6.2_release_notes/virtualization
  - Pause Loop Exiting
- svm_set_vnmi_pending : vnmi 有什么特别的吗?
- [ ] ud_interception
- svm_cache_reg : 中间的 pdptr 不太知道是什么意思?

这些 interception 似乎是
```c
static int (*const svm_exit_handlers[])(struct kvm_vcpu *vcpu) = {
	[SVM_EXIT_READ_CR0]			= cr_interception,
	[SVM_EXIT_READ_CR3]			= cr_interception,
	[SVM_EXIT_READ_CR4]			= cr_interception,
	[SVM_EXIT_READ_CR8]			= cr_interception,
	[SVM_EXIT_CR0_SEL_WRITE]		= cr_interception,
	[SVM_EXIT_WRITE_CR0]			= cr_interception,
	[SVM_EXIT_WRITE_CR3]			= cr_interception,
	[SVM_EXIT_WRITE_CR4]			= cr_interception,
	[SVM_EXIT_WRITE_CR8]			= cr8_write_interception,
	[SVM_EXIT_READ_DR0]			= dr_interception,
	[SVM_EXIT_READ_DR1]			= dr_interception,
	[SVM_EXIT_READ_DR2]			= dr_interception,
	[SVM_EXIT_READ_DR3]			= dr_interception,
	[SVM_EXIT_READ_DR4]			= dr_interception,
	[SVM_EXIT_READ_DR5]			= dr_interception,
	[SVM_EXIT_READ_DR6]			= dr_interception,
	[SVM_EXIT_READ_DR7]			= dr_interception,
	[SVM_EXIT_WRITE_DR0]			= dr_interception,
	[SVM_EXIT_WRITE_DR1]			= dr_interception,
	[SVM_EXIT_WRITE_DR2]			= dr_interception,
	[SVM_EXIT_WRITE_DR3]			= dr_interception,
	[SVM_EXIT_WRITE_DR4]			= dr_interception,
	[SVM_EXIT_WRITE_DR5]			= dr_interception,
	[SVM_EXIT_WRITE_DR6]			= dr_interception,
	[SVM_EXIT_WRITE_DR7]			= dr_interception,
	[SVM_EXIT_EXCP_BASE + DB_VECTOR]	= db_interception,
	[SVM_EXIT_EXCP_BASE + BP_VECTOR]	= bp_interception,
	[SVM_EXIT_EXCP_BASE + UD_VECTOR]	= ud_interception,
	[SVM_EXIT_EXCP_BASE + PF_VECTOR]	= pf_interception,
	[SVM_EXIT_EXCP_BASE + MC_VECTOR]	= mc_interception,
	[SVM_EXIT_EXCP_BASE + AC_VECTOR]	= ac_interception,
	[SVM_EXIT_EXCP_BASE + GP_VECTOR]	= gp_interception,
	[SVM_EXIT_INTR]				= intr_interception,
	[SVM_EXIT_NMI]				= nmi_interception,
	[SVM_EXIT_SMI]				= smi_interception,
  // ...
}
```

## 如果 disable npt ，代码的执行流程是什么?

找一些 reference :
- [ ] svm_set_efer : 为什么 shadow page 需要支持 EFER_NX
- svm_recalc_instruction_intercepts : shadow page 需要  intercept INVPCID 来实现 shadow page flush
- init_vmcb
```c
	if (npt_enabled) {
		/* Setup VMCB for Nested Paging */
		control->nested_ctl |= SVM_NESTED_CTL_NP_ENABLE;
		svm_clr_intercept(svm, INTERCEPT_INVLPG);
		clr_exception_intercept(svm, PF_VECTOR);
		svm_clr_intercept(svm, INTERCEPT_CR3_READ);
		svm_clr_intercept(svm, INTERCEPT_CR3_WRITE);
		save->g_pat = vcpu->arch.pat;
		save->cr3 = 0;
	}
```
- svm_set_cr0
- svm_set_cr4
- svm_handle_exit : 更新 cr3 有关
- svm_load_mmu_pgd
- svm_set_cpu_caps

看来，核心的流程都是在 mmu 上，其实 svm 没有很多的调整。
