## PV_TLB_FLUSH

- https://kernel.love/para-virt-remote-tlb-flush.html

kvm_arch_vcpu_ioctl_run => record_steal_time

```sh
sudo bpftrace -e 'tracepoint:kvm:kvm_pv_tlb_flush { printf("pv tlb\n"); }'
```
guest 中:
- collapse_file
  - try_to_unmap_flush
    - arch_tlbbatch_flush
      - flush_tlb_multi
        - kvm_flush_tlb_multi


发起者:
```c
static void kvm_flush_tlb_multi(const struct cpumask *cpumask,
			const struct flush_tlb_info *info)
```

通过设置 kvm_steal_time::preempted

在 host 中的接受者:
```c
		trace_kvm_pv_tlb_flush(vcpu->vcpu_id,
				       st_preempted & KVM_VCPU_FLUSH_TLB);

		if (st_preempted & KVM_VCPU_FLUSH_TLB)
			kvm_vcpu_flush_tlb_guest(vcpu);
```

最终调用到 vmx_flush_tlb_guest 中，最终使用 invlpg 或者 invpcid 指令
