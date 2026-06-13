# 记录几个相见恨晚的 tracepoint 点


## 注入中断的
```c
void vmx_deliver_interrupt(struct kvm_lapic *apic, int delivery_mode,
			   int trig_mode, int vector)
{
	struct kvm_vcpu *vcpu = apic->vcpu;

	if (vmx_deliver_posted_interrupt(vcpu, vector)) {
		kvm_lapic_set_irr(vector, apic);
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
	} else {
		trace_kvm_apicv_accept_irq(vcpu->vcpu_id, delivery_mode,
					   trig_mode, vector);
	}
}
```


## msr

trace_kvm_msr_write

四个 msr 合集:
```c
#define trace_kvm_msr_read(ecx, data)      trace_kvm_msr(0, ecx, data, false)
#define trace_kvm_msr_write(ecx, data)     trace_kvm_msr(1, ecx, data, false)
#define trace_kvm_msr_read_ex(ecx)         trace_kvm_msr(0, ecx, 0, true)
#define trace_kvm_msr_write_ex(ecx, data)  trace_kvm_msr(1, ecx, data, true)
```

bakctrace 的结果:
```txt
@[
    handle_fastpath_set_msr_irqoff+420
    handle_fastpath_set_msr_irqoff+420
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1188
```

```sh
sudo perf top -e kvm:kvm_msr
```

- vcpu_enter_guest
  - vmx_vcpu_run
    - vmx_exit_handlers_fastpath
      - handle_fastpath_set_msr_irqoff

## kvm::kvm_mmio

## kvm:kvm_set_irq

## kvm:kvm_msi_set_irq

这里记录了 msi 中断的注入过程:

```txt
Samples: 940K of event 'kvm:kvm_msi_set_irq', 1 Hz, Event count (approx.): 804098 lost: 0/0 drop: 0/0
Overhead  Trace output
  99.99%  dst 0 vec 54 (Fixed|physical|edge)
   0.01%  dst 0 vec 55 (Fixed|physical|edge)
   0.00%  dst 0 vec 41 (Fixed|physical|edge)
   0.00%  dst 0 vec 57 (Fixed|physical|edge)
   0.00%  dst 0 vec 60 (Fixed|physical|edge)
```

## kvm:kvm_userspace_exit

windows 虚拟机，enable_apicv=off 的
```txt
Samples: 8K of event 'kvm:kvm_userspace_exit', 1 Hz, Event count (approx.): 5227 lost: 0/0 drop: 0/0
Overhead  Trace output
  96.19%  reason KVM_EXIT_MMIO (6)
   3.81%  reason KVM_EXIT_IO (2)
```

enable_apicv=on 的时候:
```txt
Samples: 26K of event 'kvm:kvm_userspace_exit', 1 Hz, Event count (approx.): 20609 lost: 0/0 drop: 0/0
Overhead  Trace output
  96.82%  reason KVM_EXIT_IO (2)
   3.18%  reason KVM_EXIT_MMIO (6)
```

## kvm::kvm_apic_accept_irq

这个 tracepoint 记录了所有发送给 lapic 的中断:
```txt
@[
    __apic_accept_irq+402
    __apic_accept_irq+402
    kvm_apic_local_deliver+127
    kvm_inject_apic_timer_irqs+43
    kvm_arch_vcpu_ioctl_run+2186
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1374
@[
    __apic_accept_irq+402
    __apic_accept_irq+402
    kvm_irq_delivery_to_apic_fast+320
    kvm_irq_delivery_to_apic+103
    kvm_apic_send_ipi+175
    kvm_x2apic_icr_write+37
    handle_fastpath_set_msr_irqoff+192
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119 <------------- 似乎时钟中断，似乎是
]: 4517
@[
    __apic_accept_irq+402
    __apic_accept_irq+402
    kvm_irq_delivery_to_apic_fast+320
    kvm_irq_delivery_to_apic+103
    kvm_set_msi+138
    kvm_send_userspace_msi+120
    kvm_vm_ioctl+468
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119 <--- 这个是 iperf3 制作的中断
]: 529160
```


## 方便的 bpftrace
其实，绝大多数的时候，是不需要重新安装内核模块的，例如检查 msr io
```sh
# 检查是不是写了 MSR_IA32_TSC_ADJUST
sudo bpftrace -e 'kfunc:kvm:kvm_set_msr_common { if (args->msr_info->index == 0x0000003b) { print(args->msr_info->host_initiated) } }'

# 检查 ioctl 是不是 KVM_SET_MSRS
sudo bpftrace -e 'kfunc:kvm:kvm_arch_vcpu_ioctl { if (args->ioctl ==  1074310793) { print("hi") } }'
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
