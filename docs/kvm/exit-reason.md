## 将所有的 exit reason 都整理下

EXIT_REASON_NOTIFY

### preemempt timer

在 intel sdm 中的 25.4.2 Guest Non-Register State 中的，可以看到
VMX-preemption timer value 可以看看 kernel 的实现在那里。

### 如何理解 EXIT_REASON_NOTIFY

```c
static int handle_notify(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qual = vmx_get_exit_qual(vcpu);
	bool context_invalid = exit_qual & NOTIFY_VM_CONTEXT_INVALID;

	++vcpu->stat.notify_window_exits;

	/*
	 * Notify VM exit happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (enable_vnmi && (exit_qual & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
			      GUEST_INTR_STATE_NMI);

	if (vcpu->kvm->arch.notify_vmexit_flags & KVM_X86_NOTIFY_VMEXIT_USER ||
	    context_invalid) {
		vcpu->run->exit_reason = KVM_EXIT_NOTIFY;
		vcpu->run->notify.flags = context_invalid ?
					  KVM_NOTIFY_CONTEXT_INVALID : 0;
		return 0;
	}

	return 1;
}
```
### EXIT_REASON_INTERRUPT_WINDOW

### EXIT_REASON_EXTERNAL_INTERRUPT

### [ ] 到底会因为时钟中断退出吗?

## 这个 exit reason 如何理解
```txt
  24.98%  reason EXIT_IOIO rip 0x6b43 info 920091 6b45
  24.98%  reason EXIT_IOIO rip 0x6bd8 info 700091 6bda
  24.98%  reason EXIT_CR0_SEL_WRITE rip 0xcf5d info 8000000000000001 0
  24.98%  reason EXIT_CR0_SEL_WRITE rip 0xcfaa info 8000000000000001 0
```

## [ ] 所有的 msr 都会 exit 吗?

```c
	[EXIT_REASON_MSR_READ]                = kvm_emulate_rdmsr,
	[EXIT_REASON_MSR_WRITE]               = kvm_emulate_wrmsr,
```

## 综合测试
1 统计各种 exit 的样式

频率，比例，都是做什么的

suspend ，idle , stress cpu , stress memory , iperf

## 头疼啊
```txt
	[EXIT_REASON_EOI_INDUCED]             = handle_apic_eoi_induced,
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
