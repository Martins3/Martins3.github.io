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
