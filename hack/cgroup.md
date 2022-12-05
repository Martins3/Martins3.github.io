# cgroup
<!-- vim-markdown-toc GitLab -->

* [overview](#overview)
* [userland api](#userland-api)
* [proc](#proc)
* [subsystem](#subsystem)
* [sys](#sys)
* [kernfs](#kernfs)
* [mount](#mount)
* [css_set](#css_set)
* [fork](#fork)
* [migrate](#migrate)
* [blkio](#blkio)
* [structure](#structure)
    * [structure struct](#structure-struct)
    * [structure function](#structure-function)
* [cgroup_base_files](#cgroup_base_files)
* [cgroup1_base_files](#cgroup1_base_files)
* [namespace](#namespace)
* [idr](#idr)
* [domain threaded mask](#domain-threaded-mask)
* [cgroup ssid 的管理策略](#cgroup-ssid-的管理策略)
* [cgroup_add_dfl_cftypes 和 cgroup_add_legacy_cftypes](#cgroup_add_dfl_cftypes-和-cgroup_add_legacy_cftypes)
* [cgroup core files](#cgroup-core-files)
* [v1 v2](#v1-v2)
* [PSI](#psi)
* [thread](#thread)
* [page counter](#page-counter)
* [hugetlb cgroup](#hugetlb-cgroup)
* [cgroup.procs](#cgroupprocs)
* [TODO](#todo)
* [cgorup inode](#cgorup-inode)
* [reference](#reference)

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


cgroup_mkdir         |==> cgroup_create(only called here)
```

In v1, css is one-to-one mapped to cgroup because every hierarchy only support one subsystem.

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

- [ ] [it contains some doc](https://events.static.linuxfound.org/sites/events/files/slides/cgroup_and_namespaces.pdf)

- [ ] get_mem_cgroup_from_mm

- css_populate_dir : if `struct cgroup_subsys_state` doesn’t attach to any subsystem, only base file will be create.


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
#subsys_name  hierarchy num_cgroups enabled
cpuset  11  1 1
cpu 5 146 1
cpuacct 5 146 1
blkio 10  146 1
memory  8 156 1
devices 4 150 1
freezer 6 5 1
net_cls 7 1 1
perf_event  3 1 1
net_prio  7 1 1
hugetlb 12  1 1
pids  9 152 1
rdma  2 1 1
```

```c
    seq_printf(m, "%s\t%d\t%d\t%d\n",
         ss->legacy_name, ss->root->hierarchy_id,
         atomic_read(&ss->root->nr_cgrps),
         cgroup_ssid_enabled(i));
```
- [x] how does /proc/cgroups work ?
  - [ ] **so different subsystems can link to a different cgroup_root ?**
    - [x] If cgroup is hierarchical, why there are multiple root ?
      - this v1
- [ ] `root->hierarchy_id` is used only for root, and only for print, kind of wired

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
#define for_each_root(root)           \
  list_for_each_entry((root), &cgroup_roots, root_list)
```
- cgroup_setup_root() add root to `cgroup_roots`
    -  if we only use cgroup v2, then there is only one caller for `cgroup_setup_root`





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
  .atomic_write_len = PAGE_SIZE,
  .open     = cgroup_file_open,
  .release    = cgroup_file_release,
  .write      = cgroup_file_write,
  .poll     = cgroup_file_poll,
  .seq_show   = cgroup_seqfile_show,
};

static struct kernfs_ops cgroup_kf_ops = {
  .atomic_write_len = PAGE_SIZE,
  .open     = cgroup_file_open,
  .release    = cgroup_file_release,
  .write      = cgroup_file_write,
  .poll     = cgroup_file_poll,
  .seq_start    = cgroup_seqfile_start,
  .seq_next   = cgroup_seqfile_next,
  .seq_stop   = cgroup_seqfile_stop,
  .seq_show   = cgroup_seqfile_show,
};

static struct kernfs_syscall_ops cgroup_kf_syscall_ops = {
  .show_options   = cgroup_show_options,
  .mkdir      = cgroup_mkdir,
  .rmdir      = cgroup_rmdir,
  .show_path    = cgroup_show_path,
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

## mount
- [ ] mount

```c
static const struct fs_context_operations cgroup_fs_context_ops = {
  .free   = cgroup_fs_context_free,
  .parse_param  = cgroup2_parse_param,
  .get_tree = cgroup_get_tree,
  .reconfigure  = cgroup_reconfigure,
};

static const struct fs_context_operations cgroup1_fs_context_ops = {
  .free   = cgroup_fs_context_free,
  .parse_param  = cgroup1_parse_param,
  .get_tree = cgroup1_get_tree,
  .reconfigure  = cgroup1_reconfigure,
};

struct file_system_type cgroup_fs_type = {
  .name     = "cgroup",
  .init_fs_context  = cgroup_init_fs_context,
  .parameters   = cgroup1_fs_parameters,
  .kill_sb    = cgroup_kill_sb,
  .fs_flags   = FS_USERNS_MOUNT,
};

static struct file_system_type cgroup2_fs_type = {
  .name     = "cgroup2",
  .init_fs_context  = cgroup_init_fs_context,
  .parameters   = cgroup2_fs_parameters,
  .kill_sb    = cgroup_kill_sb,
  .fs_flags   = FS_USERNS_MOUNT,
};
```
If we have register the `file_system_type`, we can mount it as we like.

- [ ] How to mount with a cgroup with subsystem ?
    - cgroup1_parse_param
    - cgroup1_get_tree ==> cgroup1_root_to_use ==> cgroup_setup_root ==> rebind_subsystems


- [ ] How many root do we have ?
```c
/* iterate across the hierarchies */
#define for_each_root(root)           \
  list_for_each_entry((root), &cgroup_roots, root_list)
```

## css_set
- [ ] find_css_set
    - [ ] find_existing_css_set
        - [ ] compare_css_sets
    - caller is : cgroup_migrate_prepare_dst, cgroup_css_set_fork


## fork
- copy_process
  - [ ] cgroup_can_fork
    - [ ] cgroup_css_set_fork
  - [ ] cgroup_fork
  - [ ] cgroup_post_fork
- [ ] cgroup_exit


## migrate
```c
/* used to track tasks and csets during migration */
struct cgroup_taskset {
  /* the src and dst cset list running through cset->mg_node */
  struct list_head  src_csets;
  struct list_head  dst_csets;

  /* the number of tasks in the set */
  int     nr_tasks;

  /* the subsys currently being processed */
  int     ssid;

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
  struct list_head  *csets;
  struct css_set    *cur_cset;
  struct task_struct  *cur_task;
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


## structure
There are so many structures such as cgroup, cgroup_subsys_state, css_set, ....

#### structure struct
```c
// TODO 这个动态创建的 ? 还是static 的，唯一的 ? 解答分析 cgroup_setup_root
struct kernfs_root {
  /* published fields */
  struct kernfs_node  *kn;

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
 *  - the cgroup to use is file->f_path.dentry->d_parent->d_fsdata
 *  - the 'cftype' of the file is file->f_path.dentry->d_fsdata
 */
struct cftype { // TODO 一个文件对应一个这种结构体吗 ?


// XXX: v2 只有一个 root
/* the default hierarchy */
struct cgroup_root cgrp_dfl_root = { .cgrp.rstat_cpu = &cgrp_dfl_root_rstat_cpu };
EXPORT_SYMBOL_GPL(cgrp_dfl_root);
```

- [x] struct cgroup::self, every cgroup have base files
```c
// ONE
struct cgroup {
  /* self css with NULL ->ss, points back to this cgroup */
  struct cgroup_subsys_state self;

// TWO
static int css_populate_dir(struct cgroup_subsys_state *css)
{
  struct cgroup *cgrp = css->cgroup;
  struct cftype *cfts, *failed_cfts;
  int ret;

  if ((css->flags & CSS_VISIBLE) || !cgrp->kn)
    return 0;

  if (!css->ss) {
    if (cgroup_on_dfl(cgrp))
      cfts = cgroup_base_files;
    else
      cfts = cgroup1_base_files;

// THREE
struct cgroup_subsys_state *of_css(struct kernfs_open_file *of)
{
  struct cgroup *cgrp = of->kn->parent->priv;
  struct cftype *cft = of_cft(of);

  if (cft->ss)
    return rcu_dereference_raw(cgrp->subsys[cft->ss->id]);
  else
    return &cgrp->self;
}
```

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



- [ ] css_set::dfl_cgrp
  - default typically means v2

- [ ] link_css_set

```c
/*
 * A cgroup can be associated with multiple css_sets as different tasks may
 * belong to different cgroups on different hierarchies.  In the other
 * direction, a css_set is naturally associated with multiple cgroups.
 * This M:N relationship is represented by the following link structure
 * which exists for each association and allows traversing the associations
 * from both sides.
 */
struct cgrp_cset_link {
  /* the cgroup and css_set this link associates */
  struct cgroup   *cgrp;
  struct css_set    *cset;

  /* list of cgrp_cset_links anchored at cgrp->cset_links */
  struct list_head  cset_link;

  /* list of cgrp_cset_links anchored at css_set->cgrp_links */
  struct list_head  cgrp_link;
};
```

- [ ]`css_set` doesn't work as we expected.
- [ ] Comment says, *A cgroup can be associated with multiple css_sets as different tasks may belong to different cgroups on different hierarchies.*
  - [ ] Why cgroup has to associated wtih css_set ?
    - [ ] link_css_set()
      - [ ] find_css_set()
      - [ ] cgroup_setup_root()

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
```



- [ ] cgroup_add_cftypes

- [ ] css_create

```c
/**
 * css_create - create a cgroup_subsys_state
 * @cgrp: the cgroup new css will be associated with
 * @ss: the subsys of new css
 *
 * Create a new css associated with @cgrp - @ss pair.  On success, the new
 * css is online and installed in @cgrp.  This function doesn't create the
 * interface files.  Returns 0 on success, -errno on failure.
 */
static struct cgroup_subsys_state *css_create(struct cgroup *cgrp,
                struct cgroup_subsys *ss)
```

## cgroup_base_files

- [ ] cgroup_controllers_show
- [ ] cgroup_subtree_control_show


## cgroup1_base_files


## namespace
- [ ] current_cgns_cgroup_from_root
- [ ] cgroup_path_ns_locked


## idr
对于具体实现并没有什么兴趣 但是
1. 到底是什么需要 id
2. 为什么需要 id

```c
/**
 * css_from_id - lookup css by id
 * @id: the cgroup id
 * @ss: cgroup subsys to be looked into
 *
 * Returns the css if there's valid one with @id, otherwise returns NULL.
 * Should be called under rcu_read_lock().
 */
struct cgroup_subsys_state *css_from_id(int id, struct cgroup_subsys *ss)
{
  WARN_ON_ONCE(!rcu_read_lock_held());
  return idr_find(&ss->css_idr, id);
}

static void cgroup_exit_root_id(struct cgroup_root *root)
{
  lockdep_assert_held(&cgroup_mutex);

  idr_remove(&cgroup_hierarchy_idr, root->hierarchy_id);
}
```

- [ ] css_from_id : subsystem rely more on idr to find target css, check it.

## domain threaded mask


## cgroup ssid 的管理策略
- [ ] so, what's ssid ?

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

## cgroup core files
fils lies in /sys/fs/cgroup ?

## v1 v2
https://github.com/opencontainers/runc/blob/master/docs/cgroup-v2.md
> Am I using cgroup v2?
> Yes if /sys/fs/cgroup/cgroup.controllers is present.


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


[TLDR Understanding the new cgroups v2 API by Rami Rosen](https://medium.com/some-tldrs/tldr-understanding-the-new-control-groups-api-by-rami-rosen-980df476f633)

![loading](https://miro.medium.com/max/557/1*P7ZLLF_F4TMgGfaJ2XIfuQ.png)
1. single hierarchy
2. You cannot attach a process to an **internal subgroup**
3. cgroups v2, a process can belong only to a single subgroup.

- [ ] https://man7.org/conf/lca2019/cgroups_v2-LCA2019-Kerrisk.pdf
- [ ] https://lwn.net/Articles/679786/
- [ ] man : https://man7.org/linux/man-pages/man7/cgroups.7.html

## PSI
https://www.kernel.org/doc/html/latest/accounting/psi.html#
## thread
https://lwn.net/Articles/656115/


## page counter
- [ ] mm/page_counter.c


## hugetlb cgroup



## cgroup.procs
```c
static void *cgroup_seqfile_start(struct seq_file *seq, loff_t *ppos)
{
  return seq_cft(seq)->seq_start(seq, ppos);
}

static void *cgroup_seqfile_next(struct seq_file *seq, void *v, loff_t *ppos)
{
  return seq_cft(seq)->seq_next(seq, v, ppos);
}

static void cgroup_seqfile_stop(struct seq_file *seq, void *v)
{
  if (seq_cft(seq)->seq_stop)
    seq_cft(seq)->seq_stop(seq, v);
}

static int cgroup_seqfile_show(struct seq_file *m, void *arg)
{
  struct cftype *cft = seq_cft(m);
  struct cgroup_subsys_state *css = seq_css(m);

  if (cft->seq_show)
    return cft->seq_show(m, arg);

  if (cft->read_u64)
    seq_printf(m, "%llu\n", cft->read_u64(css, cft));
  else if (cft->read_s64)
    seq_printf(m, "%lld\n", cft->read_s64(css, cft));
  else
    return -EINVAL;
  return 0;
}

static ssize_t cgroup_file_write(struct kernfs_open_file *of, char *buf,
         size_t nbytes, loff_t off)
```

They are registered at here:
```c
static struct kernfs_ops cgroup_kf_single_ops = {
  .atomic_write_len = PAGE_SIZE,
  .open     = cgroup_file_open,
  .release    = cgroup_file_release,
  .write      = cgroup_file_write,
  .poll     = cgroup_file_poll,
  .seq_show   = cgroup_seqfile_show,
};

static struct kernfs_ops cgroup_kf_ops = {
  .atomic_write_len = PAGE_SIZE,
  .open     = cgroup_file_open,
  .release    = cgroup_file_release,
  .write      = cgroup_file_write,
  .poll     = cgroup_file_poll,
  .seq_start    = cgroup_seqfile_start,
  .seq_next   = cgroup_seqfile_next,
  .seq_stop   = cgroup_seqfile_stop,
  .seq_show   = cgroup_seqfile_show,
};

```
- [x] What's difference between `cgroup_kf_ops` and `cgroup_kf_single_ops`
  - cgroup_init_cftypes : some files contains multiple value while someone only contains one value
- [ ] Maybe we can do a survey about why not register theese `write`, `poll`, `seq_show` directly to seq_file ?



- [ ] `__cgroup1_procs_write`
  - [ ] cgroup_kn_lock_live
  - [ ] cgroup_procs_write_start(find pid by find_task_by_vpid, and do some locks)
  - [ ] cgroup_attach_task
    - [ ] cgroup_migrate_add_src


- [ ] css_set_move_task : used by migrate, but should have

```c
/**
 * cgroup_migrate_add_src - add a migration source css_set
 * @src_cset: the source css_set to add
 * @dst_cgrp: the destination cgroup
 * @mgctx: migration context
 *
 * Tasks belonging to @src_cset are about to be migrated to @dst_cgrp.  Pin
 * @src_cset and add it to @mgctx->src_csets, which should later be cleaned
 * up by cgroup_migrate_finish().
 *
 * This function may be called without holding cgroup_threadgroup_rwsem
 * even if the target is a process.  Threads may be created and destroyed
 * but as long as cgroup_mutex is not dropped, no new css_set can be put
 * into play and the preloaded css_sets are guaranteed to cover all
 * migrations.
 */
void cgroup_migrate_add_src(struct css_set *src_cset,
          struct cgroup *dst_cgrp,
          struct cgroup_mgctx *mgctx)
```

- [ ] *Tasks belonging to @src_cset are about to be migrated to @dst_cgrp.  Pin `@src_cset` and add it to `@mgctx->src_csets`, which should later be cleaned.*
    - it's wired, removed from css_set and added to cgroup

```c
/* look up cgroup associated with given css_set on the specified hierarchy */
static struct cgroup *cset_cgroup_from_root(struct css_set *cset,
              struct cgroup_root *root)
{
  struct cgroup *res = NULL;

  lockdep_assert_held(&cgroup_mutex);
  lockdep_assert_held(&css_set_lock);

  // 这个暂时看不懂
  if (cset == &init_css_set) {
    res = &root->cgrp;
  // 如果说是 v2 那么就靠 css_set 定义的 dfl_cgrp
  } else if (root == &cgrp_dfl_root) {
    res = cset->dfl_cgrp;
  // v1 的情况 : task should move to same root
  } else {
    struct cgrp_cset_link *link;

    list_for_each_entry(link, &cset->cgrp_links, cgrp_link) {
      struct cgroup *c = link->cgrp;

      if (c->root == root) {
        res = c;
        break;
      }
    }
  }

  BUG_ON(!res);
  return res;
}
```

## TODO
- [ ] css, css_set, ss, cgroup, memcg, so how are they create init and destroyed
```c
/*
 * css destruction is four-stage process.
 *
 * 1. Destruction starts.  Killing of the percpu_ref is initiated.
 *    Implemented in kill_css().
 *
 * 2. When the percpu_ref is confirmed to be visible as killed on all CPUs
 *    and thus css_tryget_online() is guaranteed to fail, the css can be
 *    offlined by invoking offline_css().  After offlining, the base ref is
 *    put.  Implemented in css_killed_work_fn().
 *
 * 3. When the percpu_ref reaches zero, the only possible remaining
 *    accessors are inside RCU read sections.  css_release() schedules the
 *    RCU callback.
 *
 * 4. After the grace period, the css can be freed.  Implemented in
 *    css_free_work_fn().
 *
 * It is actually hairier because both step 2 and 4 require process context
 * and thus involve punting to css->destroy_work adding two additional
 * steps to the already complex sequence.
 */
static void css_free_rwork_fn(struct work_struct *work)
```

## cgorup inode

基于某一台机器测试的:

/proc/kpagecgroup

```sh
#!/usr/bin/env bash
inodes=(
	1
	1033
	1605
	20298
	2229
	2281
	23
	2479
	2615
	2657
	2768
	28041
	283
	3000
	8314
	8526
)
for i in "${inodes[@]}"; do
	echo "${i}"
	find /sys/fs/cgroup -inum "$i"
done
```

```txt
1
/sys/fs/cgroup
1033
/sys/fs/cgroup/system.slice/systemd-udevd.service
1605
/sys/fs/cgroup/system.slice/dhcpcd.service
20298
/sys/fs/cgroup/system.slice/nscd.service
2229
/sys/fs/cgroup/system.slice/sshd.service
2281
/sys/fs/cgroup/system.slice/syncthing.service
23
/sys/fs/cgroup/init.scope
2479
/sys/fs/cgroup/system.slice/docker.service
2615
/sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service
2657
/sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/init.scope
2768
/sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/app.slice/httpd.service
28041
/sys/fs/cgroup/user.slice/user-1000.slice/session-64.scope
283
/sys/fs/cgroup/dev-hugepages.mount
3000
/sys/fs/cgroup/system.slice/nix-daemon.service
8314
/sys/fs/cgroup/system.slice/system-systemd\x2dcoredump.slice
8526
/sys/fs/cgroup/system.slice/systemd-journald.service
```

## reference
[^1]: v1 https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/index.html
