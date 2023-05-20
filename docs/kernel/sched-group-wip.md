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
```txt
#0  walk_tg_tree_from (from=0xffff888105cf9500, down=down@entry=0xffffffff8118f170 <tg_throttle_down>, up=0xffffffff81187820 <tg_nop>, data=data@entry=0xffff8883339ee3c0) at kernel/sched/core.c:1247
#1  0xffffffff81193b00 in throttle_cfs_rq (cfs_rq=0xffff888114767e00) at kernel/sched/fair.c:5432
#2  0xffffffff8119d2ed in pick_next_task_fair (rq=rq@entry=0xffff8883339ee3c0, prev=prev@entry=0xffff888108634600, rf=rf@entry=0xffffc90002577eb0) at kernel/sched/fair.c:8030
#3  0xffffffff822b718b in __pick_next_task (rf=0xffffc90002577eb0, prev=0xffff888108634600, rq=0xffff8883339ee3c0) at kernel/sched/core.c:5972
#4  pick_next_task (rf=0xffffc90002577eb0, prev=0xffff888108634600, rq=0xffff8883339ee3c0) at kernel/sched/core.c:6047
#5  __schedule (sched_mode=sched_mode@entry=0) at kernel/sched/core.c:6633
#6  0xffffffff822b82f2 in schedule () at kernel/sched/core.c:6745
#7  0xffffffff811f7abd in exit_to_user_mode_loop (ti_work=8, regs=<optimized out>) at kernel/entry/common.c:159
#8  exit_to_user_mode_prepare (regs=0xffffc90002577f58) at kernel/entry/common.c:204
#9  0xffffffff822aafd9 in irqentry_exit_to_user_mode (regs=<optimized out>) at kernel/entry/common.c:310
#10 0xffffffff8240148a in asm_sysvec_apic_timer_interrupt () at ./arch/x86/include/asm/idtentry.h:645
```

## 依赖关系是什么

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

- CGROUP 下才有 FAIR_GROUP_SCHED，之后才有的 SCHED_AUTOGROUP
- 但是很多时候都会选择 SCHED_AUTOGROUP 之后，那么 CGROUP 和 FAIR_GROUP_SCHED 就会自动存在

所以看上去，存在一堆 CONFIG，实际上，没有意义，默认都是打开的。

`struct task_group` CONFIG_FAIR_GROUP_SCHED 以及 CONFIG_CFS_BANDWIDTH 三者逐渐递进的

## struct task_group

每一个 css 都会被自己的结构体中包含

这是 cpuset 的:
```c
struct cpuset {
	struct cgroup_subsys_state css;
```

```c
/* Task group related information */
struct task_group {
	struct cgroup_subsys_state css;

#ifdef CONFIG_FAIR_GROUP_SCHED
	/* schedulable entities of this group on each CPU */
	struct sched_entity	**se;
	/* runqueue "owned" by this group on each CPU */
	struct cfs_rq		**cfs_rq;
	unsigned long		shares;

	/* A positive value indicates that this is a SCHED_IDLE group. */
	int			idle;

#ifdef	CONFIG_SMP
	/*
	 * load_avg can be heavily contended at clock tick time, so put
	 * it in its own cacheline separated from the fields above which
	 * will also be accessed at each tick.
	 */
	atomic_long_t		load_avg ____cacheline_aligned;
#endif
#endif

#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity	**rt_se;
	struct rt_rq		**rt_rq;

	struct rt_bandwidth	rt_bandwidth;
#endif

	struct rcu_head		rcu;
	struct list_head	list;

	struct task_group	*parent;
	struct list_head	siblings;
	struct list_head	children;

#ifdef CONFIG_SCHED_AUTOGROUP
	struct autogroup	*autogroup;
#endif

	struct cfs_bandwidth	cfs_bandwidth;

#ifdef CONFIG_UCLAMP_TASK_GROUP
	/* The two decimal precision [%] value requested from user-space */
	unsigned int		uclamp_pct[UCLAMP_CNT];
	/* Clamp values requested for a task group */
	struct uclamp_se	uclamp_req[UCLAMP_CNT];
	/* Effective clamp values used for a task group */
	struct uclamp_se	uclamp[UCLAMP_CNT];
#endif
```
这里包含的内容，完全就是没搞清楚的那些内容。

- task_group 会为每个 CPU 再维护一个 cfs_rq，这个 cfs_rq 用于组织挂在这个任务组上的任务以及子任务组，参考图中的 Group A；
- 调度器在调度的时候，比如调用`pick_next_task_fair`时，会从遍历队列，选择 sched_entity，如果发现 sched_entity 对应的是 task_group，则会继续往下选择；
- 由于 sched_entity 结构中存在 parent 指针，指向它的父结构，因此，系统的运行也能从下而上的进行遍历操作，通常使用函数 `walk_tg_tree_from` 进行遍历；

创建 task_group 存在两种原因:
1. ssh 因为 login 机制的时候，也会创建出来一个 group 的，首先创建
```txt
#0  alloc_fair_sched_group (tg=tg@entry=0xffff88810c7a0600, parent=parent@entry=0xffffffff83e9b340 <root_task_group>) at kernel/sched/fair.c:12362
#1  0xffffffff8118ae57 in sched_create_group (parent=0xffffffff83e9b340 <root_task_group>) at kernel/sched/core.c:10358
#2  0xffffffff811af83f in autogroup_create () at kernel/sched/build_utility.c:11034
#3  sched_autogroup_create_attach (p=0xffff888108635780) at kernel/sched/build_utility.c:11136
#4  0xffffffff8115f98a in ksys_setsid () at kernel/sys.c:1247
#5  0xffffffff8115f9ce in __do_sys_setsid (__unused=<optimized out>) at kernel/sys.c:1254
#6  0xffffffff822a5e5c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001867f58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc90001867f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#9  0x0000000000000000 in ?? ()
```

2. 因为构建 cgroup 而创建的:
```txt
#0  alloc_fair_sched_group (tg=tg@entry=0xffff88810ff39500, parent=parent@entry=0xffffffff83e9b340 <root_task_group>) at kernel/sched/fair.c:12362
#1  0xffffffff8118ae57 in sched_create_group (parent=0xffffffff83e9b340 <root_task_group>) at kernel/sched/core.c:10358
#2  0xffffffff8118af13 in cpu_cgroup_css_alloc (parent_css=<optimized out>) at kernel/sched/core.c:10523
#3  0xffffffff8123234b in css_create (ss=0xffffffff8306f840 <cpu_cgrp_subsys>, cgrp=0xffff8881139b8800) at kernel/cgroup/cgroup.c:5541
#4  cgroup_apply_control_enable (cgrp=cgrp@entry=0xffffffff83168350 <cgrp_dfl_root+16>) at kernel/cgroup/cgroup.c:3249
#5  0xffffffff81233d4b in cgroup_apply_control (cgrp=0xffffffff83168350 <cgrp_dfl_root+16>) at kernel/cgroup/cgroup.c:3331
#6  cgroup_subtree_control_write (of=0xffff888105e40d80, buf=<optimized out>, nbytes=4, off=<optimized out>) at kernel/cgroup/cgroup.c:3485
#7  0xffffffff814dec69 in kernfs_fop_write_iter (iocb=0xffffc9000185fea0, iter=<optimized out>) at fs/kernfs/file.c:334
#8  0xffffffff8142da2f in call_write_iter (iter=0xffffffff83e9b340 <root_task_group>, kio=0xffff88810ff39500, file=0xffff88810659cd00) at ./include/linux/fs.h:1868
#9  new_sync_write (ppos=0xffffc9000185ff08, len=4, buf=0x1b86d20 "+cpu", filp=0xffff88810659cd00) at fs/read_write.c:491
#10 vfs_write (file=file@entry=0xffff88810659cd00, buf=buf@entry=0x1b86d20 "+cpu", count=count@entry=4, pos=pos@entry=0xffffc9000185ff08) at fs/read_write.c:584
#11 0xffffffff8142dea3 in ksys_write (fd=<optimized out>, buf=0x1b86d20 "+cpu", count=4) at fs/read_write.c:637
#12 0xffffffff822a5e5c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000185ff58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc9000185ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

根据 mq 指针来区分，到底指向是 指的是一个 task，而有的时候是 task group 的
```c
#ifdef CONFIG_FAIR_GROUP_SCHED
/* An entity is a task if it doesn't "own" a runqueue */
#define entity_is_task(se)	(!se->my_q)
#else
#define entity_is_task(se)	1
#endif
```
