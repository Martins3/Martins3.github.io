## 时钟中断可以做 interrupt posting 吗?


当然是可以的，原始的 patch 在 2019 :

commit 0c5f81dad46c ("KVM: LAPIC: Inject timer interrupt via posted interrupt")

添加如下测试:
```c
static bool kvm_can_post_timer_interrupt(struct kvm_vcpu *vcpu)
{
	// 只有 apicv ，其余都是 0
	pr_info("pi    : %d\n",pi_inject_timer);
	pr_info("apicv : %d\n",kvm_vcpu_apicv_active(vcpu));
	pr_info("mwait : %d\n",kvm_mwait_in_guest(vcpu->kvm));
	pr_info("hlt   : %d\n",kvm_hlt_in_guest(vcpu->kvm));

	return pi_inject_timer && kvm_vcpu_apicv_active(vcpu) &&
		(kvm_mwait_in_guest(vcpu->kvm) || kvm_hlt_in_guest(vcpu->kvm));
}
```

l1 中测试的结果:
```txt
[81572.263999] kvm: pi    : 0
[81572.264160] kvm: apicv : 1
[81572.264315] kvm: mwait : 0
[81572.264496] kvm: hlt   : 0
```

物理机中也是如此:
```txt
[223009.551111] kvm: A : 0
[223009.551112] kvm: B : 1
[223009.551112] kvm: C : 0
[223009.551112] kvm: D : 0
```

### pi_inject_timer

取决于
```c
if (pi_inject_timer == -1)
		pi_inject_timer = housekeeping_enabled(HK_TYPE_TIMER);
```
housekeeping_enabled 相关到 docs/kernel/sched-isolation.md 中重新理解吧


### hlt 和 timer
需要外部配置，不过这么用实际上是有 bug 的，而且 qemu 也不支持
```c
	case KVM_CAP_X86_DISABLE_EXITS:
		r = -EINVAL;
		if (cap->args[0] & ~KVM_X86_DISABLE_VALID_EXITS)
			break;

		if (cap->args[0] & KVM_X86_DISABLE_EXITS_PAUSE)
			kvm->arch.pause_in_guest = true;

#define SMT_RSB_MSG "This processor is affected by the Cross-Thread Return Predictions vulnerability. " \
		    "KVM_CAP_X86_DISABLE_EXITS should only be used with SMT disabled or trusted guests."

		if (!mitigate_smt_rsb) {
			if (boot_cpu_has_bug(X86_BUG_SMT_RSB) && cpu_smt_possible() &&
			    (cap->args[0] & ~KVM_X86_DISABLE_EXITS_PAUSE))
				pr_warn_once(SMT_RSB_MSG);

			if ((cap->args[0] & KVM_X86_DISABLE_EXITS_MWAIT) &&
			    kvm_can_mwait_in_guest())
				kvm->arch.mwait_in_guest = true;
			if (cap->args[0] & KVM_X86_DISABLE_EXITS_HLT)
				kvm->arch.hlt_in_guest = true;
			if (cap->args[0] & KVM_X86_DISABLE_EXITS_CSTATE)
				kvm->arch.cstate_in_guest = true;
		}
```
### 结论

虚拟机靠 preempter timer 和 halt 之后，host 的 timer 注入

## 一个安静的 kvm 机器一天到晚都在 restart_apic_timer

```txt
@[
    restart_apic_timer+1
    kvm_arch_vcpu_load+419
    finish_task_switch.isra.0+257
    __schedule+911
    schedule+94
    yield_to+473
    kvm_vcpu_yield_to+64
    kvm_vcpu_on_spin+273
    handle_pause+38
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1701
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 59
@[
    restart_apic_timer+1
    kvm_arch_vcpu_load+419
    finish_task_switch.isra.0+257
    __schedule+911
    schedule+94
    xfer_to_guest_mode_handle_work+97
    kvm_arch_vcpu_ioctl_run+2174
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 8711
@[
    restart_apic_timer+1
    handle_fastpath_set_msr_irqoff+294
    kvm_arch_vcpu_ioctl_run+1397
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1519601
```

如果是的绝对的安静的时候，结果为:
- handle_fastpath_set_msr_irqoff
  - handle_fastpath_set_tscdeadline ，显然，其实就是再重新编辑 tsc 的时间

```c
static void restart_apic_timer(struct kvm_lapic *apic)
{
	preempt_disable();

	if (!apic_lvtt_period(apic) && atomic_read(&apic->lapic_timer.pending))
		goto out;

	if (!start_hv_timer(apic))
		start_sw_timer(apic);
out:
	preempt_enable();
}
```

## `start_sw_timer`

```txt
@[
    start_sw_timer+1
    restart_apic_timer+110
    handle_fastpath_set_msr_irqoff+220
    kvm_arch_vcpu_ioctl_run+3236
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 24
@[
    start_sw_timer+1
    kvm_lapic_switch_to_sw_timer+63
    kvm_arch_vcpu_ioctl_run+2628
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 290
```

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
