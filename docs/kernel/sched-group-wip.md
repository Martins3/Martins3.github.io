- [ ] 难道，只有 fair 才可以实现被 cgroup 管理?
  - 似乎也有道理


- sched_class::task_change_group 注册为 task_change_group_fair
  - detach_task_cfs_rq
    - detach_entity_cfs_rq
  - attach_task_cfs_rq

```txt
@[
    task_change_group_fair+5
    sched_move_task+151
    cpu_cgroup_attach+54
    cgroup_migrate_execute+909
    cgroup_attach_task+326
    __cgroup_procs_write+244
    cgroup_procs_write+23
    kernfs_fop_write_iter+300
    vfs_write+706
    ksys_write+99
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1
```

```txt
@[
    task_change_group_fair+5
    sched_move_task+151
    autogroup_move_group+147
    sched_autogroup_create_attach+169
    ksys_setsid+234
    __x64_sys_setsid+14
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1
```

```txt
@[
    task_change_group_fair+5
    sched_move_task+151
    do_exit+853
    __x64_sys_exit+27
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 116
```

- autogroup_move_group : autogroup 机制
- cpu_cgroup_attach :
  - sched_move_task
    - sched_change_group


```c
struct sched_entity {

#ifdef CONFIG_FAIR_GROUP_SCHED
	int				depth;
	struct sched_entity		*parent;
	/* rq on which this entity is (to be) queued: */
	struct cfs_rq			*cfs_rq;
	/* rq "owned" by this entity/group: */
	struct cfs_rq			*my_q;
#endif
}

struct rq {

#ifdef CONFIG_FAIR_GROUP_SCHED
	/* list of leaf cfs_rq on this CPU: */
	struct list_head	leaf_cfs_rq_list;
	struct list_head	*tmp_alone_branch;
#endif /* CONFIG_FAIR_GROUP_SCHED */
}
```

### CONFIG_CFS_BANDWIDTH

参考资料:
- https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt

> CFS bandwidth control is a `CONFIG_FAIR_GROUP_SCHED` extension which allows the
> specification of the maximum CPU bandwidth available to a group or hierarchy.
>
> The bandwidth allowed for a group is specified using a quota and period. Within
> each given "period" (microseconds), a group is allowed to consume only up to
> "quota" microseconds of CPU time.  When the CPU bandwidth consumption of a
> group exceeds this limit (for that period), the tasks belonging to its
> hierarchy will be throttled and are not allowed to run again until the next
> period.

总结到位。

## 依赖关系是什么?

```txt
if CGROUP_SCHED
config FAIR_GROUP_SCHED
	bool "Group scheduling for SCHED_OTHER"
	depends on CGROUP_SCHED
	default CGROUP_SCHED

config SCHED_AUTOGROUP
	bool "Automatic process group scheduling"
	select CGROUPS
	select CGROUP_SCHED
	select FAIR_GROUP_SCHED
	help
	  This option optimizes the scheduler for common desktop workloads by
	  automatically creating and populating task groups.  This separation
	  of workloads isolates aggressive CPU burners (like build jobs) from
	  desktop applications.  Task group autogeneration is currently based
	  upon task session.
```

CGROUP 下才有 FAIR_GROUP_SCHED，之后才有 auto group

- CGROUP 必须有 FAIR_GROUP_SCHED 吗?
  - 是可以没有的!

- [ ] group sched 是 cgroup 特有的


了解下这个结构体的作用
```c
/* Task group related information */
struct task_group {
```

task_group  CONFIG_FAIR_GROUP_SCHED 以及 CONFIG_CFS_BANDWIDTH 三者逐渐递进的

## task_group

- task_group 会为每个 CPU 再维护一个 cfs_rq，这个 cfs_rq 用于组织挂在这个任务组上的任务以及子任务组，参考图中的 Group A；
- 调度器在调度的时候，比如调用`pick_next_task_fair`时，会从遍历队列，选择 sched_entity，如果发现 sched_entity 对应的是 task_group，则会继续往下选择；
- 由于 sched_entity 结构中存在 parent 指针，指向它的父结构，因此，系统的运行也能从下而上的进行遍历操作，通常使用函数 walk_tg_tree_from 进行遍历；

- [ ] please notice that, when CONFIG_CFS_BADNWIDTH turned off, walk_tg_tree_from's users are disapeared, so why `bandwidth` need `walk_tg_tree_from` ?

- [ ] we create a `cfs_rq` for every group, verfiy it
- [ ] what kinds of process will be added to same process group ?

```c
/* task group related information */
struct task_group {
    /* ... */

    /* 为每个CPU都分配一个CFS调度实体和CFS运行队列 */
#ifdef CONFIG_FAIR_GROUP_SCHED
	/* schedulable entities of this group on each cpu */
	struct sched_entity **se;
	/* runqueue "owned" by this group on each cpu */
	struct cfs_rq **cfs_rq;
	unsigned long shares;
#endif

    /* 为每个CPU都分配一个RT调度实体和RT运行队列 */
#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity **rt_se;
	struct rt_rq **rt_rq;

	struct rt_bandwidth rt_bandwidth;
#endif

    /* task_group之间的组织关系 */
	struct rcu_head rcu;
	struct list_head list;

	struct task_group *parent;
	struct list_head siblings;
	struct list_head children;

    /* ... */
};
```

## 直接
- sched_entity : 应该有时候，指的是一个 thread ，而有的时候是 task group 的
  - [ ] 有待验证
  - [ ] 为什么不搞成 group 中的 entry 也是公平调度的
```c
#ifdef CONFIG_FAIR_GROUP_SCHED
/* An entity is a task if it doesn't "own" a runqueue */
#define entity_is_task(se)	(!se->my_q)
#else
#define entity_is_task(se)	1
#endif
// 有的 entity 是用于对应的用于管理的，而有的才是真的对应于process 的
```
