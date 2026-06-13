# pelt
<!-- 7f5c5ed2-605b-4137-945f-9c4bd92fd580 -->

- `util_avg`: 最近真的跑在 CPU 上的强度
- `runnable_avg`: 最近处于 runnable 的强度
- `load_avg`: runnable 强度再乘以 weight

- on_rq 了，才算 runnable
- 真正成为 cfs_rq->curr 了，才算 running

- load_avg = runnable% * weight
- runnable_avg = runnable%
- util_avg = running%

- load_avg：把“可运行时间比例”乘上 task weight。它既看你活跃不活跃，也看你这个任务在 CFS 里有多重。
- runnable_avg：只看“最近多大比例时间处于 runnable”。等待 CPU 也算。
- util_avg：只看“最近多大比例时间真的在 CPU 上跑”。

## 关键参考

- https://www.cnblogs.com/LoyenWang/p/12316660.html
- https://hackmd.io/@RinHizakura/Bk4y_5o-9
- http://www.wowotech.net/process_management/450.html

- [ ] [Per-entity load tracking in presence of task groups](https://lwn.net/Articles/639543/)
- [Load tracking in the scheduler](https://lwn.net/Articles/639543/)
- [Per-entity load tracking](https://lwn.net/Articles/531853/)
- [ ]  http://www.wowotech.net/process_management/pelt.html

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

## 消费者
- load balance
- task placement
- EAS
- schedutil cpufreq

> [!NOTE]
> 参考神奇海螺的意见，有待验证

7. 你最容易卡住的一点：PELT 不是直接用真实时间，而是用 rq_clock_pelt
- 如果 CPU 容量低，或者当前频率低
- 那同样一段 wall-clock time，不应该被当成“做了同样多的工作”
- 所以 update_rq_clock_pelt() 会先按 capacity 和 freq 缩放时间，再推进 PELT 时钟

## 基本流程

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt

  pelt 它会在很多调度事件里被调用，典型包括：

  - task 入队时
    见 enqueue_entity() 路径
  - task 出队时
  - task tick 时
  - task migrate 时
  - blocked load 周期更新时

  另外，阻塞任务即使不在 rq 上，也会在 kernel/sched/pelt.c:296 这类路径里继续衰减。

  所以 PELT 的“统计时间点”更准确地说是：

  - 调度事件发生时更新
  - 按当前时间与上次更新时间的差值补账
  - 不是“定时器扫一遍所有 task”

  3. /proc/loadavg 是如何统计的

  /proc/loadavg 是传统系统 load average，不是 PELT。

  它统计的是全局的：

  nr_running + nr_uninterruptible

  源码在 kernel/sched/loadavg.c:12：

  - global load average 是 nr_running + nr_uninterruptible 的指数衰减平均

  结果存在全局变量：

  - kernel/sched/loadavg.c:60

  而 /proc/loadavg 只是把它打印出来，见 fs/proc/loadavg.c:14。

  它的采集原始输入
  每个 CPU 上，先折算当前 active task 数，函数是 kernel/sched/loadavg.c:80：

  nr_active = this_rq->nr_running - adjust;
  nr_active += this_rq->nr_uninterruptible;
```


更新  update_load_avg 的位置非常多，具体可以参考: https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200216135939689-531768656.png

- enqueue_task
  - enqueue_task_fair
    - enqueue_entity
      - update_load_avg
        - __update_load_avg_se
          - ___update_load_avg

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_clone
        - kernel_clone
          - wake_up_new_task
            - propagate_entity_cfs_rq
              - propagate_entity_cfs_rq
                - update_load_avg
                  - propagate_entity_load_avg
                    - update_tg_cfs_runnable

- update_load_avg
  - update_cfs_rq_load_avg
    - `__update_load_avg_cfs_rq`
      - `___update_load_sum`
  - propagate_entity_load_avg


- sysvec_apic_timer_interrupt
  - __sysvec_apic_timer_interrupt
    - local_apic_timer_interrupt
      - hrtimer_interrupt
        - __hrtimer_run_queues
          - __run_hrtimer
            - tick_sched_timer
              - tick_sched_handle
                - update_process_times
                  - scheduler_tick
                    - task_tick_fair
                      - entity_tick
                        - update_load_avg
                          - __update_load_avg_se
                            - ___update_load_sum
                              - accumulate_sum

## pelt 关键结构体
<!-- 60ae15b3-195e-49a4-a828-be4659a2162d -->

使用 struct sched_avg 来记录:
```c
/*
 * The load/runnable/util_avg accumulates an infinite geometric series
 * (see __update_load_avg_cfs_rq() in kernel/sched/pelt.c).
 *
 * [load_avg definition]
 *
 *   load_avg = runnable% * scale_load_down(load)
 *
 * [runnable_avg definition]
 *
 *   runnable_avg = runnable% * SCHED_CAPACITY_SCALE
 *
 * [util_avg definition]
 *
 *   util_avg = running% * SCHED_CAPACITY_SCALE
 *
 * where runnable% is the time ratio that a sched_entity is runnable and
 * running% the time ratio that a sched_entity is running.
 *
 * For cfs_rq, they are the aggregated values of all runnable and blocked
 * sched_entities.
 *
 * The load/runnable/util_avg doesn't directly factor frequency scaling and CPU
 * capacity scaling. The scaling is done through the rq_clock_pelt that is used
 * for computing those signals (see update_rq_clock_pelt())
 *
 * N.B., the above ratios (runnable% and running%) themselves are in the
 * range of [0, 1]. To do fixed point arithmetics, we therefore scale them
 * to as large a range as necessary. This is for example reflected by
 * util_avg's SCHED_CAPACITY_SCALE.
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
skjtruct sched_avg {
	u64				last_update_time;
	u64				load_sum;
	u64				runnable_sum;
	u32				util_sum;
	u32				period_contrib;
	unsigned long			load_avg;
	unsigned long			runnable_avg;
	unsigned long			util_avg;
	unsigned int			util_est;
} ____cacheline_aligned;
```

  - 每个 sched_entity 都有一个 struct sched_avg avg
  - 每个 cfs_rq 也有一个 struct sched_avg avg
  - 每个 rq 也有几份 struct sched_avg，但不是一份统一的 avg，而是 avg_rt / avg_dl / avg_irq / avg_hw
  - 不是所有调度类实体都有 sched_avg
      - sched_entity 有
      - sched_rt_entity 没有单独的 sched_avg
      - RT/DL/IRQ/HW 的 PELT 主要挂在 rq 上

  可以按三层看。

  1. task 这一层：task_struct 通过 sched_entity 间接带一个 sched_avg

  而普通 CFS 任务的 task_struct 里本身有一个 se，也就是 struct sched_entity se;。
  所以对普通任务来说，链路是：

  task_struct
    -> se   (struct sched_entity)
      -> avg (struct sched_avg)

  所以你在 /proc/<tid>/sched 里看到的 se.avg.xxx，本质上就是这个。

  2. runqueue 的 CFS 层：每个 cfs_rq 也有一份 sched_avg
  所以 task 的 se.avg 会汇总进 cfs_rq->avg。

  3. CPU runqueue 顶层：rq 上还有 RT/DL/IRQ/HW 的 sched_avg

  在 kernel/sched/sched.h 中可以看到
```c
  /*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct rq {
	// ...
	struct sched_avg	avg_rt;
	struct sched_avg	avg_dl;
#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
	struct sched_avg	avg_irq;
#endif
#ifdef CONFIG_SCHED_HW_PRESSURE
	struct sched_avg	avg_hw;
#endif

```

- CFS 的 PELT 聚合值放在 rq->cfs.avg
- RT 的聚合信号放在 rq->avg_rt
- DL 的聚合信号放在 rq->avg_dl
- IRQ / HW pressure 也各有一份

所以 每个 rq 确实有多份 sched_avg，但用途不同。

4. 不是“每个调度实体”都有

RT 类不是给每个 RT entity 都单独维护一套 task 级 PELT 平均值，而是更多在 rq->avg_rt 这一层做聚合跟踪。
- RT/DL 的 load_avg/runnable_avg 对单独 entity 没有同样的意义
- 主要跟踪的是 rq 层面的 util 信号

## 计算 /proc/loadavg 的方法

- /proc/loadavg：传统系统 load average，面向用户态展示
- PELT：scheduler 内部 per-entity / per-rq 信号

1. 两个函数来统计，分别处理 hz 和 nohz 的情况，使用 calc_load_fold_active

  - secondary_startup_64
    - start_kernel
      - arch_call_rest_init
        - rest_init
          - cpu_startup_entry
            - do_idle
              - cpuidle_idle_call
                - tick_nohz_idle_stop_tick
                  - __tick_nohz_idle_stop_tick
                    - tick_nohz_stop_tick
                      - calc_load_nohz_start
                        - calc_load_nohz_fold
                          - calc_load_fold_active

  - secondary_startup_64
    - start_secondary
      - cpu_startup_entry
        - do_idle
          - cpuidle_idle_call
            - tick_nohz_idle_stop_tick
              - __tick_nohz_idle_stop_tick
                - tick_nohz_stop_tick
                  - calc_load_nohz_start

2. 进行统计

  - secondary_startup_64
    - start_secondary
      - cpu_startup_entry
        - do_idle
          - tick_nohz_idle_exit
            - tick_nohz_idle_update_tick
              - tick_nohz_restart_sched_tick
                - tick_do_update_jiffies64
                  - tick_do_update_jiffies64
                    - calc_global_load

- calc_global_load 调用 calc_global_nohz，同时处理两种情况的数据。

读读这个文档: https://tanelpoder.com/posts/high-system-load-low-cpu-utilization-on-linux/

## 那么这个如何理解?
> task_group.load_avg : 整个用户组的负载贡献总和。

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
