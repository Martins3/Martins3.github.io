# RCU

- [ ] 思考一下，RCU 在用户态和内核态中实现的差异
- [ ] 将 QEMU 中对于 RCU 的使用移动到这里
- [ ] https://liburcu.org/ : 提供了三个很好的资源
- https://mp.weixin.qq.com/s/SZqmxMGMyruYUH5n_kobYQ


## What is Rcu

```c
// if debug config is closed
static __always_inline void rcu_read_lock(void)
{
  __rcu_read_lock(); // preempt_disable();
  // NO !!!!!!!!!!!!! this is impossible
}

#define rcu_assign_pointer(p, v)                          \
do {                                          \
    uintptr_t _r_a_p__v = (uintptr_t)(v);                     \
                                          \
    if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)        \
        WRITE_ONCE((p), (typeof(p))(_r_a_p__v));              \
    else                                      \
        smp_store_release(&p, RCU_INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)

void synchronize_rcu(void)
{
    RCU_LOCKDEP_WARN(lock_is_held(&rcu_bh_lock_map) ||
             lock_is_held(&rcu_lock_map) ||
             lock_is_held(&rcu_sched_lock_map),
             "Illegal synchronize_rcu() in RCU read-side critical section");
    if (rcu_blocking_is_gp())
        return;
    if (rcu_gp_is_expedited())
        synchronize_rcu_expedited();
    else
        wait_rcu_gp(call_rcu);
}
```

## SRCU
e.g., `kvm_mmu_notifier_invalidate_range_start`

sleepable rcu

## 中断也是和 RCU 相关的
```c
void irq_exit(void)
{
#ifndef __ARCH_IRQ_EXIT_IRQS_DISABLED
    local_irq_disable();
#else
    lockdep_assert_irqs_disabled();
#endif
    account_irq_exit_time(current);
    preempt_count_sub(HARDIRQ_OFFSET);
    if (!in_interrupt() && local_softirq_pending())
        invoke_softirq(); ==================================》 __do_softirq

    tick_irq_exit();
    rcu_irq_exit();
    trace_hardirq_exit(); /* must be last! */
}
```
调用了 `rcu_irq_exit`

## 同时 DPDK 中间也是有 RCU 的: https://doc.dpdk.org/guides/prog_guide/rcu_lib.html

## kernel functions
- [ ] `rcu_read_lock_bh` ：使用 ./hack/iperf.svg 中可以参考，就是因为在此处

## 读读 LoyenWang 的 blog

###  https://www.cnblogs.com/LoyenWang/p/12681494.html

- [ ] 没有优先级反转的问题；
- [ ] 当使用不可抢占的 RCU 时，`rcu_read_lock`/`rcu_read_unlock`之间不能使用可以睡眠的代码
  - [ ] 什么代码会导致睡眠?

### https://www.cnblogs.com/LoyenWang/p/12770878.html

- [ ] 为什么需要组织成为 tree 的啊?

### 代码分析

softirq：
1. 时钟中断的时候
```txt
invoke_rcu_core+1
rcu_sched_clock_irq+497
update_process_times+147
tick_sched_handle+34
tick_sched_timer+109
__hrtimer_run_queues+298
hrtimer_interrupt+262
__sysvec_apic_timer_interrupt+127
sysvec_apic_timer_interrupt+157
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```

2. 中断结束的位置开始执行 softirq 的
```txt
rcu_core_si+1
__softirqentry_text_start+238
__irq_exit_rcu+181
sysvec_apic_timer_interrupt+162
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```
- 为什么 `__irq_exit_rcu` 会调用到 `__softirqentry_text_start`，是 backtrace 的 bug 吧！

### 参考资料
- [What is RCU, Fundamentally?](https://lwn.net/Articles/262464/)
- [What is RCU? Part 2: Usage](https://lwn.net/Articles/263130/)
- [RCU part 3: the RCU API](https://lwn.net/Articles/264090/)
- [kernel doc](https://www.kernel.org/doc/Documentation/RCU/)
