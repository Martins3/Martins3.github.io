- loyenwang
- wowo
- 奔跑
# linux kernel scheduler
1. cpu domain
2. nohz

4. variables
  1. weight
  2. nice
  3. priority
  4. share : group
  4. util
  5. load

5. load 到底被多少人使用过
  1. task
  2. rq
  3. cfs_rq
  4. task_group(cgroup)
  5. sched_group
  6. sched_domain

5. bandwidth

build_sched_group

cpu_attach_domain


- [ ] so many update load avg:
  - update_tg_load_avg
  - update_load_avg
  - rebalance_domains : 处理 idle , balance 的时机
    - load_balance

- [ ] sched_group_span
- [ ] load_balance_mask
```c
	struct cpumask *cpus = this_cpu_cpumask_var_ptr(load_balance_mask);
```

- [ ] 计算 sched_group 的时候的 busy 程度的时候，是使用什么来分析的。

- [ ] 存在多个调度类还是可以理解的，优先级高的队列首先清空，但是各种调度策略的影响如何在代码中间具体体现。
```c
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
```


## smp balance

```c
__init void init_sched_fair_class(void)
{
#ifdef CONFIG_SMP
	open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);

#ifdef CONFIG_NO_HZ_COMMON
	nohz.next_balance = jiffies;
	nohz.next_blocked = jiffies;
	zalloc_cpumask_var(&nohz.idle_cpus_mask, GFP_NOWAIT);
#endif
#endif /* SMP */

}
```
- [ ] 不知道 softirq 的实现，为什么不是直接调用，而是通过一次 softirq 的封装

```c
enum migration_type {
	migrate_load = 0,
	migrate_util,
	migrate_task,
	migrate_misfit
};

/*
 * 'group_type' describes the group of CPUs at the moment of load balancing.
 *
 * The enum is ordered by pulling priority, with the group with lowest priority
 * first so the group_type can simply be compared when selecting the busiest
 * group. See update_sd_pick_busiest().
 */
enum group_type {
	/* The group has spare capacity that can be used to run more tasks.  */
	group_has_spare = 0,
	/*
	 * The group is fully used and the tasks don't compete for more CPU
	 * cycles. Nevertheless, some tasks might wait before running.
	 */
	group_fully_busy,
	/*
	 * SD_ASYM_CPUCAPACITY only: One task doesn't fit with CPU's capacity
	 * and must be migrated to a more powerful CPU.
	 */
	group_misfit_task,
	/*
	 * SD_ASYM_PACKING only: One local CPU with higher capacity is available,
	 * and the task should be migrated to it instead of running on the
	 * current CPU.
	 */
	group_asym_packing,
	/*
	 * The tasks' affinity constraints previously prevented the scheduler
	 * from balancing the load across the system.
	 */
	group_imbalanced,
	/*
	 * The CPU is overloaded and can't provide expected CPU cycles to all
	 * tasks.
	 */
	group_overloaded
};
```

- [ ] migration 和 group_type 的关系是什么 ?


- run_rebalance_domains
  - * trigger_load_balance
  - * nohz_csd_func
  - nohz_csd_func
  - update_blocked_averages
  - rebalance_domains
    - should_we_balance
    - find_busiest_group
      - update_sd_lb_stats
        - update_group_capacity
        - update_sg_lb_stats
        - update_sd_pick_busiest
      - calculate_imbalance : 计算需要迁移多少负载量才能达到均衡
    - find_busiest_queue
    - stop_one_cpu_nowait
    - detach_tasks : 注意，我们只是迁移没有运行的 cpu
      - detach_task
        - deactivate_task
        - set_task_cpu


- [ ] there are **two** cpumask
  - 应该是一个描述这个 sched_group 包含那些 cpu，一个描述那些做 load balance 的 cpu
  - struct sched_group_capacity::cpumask
  - struct sched_group::cpumask

- select_task_rq
  - * sched_exec
  - * select_task_rq
    - * wake_up_new_task 
    - * try_to_wake_up
  - select_task_rq_fair
    - wake_affine
      - wake_affine_idle
      - wake_affine_weight
    - select_idle_sibling : 快速路径，首先在附近进行查找
      - available_idle_cpu
      - sched_idle_cpu
      - select_idle_core
      - select_idle_cpu
      - select_idle_smt
    - find_idlest_cpu : 慢速路径，是在不行就在全局查找
      - find_idlest_group
      - find_idlest_group_cpu

- [ ] SD_WAKE_AFFINE 标志位 : 表示运行唤醒进程的CPU可以运行这个被唤醒的进程。
  
## load
```c
static unsigned long cpu_load(struct rq *rq)
{
	return cfs_rq_load_avg(&rq->cfs);
}
```

