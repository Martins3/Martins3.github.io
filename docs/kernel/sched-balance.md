# balance

## `select_idle_*` 的作用是什么

* ***select_idle_sibling***

```c
/*
 * Try and locate an idle core/thread in the LLC cache domain.
 */
static int select_idle_sibling(struct task_struct *p, int prev, int target)
```
调用位置

```c
/*
 * select_task_rq_fair: Select target runqueue for the waking task in domains
 * that have the 'sd_flag' flag set. In practice, this is SD_BALANCE_WAKE,
 * SD_BALANCE_FORK, or SD_BALANCE_EXEC.
 *
 * Balances load by selecting the idlest CPU in the idlest group, or under
 * certain conditions an idle sibling CPU if the domain has SD_WAKE_AFFINE set.
 *
 * Returns the target CPU number.
 *
 * preempt must be disabled.
 */
static int
select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag, int wake_flags)
```

2. task_numa_compare
    - task_numa_find_cpu
        - task_numa_migrate
            - numa_migrate_perfer
              - task_numa_fault : 这个函数调用位置来自于 memory.c huge_memory.c 中的 do_numa_page 之类的函数

select_idle_sibling 中间的部分片段:

```c
	i = select_idle_core(p, sd, target);
	if ((unsigned)i < nr_cpumask_bits)
		return i;

	i = select_idle_cpu(p, sd, target);
	if ((unsigned)i < nr_cpumask_bits)
		return i;

	i = select_idle_smt(p, sd, target);
	if ((unsigned)i < nr_cpumask_bits)
		return i;
```

## lb 的关键内容 : find_busiest_group 和 find_busiest_queue
被 load_balance 唯一调用

```c
/*
 * find_busiest_queue - find the busiest runqueue among the CPUs in the group.
 */
static struct rq *find_busiest_queue(struct lb_env *env,
				     struct sched_group *group)
```

在 group 中间的找到 cfs_rq，因为迁移都是在文件夹中间进行迁移的。

task_group 是描述，

```c
/**
 * find_busiest_group - Returns the busiest group within the sched_domain
 * if there is an imbalance.
 *
 * Also calculates the amount of weighted load which should be moved
 * to restore balance.
 *
 * @env: The load balancing environment.
 *
 * Return:	- The busiest group if imbalance exists.
 */
static struct sched_group *find_busiest_group(struct lb_env *env)
```

find_busiest_group 的 helper 函数 7000 ~ 8200 。

find_idlest_cpu 和 find_idlest_group : select_task_rq_fair 相关的
@todo 为什么 lb 只需要 busiest 而不需要 idlest 的


## load_balance 的实现
kernel/sched/fair.c:6798 的注释

> 了解一下其中的函数

## 触发 rebalance 的方法和位置是什么
1. domain 的概念是什么 ?

```txt
idle_balance : schedule() 调用，应该是最容易分析了
rebalance_domains:
  load_balance : 核心业务 ?

_nohz_idle_balance()
run_rebalance_domains : 被注册到 softirq 中间了
  rebalance_domains

nohz_idle_balance(): 被 run_rebalance_domains 唯一调用
nohz_newidle_balance(): 被 idle_balance 唯一调用
  __nohz_idle_balance():


scheduler_tick()
  tigger_load_balance
    nohz_balance_kick() : 值的关注一下，好像和之前的所有的东西都不是一个东西呀!
```

> 好像就是 softirq 触发的，然后进行整个流程走一下
> 以及从 timer 中间触发!

# 首先，理解了
- https://docs.kernel.org/admin-guide/pm/intel-speed-select.html

- 打印出来当前正在 CPU 上运行的:
```sh
ps -e -o pid,ppid,sgi_p,state,args | awk '$3!="*" {print}'
```

- Scheduler Domain 从 acpi 中获取吗?


- (CONFIG_ENERGY_MODEL) && (CONFIG_CPU_FREQ_GOV_SCHEDUTIL) : 做什么的 ?
- get_group 和 build_balance_mask 上有非常多暂时看不懂的注释

# topology
- [ ] build_balance_mask
  - span
  - sched_domain_topology_level
- sched_group
- root_domain
  - init_rootdomain

- cpu_attach_domain

- degenerate

- rq_attach_root

## 如何初始化

## 什么时候切换

## 大核小核的影响

## 如何实现 taskset 的效果

## 关键结构体

### root_domain

```c
/*
 * We add the notion of a root-domain which will be used to define per-domain
 * variables. Each exclusive cpuset essentially defines an island domain by
 * fully partitioning the member CPUs from any other cpuset. Whenever a new
 * exclusive cpuset is created, we also create and attach a new root-domain
 * object.
 *
 */
struct root_domain {
	atomic_t		refcount;
	atomic_t		rto_count;
	struct rcu_head		rcu;
	cpumask_var_t		span;
	cpumask_var_t		online;

	/*
	 * Indicate pullable load on at least one CPU, e.g:
	 * - More than one runnable task
	 * - Running task is misfit
	 */
	int			overload;

	/* Indicate one or more cpus over-utilized (tipping point) */
	int			overutilized;

	/*
	 * The bit corresponding to a CPU gets set here if such CPU has more
	 * than one runnable -deadline task (as it is below for RT tasks).
	 */
	cpumask_var_t		dlo_mask;
	atomic_t		dlo_count;
	struct dl_bw		dl_bw;
	struct cpudl		cpudl;

	/*
	 * Indicate whether a root_domain's dl_bw has been checked or
	 * updated. It's monotonously increasing value.
	 *
	 * Also, some corner cases, like 'wrap around' is dangerous, but given
	 * that u64 is 'big enough'. So that shouldn't be a concern.
	 */
	u64 visit_gen;

#ifdef HAVE_RT_PUSH_IPI
	/*
	 * For IPI pull requests, loop across the rto_mask.
	 */
	struct irq_work		rto_push_work;
	raw_spinlock_t		rto_lock;
	/* These are only updated and read within rto_lock */
	int			rto_loop;
	int			rto_cpu;
	/* These atomics are updated outside of a lock */
	atomic_t		rto_loop_next;
	atomic_t		rto_loop_start;
#endif
	/*
	 * The "RT overload" flag: it gets set if a CPU has more than
	 * one runnable RT task.
	 */
	cpumask_var_t		rto_mask;
	struct cpupri		cpupri;

	unsigned long		max_cpu_capacity;

	/*
	 * NULL-terminated list of performance domains intersecting with the
	 * CPUs of the rd. Protected by RCU.
	 */
	struct perf_domain __rcu *pd;
};
```


### sched_group
```c
struct sched_group {
	struct sched_group	*next;			/* Must be a circular list */
	atomic_t		ref;

	unsigned int		group_weight;
	struct sched_group_capacity *sgc;
	int			asym_prefer_cpu;	/* CPU of highest priority in group */
	int			flags;

	/*
	 * The CPUs this group covers.
	 *
	 * NOTE: this field is variable length. (Allocated dynamically
	 * by attaching extra space to the end of the structure,
	 * depending on how many CPUs the kernel has booted up with)
	 */
	unsigned long		cpumask[];
};
```

### sched_domain
```c
struct sched_domain {
	/* These fields must be setup */
	struct sched_domain __rcu *parent;	/* top domain must be null terminated */
	struct sched_domain __rcu *child;	/* bottom domain must be null terminated */
	struct sched_group *groups;	/* the balancing groups of the domain */
	unsigned long min_interval;	/* Minimum balance interval ms */
	unsigned long max_interval;	/* Maximum balance interval ms */
	unsigned int busy_factor;	/* less balancing by factor if busy */
	unsigned int imbalance_pct;	/* No balance until over watermark */
	unsigned int cache_nice_tries;	/* Leave cache hot tasks for # tries */
	unsigned int imb_numa_nr;	/* Nr running tasks that allows a NUMA imbalance */

	int nohz_idle;			/* NOHZ IDLE status */
	int flags;			/* See SD_* */
	int level;

	/* Runtime fields. */
	unsigned long last_balance;	/* init to jiffies. units in jiffies */
	unsigned int balance_interval;	/* initialise to 1. units in ms. */
	unsigned int nr_balance_failed; /* initialise to 0 */

	/* idle_balance() stats */
	u64 max_newidle_lb_cost;
	unsigned long last_decay_max_lb_cost;

	u64 avg_scan_cost;		/* select_idle_sibling */

#ifdef CONFIG_SCHEDSTATS
	/* load_balance() stats */
	unsigned int lb_count[CPU_MAX_IDLE_TYPES];
	unsigned int lb_failed[CPU_MAX_IDLE_TYPES];
	unsigned int lb_balanced[CPU_MAX_IDLE_TYPES];
	unsigned int lb_imbalance[CPU_MAX_IDLE_TYPES];
	unsigned int lb_gained[CPU_MAX_IDLE_TYPES];
	unsigned int lb_hot_gained[CPU_MAX_IDLE_TYPES];
	unsigned int lb_nobusyg[CPU_MAX_IDLE_TYPES];
	unsigned int lb_nobusyq[CPU_MAX_IDLE_TYPES];

	/* Active load balancing */
	unsigned int alb_count;
	unsigned int alb_failed;
	unsigned int alb_pushed;

	/* SD_BALANCE_EXEC stats */
	unsigned int sbe_count;
	unsigned int sbe_balanced;
	unsigned int sbe_pushed;

	/* SD_BALANCE_FORK stats */
	unsigned int sbf_count;
	unsigned int sbf_balanced;
	unsigned int sbf_pushed;

	/* try_to_wake_up() stats */
	unsigned int ttwu_wake_remote;
	unsigned int ttwu_move_affine;
	unsigned int ttwu_move_balance;
#endif
#ifdef CONFIG_SCHED_DEBUG
	char *name;
#endif
	union {
		void *private;		/* used during construction */
		struct rcu_head rcu;	/* used during destruction */
	};
	struct sched_domain_shared *shared;

	unsigned int span_weight;
	/*
	 * Span of all CPUs in this domain.
	 *
	 * NOTE: this field is variable length. (Allocated dynamically
	 * by attaching extra space to the end of the structure,
	 * depending on how many CPUs the kernel has booted up with)
	 */
	unsigned long span[];
};
```

# cpumask
- start_kernel
  - setup_arch
      - smp_init_cpus :
        - of_parse_and_init_cpus
        - acpi_parse_and_init_cpus
        - smp_cpu_setup
          - set_cpu_possible
            - **cpumask_set_cpu(cpu, &__cpu_possible_mask);**
  - arch_call_rest_init
    - rest_init
      - kernel_init
        - kernel_init_freeable
          - smp_prepare_cpus
            - set_cpu_present : 如果这只是将 possible 拷贝到 present，其意义何在 ?
              - **cpumask_set_cpu(cpu, &__cpu_present_mask);**
          - smp_init
            - bringup_nonboot_cpus
              - cpu_up : 实际上，跟丢了

SMT : L1 高速共享
MC  : 共享 LLC
SOC : DIE

```plain
config SCHED_SMT
    bool "SMT (Hyperthreading) scheduler support"
    depends on SPARC64 && SMP
    default y
    help
      SMT scheduler support improves the CPU scheduler's decision making
      when dealing with SPARC cpus at a cost of slightly increased overhead
      in some places. If unsure say N here.

config SCHED_MC
    bool "Multi-core scheduler support"
    depends on SPARC64 && SMP
    default y
    help
      Multi-core scheduler support improves the CPU scheduler's decision
      making when dealing with multi-core CPU chips at a cost of slightly
      increased overhead in some places. If unsure say N here.
```

```c
/*
 * Topology list, bottom-up.
 */
static struct sched_domain_topology_level default_topology[] = {
#ifdef CONFIG_SCHED_SMT
    { cpu_smt_mask, cpu_smt_flags, SD_INIT_NAME(SMT) },
#endif
#ifdef CONFIG_SCHED_MC
    { cpu_coregroup_mask, cpu_core_flags, SD_INIT_NAME(MC) },
#endif
    { cpu_cpu_mask, SD_INIT_NAME(DIE) },
    { NULL, },
};

typedef const struct cpumask *(*sched_domain_mask_f)(int cpu);
typedef int (*sched_domain_flags_f)(void);

struct sched_domain_topology_level {
    sched_domain_mask_f mask;      // 返回某个 cpu 在该 topology level 下的 CPU 的兄弟 cpu 的 mask
    sched_domain_flags_f sd_flags; // 用于返回 domain 的属性
    int         flags;
    int         numa_level;
    struct sd_data      data;
};

struct sd_data {
    struct sched_domain *__percpu *sd; // 优秀啊，每一个 cpu 都保存一份所有人的 sched_domain
    struct sched_domain_shared *__percpu *sds;
    struct sched_group *__percpu *sg;
    struct sched_group_capacity *__percpu *sgc;
};
```

- 在 sched_domain 被划分为 sched_group, sched_group 是调度最小单位。
  - [ ] 感觉这么定义的话，岂不是下一级的 sched_domain 就上级的 sched_group
- sched_domain_span 表示 cpu 当前的 domain 管辖的 cpu 范围


- sched_init_domains
  - build_sched_domains
  - `__visit_domain_allocation_hell`
    - `__sdt_alloc`
    - alloc_rootdomain
  - build_sched_domain
    - sd_init
  - build_sched_groups

- 构建 domain 的结果是，在每一个 topology level 中间都存在 NR_cpu 个 sched_domain
  - 这个 sched_domain 包含一定数量的 cpu
  - sched_domain 指向一个链表的 sched_group

## CONFIG_NUMA_BALANCING

This option adds support for automatic NUMA aware memory/task placement.
The mechanism is quite primitive and is based on migrating memory when it has references to the node the task is running on.

This system will be inactive on UMA systems.

> 似乎是为了处理当其中的 NUMA 远程访问，然后直接将 CPU 迁移过去的操作。


```c
// 请问 numa
void init_numa_balancing(unsigned long clone_flags, struct task_struct *p)

struct numa_group{
```

## TODO
- [ ] cpu_attach_domain
- [ ] sched_group_span
- [ ] load_balance_mask
```c
    struct cpumask *cpus = this_cpu_cpumask_var_ptr(load_balance_mask);
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

- [ ] SD_WAKE_AFFINE 标志位 : 表示运行唤醒进程的 CPU 可以运行这个被唤醒的进程。
