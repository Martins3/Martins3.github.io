## task_struct::policy
通过 task_struct::policy 来控制 sched_class

## scheduler 和 sched policy 之间是什么关系?

- [ ] How to make so many policy work together ?
    - bandwidth :
    - cgroup
    - every process should run in the period
    - different scheduler : dead rt
    - different policy

## sched class and policy
[Fixing SCHED_IDLE](https://lwn.net/Articles/805317/)

I only skim one or two paragraph of it, there is some notes:
> The CFS (completely fair scheduling) class hosts most of the user tasks; it implements three scheduling policies: SCHED_NORMAL, SCHED_BATCH, and SCHED_IDLE. A task under any of these policies gets a chance to run only if no other tasks are enqueued in the deadline or realtime classes (though by default the scheduler reserves 5% of the CPU for CFS tasks regardless). The scheduler tracks the virtual runtime (vruntime) for all tasks, runnable and blocked. The lower a task's vruntime, the more deserving the task is for time on the processor. CFS accordingly moves low-vruntime tasks toward the front of the scheduling queue.
>
> The priority of a task is calculated by adding 120 to its nice value, which ranges from -20 to +19. The priority of the task is used to set the weight of the task, which in turn affects the vruntime of the task; the lower the nice value, the higher the priority. The task's weight will thus be higher, and its vruntime will increase more slowly as the task runs.
>
> The SCHED_NORMAL policy (called SCHED_OTHER in user space) is used for most of the tasks that run in a Linux environment, like the shell. The SCHED_BATCH policy is used for batch processing by non-interactive tasks — tasks that should run uninterrupted for a period of time and hence are normally scheduled only after finishing all the SCHED_NORMAL activity. The SCHED_IDLE policy is designed for the lowest-priority tasks in the system; these tasks get a chance to run only if there is nothing else to run. Though, in practice, even in the presence of other SCHED_NORMAL tasks a SCHED_IDLE task will get some time to run (around 1.4% for a task with a nice value of zero). This policy isn't widely used currently and efforts are being made to improve how it works.

5. `policy` holds the scheduling policy applied to the process. Linux supports five possible values:
    * `SCHED_NORMAL` is used for normal processes on which our description focuses. They are
handled by the completely fair scheduler. `SCHED_BATCH` and `SCHED_IDLE` are also handled
by the completely fair scheduler but can be used for less important tasks.
    * `SCHED_RR` and `SCHED_FIFO` are used to implement soft real-time processes. `SCHED_RR` implements a round robin method, while SCHED_FIFO uses a first in, first out mechanism. These
are not handled by the completely fair scheduler class, but by the real-time scheduler class

## ps 可以看每一个 process 的状态
- https://unix.stackexchange.com/questions/407497/how-to-find-scheduling-policy-and-active-processes-priority

可以通过 cat /proc/pid/sched 获取到:

提供 -c ，之后的 CLS 就是了

```txt
🧀  ps -elf -c | head
```

检查 ps 的文档:
```txt
       cls         CLS       scheduling class of the process.  (alias policy, cls).  Field's possible values are:

                                      -    not reported
                                      TS   SCHED_OTHER
                                      FF   SCHED_FIFO
                                      RR   SCHED_RR
                                      B    SCHED_BATCH
                                      ISO  SCHED_ISO
                                      IDL  SCHED_IDLE
                                      DLN  SCHED_DEADLINE
                                      ?    unknown value
```

在 13900K 中可以找到下面的:
- B
- IDL
- FF
- TS

```txt
0 S martins3   51376    3552 IDL   0 - 258059 do_sys Jul11 ?       00:00:07 /nix/store/gq81ix20x1qm7nqjpd9cdy5bd9n9x3yn-tracker-miners-3.7.3/libexec/tracker-miner-fs-3
```

```txt
4 S 201         6386    6364 B     0 - 553780 -     Jul11 ?        01:00:34 /usr/sbin/netdata -u netdata -D -s /host -p 19999
0 S 201         6436    6386 B     0 - 17789 -      Jul11 ?        00:00:00 /usr/sbin/netdata --special-spawn-server
4 S root        6851    6386 B     0 - 31870 -      Jul11 ?        00:01:03 /usr/libexec/netdata/plugins.d/network-viewer.plugin 1
4 S root        6855    6386 B     0 - 325415 -     Jul11 ?        00:22:49 /usr/libexec/netdata/plugins.d/go.d.plugin 1
4 S root        6863    6386 B     0 - 39333 -      Jul11 ?        01:59:33 /usr/libexec/netdata/plugins.d/apps.plugin 1
4 S root        6864    6386 B     0 - 50481 -      Jul11 ?        00:01:36 /usr/libexec/netdata/plugins.d/systemd-journal.plugin 1
```

属于 FF 的 thread 全部都是 kernel thread ，例如:
```txt
1 S root          20       2 FF  139 -     0 -      Jul11 ?        00:00:05 [migration/0]
1 S root          21       2 FF   90 -     0 -      Jul11 ?        00:00:00 [idle_inject/0]
```

scheduler 被修改为 martins3 之后:
```txt policy                                       :                    8
prio                                         :                  120
```
### policy 和 sched class 的关系

```txt
static inline int idle_policy(int policy) { return policy == SCHED_IDLE; }
static inline int fair_policy(int policy) { return policy == SCHED_NORMAL || policy == SCHED_BATCH; } // TODO 为什么使用
static inline int rt_policy(int policy) { return policy == SCHED_FIFO || policy == SCHED_RR; }
static inline int dl_policy(int policy) { return policy == SCHED_DEADLINE; }
```

### 变化关系

```c
/**
 * task_prio - return the priority value of a given task.
 * @p: the task in question.
 *
 * Return: The priority value as seen by users in /proc.
 *
 * sched policy         return value   kernel prio    user prio/nice
 *
 * normal, batch, idle     [0 ... 39]  [100 ... 139]          0/[-20 ... 19]
 * fifo, rr             [-2 ... -100]     [98 ... 0]  [1 ... 99]
 * deadline                     -101             -1           0
 */
int task_prio(const struct task_struct *p)
{
	return p->prio - MAX_RT_PRIO;
}
```

```c
static inline int __normal_prio(int policy, int rt_prio, int nice)
{
	int prio;

	if (dl_policy(policy))
		prio = MAX_DL_PRIO - 1;
	else if (rt_policy(policy))
		prio = MAX_RT_PRIO - 1 - rt_prio;
	else
		prio = NICE_TO_PRIO(nice);

	return prio;
}
```

### 可以用于确定属于哪一个 sched class
1. sched_fork
```c
	if (rt_prio(p->prio)) {
		p->sched_class = &rt_sched_class;
#ifdef CONFIG_SCHED_CLASS_EXT
	} else if (task_should_scx(p->policy)) {
		p->sched_class = &ext_sched_class;
#endif
	} else if(p->policy == SCHED_MARTINS3) {
		p->sched_class = &martins3_sched_class;
	} else {
		p->sched_class = &fair_sched_class;
	}
```

## 不同的 sched policy 的 priority 的结果不同

- https://askubuntu.com/questions/656771/process-niceness-vs-priority
> It is important to note that for real time processes, the nice value is not used.
>
> for normal processes: PR = 20 + NI (NI is nice and ranges from -20 to 19)
> for real time processes: PR = - 1 - real_time_priority (real_time_priority ranges from 1 to 99)

The scheduler is the kernel component that decides which runnable thread will be executed by the CPU next.
Each thread has an associated scheduling policy and a static scheduling priority, sched_priority.
The scheduler makes its decisions based on knowledge of the scheduling policy and static priority of all threads on the system.
> policy 和 static priority 决定

For threads scheduled under one of the normal scheduling policies (SCHED_OTHER, SCHED_IDLE, SCHED_BATCH),
sched_priority is not used in scheduling decisions (it must be specified as 0).
> 虽然的确是映射为 1~140 之间，描述两个区间的词汇其实都是不同的 sched_priority 和 nice


```txt
       Any  processes or threads using SCHED_FIFO or SCHED_RR shall be unaffected by a call to setpri‐
       ority().  This is not considered an error. A process which subsequently reverts to  SCHED_OTHER
       need not have its priority affected by such a setpriority() call.
```

```c
// 存在 rq ，cfs_rq ，sched_entity
struct task_struct {

    /* Scheduler bits, serialized by scheduler locks: */
    unsigned            sched_reset_on_fork:1;
    unsigned            sched_contributes_to_load:1;
    unsigned            sched_migrated:1;
    unsigned            sched_remote_wakeup:1;
    /* Force alignment to the next boundary: */
    unsigned            :0;


    int             on_rq; // 和迁移有关的

    int             prio;
    int             static_prio;
    int             normal_prio;
    unsigned int            rt_priority;

    const struct sched_class    *sched_class;

    struct sched_entity         se;
    struct sched_rt_entity      rt;
    struct sched_dl_entity      dl;
#ifdef CONFIG_CGROUP_SCHED
    struct task_group       *sched_task_group;
#endif

#ifdef CONFIG_PREEMPT_NOTIFIERS
    /* List of struct preempt_notifier: */
    struct hlist_head       preempt_notifiers;
#endif

#ifdef CONFIG_BLK_DEV_IO_TRACE
    unsigned int            btrace_seq;
#endif

    // 同时可以设置 policy 为 : SCHED_NORMAL / SCHED_FIFO / SCHED_RR / SCHED_BATCH
    unsigned int            policy;
    int             nr_cpus_allowed;
    cpumask_t           cpus_allowed;

#ifdef CONFIG_PREEMPT_RCU
    int             rcu_read_lock_nesting;
    union rcu_special       rcu_read_unlock_special;
    struct list_head        rcu_node_entry;
    struct rcu_node         *rcu_blocked_node;
#endif /* #ifdef CONFIG_PREEMPT_RCU */

#ifdef CONFIG_TASKS_RCU
    unsigned long           rcu_tasks_nvcsw;
    u8              rcu_tasks_holdout;
    u8              rcu_tasks_idx;
    int             rcu_tasks_idle_cpu;
    struct list_head        rcu_tasks_holdout_list;
#endif /* #ifdef CONFIG_TASKS_RCU */
}
```

# 分析 sched policy 和 sched class

使用命令行工具 chrt

chrt -i 0 make -j32

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/tuning-scheduling-policy_monitoring-and-managing-system-status-and-performance

## 总结
感觉，就是 sched_class 是内核概念, policy 是用户态的概念，而 SCHED_IDLE, SCHED_BATCH , SCHED_NORMAL 之类的都是一个在
一个 sched_class 存在细微的调整。
注意 SCHED_IDLE 和 idle scheduler 不是一个东西。

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
	int				prio;         // /proc/self/schedstat
	int				static_prio;  //
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

## weight

- set_load_weight 中设置

update_curr 中的计算方法就是教科书中的说的
```c
	curr->vruntime += calc_delta_fair(delta_exec, curr);
```

## share
就是 group 的 share 的数值几乎就是 weight ，都是 nice 通过 sched_prio_to_weight 来计算得到的

- `__sched_group_set_shares`
  - tg->shares = shares;
  - update_cfs_group
    - calc_group_shares : 近似计算分析的非常复杂，但是总体就是跟踪
    - reweight_entity : for_each_possible_cpu 来计算 sched_entity 的 weight



可以从多个位置写入:
1. 第一个是 cgroup 中 cpu.weight
```txt
#0  __sched_group_set_shares (tg=0xffff888141e9a300, shares=104448) at kernel/sched/fair.c:11845
#1  0xffffffff8114be9a in sched_group_set_shares (tg=0xffff888141e9a300, shares=104448) at kernel/sched/fair.c:11880
#2  0xffffffff811bb84b in cgroup_file_write (of=<optimized out>, buf=0xffff8881547244a0 "10\n", nbytes=3, off=<optimized out>) at kernel/cgroup/cgroup.c:3983
```

默认的是 100，范围是 `[1, 10000]`， 控制粒度的更加精细。

2. 第二个是 cgroup.weight.nice ，范围是 `[-20, 19]`

2. 第二个是 /proc/pdi/autogroup
- proc_sched_autogroup_set_nice ，范围是 `[-20, 19]`



参考:
1. https://blog.shichao.io/2015/07/22/relationships_among_nice_priority_and_weight_in_linux_kernel.html
2. https://stackoverflow.com/questions/5770770/prio-static-prio-rt-priority-in-linux-kernel

## 如果将 gcc 和 ccls 的 priority 直接修改为 SCHED_IDLE
而不是降低优先级

## [ ] 这里的两个解释都是错误的
https://stackoverflow.com/questions/9392415/linux-sched-other-sched-fifo-and-sched-rr-differences

1. SCHED_FIFO: First in-first out scheduling 和 SCHED_RR: Round-robin scheduling 如果都在一个 scheduler 下，如何
实现两个策略啊


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
