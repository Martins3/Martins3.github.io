# pelt

- weight 的计算, share 的计算， load avg 的计算是两个事情吧
  - weight : 就是将
- 如果 pelt 不是必须的，其替代是什么?
  - 是必须的，将原来的 per-run-queue 的统计替代了
- 不使用 per-entity load tracking 的问题是什么 ?
  - [Per-entity load tracking](https://lwn.net/Articles/531853/)

## 主要参考

- https://www.cnblogs.com/LoyenWang/p/12316660.html

## 基本流程

更新  update_load_avg 的位置非常多，具体可以参考: https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200216135939689-531768656.png
```txt
#0  ___update_load_avg (load=1024, sa=0xffff88814a11afc0) at kernel/sched/pelt.h:44
#1  __update_load_avg_se (now=now@entry=48878086392862, cfs_rq=cfs_rq@entry=0xffff888148fa5e00, se=se@entry=0xffff88814a11af00) at kernel/sched/build_policy.c:4239
#2  0xffffffff8114393c in update_load_avg (cfs_rq=0xffff888148fa5e00, se=0xffff88814a11af00, flags=5) at kernel/sched/fair.c:4018
#3  0xffffffff81144fc0 in enqueue_entity (cfs_rq=cfs_rq@entry=0xffff888148fa5e00, se=se@entry=0xffff88814a11af00, flags=9) at kernel/sched/fair.c:4442
#4  0xffffffff81145461 in enqueue_task_fair (rq=0xffff888333c2b2c0, p=<optimized out>, flags=<optimized out>) at kernel/sched/fair.c:5757
#5  0xffffffff8113c53f in enqueue_task (flags=9, p=0xffff88814a11ae80, rq=0xffff888333c2b2c0) at kernel/sched/core.c:2066
```

```txt
#0  update_tg_cfs_runnable (gcfs_rq=0xffff888145898c00, gcfs_rq=0xffff888145898c00, se=0xffff888145898a00, cfs_rq=0xffff888333c2b3c0) at kernel/sched/fair.c:3584
#1  propagate_entity_load_avg (se=0xffff888145898a00) at kernel/sched/fair.c:3700
#2  update_load_avg (cfs_rq=0xffff888333c2b3c0, se=0xffff888145898a00, flags=1) at kernel/sched/fair.c:4021
#3  0xffffffff81146254 in propagate_entity_cfs_rq (se=0xffff888145898a00, se=0xffff888145898a00) at kernel/sched/fair.c:11538
#4  propagate_entity_cfs_rq (se=<optimized out>) at kernel/sched/fair.c:11522
#5  0xffffffff8113edda in wake_up_new_task (p=p@entry=0xffff888100219740) at kernel/sched/core.c:4678
#6  0xffffffff811039b4 in kernel_clone (args=args@entry=0xffffc90000753eb0) at kernel/fork.c:2695
#7  0xffffffff81103de6 in __do_sys_clone (clone_flags=<optimized out>, newsp=<optimized out>, parent_tidptr=<optimized out>, child_tidptr=<optimized out>, tls=<optimized out>) at kernel/fork.c:2805
#8  0xffffffff81f3d6e8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000753f58) at arch/x86/entry/common.c:50
#9  do_syscall_64 (regs=0xffffc90000753f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#10 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

- update_load_avg
  - update_cfs_rq_load_avg
    - `__update_load_avg_cfs_rq`
      - `___update_load_sum`
  - propagate_entity_load_avg


## 记录

使用 struct sched_avg 来记录:
```c
/*
 * The load_avg/util_avg accumulates an infinite geometric series
 * (see __update_load_avg() in kernel/sched/fair.c).
 *
 * [load_avg definition]
 *
 *   load_avg = runnable% * scale_load_down(load)
 *
 * where runnable% is the time ratio that a sched_entity is runnable.
 * For cfs_rq, it is the aggregated load_avg of all runnable and
 * blocked sched_entities.
 *
 * load_avg may also take frequency scaling into account:
 *
 *   load_avg = runnable% * scale_load_down(load) * freq%
 *
 * where freq% is the CPU frequency normalized to the highest frequency.
 *
 * [util_avg definition]
 *
 *   util_avg = running% * SCHED_CAPACITY_SCALE
 *
 * where running% is the time ratio that a sched_entity is running on
 * a CPU. For cfs_rq, it is the aggregated util_avg of all runnable
 * and blocked sched_entities.
 *
 * util_avg may also factor frequency scaling and CPU capacity scaling:
 *
 *   util_avg = running% * SCHED_CAPACITY_SCALE * freq% * capacity%
 *
 * where freq% is the same as above, and capacity% is the CPU capacity
 * normalized to the greatest capacity (due to uarch differences, etc).
 *
 * N.B., the above ratios (runnable%, running%, freq%, and capacity%)
 * themselves are in the range of [0, 1]. To do fixed point arithmetics,
 * we therefore scale them to as large a range as necessary. This is for
 * example reflected by util_avg's SCHED_CAPACITY_SCALE.
 *
 * [Overflow issue]
 *
 * The 64-bit load_sum can have 4353082796 (=2^64/47742/88761) entities
 * with the highest load (=88761), always runnable on a single cfs_rq,
 * and should not overflow as the number already hits PID_MAX_LIMIT.
 *
 * For all other cases (including 32-bit kernels), struct load_weight's
 * weight will overflow first before we do, because:
 *
 *    Max(load_avg) <= Max(load.weight)
 *
 * Then it is the load_weight's responsibility to consider overflow
 * issues.
 */

struct sched_avg {
	u64				last_update_time;
	u64				load_sum;
	u64				runnable_load_sum;
	u32				util_sum;
	u32				period_contrib;
	unsigned long			load_avg;
	unsigned long			runnable_load_avg;
	unsigned long			util_avg;
	struct util_est			util_est;
} ____cacheline_aligned;
```

## 计算 /proc/loadavg 的方法

1. 两个函数来统计，分别处理 hz 和 nohz 的情况，使用 calc_load_fold_active
```txt
#0  calc_load_fold_active (adjust=0, this_rq=0xffff88807dc2b2c0) at kernel/sched/build_utility.c:3245
#1  calc_load_nohz_fold (rq=0xffff88807dc2b2c0) at kernel/sched/build_utility.c:3399
#2  calc_load_nohz_start () at kernel/sched/build_utility.c:3413
#3  0xffffffff811a7389 in tick_nohz_stop_tick (cpu=0, ts=0xffff88807dc1e5c0) at kernel/time/tick-sched.c:913
#4  __tick_nohz_idle_stop_tick (ts=0xffff88807dc1e5c0) at kernel/time/tick-sched.c:1108
#5  tick_nohz_idle_stop_tick () at kernel/time/tick-sched.c:1129
#6  0xffffffff8114b0e8 in cpuidle_idle_call () at kernel/sched/build_policy.c:231
#7  do_idle () at kernel/sched/build_policy.c:345
#8  0xffffffff8114b324 in cpu_startup_entry (state=state@entry=CPUHP_ONLINE) at kernel/sched/build_policy.c:442
#9  0xffffffff81f383ab in rest_init () at init/main.c:727
#10 0xffffffff8330dc05 in arch_call_rest_init () at init/main.c:883
#11 0xffffffff8330e27d in start_kernel () at init/main.c:1138
#12 0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#13 0x0000000000000000 in ?? ()
```

```txt
#0  calc_load_nohz_start () at kernel/sched/build_utility.c:3413
#1  0xffffffff811a7389 in tick_nohz_stop_tick (cpu=3, ts=0xffff88813dd1e5c0) at kernel/time/tick-sched.c:913
#2  __tick_nohz_idle_stop_tick (ts=0xffff88813dd1e5c0) at kernel/time/tick-sched.c:1108
#3  tick_nohz_idle_stop_tick () at kernel/time/tick-sched.c:1129
#4  0xffffffff8114b0e8 in cpuidle_idle_call () at kernel/sched/build_policy.c:231
#5  do_idle () at kernel/sched/build_policy.c:345
#6  0xffffffff8114b324 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at kernel/sched/build_policy.c:442
#7  0xffffffff810e0518 in start_secondary (unused=<optimized out>) at arch/x86/kernel/smpboot.c:262
#8  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#9  0x0000000000000000 in ?? ()
```

2. 进行统计
```txt
#0  calc_global_load () at kernel/sched/build_utility.c:3516
#1  0xffffffff811a6d0a in tick_do_update_jiffies64 (now=<optimized out>) at kernel/time/tick-sched.c:148
#2  tick_do_update_jiffies64 (now=<optimized out>) at kernel/time/tick-sched.c:57
#3  0xffffffff811a7897 in tick_nohz_restart_sched_tick (now=389458267506, ts=0xffff88813dd1e5c0) at kernel/time/tick-sched.c:962
#4  tick_nohz_idle_update_tick (now=389458267506, ts=0xffff88813dd1e5c0) at kernel/time/tick-sched.c:1315
#5  tick_nohz_idle_exit () at kernel/time/tick-sched.c:1349
#6  0xffffffff8114b044 in do_idle () at kernel/sched/build_policy.c:358
#7  0xffffffff8114b324 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at kernel/sched/build_policy.c:442
#8  0xffffffff810e0518 in start_secondary (unused=<optimized out>) at arch/x86/kernel/smpboot.c:262
#9  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#10 0x0000000000000000 in ?? ()
```

- calc_global_load 调用 calc_global_nohz，同时处理两种情况的数据。


```txt
0  0xffffffff8114d371 in accumulate_sum (running=<optimized out>, runnable=<optimized out>, load=<optimized out>, sa=<optimized out>, delta=<optimized out>) at kernel/sched/build_policy.c:4029
#1  ___update_load_sum (running=1, runnable=<optimized out>, load=1, sa=0xffff8881485f5b40, now=390223811783) at kernel/sched/build_policy.c:4150
#2  __update_load_avg_se (now=now@entry=390223811783, cfs_rq=cfs_rq@entry=0xffff88813dc2b340, se=se@entry=0xffff8881485f5a80) at kernel/sched/build_policy.c:4232
#3  0xffffffff811404d4 in update_load_avg (cfs_rq=0xffff88813dc2b340, se=0xffff8881485f5a80, flags=1) at kernel/sched/fair.c:4018
#4  0xffffffff81142322 in entity_tick (queued=0, curr=0xffff8881485f5a80, cfs_rq=0xffff88813dc2b340) at kernel/sched/fair.c:4740
#5  task_tick_fair (rq=0xffff88813dc2b2c0, curr=0xffff8881485f5a00, queued=0) at kernel/sched/fair.c:11416
#6  0xffffffff8113c7c9 in scheduler_tick () at kernel/sched/core.c:5453
#7  0xffffffff811946d1 in update_process_times (user_tick=0) at kernel/time/timer.c:1844
#8  0xffffffff811a6c7f in tick_sched_handle (ts=ts@entry=0xffff88813dc1e5c0, regs=regs@entry=0xffffc900014e3b28) at kernel/time/tick-sched.c:243
#9  0xffffffff811a6e5c in tick_sched_timer (timer=0xffff88813dc1e5c0) at kernel/time/tick-sched.c:1480
#10 0xffffffff81195205 in __run_hrtimer (flags=2, now=0xffffc9000012cf48, timer=0xffff88813dc1e5c0, base=0xffff88813dc1e0c0, cpu_base=0xffff88813dc1e080) at kernel/time/hrtimer.c:1685
#11 __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff88813dc1e080, now=389462153629, flags=flags@entry=2, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#12 0xffffffff81195e91 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#13 0xffffffff810e256a in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#14 __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#15 0xffffffff81f371fd in sysvec_apic_timer_interrupt (regs=0xffffc900014e3b28) at arch/x86/kernel/apic/apic.c:1106
```


[Load tracking in the scheduler](https://lwn.net/Articles/639543/)

The load of a CPU could have simply been the sum of the load of all the scheduling entities running on its run queue.
In fact, that was once all there was to it.
This approach has a disadvantage, though, in that tasks are associated with load values based only on their priorities.
This approach does not take into account the nature of a task, such as whether it is a bursty or a steady task, or whether it is a CPU-intensive or an I/O-bound task.
*While this does not matter for scheduling within a CPU, it does matter when load balancing across CPUs because it helps estimate the CPU load more accurately.*


Therefore the per-entity load tracking metric was introduced to estimate the nature of a task numerically.
**This metric calculates task load as the amount of time that the task was runnable during the time that it was alive.**
This is kept track of in the `sched_avg` data structure (stored in the `sched_entity` structure):

Given a task p, if the `sched_entity` associated with it is se and the `sched_avg` of se is sa, then:
```plain
sa.load_avg_contrib = (sa.runnable_sum * se.load.weight) / sa.runnable_period;
```
where `runnable_sum` is the amount of time that the task was runnable, `runnable_period` is the period during which the task could have been runnable.

The load on a CPU is the sum of the `load_avg_contrib` of all the scheduling entities on its run queue;
it is accumulated in a field called `runnable_load_avg` in the `cfs_rq` data structure.
This is roughly a measure of how heavily contended the CPU is. The kernel also tracks the load associated with blocked tasks. When a task gets blocked, its load is accumulated in the blocked_load_avg metric of the cfs_rq structure.

- [ ] [Per-entity load tracking in presence of task groups](https://lwn.net/Articles/639543/) : Continue the reading if other parts finished.
