
这个文档整理的非常清楚了: https://liujunming.top/2022/08/20/Notes-about-KVM-steal-time/

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

## guest os
在 guest 这一侧的实现:
```txt
[root@localhost 14:25:14 ~]$  /usr/share/bcc/tools/stackcount account_steal_time
Tracing 1 functions for "account_steal_time"... Hit Ctrl-C to end.
^C
  b'account_steal_time'
  b'irqtime_account_process_tick.isra.10'
  b'update_process_times'
  b'tick_sched_handle'
  b'tick_sched_timer'
  b'__hrtimer_run_queues'
  b'hrtimer_interrupt'
  b'smp_apic_timer_interrupt'
  b'apic_timer_interrupt'
```

record_steal_time 中将 /proc/self/schedstat 的 run_delay 传递到 Guest 中
```c
vcpu->arch.st.last_steal = current->sched_info.run_delay;
```

guest os 的行为，只有先检测到 KVM_VCPU_PREEMPTED 之后，才去使用 flush
```c
		if ((state & KVM_VCPU_PREEMPTED)) {
			if (try_cmpxchg(&src->preempted, &state,
					state | KVM_VCPU_FLUSH_TLB))
				__cpumask_clear_cpu(cpu, flushmask);
		}
```

## host 写入 和 guest 读出

record_steal_time

```c
	unsafe_get_user(steal, &st->steal, out);
	steal += current->sched_info.run_delay -
		vcpu->arch.st.last_steal;
	vcpu->arch.st.last_steal = current->sched_info.run_delay;
	unsafe_put_user(steal, &st->steal, out);
```

```c
/*
 * When a guest is interrupted for a longer amount of time, missed clock
 * ticks are not redelivered later. Due to that, this function may on
 * occasion account more time than the calling functions think elapsed.
 */
static __always_inline u64 steal_account_process_time(u64 maxtime)
{
#ifdef CONFIG_PARAVIRT
	if (static_key_false(&paravirt_steal_enabled)) {
		u64 steal;

		steal = paravirt_steal_clock(smp_processor_id());
		steal -= this_rq()->prev_steal_time;
		steal = min(steal, maxtime);
		account_steal_time(steal);
		this_rq()->prev_steal_time += steal;

		return steal;
	}
#endif
	return 0;
}
```

show_stat 中导出。
## 为什么不是直接将 host 的 run delay 传递进去，而是总是使用 prev_steal_time 以及 last_steal 之类的


## 测试
总体来说，数值是可以对应上的，但是
![](./stealtime.png)

## 其他
- https://github.com/GiantVM/KVM-Annotation/wiki/Steal-Time


## host 的行为
```c
struct kvm_steal_time {
	__u64 steal;
	__u32 version;
	__u32 flags;
	__u8  preempted; // 需要记录两个 bit
	__u8  u8_pad[3];
	__u32 pad[11];
};

#define KVM_VCPU_PREEMPTED          (1 << 0)
#define KVM_VCPU_FLUSH_TLB          (1 << 1)
```

```c
struct kvm_vcpu_arch {
  // ...
	struct {
		u8 preempted;               // 用于 kvm_steal_time_set_preempted 提前退出的
		u64 msr_val;                // 获取到 gpa ，是通过虚拟机传递过来的
		u64 last_steal;             // 存储 run_delay 的
		struct gfn_to_hva_cache cache;
	} st;
  // ...
}
```

kvm_arch_vcpu_put -> `kvm_steal_time_set_preempted` ，是 vcpu 离开的地方，如果知道了
是因为 preempted 的，那么告知 guest os :
```c
	if (!copy_to_user_nofault(&st->preempted, &preempted, sizeof(preempted)))
		vcpu->arch.st.preempted = KVM_VCPU_PREEMPTED;
```
这里的东西，为什么?


当 vcpu 进入的时候，也就是 kvm_arch_vcpu_load 无条件的触发 KVM_REQ_STEAL_UPDATE ，在即将运行之前:
```c
		if (kvm_check_request(KVM_REQ_STEAL_UPDATE, vcpu))
			record_steal_time(vcpu);
    // 1. 将到底 steal 了多少 time 写入 guest 的内存中
    // 2. 读取虚拟机的内存，看 guest os 是否需要 flush remote TLB
```

## 问题 1 : kvm_steal_time_set_preempted 什么时候可以被多次调用

好吧，vcpu_put 被调用的地方很多，可以检查一下。

## 问题 2 : 第一个 patch 就可以了吧


## 基本原理

发起 remote tlb flush 的 cpu 检查其他所有 CPU 的 kvm_steal_time::preempted
如果发现有 flag  KVM_VCPU_PREEMPTED ，说明 host 已经将这个 cpu 换出了，所以，host 在这个 CPU 换入的时候需要来做
remote tlb shoot

```txt
@[
    kvm_flush_tlb_multi+5
    arch_tlbbatch_flush+217
    try_to_unmap_flush+43
    migrate_pages_batch+2063
    migrate_pages+1946
    compact_zone+3705
    compact_node+195
    kcompactd+1411
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 44
```

## TODO
才意识到 arm steal time 和 x86 的不同:
- Documentation/virt/kvm/arm/pvtime.rst
- arch/arm64/kvm/pvtime.c


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
