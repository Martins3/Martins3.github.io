# kvm feautres
```txt
CPUID 40000000:00 = 40000001 4b4d564b 564b4d56 0000004d | ...@KVMKVMKVM...
CPUID 40000001:00 = 01007afb 00000000 00000000 00000000 | .z..............
```
在 tools/testing/selftests/kvm/include/x86_64/processor.h 可以找到:

- [ ] 这里的每一个 feature 都可以总结下
```c
/* This CPUID returns two feature bitmaps in eax, edx. Before enabling
 * a particular paravirtualization, the appropriate feature bit should
 * be checked in eax. The performance hint feature bit should be checked
 * in edx.
 */
#define KVM_CPUID_FEATURES	0x40000001
#define KVM_FEATURE_CLOCKSOURCE		0
#define KVM_FEATURE_NOP_IO_DELAY	1
#define KVM_FEATURE_MMU_OP		2
/* This indicates that the new set of kvmclock msrs
 * are available. The use of 0x11 and 0x12 is deprecated
 */
#define KVM_FEATURE_CLOCKSOURCE2        3
#define KVM_FEATURE_ASYNC_PF		4
#define KVM_FEATURE_STEAL_TIME		5
#define KVM_FEATURE_PV_EOI		6
#define KVM_FEATURE_PV_UNHALT		7
#define KVM_FEATURE_PV_TLB_FLUSH	9
#define KVM_FEATURE_ASYNC_PF_VMEXIT	10
#define KVM_FEATURE_PV_SEND_IPI	11
#define KVM_FEATURE_POLL_CONTROL	12
#define KVM_FEATURE_PV_SCHED_YIELD	13
#define KVM_FEATURE_ASYNC_PF_INT	14
#define KVM_FEATURE_MSI_EXT_DEST_ID	15
#define KVM_FEATURE_HC_MAP_GPA_RANGE	16
#define KVM_FEATURE_MIGRATION_CONTROL	17
```

## KVM_FEATURE_STEAL_TIME
1. `kvm_register_steal_time` : 通过 msr 寄存器告知
2. kvm_steal_clock
```txt
#0  kvm_steal_clock (cpu=18) at arch/x86/kernel/kvm.c:404
#1  0xffffffff811a82d4 in paravirt_steal_clock (cpu=<optimized out>) at ./arch/x86/include/asm/paravirt.h:45
#2  steal_account_process_time (maxtime=18446744073709551615) at kernel/sched/build_policy.c:4660
#3  account_process_tick (p=0xffff888100a21180, user_tick=user_tick@entry=0) at kernel/sched/build_policy.c:4901
#4  0xffffffff81206bd4 in update_process_times (user_tick=0) at kernel/time/timer.c:2069
#5  0xffffffff8121bf31 in tick_sched_handle (ts=ts@entry=0xffff888333a9fac0, regs=regs@entry=0xffffc90000137e38) at kernel/time/tick-sched.c:243
#6  0xffffffff8121c2f1 in tick_sched_timer (timer=0xffff888333a9fad0) at kernel/time/tick-sched.c:1481
#7  0xffffffff81207a86 in __run_hrtimer (flags=6, now=0xffffc90000560f48, timer=0xffff888333a9fad0, base=0xffff888333a9f5c0, cpu_base=0xffff888333a9f580) at kernel/time/hrtimer.c:1685
#8  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333a9f580, now=11370726342, flags=flags@entry=6, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#9  0xffffffff81208650 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#10 0xffffffff811092b0 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#11 __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#12 0xffffffff822a94d1 in sysvec_apic_timer_interrupt (regs=0xffffc90000137e38) at arch/x86/kernel/apic/apic.c:1106
```

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
