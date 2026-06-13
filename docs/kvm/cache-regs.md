vmcs 的很多字段大多数时候不用刷新，所以

arch/x86/kvm/kvm_cache_regs.h

- vcpu_load_eoi_exitmap
  - vmx_load_eoi_exitmap


1. 这里为什么要调整?
2. hyperv 为什么需要有区别?
```c
static void vcpu_load_eoi_exitmap(struct kvm_vcpu *vcpu)
{
	if (!kvm_apic_hw_enabled(vcpu->arch.apic))
		return;

#ifdef CONFIG_KVM_HYPERV
	if (to_hv_vcpu(vcpu)) {
		u64 eoi_exit_bitmap[4];

		bitmap_or((ulong *)eoi_exit_bitmap,
			  vcpu->arch.ioapic_handled_vectors,
			  to_hv_synic(vcpu)->vec_bitmap, 256);
		static_call_cond(kvm_x86_load_eoi_exitmap)(vcpu, eoi_exit_bitmap);
		return;
	}
#endif
	static_call_cond(kvm_x86_load_eoi_exitmap)(
		vcpu, (u64 *)vcpu->arch.ioapic_handled_vectors);
}
```
这里为什么需要将 ioapic 处理的中断放到 。

表示，如果写 1 ，那么这些位置 eoi 需要 exit ?

## 复杂啊 : 看看 reference 的位置!

```c
		if (kvm_check_request(KVM_REQ_IOAPIC_EOI_EXIT, vcpu)) {
			BUG_ON(vcpu->arch.pending_ioapic_eoi > 255);
			if (test_bit(vcpu->arch.pending_ioapic_eoi,
				     vcpu->arch.ioapic_handled_vectors)) {
				vcpu->run->exit_reason = KVM_EXIT_IOAPIC_EOI;
				vcpu->run->eoi.vector =
						vcpu->arch.pending_ioapic_eoi;
				r = 0;
				goto out;
			}
		}
```

## 为什么要区分 ioapic 的 io ?

```c
static bool kvm_ioapic_handles_vector(struct kvm_lapic *apic, int vector)
{
	return test_bit(vector, apic->vcpu->arch.ioapic_handled_vectors);
}
```

apic_set_eoi 中为什么需要调用 kvm_ioapic_send_eoi

```c
static void kvm_ioapic_send_eoi(struct kvm_lapic *apic, int vector)
{
	int trigger_mode;

	/* Eoi the ioapic only if the ioapic doesn't own the vector. */
	if (!kvm_ioapic_handles_vector(apic, vector))
		return;

	/* Request a KVM exit to inform the userspace IOAPIC. */
	if (irqchip_split(apic->vcpu->kvm)) {
		apic->vcpu->arch.pending_ioapic_eoi = vector;
		kvm_make_request(KVM_REQ_IOAPIC_EOI_EXIT, apic->vcpu);
		return;
	}

	if (apic_test_vector(vector, apic->regs + APIC_TMR))
		trigger_mode = IOAPIC_LEVEL_TRIG;
	else
		trigger_mode = IOAPIC_EDGE_TRIG;

	kvm_ioapic_update_eoi(apic->vcpu, vector, trigger_mode);
}
```

## 如何理解这里的 trap-like exit ?
似乎就是问题的关键了；

## 难道和 io 路由有关系?
```c
/*
 * this interface assumes a trap-like exit, which has already finished
 * desired side effect including vISR and vPPR update.
 */
void kvm_apic_set_eoi_accelerated(struct kvm_vcpu *vcpu, int vector)
{
	struct kvm_lapic *apic = vcpu->arch.apic;

	trace_kvm_eoi(apic, vector);

	kvm_ioapic_send_eoi(apic, vector);
	kvm_make_request(KVM_REQ_EVENT, apic->vcpu);
}
```

## 串口，exit 的时候，为什么是 io instruction

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
