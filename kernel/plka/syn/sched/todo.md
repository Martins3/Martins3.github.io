1. cfs_rq 和 rq 中间的 cfs
2. wake up 机制


而且还有两个大山:
1. low lantency 的处理: period grandularity 
2. balance 的处理

## 总结
| func             | desc                                                                                                  |
|------------------|-------------------------------------------------------------------------------------------------------|
| update_curr      | Update the current task's runtime statistics : 针对其中运行时间的统计结果                             |
| update_cfs_group | calc_group_shares 和 calc_group_runnable 然后更新 task_group 中间的 se 的数值                         |
| update_load_avg  | `__update_load_avg_se` 以及 `update_cfs_rq_load_avg` `update_tg_load_avg` 就是各种无死角更新 load_avg |

## 备忘
1. update_curr 和 update_cfs_group 之间的区别是什么 ?
2. task group 在所有的cpu 中间都创建一个，从而保证 hierarchy 在所有地方相同
3. `tg->load_avg := \sum tg->cfs_rq[]->avg.load_avg`. : update_tg_load_avg()
4. tg.share : 当我们dequeue task、enqueue task以及task tick的时候会通过update_cfs_group()函数更新group se的权重信息。也就是说，tg.share 权重就是静态计算的。
  1. update_cfs_group 更新的是 group se 权重。
5. calc_group_shares()根据当前group cfs_rq负载情况计算新的权重。
6. `scale_load_down` `scale_load` 是很简单的内容，只是为了实现高精度而已。
```c
# define scale_load(w)		((w) << SCHED_FIXEDPOINT_SHIFT)
# define scale_load_down(w)	((w) >> SCHED_FIXEDPOINT_SHIFT)

# define scale_load(w)		(w)
# define scale_load_down(w)	(w)
```
7. sched_avg 中的四个成员 : runable_load_avg load_avg load_sum runable_load_sum


## 分析的入口
- kernel/sched/fair.c:6798 的注释

- propagate_entity_load_avg

- load_sum runable_load_sum 就是为了对应的计算 avg 而产生的，可以验证一下
  - 进一步的怀疑，之所以需要 load_avg 的存在，就是之前的各种近似公式的计算导致的
  - 如果不是多核，那么，


- load_avg 和 load 的关系是什么 ? 依靠几何级数吗 ? 

- calc_group_runnable 的作用，而且其也是具有对应的一大段注释的

1. 各种 numa 函数 : account_numa_enqueue 
2. `se->on_rq` 和 `taks_struct->on_rq` 两者的对比

1. include/linux/sched.h:400 的注释
2. task_group::shares 的计算
3. 关于新创建的 cfs_rq 的位置，可以查看一下 online 和 offline 的函数

```c
#ifdef CONFIG_FAIR_GROUP_SCHED
	/* list of leaf cfs_rq on this CPU: */
	struct list_head	leaf_cfs_rq_list;
	struct list_head	*tmp_alone_branch;
#endif /* CONFIG_FAIR_GROUP_SCHED */
```
> 其中的 rq 的更新的内容是什么 ?

5. attach_entity_cfs_rq 和向 group 中间添加 setsid 之后fork 出来的，有没有关系 ?

6. static void update_blocked_averages(int cpu) block 在此处的含义是什么 ?

7. `cfs_rq->avg.load` 被 tg.load_avg 收集了，其他的属性怎么说 ?

9. balance 的测量标准是什么 ?

10. reweight_entity 中间依赖的函数 ?

11. 为什么需要分析 runable_load_avg 中间的　runable ?
  1. time_slice 和 vruntime 的各自的作用是什么 ?
  4. time_slice 和 time_period 的概念，导致 runnable 的总是运行其拥有的比例，那么现在还受 vruntime 控制了吗？
      1. vruntime 会选择正确的，如果没有 time_slice 的存在，那么可以在整个 period 中间始终运行该变量
      2. 而 time_slice 的存在只是为了处理 lantency 的存在，但是依旧高优先级持有更多的运行时间。
          1. 但是会不会导致出现所有人都是只能运行 min_grandlurity 的数值，即使是优先级更高的
3. 对于一个新添加的process 显然其 vruntime 最少的，那岂不是总是选择执行新加入的 ?
    2. 我觉得可以通过 min_vruntime 确定的，新加入的process 可能会马上开始运行，但是不会无穷无尽的占用。
    3. 所以 min_vruntime 的数值溢出如何处理 ? (难道没有一个定时一并处理的时间点吗 ?)

12. numa 到底意味着什么东西 ?
1. 所以，busiest 的确定的标准是什么 ?
