# RCU

- https://www.zhihu.com/question/27943222/answer/2174857178

- [ ] 思考一下，RCU 在用户态和内核态中实现的差异
- [ ] 将 QEMU 中对于 RCU 的使用移动到这里
- [ ] https://liburcu.org/ : 提供了三个很好的资源
- https://mp.weixin.qq.com/s/SZqmxMGMyruYUH5n_kobYQ
- https://hackmd.io/@sysprog/linux-rcu?type=view
- `__d_lookup_rcu`
  - 实际上，rcu 的组件比想想的更加多


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

## 使用 QEMU 调试的过程中，Guest 首先一致卡在 idel 中，然后触发这个 bug

```txt
[ 4192.186591] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[ 4192.187264]  (detected by 7, t=42141 jiffies, g=10393, q=61 ncpus=8)
[ 4192.187264] rcu: All QSes seen, last rcu_preempt kthread activity 42025 (4298858205-4298816180), jiffies_till_next_fqs=3, root ->qsmask 0x0
[ 4192.187264] rcu: rcu_preempt kthread timer wakeup didn't happen for 42031 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200
[ 4192.187264] rcu:     Possible timer handling issue on cpu=1 timer-softirq=2124
[ 4192.187264] rcu: rcu_preempt kthread starved for 42048 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200 ->cpu=1
[ 4192.187264] rcu:     Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[ 4192.187264] rcu: RCU grace-period kthread stack dump:
[ 4192.187264] task:rcu_preempt     state:R stack:14976 pid:   14 ppid:     2 flags:0x00004000
[ 4192.187264] Call Trace:
[ 4192.187264]  <TASK>
[ 4192.187264]  __schedule+0x2a4/0x7a0
[ 4192.187264]  ? rcu_gp_cleanup+0x4f0/0x4f0
[ 4192.187264]  schedule+0x55/0xa0
[ 4192.187264]  schedule_timeout+0x83/0x150
[ 4192.187264]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 4192.187264]  ? timer_migration_handler+0x90/0x90
[ 4192.187264]  rcu_gp_fqs_loop+0x129/0x5d0
[ 4192.187264]  rcu_gp_kthread+0x19b/0x240
[ 4192.187264]  kthread+0xe0/0x110
[ 4192.187264]  ? kthread_complete_and_exit+0x20/0x20
[ 4192.187264]  ret_from_fork+0x1f/0x30
[ 4192.187264]  </TASK>
[ 4192.187264] rcu: Stack dump where RCU GP kthread last ran:
[ 4192.187264] Sending NMI from CPU 7 to CPUs 1:
[ 4150.290203] NMI backtrace for cpu 1 skipped: idling at default_idle+0xb/0x10
[ 4213.292264] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[ 4213.293260]  (detected by 0, t=63241 jiffies, g=10393, q=151 ncpus=8)
[ 4213.293260] rcu: All QSes seen, last rcu_preempt kthread activity 63126 (4298879306-4298816180), jiffies_till_next_fqs=3, root ->qsmask 0x0
[ 4213.293260] rcu: rcu_preempt kthread timer wakeup didn't happen for 63133 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200
[ 4213.293260] rcu:     Possible timer handling issue on cpu=1 timer-softirq=2124
[ 4213.293260] rcu: rcu_preempt kthread starved for 63151 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200 ->cpu=1
[ 4213.293260] rcu:     Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[ 4213.293260] rcu: RCU grace-period kthread stack dump:
[ 4213.293260] task:rcu_preempt     state:R stack:14976 pid:   14 ppid:     2 flags:0x00004000
[ 4213.293260] Call Trace:
[ 4213.293260]  <TASK>
[ 4213.293260]  __schedule+0x2a4/0x7a0
[ 4213.293260]  ? rcu_gp_cleanup+0x4f0/0x4f0
[ 4213.293260]  schedule+0x55/0xa0
[ 4213.293260]  schedule_timeout+0x83/0x150
[ 4213.293260]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 4213.293260]  ? timer_migration_handler+0x90/0x90
[ 4213.293260]  rcu_gp_fqs_loop+0x129/0x5d0
[ 4213.293260]  rcu_gp_kthread+0x19b/0x240
[ 4213.293260]  kthread+0xe0/0x110
[ 4213.293260]  ? kthread_complete_and_exit+0x20/0x20
[ 4213.293260]  ret_from_fork+0x1f/0x30
[ 4213.293260]  </TASK>
[ 4213.293260] rcu: Stack dump where RCU GP kthread last ran:
[ 4213.293260] Sending NMI from CPU 0 to CPUs 1:
[ 4192.297288] NMI backtrace for cpu 1 skipped: idling at default_idle+0xb/0x10
```

在 mount 的时候暂停了特别久之后，出现了这个 bug:
```txt
[ 5973.259566] rcu: INFO: rcu_preempt self-detected stall on CPU
[ 5973.259566] rcu:     3-...!: (4 ticks this GP) idle=f824/0/0x1 softirq=4614/4614 fqs=0
[ 5973.259566]  (t=39404443 jiffies g=18581 q=33 ncpus=8)
[ 5973.259566] NMI backtrace for cpu 3
[ 5973.259566] CPU: 3 PID: 0 Comm: swapper/3 Not tainted 6.0.0-rc1-00399-g15b3f48a4339 #23
[ 5973.259566] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.0-0-gd239552ce722-prebuilt.qemu.org 04/01/2014

[ 5973.259566] Call Trace:
[ 5973.259566]  <IRQ>
[ 5973.259566]  dump_stack_lvl+0x34/0x48
[ 5973.259566]  nmi_cpu_backtrace.cold+0x30/0x70
[ 5973.259566]  ? lapic_can_unplug_cpu+0x80/0x80
[ 5973.259566]  nmi_trigger_cpumask_backtrace+0xc6/0xe0
[ 5973.259566]  rcu_dump_cpu_stacks+0x106/0x150
[ 5973.259566]  rcu_sched_clock_irq.cold+0x63/0x2f2
[ 5973.259566]  ? raw_notifier_call_chain+0x3c/0x60
[ 5973.259566]  ? timekeeping_update+0xab/0x280
[ 5973.259566]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 5973.259566]  ? timekeeping_advance+0x373/0x550
[ 5973.259566]  update_process_times+0x5d/0x90
[ 5973.259566]  tick_sched_handle+0x2f/0x40
[ 5973.259566]  tick_sched_timer+0x6c/0x80
[ 5973.259566]  ? tick_sched_do_timer+0x90/0x90
[ 5973.259566]  __hrtimer_run_queues+0x122/0x2c0
[ 5973.259566]  hrtimer_interrupt+0x101/0x220
[ 5973.259566]  __sysvec_apic_timer_interrupt+0x77/0x160
[ 5973.259566]  sysvec_apic_timer_interrupt+0x9d/0xd0
[ 5973.259566]  </IRQ>
[ 5973.259566]  <TASK>
[ 5973.259566]  asm_sysvec_apic_timer_interrupt+0x16/0x20
[ 5973.259566] RIP: 0010:default_idle+0xb/0x10
[ 5973.259566] Code: 0a 00 00 eb 81 4c 89 ef e8 d2 b3 84 ff eb c4 e8 bb 65 ff ff cc cc cc cc cc cc cc cc cc cc cc 66 90 0f 00 2d 77 68 56 00 fb f4 <c3> cc cc cc c
[ 5973.259566] RSP: 0018:ffffc9000009bee8 EFLAGS: 00000256
[ 5973.259566] RAX: ffffffff81eb71d0 RBX: 0000000000000003 RCX: 0000000000000000
[ 5973.259566] RDX: 0000000000000001 RSI: ffffffff82813bd9 RDI: 00000000000ef81c
[ 5973.259566] RBP: ffff8883002b2e00 R08: 0000039833046055 R09: 0000000000000115
[ 5973.259566] R10: 0000000000000001 R11: 0000000000000001 R12: 0000000000000000
[ 5973.259566] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[ 5973.259566]  ? __cpuidle_text_start+0x8/0x8
[ 5973.259566]  default_idle_call+0x2c/0xe0
[ 5973.259566]  do_idle+0x1ed/0x260
[ 5973.259566]  cpu_startup_entry+0x14/0x20
[ 5973.259566]  start_secondary+0xe8/0xf0
[ 5973.259566]  secondary_startup_64_no_verify+0xe0/0xeb
[ 5973.259566]  </TASK>
```
那么，我是不是大可怀疑，当一个 Guest 机器被 freeze 之后，是不是也会出现这个问题的。


暂停之后的时间也很短:
```txt
[root@localhost huge]# date
Sun Aug 21 23:21:23 EDT 2022
```

### 有一个类似的错误，但是似乎和 rcu 无关
出错位置 : virtblk_setup_cmd

```txt
[   38.630644] Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00
[   38.631092] CPU: 3 PID: 1 Comm: init Not tainted 6.0.0-rc5-dirty #61
[   38.631092] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.0-0-gd239552ce722-prebuilt.qemu.org 04/01/2014
[   38.631092] Call Trace:
[   38.631092]  <TASK>
[   38.631092]  dump_stack_lvl+0x34/0x48
[   38.631092]  panic+0x102/0x27b
[   38.631092]  ? _raw_spin_trylock+0xe/0x50
In[c o n s38.631092]  do_exit.cold+0x15/0x45
[   38.631092]  do_group_exit+0x28/0x90
[   38.631092istency de]  __x64_sys_exit_group+0xf/0x10
[   38.631092]  do_syscall_64+0x3b/0x90
tec[t e d  3b8.631092]  entry_SYSCALL_64_after_hwframe+0x63/0xcd
[   38.631092] RIP: 0033:0x7fe729e23aae
[   38.631092] Code:y ld.so: 89 fa 41 b8 e7 00 00 00 be 3c 00 00 00 eb 14 0f 1f 44 00 00 89 d7 89 f0 0f 05 48 3d 00 f0 ff ff 77 1a f4 89 d7 44 89 c0 0f 05 <48> 3d 00 f0 ff ff 76 e2 f7 d8 89 05 0a b7 20 00 eb d8 f7 d8 89 05
[   38.631092] RSP: 002b:00007ffeace91708 EFLAGS: 00000246 ORIG_RAX: 00000000000000e7
[   38.631092] RAX: ffffffffffffffda RBX: 00007fe727c020d2 RCX: 00007fe729e23aae
[   38.631092] RDX: 000000000000007f RSI: 000000000000003c RDI: 000000000000007f
[   38.631092] RBP: 0000000000000000 R08: 00000000000000e7 R09: 0000000000000000
[   38.631092] R10: 0000000000000020 R11: 0000000000000246 R12: 00007fe727c02438
[   38.631092] R13: 00007fe7278021d4 R14: 00007fe72a1714f0 R15: 000000000000000f
[   38.631092]  </TASK>
 dl-version.c: 205: _dl_check_map_versions: Assertion `needed != NULL' failed!
[   38.631092] Kernel Offset: disabled
[   38.631092] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00 ]---
```


### 参考资料
- [What is RCU, Fundamentally?](https://lwn.net/Articles/262464/)
- [What is RCU? Part 2: Usage](https://lwn.net/Articles/263130/)
- [RCU part 3: the RCU API](https://lwn.net/Articles/264090/)
- [kernel doc](https://www.kernel.org/doc/Documentation/RCU/)
