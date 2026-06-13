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
