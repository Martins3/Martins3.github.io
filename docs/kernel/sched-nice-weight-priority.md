# 区分 nice weight priority



## nice
使用 nice 命令设置，`[-20 19]` 默认是 0

sched_setattr

## priority
通过 macro 转换
```c
/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
#define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)
```
将范围从 `[-20, 19]` 装换为 `[120, 139]`



struct task_struct 一共存在四个 prio
```c
	int				prio;
	int				static_prio;
	int				normal_prio;
	unsigned int			rt_priority;
```
- prio : 提供给调度器的接口 `[0, 139]`，通过函数 effective_prio 计算得到，其中 rt `[0, 99]`，normal 是 `[100, 139]`
- static_prio : set_user_nice， sched_fork 以及 nice 等用户接口设置，范围是 `[100, 139]`, 计算方法 NICE_TO_PRIO(nice)
- normal_prio : 通过 normal_prio 计算得到，取决于 policy，范围不同。
- rt_priority : rt 专用

好吧，这几个程序的作用还是非常迷惑，例如
- set_load_weight 中还是使用 static_prio 来计算
- 感觉所有的数值都是可以根据 sched policy 和 nice 计算出来，为什么搞额外的三个

问题:
1. 通过 sched_get_priority_min 和 sched_get_priority_max 的范围是
  - SCHED_FIFO 和 SCHED_RR 是 `[1, 99]`
  - SCHED_DEADLINE SCHED_NORMAL SCHED_BATCH SCHED_IDLE 是 `[0, 0]`
2. 可以看懂 task_prio 上的注释吗?

## weight

- set_load_weight 中设置

## share
group sched

- update_cfs_group
  - calc_group_shares

- sched_group_set_shares

## 更多的计算
```c
int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)

	tg->shares = NICE_0_LOAD;
```

1. tg->shares 相关的计算
    1. 赋值永远都是 NICE_0_LOAD

```c

/*
 * Increase resolution of nice-level calculations for 64-bit architectures.
 * The extra resolution improves shares distribution and load balancing of
 * low-weight task groups (eg. nice +19 on an autogroup), deeper taskgroup
 * hierarchies, especially on larger systems. This is not a user-visible change
 * and does not change the user-interface for setting shares/weights.
 *
 * We increase resolution only if we have enough bits to allow this increased
 * resolution (i.e. 64-bit). The costs for increasing resolution when 32-bit
 * are pretty high and the returns do not justify the increased costs.
 *
 * Really only required when CONFIG_FAIR_GROUP_SCHED=y is also set, but to
 * increase coverage and consistency always enable it on 64-bit platforms.
 */
#ifdef CONFIG_64BIT
# define NICE_0_LOAD_SHIFT	(SCHED_FIXEDPOINT_SHIFT + SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)		((w) << SCHED_FIXEDPOINT_SHIFT)
# define scale_load_down(w)	((w) >> SCHED_FIXEDPOINT_SHIFT)
#else
# define NICE_0_LOAD_SHIFT	(SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)		(w)
# define scale_load_down(w)	(w)
#endif

/*
 * Task weight (visible to users) and its load (invisible to users) have
 * independent resolution, but they should be well calibrated. We use
 * scale_load() and scale_load_down(w) to convert between them. The
 * following must be true:
 *
 *  scale_load(sched_prio_to_weight[USER_PRIO(NICE_TO_PRIO(0))]) == NICE_0_LOAD
 *
 */
#define NICE_0_LOAD		(1L << NICE_0_LOAD_SHIFT)


/*
 * Integer metrics need fixed point arithmetic, e.g., sched/fair
 * has a few: load, load_avg, util_avg, freq, and capacity.
 *
 * We define a basic fixed point arithmetic range, and then formalize
 * all these metrics based on that basic range.
 */
# define SCHED_FIXEDPOINT_SHIFT		10
# define SCHED_FIXEDPOINT_SCALE		(1L << SCHED_FIXEDPOINT_SHIFT)
```
> @todo 还有 freq capacity load_avg 等

A priority number of 120, which is the priority of a normal task, is mapped to a load of 1024, which is the value that the kernel uses to represent the capacity of a single standard CPU.
> @todo 为什么会映射到 1024 上，利用 prio_to_weight 吗 ?


```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight; // 难道 bandwidth 使用的 ?
	struct rb_node			run_node;
	struct list_head		group_node; // task group ?
	unsigned int			on_rq; // why not boolean ?

  // @todo how runtime works ?
	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	u64				nr_migrations;
```
> 1. load 和 runnable_weight 之间的关系是什么 ?



参考:
1. https://blog.shichao.io/2015/07/22/relationships_among_nice_priority_and_weight_in_linux_kernel.html
2. https://stackoverflow.com/questions/5770770/prio-static-prio-rt-priority-in-linux-kernel
