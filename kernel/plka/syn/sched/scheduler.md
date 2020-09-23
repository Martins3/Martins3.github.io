# scheduler

## TODO
1. 阅读完成之前的所有的文章(整理) => 尽量搞清楚其中的问题
2. 重新阅读一遍发
3. 网课
4. 阅读fair.c 和 core.c 中间的函数，大致确定函数的返回
5. vruntime min_vruntime 和 period 的分析
6. advance 部分分析一下

## 问题

#### 核心问题
1. 实现进程跨CPU之间调度的方法: 
2. 不同粒度的实现 getpriority 中的
3. yield 和抢断等似乎比想象的更加复杂啊!
4. priority 到底如何计算得到 load weight 的 ?
5. sleeper 的 vruntime 是什么东西，为什么vruntime 需要单独分析啊 ?
6. period : check_preempt_tick 中间需要保证最少执行 4ms
7. group 机制 for_each_sched_entity 



#### 细节问题
1. `rq->min_vruntime` 的计算 似乎和 `en->load_weight` `sum_exec_runtime` `curr->vruntime`
2. `rq->vruntime` 的作用是什么 ? 如何处理数值溢出?
3. sched_slice 相关的内容
4. 了解一下这一个函数
```c
/*
 * resched_curr - mark rq's current task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a cross-CPU call to trigger the scheduler on
 * the target CPU.
 */
void resched_curr(struct rq *rq)
```


## 统计一些数据
**Man sched_setattr(2)** 
> 对于这些疑惑的最佳总结


| Category            | function                                      | 问题                                                                                                                                               |
|---------------------|-----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| rq                  | 每一个CPU持有的队列，实际上是一个RB tree 中间 | 1. CPU 不只是持有一个rq                                                                                                                            |
| sched_entity        | 放到 task_struct 中间，用于在rq 沟通的        | 1. sched_entity 为什么有那么多                                                                                                                     |
| Scheduling policies |                                               |                                                                                                                                                    |
| Scheduling class    | 实现调度器具体方法的                          | 1. 为什么 scheduling class 不是和 policy 一一对应的 2. 或者 class 如何和 policy 对应，以及为什么一个class 可以实现多个 policy 3. policy 决定了什么 |


1. Scheduling policies
```c
// include/uapi/linux/sched.h
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5

#define SCHED_DEADLINE		6
```

2. Scheduling class
```c
/* kernel/sched/sched.h:1586 */
extern const struct sched_class stop_sched_class;
extern const struct sched_class dl_sched_class;
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class; 
// 只有一种可能，此处的 idle 是处理真正的无事可做，SCHED_IDLE 是表示
// 其实还是对应的，但是多出来了 stop 和 idle 的class 来处理两种蛇皮事情。
```


3. rq
```c
/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct rq {
  ...
  // sched_class 和 rq 一一对应的
  // 认为之所以没有其他的rq ， stop 是用于迁移进程于多个CPU的，idle 可能也 todo
  // 认为根基就是三种 : dl 是单独的一种
	struct cfs_rq		cfs;
	struct rt_rq		rt;
	struct dl_rq		dl;
  ...
}
```


```c
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
...
again:
	for_each_class(class) {
		p = class->pick_next_task(rq, prev, rf);
		if (p) {
			if (unlikely(p == RETRY_TASK))
				goto again;
			return p;
		}
	}
}


#ifdef CONFIG_SMP
#define sched_class_highest (&stop_sched_class)
#else
#define sched_class_highest (&dl_sched_class)
#endif

#define for_each_class(class) \
   for (class = sched_class_highest; class; class = class->next)
```


4. entity
```c
// 如果把 se 当做 fair 的内容，也是 rt dl 和 fair
// 那么就是rq 完全对应起来的
// include/linux/sched.h
	struct sched_entity		se;
	struct sched_rt_entity		rt;
#ifdef CONFIG_CGROUP_SCHED
	struct task_group		*sched_task_group;
#endif
	struct sched_dl_entity		dl;
```

> 1. policy 的含义是什么，没有STOP的原因很简单

* SCHED_IDLE的使用位置，fair.c 

> 2. 三种 policy 如何影响到 fair 的工作的不同的

> 3. entity 如果像是list 的东西，那么为什么这么大，到底放了什么东西，如果仅仅是为了放到 rb tree 显然就不用拆分出来是三个了!

* 观察 entity 中间的内容，会发现其中只是当然含有 rbnode 或者 list_head 之类的，还存在一些统计信息。


## Something should be clean


> 可不可以说: 调度器算法核心在于对于RUNNABLE的进程排序，这也是唯一可以获取。

**policy + priority**

对于调度策略，其中SCHED_FIFO、SCHED_RR、SCHED_DEADLINE是实时进程的调度策略。

sched_class有几种实现：
* stop_sched_class优先级最高的任务会使用这种策略，会中断所有其他线程，且不会被其他任务打断；
* dl_sched_class就对应上面的deadline调度策略；
* rt_sched_class就对应RR算法或者FIFO算法的调度策略，具体调度策略由进程的task_struct->policy指定；
* fair_sched_class就是普通进程的调度策略；
* idle_sched_class就是空闲进程的调度策略。

> 进程为什么会持有调度器指针啊 ?
> 在ucore中间，统一的接口，最后放入到队列中间，其实根本看不到!

## CFS 
在Linux里面，实现了一个基于CFS的调度算法。
CFS全称Completely Fair Scheduling，叫完全公平调度。
听起来很“公平”。那这个算法的原理是什么呢？我们来看看。

> 这是对应哪一个具体的调度器算法吗 ?

在task_struct中有这样的成员变量：

struct sched_entity se;
struct sched_rt_entity rt;
struct sched_dl_entity dl;
> 为了支持RB tree


CPU也是这样的， 每个CPU都有自己的 struct rq 结构，
其用于描述在此CPU上所运行的所有进程，其包括一个实时进程队列rt_rq和一个CFS运行队列cfs_rq，在调度时，调度器首先会先去实时进程队列找是否有实时进程需要运行，如果没有才会去CFS运行队列找是否有进行需要运行
> 一个rq 如何支持三种调度器算法 ?

sched_class 定义一堆函数指针. dl rt fair 只是其函数填充的具体内容。

它们其实是放在一个链表上的。这里我们以调度最常见的操作，取下一个任务为例: 



切换进程仅仅切换的内容 : 寄存器 和 地址空间(似乎就是 mm_struct)
context_switch() 函数:
1. switch_mm_irqs_off(oldmm, mm, next);
2. switch_to
    1. `__switch_to_asm`
    2. `__switch_to`

所谓的进程切换，就是将某个进程的thread_struct里面的寄存器的值，写入到CPU的TR指向的tss_struct，对于CPU来讲，这就算是完成了切换.

## 各种数量不一致

