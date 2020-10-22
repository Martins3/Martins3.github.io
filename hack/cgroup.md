# cgroup

* http://www.haifux.org/lectures/299/netLec7.pdf

## overview
| desc                                                                                                                             | lines |
|----------------------------------------------------------------------------------------------------------------------------------|-------|
| 对于 cgroup_root 的各种处理，css 的 thread populated 等等的, idr  `*_from_root`, 似乎提供了各种辅助函数 @todo 但是不知道被谁使用 | 1300  |
| 搞了半天的root 的事情，似乎都是为了处理 mount                                                                                    | 2000  |
| cftype 的各种函数: cgroup_base_files cgroup_kf_single_ops                                                                        | 4000  |
| `css_task_iter_*`                                                                                                                | 4500  |
| css destruction fork init 以及 `cgroup_get_from_*`                                                                               |

Linux kernel provides support for following twelve control group subsystems:
- cpuset - assigns individual processor(s) and memory nodes to task(s) in a group;
- cpu - uses the scheduler to provide cgroup tasks access to the processor resources;
- cpuacct - generates reports about processor usage by a group;
- io - sets limit to read/write from/to block devices;
- memory - sets limit on memory usage by a task(s) from a group;
- devices - allows access to devices by a task(s) from a group;
- freezer - allows to suspend/resume for a task(s) from a group;
- net_cls - allows to mark network packets from task(s) from a group;
- net_prio - provides a way to dynamically set the priority of network traffic per network interface for a group;
- perf_event - provides access to perf events) to a group;
- hugetlb - activates support for huge pages for a group;
- pid - sets limit to number of processes in a group.

Entries list above can be verified in sys and proc.

```
+-------------+         +---------------------+    +------------->+---------------------+          +----------------+
| task_struct |         |       css_set       |    |              | cgroup_subsys_state |          |     cgroup     |
+-------------+         |                     |    |              +---------------------+          +----------------+
|             |         |                     |    |              |                     |          |     flags      |
|             |         |                     |    |              +---------------------+          |  cgroup.procs  |
|             |         |                     |    |              |        cgroup       |--------->|       id       |
|             |         |                     |    |              +---------------------+          |      ....      | 
|-------------+         |---------------------+----+                                               +----------------+
|   cgroups   | ------> | cgroup_subsys_state | array of cgroup_subsys_state
|-------------+         +---------------------+------------------>+---------------------+          +----------------+
|             |         |                     |                   | cgroup_subsys_state |          |      cgroup    |
+-------------+         +---------------------+                   +---------------------+          +----------------+
                                                                  |                     |          |      flags     |
                                                                  +---------------------+          |   cgroup.procs |
                                                                  |        cgroup       |--------->|        id      |
                                                                  +---------------------+          |       ....     |
                                                                  |    cgroup_subsys    |          +----------------+
                                                                  +---------------------+
                                                                             |
                                                                             |
                                                                             ↓
                                                                  +---------------------+
                                                                  |    cgroup_subsys    |
                                                                  +---------------------+
                                                                  |         id          |
                                                                  |        name         |
                                                                  |      css_online     |
                                                                  |      css_ofline     |
                                                                  |        attach       |
                                                                  |         ....        |
                                                                  +---------------------+
```

## userland api
[kernel doc](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)

- cgroup is largely composed of two parts - the core and controllers.
    - cgroup core is primarily responsible for **hierarchically** organizing processes.
    - A cgroup controller is usually responsible for distributing a specific type of system resource along the hierarchy although there are utility controllers which serve purposes other than resource distribution.
- cgroups form a tree structure and every process in the system belongs to one and only one cgroup. 
- All threads of a process belong to the same cgroup.
- On creation, all processes are put in the cgroup that the parent process belongs to at the time.
- A process can be *migrated* to another cgroup. Migration of a process doesn’t affect already existing descendant processes.

- All controller behaviors are hierarchical - if a controller is enabled on a cgroup, it affects all processes which belong to the cgroups consisting the inclusive sub-hierarchy of the cgroup.



[Linux Cgroup系列（01）：Cgroup概述](https://segmentfault.com/a/1190000006917884)

subsystem所关联到的cgroup树的ID，如果多个subsystem关联到同一颗cgroup树，那么他们的这个字段将一样，比如这里的cpu和cpuacct就一样，表示他们绑定到了同一颗树。如果出现下面的情况，这个字段将为0：
- 当前subsystem没有和任何cgroup树绑定
- 当前subsystem已经和cgroup v2的树绑定
- 当前subsystem没有被内核开启

- [ ] stuck here, I want to find how /proc/cgroups works



## proc
```c
/* cgroup core interface files for the default hierarchy */
static struct cftype cgroup_base_files[] = {
    //  cgroup.controllers
    //  cgroup.max.depth
    //  cgroup.max.descendants
    //  cgroup.procs
    //  cgroup.stat
    //  cgroup.subtree_control
    //  cgroup.threads
}

/*
 * cgroup_file is the handle for a file instance created in a cgroup which
 * is used, for example, to generate file changed notifications.  This can
 * be obtained by setting cftype->file_offset.
 */
struct cgroup_file {
	/* do not access any fields from outside cgroup core */
	struct kernfs_node *kn;
	unsigned long notified_at;
	struct timer_list notify_timer;
};


static struct kernfs_ops cgroup_kf_single_ops = {
	.atomic_write_len	= PAGE_SIZE,
	.open			= cgroup_file_open,
	.release		= cgroup_file_release,
	.write			= cgroup_file_write,
	.poll			= cgroup_file_poll,
	.seq_show		= cgroup_seqfile_show,
};

static struct kernfs_ops cgroup_kf_ops = {
	.atomic_write_len	= PAGE_SIZE,
	.open			= cgroup_file_open,
	.release		= cgroup_file_release,
	.write			= cgroup_file_write,
	.poll			= cgroup_file_poll,
	.seq_start		= cgroup_seqfile_start,
	.seq_next		= cgroup_seqfile_next,
	.seq_stop		= cgroup_seqfile_stop,
	.seq_show		= cgroup_seqfile_show,
};
```

- [ ] cftype::kf_ops and cftype::open: now that we can use kf_ops to show files, but we still have to define all kinds of operations ?

## 和虚拟文件系统是如何勾连上的 ?

1. kernfs
2. kernfs_syscall_ops 诡异的操作

```c
static struct kernfs_syscall_ops cgroup_kf_syscall_ops = {
	.show_options		= cgroup_show_options,
	.remount_fs		= cgroup_remount,
	.mkdir			= cgroup_mkdir,
	.rmdir			= cgroup_rmdir,
	.show_path		= cgroup_show_path,
};


static struct kernfs_ops cgroup_kf_single_ops = {
	.atomic_write_len	= PAGE_SIZE,
	.open			= cgroup_file_open,
	.release		= cgroup_file_release,
	.write			= cgroup_file_write,
	.seq_show		= cgroup_seqfile_show,
};

static struct kernfs_ops cgroup_kf_ops = {
	.atomic_write_len	= PAGE_SIZE,
	.open			= cgroup_file_open,
	.release		= cgroup_file_release,
	.write			= cgroup_file_write,
	.seq_start		= cgroup_seqfile_start,
	.seq_next		= cgroup_seqfile_next,
	.seq_stop		= cgroup_seqfile_stop,
	.seq_show		= cgroup_seqfile_show,
};
```

## fs

- css_populate_dir

## memcg
- [ ] why swap works different with memory_cgrp_subsys ? 
  - [ ] mem_cgroup_swap_init

```c
struct cgroup_subsys memory_cgrp_subsys = {
	.css_alloc = mem_cgroup_css_alloc,
	.css_online = mem_cgroup_css_online,
	.css_offline = mem_cgroup_css_offline,
	.css_released = mem_cgroup_css_released,
	.css_free = mem_cgroup_css_free,
	.css_reset = mem_cgroup_css_reset,
	.can_attach = mem_cgroup_can_attach,
	.cancel_attach = mem_cgroup_cancel_attach,
	.post_attach = mem_cgroup_move_task,
	.bind = mem_cgroup_bind,
	.dfl_cftypes = memory_files,
	.legacy_cftypes = mem_cgroup_legacy_files,
	.early_init = 0,
};
```


## mount 和 ext4 有什么不同的地方 ?

```c
struct file_system_type cgroup_fs_type = {
	.name = "cgroup",
	.mount = cgroup_mount,
	.kill_sb = cgroup_kill_sb,
	.fs_flags = FS_USERNS_MOUNT,
};
```


```c
/* cgroup core interface files for the default hierarchy */
static struct cftype cgroup_base_files[] = {
```



## migrate
```c
/* used to track tasks and csets during migration */
struct cgroup_taskset {
	/* the src and dst cset list running through cset->mg_node */
	struct list_head	src_csets;
	struct list_head	dst_csets;

	/* the number of tasks in the set */
	int			nr_tasks;

	/* the subsys currently being processed */
	int			ssid;

	/*
	 * Fields for cgroup_taskset_*() iteration.
	 *
	 * Before migration is committed, the target migration tasks are on
	 * ->mg_tasks of the csets on ->src_csets.  After, on ->mg_tasks of
	 * the csets on ->dst_csets.  ->csets point to either ->src_csets
	 * or ->dst_csets depending on whether migration is committed.
	 *
	 * ->cur_csets and ->cur_task point to the current task position
	 * during iteration.
	 */
	struct list_head	*csets;
	struct css_set		*cur_cset;
	struct task_struct	*cur_task;
};

cgroup_migrate

/**
 * cgroup_attach_task - attach a task or a whole threadgroup to a cgroup
 * @dst_cgrp: the cgroup to attach to
 * @leader: the task or the leader of the threadgroup to be attached
 * @threadgroup: attach the whole threadgroup?
 *
 * Call holding cgroup_mutex and cgroup_threadgroup_rwsem.
 */
int cgroup_attach_task(struct cgroup *dst_cgrp, struct task_struct *leader,
		       bool threadgroup)
// todo 看到了一个 thread 
```







## 文档整理一波
1. https://lwn.net/Articles/753162/
3. https://en.wikipedia.org/wiki/Cgroups : redesign
4. http://man7.org/linux/man-pages/man7/cpuset.7.html
5. Man cgroup(7)

## struct
其实需要处理两个关系
1. group 之间的层级结构
2. 多个限制的共同约束 cpu mem

```c
// TODO 这个动态创建的 ? 还是static 的，唯一的 ? 解答分析 cgroup_setup_root
struct kernfs_root {
	/* published fields */
	struct kernfs_node	*kn;


/*
 * A cgroup_root represents the root of a cgroup hierarchy, and may be
 * associated with a kernfs_root to form an active hierarchy.  This is
 * internal to cgroup core.  Don't access directly from controllers.
 */
struct cgroup_root { // TODO 这个结构体只有一个 instance 吧?
	struct kernfs_root *kf_root;

/*
 * The default hierarchy, reserved for the subsystems that are otherwise
 * unattached - it never has more than a single cgroup, and all tasks are
 * part of that cgroup.
 */
struct cgroup_root cgrp_dfl_root = { .cgrp.rstat_cpu = &cgrp_dfl_root_rstat_cpu };
EXPORT_SYMBOL_GPL(cgrp_dfl_root);


/*
 * The default css_set - used by init and its children prior to any
 * hierarchies being mounted. It contains a pointer to the root state
 * for each subsystem. Also used to anchor the list of css_sets. Not
 * reference-counted, to improve performance when child cgroups
 * haven't been created.
 */
struct css_set init_css_set = { // init_css_set 和 cgrp_dfl_root 的关系是什么 ? 以及 kernfs_root 结构体
	.refcount		= REFCOUNT_INIT(1),
	.dom_cset		= &init_css_set,
	.tasks			= LIST_HEAD_INIT(init_css_set.tasks),
	.mg_tasks		= LIST_HEAD_INIT(init_css_set.mg_tasks),
	.task_iters		= LIST_HEAD_INIT(init_css_set.task_iters),
	.threaded_csets		= LIST_HEAD_INIT(init_css_set.threaded_csets),
	.cgrp_links		= LIST_HEAD_INIT(init_css_set.cgrp_links),
	.mg_preload_node	= LIST_HEAD_INIT(init_css_set.mg_preload_node),
	.mg_node		= LIST_HEAD_INIT(init_css_set.mg_node),

	/*
	 * The following field is re-initialized when this cset gets linked
	 * in cgroup_init().  However, let's initialize the field
	 * statically too so that the default cgroup can be accessed safely
	 * early during boot.
	 */
	.dfl_cgrp		= &cgrp_dfl_root.cgrp, // TODO 这就是证据!
};


/*
 * A css_set is a structure holding pointers to a set of
 * cgroup_subsys_state objects. This saves space in the task struct
 * object and speeds up fork()/exit(), since a single inc/dec and a
 * list_add()/del() can bump the reference count on the entire cgroup
 * set for a task.
 */
struct css_set { // task_struct 

/*
 * Per-subsystem/per-cgroup state maintained by the system.  This is the
 * fundamental structural building block that controllers deal with.
 *
 * Fields marked with "PI:" are public and immutable and may be accessed
 * directly without synchronization.
 */
struct cgroup_subsys_state { // 分析 sched 的内容，这是保存了具体的 subsys 的限制状态的之类的吧!
// 其中持有cftype 的内容

struct cgroup {　// 保存一些 tree 相关的信息吧!

/*
 * Control Group subsystem type.
 * See Documentation/cgroup-v1/cgroups.txt for details
 */
struct cgroup_subsys {  // 子系统的数量就是那几个

/*
 * struct cftype: handler definitions for cgroup control files
 *
 * When reading/writing to a file:
 *	- the cgroup to use is file->f_path.dentry->d_parent->d_fsdata
 *	- the 'cftype' of the file is file->f_path.dentry->d_fsdata
 */
struct cftype { // TODO 一个文件对应一个这种结构体吗 ?

/*
 * The returned cgroup is fully initialized including its control mask, but
 * it isn't associated with its kernfs_node and doesn't have the control
 * mask applied.
 */
static struct cgroup *cgroup_create(struct cgroup *parent)
// 1. control mask
// 2. cgroup 似乎才是构成tree 的方法呀!


/**
 * cgroup_css - obtain a cgroup's css for the specified subsystem
 * @cgrp: the cgroup of interest
 * @ss: the subsystem of interest (%NULL returns @cgrp->self)
 *
 * Return @cgrp's css (cgroup_subsys_state) associated with @ss.  This
 * function must be called either under cgroup_mutex or rcu_read_lock() and
 * the caller is responsible for pinning the returned css if it wants to
 * keep accessing it outside the said locks.  This function may return
 * %NULL if @cgrp doesn't have @subsys_id enabled.
 */
static struct cgroup_subsys_state *cgroup_css(struct cgroup *cgrp,
					      struct cgroup_subsys *ss)
{
	if (ss)
		return rcu_dereference_check(cgrp->subsys[ss->id],
					lockdep_is_held(&cgroup_mutex));
	else
		return &cgrp->self;
}
// cgroup_subsys 似乎就是那么几个!
```


## namespace
```c
/*
 * look up cgroup associated with current task's cgroup namespace on the
 * specified hierarchy
 */
static struct cgroup *
current_cgns_cgroup_from_root(struct cgroup_root *root)
```


## idr

对于具体实现并没有什么兴趣 但是
1. 到底是什么需要 id
2. 为什么需要 id 

## domain threaded mask



## bpf 不可避免 ?
## cgroup ssid 的管理策略

## cftypes 的作用是什么 ?
1. 那么 cftype 的作用是什么 ?
2. 


```c
cgroup_add_cftypes : 将 cftype->kf_ops cftype->ss 的内容
```



## control

```c
/**
 * cgroup_apply_control - apply control mask updates to the subtree
 * @cgrp: root of the target subtree
 *
 * subsystems can be enabled and disabled in a subtree using the following
 * steps.
 *
 * 1. Call cgroup_save_control() to stash the current state.
 * 2. Update ->subtree_control masks in the subtree as desired.
 * 3. Call cgroup_apply_control() to apply the changes.
 * 4. Optionally perform other related operations.
 * 5. Call cgroup_finalize_control() to finish up.
 *
 * This function implements step 3 and propagates the mask changes
 * throughout @cgrp's subtree, updates csses accordingly and perform
 * process migrations.
 */
static int cgroup_apply_control(struct cgroup *cgrp)
```



## cgroup_add_dfl_cftypes 和 cgroup_add_legacy_cftypes
实际上，两个函数只有cft-> flags 的区别

但是，legacy_cftypes 就是 cgroup-v1

```c
/**
 * cgroup_add_dfl_cftypes - add an array of cftypes for default hierarchy
 * @ss: target cgroup subsystem
 * @cfts: zero-length name terminated array of cftypes
 *
 * Similar to cgroup_add_cftypes() but the added files are only used for
 * the default hierarchy.
 */
int cgroup_add_dfl_cftypes(struct cgroup_subsys *ss, struct cftype *cfts)
{
	struct cftype *cft;

	for (cft = cfts; cft && cft->name[0] != '\0'; cft++)
		cft->flags |= __CFTYPE_ONLY_ON_DFL;
	return cgroup_add_cftypes(ss, cfts);
}

/**
 * cgroup_add_legacy_cftypes - add an array of cftypes for legacy hierarchies
 * @ss: target cgroup subsystem
 * @cfts: zero-length name terminated array of cftypes
 *
 * Similar to cgroup_add_cftypes() but the added files are only used for
 * the legacy hierarchies.
 */
int cgroup_add_legacy_cftypes(struct cgroup_subsys *ss, struct cftype *cfts)
{
	struct cftype *cft;

	for (cft = cfts; cft && cft->name[0] != '\0'; cft++)
		cft->flags |= __CFTYPE_NOT_ON_DFL;
	return cgroup_add_cftypes(ss, cfts);
}
```


# kernel/cgroup/cpuset.md
> 以后再说吧! 这他妈是什么啊 ?

## 外部interface cpuset_cgrp_subsys

```c
struct cgroup_subsys cpuset_cgrp_subsys = {
	.css_alloc	= cpuset_css_alloc,
	.css_online	= cpuset_css_online,
	.css_offline	= cpuset_css_offline,
	.css_free	= cpuset_css_free,
	.can_attach	= cpuset_can_attach,
	.cancel_attach	= cpuset_cancel_attach,
	.attach		= cpuset_attach,
	.post_attach	= cpuset_post_attach,
	.bind		= cpuset_bind,
	.fork		= cpuset_fork,
	.legacy_cftypes	= files,
	.early_init	= true,
};

```

## struct cpuset 
```c
struct cpuset {
	struct cgroup_subsys_state css;
```
怀疑所有的subsys 采用这种机制
