# cgroup
<!-- vim-markdown-toc GitLab -->

- [overview](#overview)
- [userland api](#userland-api)
- [proc](#proc)
- [subsystem](#subsystem)
- [sys](#sys)
- [kernfs](#kernfs)
- [fs](#fs)
- [memcg](#memcg)
    - [memcg force_empty](#memcg-force_empty)
    - [memcg shrinker](#memcg-shrinker)
    - [memcg oom](#memcg-oom)
    - [memcg writeback](#memcg-writeback)
    - [memcg memory.stat](#memcg-memorystat)
- [migrate](#migrate)
- [blkio](#blkio)
- [文档整理一波](#文档整理一波)
- [structure](#structure)
    - [structure struct](#structure-struct)
    - [structure defines](#structure-defines)
    - [structure function](#structure-function)
- [namespace](#namespace)
- [idr](#idr)
- [domain threaded mask](#domain-threaded-mask)
- [bpf 不可避免 ?](#bpf-不可避免-)
- [cgroup ssid 的管理策略](#cgroup-ssid-的管理策略)
- [cftypes 的作用是什么 ?](#cftypes-的作用是什么-)
- [control](#control)
- [cgroup_add_dfl_cftypes 和 cgroup_add_legacy_cftypes](#cgroup_add_dfl_cftypes-和-cgroup_add_legacy_cftypes)
- [cpuset](#cpuset)
- [cgroup core files](#cgroup-core-files)
- [v1 v2](#v1-v2)
- [PSI](#psi)
- [cpu](#cpu)

<!-- vim-markdown-toc -->


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

- [ ] what's relation with `css` and `cgroup`

```
cgroup_mkdir         |
                     |==> cgroup_apply_control_enable ==> css_create
cgroup_apply_control |
```

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

/proc/cgroups's entry *hierarchy*
subsystem所关联到的cgroup树的ID，如果多个subsystem关联到同一颗cgroup树，那么他们的这个字段将一样，比如这里的cpu和cpuacct就一样，表示他们绑定到了同一颗树。如果出现下面的情况，这个字段将为0：
- 当前subsystem没有和任何cgroup树绑定
- 当前subsystem已经和cgroup v2的树绑定
- 当前subsystem没有被内核开启

挂载一颗和所有subsystem关联的cgroup树到/sys/fs/cgroup

    mount -t cgroup xxx /sys/fs/cgroup

挂载一颗和cpuset subsystem关联的cgroup树到/sys/fs/cgroup/cpuset

    mkdir /sys/fs/cgroup/cpuset
    mount -t cgroup -o cpuset xxx /sys/fs/cgroup/cpuset

挂载一颗与cpu和cpuacct subsystem关联的cgroup树到/sys/fs/cgroup/cpu,cpuacct

    mkdir /sys/fs/cgroup/cpu,cpuacct
    mount -t cgroup -o cpu,cpuacct xxx /sys/fs/cgroup/cpu,cpuacct

挂载一棵cgroup树，但不关联任何subsystem，下面就是systemd所用到的方式

    mkdir /sys/fs/cgroup/systemd
    mount -t cgroup -o none,name=systemd xxx /sys/fs/cgroup/systemd


## proc
- **WARN_ON(!proc_create_single("cgroups", 0, NULL, proc_cgroupstats_show));**

```
➜  vn git:(master) ✗ cat /proc/cgroups                 
#subsys_name	hierarchy	num_cgroups	enabled
cpuset	11	1	1
cpu	5	146	1
cpuacct	5	146	1
blkio	10	146	1
memory	8	156	1
devices	4	150	1
freezer	6	5	1
net_cls	7	1	1
perf_event	3	1	1
net_prio	7	1	1
hugetlb	12	1	1
pids	9	152	1
rdma	2	1	1
```

```c
		seq_printf(m, "%s\t%d\t%d\t%d\n",
			   ss->legacy_name, ss->root->hierarchy_id,
			   atomic_read(&ss->root->nr_cgrps),
			   cgroup_ssid_enabled(i));
```
- [x] how does /proc/cgroups work ?
  - [ ] **so different subsystems can link to a different cgroup_root ?**
    - [ ] If cgroup is hierarchical, why there are multiple root ?

- **proc_cgroup_show()**

```
➜  vn git:(master) ✗ cat /proc/self/cgroup 
12:hugetlb:/
11:cpuset:/
10:blkio:/user.slice
9:pids:/user.slice/user-1000.slice/user@1000.service
8:memory:/user.slice/user-1000.slice/user@1000.service
7:net_cls,net_prio:/
6:freezer:/
5:cpu,cpuacct:/user.slice
4:devices:/user.slice
3:perf_event:/
2:rdma:/
1:name=systemd:/user.slice/user-1000.slice/user@1000.service/vte-spawn-8babf497-024b-43ad-917f-044ac93e2c9e.scope
0::/user.slice/user-1000.slice/user@1000.service/vte-spawn-8babf497-024b-43ad-917f-044ac93e2c9e.scope
```

```c
/* The list of hierarchy roots */
LIST_HEAD(cgroup_roots);

/* iterate across the hierarchies */
#define for_each_root(root)						\
	list_for_each_entry((root), &cgroup_roots, root_list)
```

- cgroup_setup_root() add root to `cgroup_roots`
    - [ ] if we only one cgroupv2, then there is only one caller for `cgroup_setup_root`



## subsystem
cgroup_init_subsys

```c
/* generate an array of cgroup subsystem pointers */
#define SUBSYS(_x) [_x ## _cgrp_id] = &_x ## _cgrp_subsys,
struct cgroup_subsys *cgroup_subsys[] = {
#include <linux/cgroup_subsys.h>
};
#undef SUBSYS
```



## sys
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

static struct kernfs_syscall_ops cgroup_kf_syscall_ops = {
	.show_options		= cgroup_show_options,
	.mkdir			= cgroup_mkdir,
	.rmdir			= cgroup_rmdir,
	.show_path		= cgroup_show_path,
};
```

- [ ] cftype::kf_ops and cftype::open: now that we can use kf_ops to show files, but we still have to define all kinds of operations ?

1. /sys/kernel/cgroup
2. last several lines in cgroup.c is related

cgroup fs is mount at /proc/fs/cgroup
## kernfs
- [x] why so many mount points for cgroup ?
  - [ ] check the code that we mount all kinds all subsystem seperately.
  - I guess, because cgroup can mount multiple times, everytime we mount with different option which specify the subsystem to use.

```
cgroup2 /sys/fs/cgroup/unified cgroup2 rw,nosuid,nodev,noexec,relatime,nsdelegate 0 0
cgroup /sys/fs/cgroup/net_cls,net_prio cgroup rw,nosuid,nodev,noexec,relatime,net_cls,net_prio 0 0
cgroup /sys/fs/cgroup/systemd cgroup rw,nosuid,nodev,noexec,relatime,xattr,name=systemd 0 0
cgroup /sys/fs/cgroup/cpu,cpuacct cgroup rw,nosuid,nodev,noexec,relatime,cpu,cpuacct 0 0
cgroup /sys/fs/cgroup/perf_event cgroup rw,nosuid,nodev,noexec,relatime,perf_event 0 0
cgroup /sys/fs/cgroup/hugetlb cgroup rw,nosuid,nodev,noexec,relatime,hugetlb 0 0
cgroup /sys/fs/cgroup/freezer cgroup rw,nosuid,nodev,noexec,relatime,freezer 0 0
cgroup /sys/fs/cgroup/devices cgroup rw,nosuid,nodev,noexec,relatime,devices 0 0
cgroup /sys/fs/cgroup/cpuset cgroup rw,nosuid,nodev,noexec,relatime,cpuset 0 0
cgroup /sys/fs/cgroup/memory cgroup rw,nosuid,nodev,noexec,relatime,memory 0 0
cgroup /sys/fs/cgroup/blkio cgroup rw,nosuid,nodev,noexec,relatime,blkio 0 0
cgroup /sys/fs/cgroup/pids cgroup rw,nosuid,nodev,noexec,relatime,pids 0 0
cgroup /sys/fs/cgroup/rdma cgroup rw,nosuid,nodev,noexec,relatime,rdma 0 0
```

- [ ] cgroup_setup_root

## fs

- css_populate_dir

## memcg
- [ ] [In fact, we can check the user api before hack it](https://segmentfault.com/a/1190000008125359)

- [ ] why swap works different with memory_cgrp_subsys ? 
  - [ ] mem_cgroup_swap_init

- [ ] mem_cgroup_scan_tasks

- [ ] parent_mem_cgroup

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

- [ ] css

- [ ] both mem_cgroup_legacy_files and memory_files is 
```c
int __init cgroup_init(void){
    // ...
		if (ss->dfl_cftypes == ss->legacy_cftypes) {
			WARN_ON(cgroup_add_cftypes(ss, ss->dfl_cftypes));
		} else {
			WARN_ON(cgroup_add_dfl_cftypes(ss, ss->dfl_cftypes));
			WARN_ON(cgroup_add_legacy_cftypes(ss, ss->legacy_cftypes));
		}
    // ...
```


```
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  cgroup.clone_children
.-w--w--w- root root 0 B Fri Oct 23 13:41:52 2020  cgroup.event_control
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  cgroup.procs
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.failcnt
.-w------- root root 0 B Fri Oct 23 13:41:52 2020  memory.force_empty
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.failcnt
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.limit_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.max_usage_in_bytes
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.slabinfo
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.tcp.failcnt
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.tcp.limit_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.tcp.max_usage_in_bytes
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.tcp.usage_in_bytes
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.kmem.usage_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.limit_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.max_usage_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.move_charge_at_immigrate
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.numa_stat
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.oom_control
.--------- root root 0 B Fri Oct 23 13:41:52 2020  memory.pressure_level
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.soft_limit_in_bytes
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.stat
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.swappiness
.r--r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.usage_in_bytes
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  memory.use_hierarchy
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  notify_on_release
drwxr-xr-x root root 0 B Fri Oct 23 13:41:52 2020  session-1.scope
.rw-r--r-- root root 0 B Fri Oct 23 13:41:52 2020  tasks
drwxr-xr-x root root 0 B Fri Oct 23 13:41:52 2020  user-runtime-dir@1000.service
drwxr-xr-x root root 0 B Fri Oct 23 10:54:42 2020  user@1000.service
```

#### memcg force_empty

```c
static ssize_t mem_cgroup_force_empty_write(struct kernfs_open_file *of,
					    char *buf, size_t nbytes,
					    loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));

	if (mem_cgroup_is_root(memcg))
		return -EINVAL;
	return mem_cgroup_force_empty(memcg) ?: nbytes;
}

```

- [ ] kernfs_open_file : oh shit, the fucking kernfs

- [ ]



#### memcg shrinker
- [ ] shrink_node_memcgs
  - [ ] mem_cgroup_calculate_protection

#### memcg oom
mem_cgroup_oom_synchronize

#### memcg writeback
- [ ] mem_cgroup_wb_stats
- [ ] domain_dirty_limits

```c
static struct dirty_throttle_control *mdtc_gdtc(struct dirty_throttle_control *mdtc)
{
	return mdtc->gdtc;
}
```


#### memcg memory.stat
```
➜  user-1000.slice cat memory.stat 
cache 0
rss 0
rss_huge 0
shmem 0
mapped_file 0
dirty 0
writeback 0
pgpgin 0
pgpgout 0
pgfault 0
pgmajfault 0
inactive_anon 0
active_anon 0
inactive_file 0
active_file 0
unevictable 0
hierarchical_memory_limit 9223372036854771712
total_cache 4071890944
total_rss 9852420096
total_rss_huge 0
total_shmem 999370752
total_mapped_file 1125949440
total_dirty 1351680
total_writeback 405504
total_pgpgin 294122860
total_pgpgout 290723233
total_pgfault 255027451
total_pgmajfault 617992
total_inactive_anon 1164046336
total_active_anon 9625313280
total_inactive_file 1025933312
total_active_file 2080468992
total_unevictable 29466624
```
memory_stat_show


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


## blkio
/home/maritns3/core/linux/block/blk-throttle.c

what the fuck
## 文档整理一波
1. https://lwn.net/Articles/753162/
3. https://en.wikipedia.org/wiki/Cgroups : redesign
4. http://man7.org/linux/man-pages/man7/cpuset.7.html
5. Man cgroup(7)

## structure
其实需要处理两个关系
1. group 之间的层级结构
2. 多个限制的共同约束 cpu mem

#### structure struct
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

```

- [ ] struct cgroup::self, some cgroup may be link to any subsystem


- [x] cgroup_add_cftypes ==> cgroup_init_cftypes
  - array of cftype bounds to subsystem , only static message of cftpe is initialize, dynamic message such as which subsystem the cftype is bounded need help of cgroup_init_cftypes  
  - cgroup may bound to subsystem, but still contains cftype
```c
int __init cgroup_init(void)
{
	struct cgroup_subsys *ss;
	int ssid;

	BUILD_BUG_ON(CGROUP_SUBSYS_COUNT > 16);
	BUG_ON(cgroup_init_cftypes(NULL, cgroup_base_files));
	BUG_ON(cgroup_init_cftypes(NULL, cgroup1_base_files));
```


#### structure defines
```c
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
```


#### structure function

```c
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


## cpuset 
```c
struct cpuset {
	struct cgroup_subsys_state css;
```
怀疑所有的subsys 采用这种机制

- [ ] kernel/cgroup/cpuset.md
> 以后再说吧! 这他妈是什么啊 ?

## cgroup core files
fils lies in /sys/fs/cgroup ?

## v1 v2

It's clear v1 and v2 core files is different.
```
➜  v2 ls
 cgroup.controllers   cgroup.max.descendants   cgroup.stat              cgroup.threads   init.scope    machine.slice     system.slice
 cgroup.max.depth     cgroup.procs             cgroup.subtree_control   cpu.pressure     io.pressure   memory.pressure   user.slice

➜  v1 ls
 cgroup.clone_children   cgroup.procs   cgroup.sane_behavior   notify_on_release   release_agent   tasks
```

- [ ] mount option can specific


memory subsystem's control file:
```
 cgroup.clone_children        memory.kmem.max_usage_in_bytes       memory.limit_in_bytes             memory.stat
 cgroup.event_control         memory.kmem.slabinfo                 memory.max_usage_in_bytes         memory.swappiness
 cgroup.procs                 memory.kmem.tcp.failcnt              memory.move_charge_at_immigrate   memory.usage_in_bytes
 memory.failcnt               memory.kmem.tcp.limit_in_bytes       memory.numa_stat                  memory.use_hierarchy
 memory.force_empty           memory.kmem.tcp.max_usage_in_bytes   memory.oom_control                notify_on_release
 memory.kmem.failcnt          memory.kmem.tcp.usage_in_bytes       memory.pressure_level             tasks
 memory.kmem.limit_in_bytes   memory.kmem.usage_in_bytes           memory.soft_limit_in_bytes       
```

[Man cgroup(7)](https://man7.org/linux/man-pages/man7/cgroups.7.html)

*In cgroups v1, the ability to mount different controllers against
different hierarchies was intended to allow great flexibility for
application design.*  In practice, though, the flexibility turned out
to be less useful than expected, and in many cases added complexity.

- [ ] different controller against different hierarchy 

- [x] kernel boot parameter : cgroup_no_v1=list

[The current adoption status of cgroup v2 in containers](https://medium.com/nttlabs/cgroup-v2-596d035be4d7)
cgroup v1 has independent trees for each of controllers. eg. a process can join group "foo” for CPU (/sys/fs/cgroup/cpu/foo ) while joining group “bar” for memory ( /sys/fs/cgroup/memory/bar ).
While this design seemed to provide good flexibility, it wasn’t proved to be useful in practice.

- [x] one process can join multiple subsystem hierarchy

cgroup v2 focuses on simplicity: `/sys/fs/cgroup/cpu/$GROUPNAME` and `/sys/fs/cgroup/memory/$GROUPNAME` in v1 are now unified as `/sys/fs/cgroup/$GROUPNAME` ,
and a process can no longer join different groups for different controllers. If the process joins foo ( /sys/fs/cgroup/foo ), all controllers enabled for foo will take the control of the process.

In cgroup v2, the device access control is implemented by attaching an eBPF program (`BPF_PROG_TYPE_CGROUP_DEVICE`)to the file descriptor of `/sys/fs/cgroup/foo` directory. 


[](https://medium.com/some-tldrs/tldr-understanding-the-new-control-groups-api-by-rami-rosen-980df476f633)
1. single hierarchy
2. You cannot attach a process to an **internal subgroup**
3. cgroups v2, a process can belong only to a single subgroup.


## PSI
https://www.kernel.org/doc/html/latest/accounting/psi.html#

## cpu
[Linux Cgroup系列（05）：限制cgroup的CPU使用（subsystem之cpu）](https://segmentfault.com/a/1190000008323952)
