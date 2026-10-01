# qemu 如何支持 kvm 的

4.2 中 cpus.c::qemu_init_vcpu 中间分析了多种 kvm 和 tcg 的引擎, 由于 tcg 和 kvm 被放到 accel 下，但是
whpx, hvf[^1] 和 hax 的支持都是 intel 特有的，所以在 qemu/target/i386 下面, 说着是加速，
其实这应该是 exec engine, 在 latest 的版本，这一些内容都被移动到这里了

对于 kvm 支持的位置:
hw/i386/kvm : 主要是设备模拟
accel/kvm


## accel/kvm/kvm-all.c:do_kvm_cpu_synchronize_state 到底是什么东西?
bool CPUState::vcpu_dirty

似乎只有进入到特殊状态之后，才会执行这些奇怪的操作。
kvm_cpu_synchronize_state

除了一个情况，
```c
static void kvm_accel_ops_class_init(ObjectClass *oc, const void *data)
{
    AccelOpsClass *ops = ACCEL_OPS_CLASS(oc);

    ops->create_vcpu_thread = kvm_start_vcpu_thread;
    ops->cpu_thread_is_idle = kvm_vcpu_thread_is_idle;
    ops->cpus_are_resettable = kvm_cpus_are_resettable;
    ops->synchronize_post_reset = kvm_cpu_synchronize_post_reset;
    ops->synchronize_post_init = kvm_cpu_synchronize_post_init;
    ops->synchronize_state = kvm_cpu_synchronize_state;
    ops->synchronize_pre_loadvm = kvm_cpu_synchronize_pre_loadvm;

#ifdef TARGET_KVM_HAVE_GUEST_DEBUG
    ops->update_guest_debug = kvm_update_guest_debug_ops;
    ops->supports_guest_debug = kvm_supports_guest_debug;
    ops->insert_breakpoint = kvm_insert_breakpoint;
    ops->remove_breakpoint = kvm_remove_breakpoint;
    ops->remove_all_breakpoints = kvm_remove_all_breakpoints;
#endif
}
```
这个导致，当 qhp 执行的时候，这个函数可以被调用。

### 如何理解 kvm_arch_process_async_events

kvm_cpu_exec 调用的开始会现执行一下 kvm_arch_process_async_events

但是在正常的配置中，这里的正常配置值得是 kvm_irqchip_in_kernel() ，只有这两个:
- CPU_INTERRUPT_MCE
- CPU_INTERRUPT_INIT


## 看看 kvm_put_msrs 的实现
1. 如何实现一次性提交多个 msr 的。

2. 这个是如何知道的 has_msr_tsc_aux 或者 has_msr_tsc_adjust ，需要取决于 guest 的 id 吗?
```c
    if (has_msr_tsc_aux) {
        kvm_msr_entry_add(cpu, MSR_TSC_AUX, env->tsc_aux);
    }
    if (has_msr_tsc_adjust) {
        kvm_msr_entry_add(cpu, MSR_TSC_ADJUST, env->tsc_adjust);
    }
```

## 在进行对于 vcpu 的 ioctl 的时候，vcpu 是继续运行的状态吗?

```txt
@[
    kvm_vcpu_write_tsc_offset+1
    __kvm_synchronize_tsc+78
    kvm_synchronize_tsc+232
    kvm_set_msr_common+2202
    vmx_set_msr+1262
    __kvm_set_msr+145
    kvm_arch_vcpu_ioctl+3121
    kvm_vcpu_ioctl+896
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 8
```
我倾向于认为 vcpu 执行不受 vcpu ioctl 影响。
因为最后还是修改 vmcs 之类的，然后让 vcpu 下次进入的时候使用全新的
内容。


看这个注释，的确是异步的，但是
```c
static void kvm_vcpu_write_tsc_multiplier(struct kvm_vcpu *vcpu, u64 l1_multiplier)
{
	vcpu->arch.l1_tsc_scaling_ratio = l1_multiplier;

	/* Userspace is changing the multiplier while L2 is active */
	if (is_guest_mode(vcpu))
		vcpu->arch.tsc_scaling_ratio = kvm_calc_nested_tsc_multiplier(
			l1_multiplier,
			static_call(kvm_x86_get_l2_tsc_multiplier)(vcpu));
	else
		vcpu->arch.tsc_scaling_ratio = l1_multiplier;

	if (kvm_caps.has_tsc_control)
		static_call(kvm_x86_write_tsc_multiplier)(vcpu);
}
```

通过这个 bftrace 来看，不是的:
```txt
sudo bpftrace -e 'kprobe:kvm_synchronize_tsc { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
```

```txt
@[CPU 1/KVM]: 3
@[CPU 0/KVM]: 3

```

当执行 info registers 的时候:
```txt
+ sudo bpftrace -e 'kprobe:kvm_arch_vcpu_ioctl_get_regs { @[curtask->comm]
 = count() } interval:s:1000 { exit(); }'
Attached 2 probes
^C

@[CPU 0/KVM]: 1
```

先不去看具体是如何 kick ，让 vCPU thread 来执行 cpu ioctl 从设计上最好的，
因为这样避免 vCPU thread 被多线程访问，是在方便不过了。

## qemu 如何处理 nested 状态问题

https://lkml.iu.edu/1804.1/05279.html 这里引入的
```txt
History:        #0
Commit:         8fcc4b5923af5de58b80b53a069453b135693304
Author:         Jim Mattson <jmattson@google.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Tue 10 Jul 2018 05:27:20 PM CST
Committer Date: Mon 06 Aug 2018 11:58:30 PM CST

kvm: nVMX: Introduce KVM_CAP_NESTED_STATE

For nested virtualization L0 KVM is managing a bit of state for L2 guests,
this state can not be captured through the currently available IOCTLs. In
fact the state captured through all of these IOCTLs is usually a mix of L1
and L2 state. It is also dependent on whether the L2 guest was running at
the moment when the process was interrupted to save its state.

With this capability, there are two new vcpu ioctls: KVM_GET_NESTED_STATE
and KVM_SET_NESTED_STATE. These can be used for saving and restoring a VM
that is in VMX operation.

Cc: Paolo Bonzini <pbonzini@redhat.com>
Cc: Radim Krčmář <rkrcmar@redhat.com>
Cc: Thomas Gleixner <tglx@linutronix.de>
Cc: Ingo Molnar <mingo@redhat.com>
Cc: H. Peter Anvin <hpa@zytor.com>
Cc: x86@kernel.org
Cc: kvm@vger.kernel.org
Cc: linux-kernel@vger.kernel.org
Signed-off-by: Jim Mattson <jmattson@google.com>
[karahmed@ - rename structs and functions and make them ready for AMD and
             address previous comments.
           - handle nested.smm state.
           - rebase & a bit of refactoring.
           - Merge 7/8 and 8/8 into one patch. ]
Signed-off-by: KarimAllah Ahmed <karahmed@amazon.de>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

```c
struct kvm_x86_nested_ops vmx_nested_ops = {
	.leave_nested = vmx_leave_nested,
	.is_exception_vmexit = nested_vmx_is_exception_vmexit,
	.check_events = vmx_check_nested_events,
	.has_events = vmx_has_nested_events,
	.triple_fault = nested_vmx_triple_fault,

	.get_state = vmx_get_nested_state,
	.set_state = vmx_set_nested_state,
  // 这个是和 hyperv 相关的
	.get_nested_state_pages = vmx_get_nested_state_pages,

	.write_log_dirty = nested_vmx_write_pml_buffer,
#ifdef CONFIG_KVM_HYPERV
	.enable_evmcs = nested_enable_evmcs,
	.get_evmcs_version = nested_get_evmcs_version,
	.hv_inject_synthetic_vmexit_post_tlb_flush = vmx_hv_inject_synthetic_vmexit_post_tlb_flush,
#endif
};
```

## kvm_arch_vcpu_ioctl_run 的调用频率非常低
<!-- 6aff7bfd-ef04-41fe-856a-3973fea1d905 -->

总体来说，这是预期的，只有那些导致需要 exit 到 userspace 的 io
才会最后导致 kvm_arch_vcpu_ioctl_run 。不过观察到 1 分钟可能只有几次
kvm_arch_vcpu_ioctl_run 的调用，这还是让人有点惊讶的。

观测下，到底是什么导致 kvm_arch_vcpu_ioctl_run 被调用

TODO : 可以观察这个的 function graph ，看看主要都是在哪里循环，构建一个基本印象。

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
