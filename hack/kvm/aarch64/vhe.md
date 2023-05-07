# vhe 和 non-vhe

## [To EL2, and Beyond! Optimizing the Design and Implementation of KVM/ARM](http://events17.linuxfoundation.org/sites/events/files/slides/To%20EL2%20and%20Beyond_0.pdf)
分析的太好了

配套的文章:
[ARM Virtualization: Performance and Architectural Implications](https://par.nsf.gov/servlets/purl/10310788)

## 分析下 Asahi Linux 是那种模式 vhe 还是 non-vhe 的

- kvm_arch_vcpu_ioctl_run
  - kvm_arm_vcpu_enter_exit
    - `__kvm_vcpu_run`

```c
/*
 * Actually run the vCPU, entering an RCU extended quiescent state (EQS) while
 * the vCPU is running.
 *
 * This must be noinstr as instrumentation may make use of RCU, and this is not
 * safe during the EQS.
 */
static int noinstr kvm_arm_vcpu_enter_exit(struct kvm_vcpu *vcpu)
{
	int ret;

	guest_state_enter_irqoff();
	ret = kvm_call_hyp_ret(__kvm_vcpu_run, vcpu);
	guest_state_exit_irqoff();

	return ret;
}
```

- `__kvm_vcpu_run` : nvhe 和 vhe 都存在一份代码
  - __guest_enter
  - fixup_guest_exit
    - kvm_hyp_handle_exit


nvhe 的处理分为两种情况:
```c
static const exit_handler_fn hyp_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_CP15_32]		= kvm_hyp_handle_cp15_32,
	[ESR_ELx_EC_SYS64]		= kvm_hyp_handle_sysreg,
	[ESR_ELx_EC_SVE]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn pvm_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_SYS64]		= kvm_handle_pvm_sys64,
	[ESR_ELx_EC_SVE]		= kvm_handle_pvm_restricted,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn *kvm_get_exit_handler_array(struct kvm_vcpu *vcpu)
{
	if (unlikely(kvm_vm_is_protected(kern_hyp_va(vcpu->kvm))))
		return pvm_exit_handlers;

	return hyp_exit_handlers;
}
```

而 vhe 中只有一种:
```c
static const exit_handler_fn *kvm_get_exit_handler_array(struct kvm_vcpu *vcpu)
{
	return hyp_exit_handlers;
}
```
