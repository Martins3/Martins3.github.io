# cgroup

> Linux kernel provides support for following twelve control group subsystems:
> - cpuset - assigns individual processor(s) and memory nodes to task(s) in a group;
> - cpu - uses the scheduler to provide cgroup tasks access to the processor resources;
> - cpuacct - generates reports about processor usage by a group;
> - io - sets limit to read/write from/to block devices;
> - memory - sets limit on memory usage by a task(s) from a group;
> - devices - allows access to devices by a task(s) from a group;
> - freezer - allows to suspend/resume for a task(s) from a group;
> - net_cls - allows to mark network packets from task(s) from a group;
> - net_prio - provides a way to dynamically set the priority of network traffic per network interface for a group;
> - perf_event - provides access to perf events) to a group;
> - hugetlb - activates support for huge pages for a group;
> - pid - sets limit to number of processes in a group.
>
> from : https://0xax.gitbooks.io/linux-insides/content/Cgroups/linux-cgroups-1.html

## overview

- [ ] cset_cgroup_from_root / task_cgroup_from_root

- `css_task_iter_start`
```txt
#0  css_task_iter_start (it=0xffff88811ca47bd8, flags=3, css=0xffff888108a42000) at kernel/cgroup/cgroup.c:4925
#1  __cgroup_procs_start (s=0xffff888110735000, pos=0xffff888110735028, iter_flags=3) at kernel/cgroup/cgroup.c:5033
#2  0xffffffff814ddfed in kernfs_seq_start (sf=0xffff888110735000, ppos=0xffff888110735028) at fs/kernfs/file.c:160
#3  0xffffffff81460b08 in seq_read_iter (iocb=0xffffc90001843e98, iter=0xffffc90001843e70) at fs/seq_file.c:225
```
- [ ] `cgroup_get_from_*` : 各种类似的函数

- [ ] what's relation with `css` and `cgroup`

```plain
cgroup_mkdir         |
                     |==> cgroup_apply_control_enable ==> css_create
cgroup_apply_control |


cgroup_mkdir         |==> cgroup_create(only called here)
```

In v1, css is one-to-one mapped to cgroup because every hierarchy only support one subsystem.

- css_populate_dir : if `struct cgroup_subsys_state` doesn’t attach to any subsystem, only base file will be create.


## userland api

[Linux Cgroup 系列（01）：Cgroup 概述](https://segmentfault.com/a/1190000006917884)

挂载一颗和所有 subsystem 关联的 cgroup 树到/sys/fs/cgroup

   plain mount -t cgroup xxx /sys/fs/cgroup

挂载一颗和 cpuset subsystem 关联的 cgroup 树到/sys/fs/cgroup/cpuset

   plain mkdir /sys/fs/cgroup/cpuset
    mount -t cgroup -o cpuset xxx /sys/fs/cgroup/cpuset

挂载一颗与 cpu 和 cpuacct subsystem 关联的 cgroup 树到/sys/fs/cgroup/cpu,cpuacct

   plain mkdir /sys/fs/cgroup/cpu,cpuacct
    mount -t cgroup -o cpu,cpuacct xxx /sys/fs/cgroup/cpu,cpuacct

挂载一棵 cgroup 树，但不关联任何 subsystem，下面就是 systemd 所用到的方式

   plain mkdir /sys/fs/cgroup/systemd
    mount -t cgroup -o none,name=systemd xxx /sys/fs/cgroup/systemd


## proc


## /proc/kpagecgroup
记录每一个页所属的 kpagecgroup 的 inode

```sh
#!/usr/bin/env bash
inodes=(
	1
)
for i in "${inodes[@]}"; do
	echo "${i}"
	find /sys/fs/cgroup -inum "$i"
done
```
使用 ls -i /sys/fs/cgroup 中的目录的 cgroup 就是

### /proc/cgroups

```c
WARN_ON(!proc_create_single("cgroups", 0, NULL, proc_cgroupstats_show));
```

v1
```txt
➜  ~ cat /proc/cgroups
#subsys_name    hierarchy       num_cgroups     enabled
cpuset  7       1       1
cpu     6       1       1
cpuacct 6       1       1
blkio   11      1       1
memory  10      91      1
devices 8       46      1
freezer 14      1       1
net_cls 4       1       1
perf_event      13      1       1
net_prio        4       1       1
hugetlb 5       1       1
pids    9       55      1
rdma    2       1       1
misc    3       1       1
debug   12      1       1
```

v2
```txt
➜  ~ cat /proc/cgroups
#subsys_name    hierarchy       num_cgroups     enabled
cpuset  0       87      1
cpu     0       87      1
cpuacct 0       87      1
blkio   0       87      1
memory  0       87      1
devices 0       87      1
freezer 0       87      1
net_cls 0       87      1
perf_event      0       87      1
net_prio        0       87      1
hugetlb 0       87      1
pids    0       87      1
rdma    0       87      1
misc    0       87      1
debug   0       87      1
```

可见，在 v2 中，/proc/cgroups 是个没有必要存在的。

### /proc/pid/cgroup
proc_cgroup_show()

```txt
🧀  cat /proc/self/cgroup
0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-wezterm-5210.scope
```
```txt
➜  ~ cat /proc/self/cgroup
0::/user.slice/user-0.slice/session-3.scope
➜  ~ cgexec -g memory:mem  cat /proc/self/cgroup
0::/mem
```
但是如果是 v1 ，其结果为

guest 中
```txt
14:freezer:/
13:perf_event:/
12:debug:/
11:blkio:/
10:memory:/user.slice/user-0.slice/session-1.scope
9:pids:/user.slice/user-0.slice/session-1.scope
8:devices:/user.slice
7:cpuset:/
6:cpu,cpuacct:/
5:hugetlb:/
4:net_cls,net_prio:/
3:misc:/
2:rdma:/
1:name=systemd:/user.slice/user-0.slice/session-1.scope
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

## v1 v2
[aaa](https://chrisdown.name/talks/cgroupv2/cgroupv2-fosdem.pdf)

https://github.com/opencontainers/runc/blob/master/docs/cgroup-v2.md
> /sys/fs/cgroup/cgroup.controllers is present.

[Man cgroup(7)](https://man7.org/linux/man-pages/man7/cgroups.7.html)

*In cgroups v1, the ability to mount different controllers against
different hierarchies was intended to allow great flexibility for
application design.*  In practice, though, the flexibility turned out
to be less useful than expected, and in many cases added complexity.

[The current adoption status of cgroup v2 in containers](https://medium.com/nttlabs/cgroup-v2-596d035be4d7)
cgroup v1 has independent trees for each of controllers. eg. a process can join group "foo” for CPU (/sys/fs/cgroup/cpu/foo ) while joining group “bar” for memory ( /sys/fs/cgroup/memory/bar ).
While this design seemed to provide good flexibility, it wasn’t proved to be useful in practice.

- [x] one process can join multiple subsystem hierarchy

cgroup v2 focuses on simplicity: `/sys/fs/cgroup/cpu/$GROUPNAME` and `/sys/fs/cgroup/memory/$GROUPNAME` in v1 are now unified as `/sys/fs/cgroup/$GROUPNAME` ,
and a process can no longer join different groups for different controllers. If the process joins foo ( /sys/fs/cgroup/foo ), all controllers enabled for foo will take the control of the process.

In cgroup v2, the device access control is implemented by attaching an eBPF program (`BPF_PROG_TYPE_CGROUP_DEVICE`)to the file descriptor of `/sys/fs/cgroup/foo` directory.


[TLDR Understanding the new cgroups v2 API by Rami Rosen](https://medium.com/some-tldrs/tldr-understanding-the-new-control-groups-api-by-rami-rosen-980df476f633)

1. single hierarchy
  如图所示 ![loading](https://miro.medium.com/max/557/1*P7ZLLF_F4TMgGfaJ2XIfuQ.png)
2. You cannot attach a process to an **internal subgroup**，process 只能挂载到末端上。
3. cgroups v2, a process can belong only to a single subgroup.

- [ ] https://man7.org/conf/lca2019/cgroups_v2-LCA2019-Kerrisk.pdf
- [ ] https://lwn.net/Articles/679786/
- [ ] man : https://man7.org/linux/man-pages/man7/cgroups.7.html

[kernel doc](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)
- cgroup is largely composed of two parts - the core and controllers.
    - cgroup core is primarily responsible for **hierarchically** organizing processes.
    - A cgroup controller is usually responsible for distributing a specific type of system resource along the hierarchy although there are utility controllers which serve purposes other than resource distribution.
- cgroups form a tree structure and every process in the system belongs to one and only one cgroup.
- All threads of a process belong to the same cgroup.
- On creation, all processes are put in the cgroup that the parent process belongs to at the time.
- A process can be *migrated* to another cgroup. Migration of a process doesn’t affect already existing descendant processes.

- All controller behaviors are hierarchical - if a controller is enabled on a cgroup, it affects all processes which belong to the cgroups consisting the inclusive sub-hierarchy of the cgroup.

## cgroup 支持将进程动态的迁移进入 cgroup 中的

```sh
cgclassify -g subsystems:path_to_cgroup pidlist
```

## 一些接口的使用说明
### memory.usage_in_bytes 和 memory.memsw.usage_in_bytes 区别
在 v1 中，memsw 包括 memory + swap

v2 中使用 `memory.swap.*` 来描述。

### cgroup.procs
v1 以前叫 tasks，后来都是

### cgroup.events
```txt
populated 0
frozen 0
```
1. populated : 描述一个 cgroup 中是否存在进程
2. frozen : @todo

### cgroup.misc
细节看 : https://lwn.net/Articles/856438/

处理一些 ASID 之类的分配:

```txt
/sys/fs/cgroup/misc.capacity
user.slice/misc.current
user.slice/misc.events
user.slice/misc.max
```

## 其他的实现
- ./mm-memcontrol.md
- ./storage-blk-wbt.md
- ./sched-cgroup-cpuset.md
- ./sched-cgroup-cpu.md
- hugepage_cgroup
- blk-throttle.c

- mm/page_counter.c 是 memcontrol 和 hugepage_cgroup 的公共部分组件

## reference

- [内核 v1](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/index.html)

## 这个函数做啥用的
```c
static int cgroup_apply_control(struct cgroup *cgrp)
```

## 如何理解
这几个结构体的基本关系:
	struct cgroup_subsys_state *d_css;
	struct cgroup *dsct;
	struct css_set *src_cset;
cgroup_root

```c
/*
 * The default css_set - used by init and its children prior to any
 * hierarchies being mounted. It contains a pointer to the root state
 * for each subsystem. Also used to anchor the list of css_sets. Not
 * reference-counted, to improve performance when child cgroups
 * haven't been created.
 */
struct css_set init_css_set = {
	.refcount		= REFCOUNT_INIT(1),
	.dom_cset		= &init_css_set,
	.tasks			= LIST_HEAD_INIT(init_css_set.tasks),
	.mg_tasks		= LIST_HEAD_INIT(init_css_set.mg_tasks),
	.dying_tasks		= LIST_HEAD_INIT(init_css_set.dying_tasks),
	.task_iters		= LIST_HEAD_INIT(init_css_set.task_iters),
	.threaded_csets		= LIST_HEAD_INIT(init_css_set.threaded_csets),
	.cgrp_links		= LIST_HEAD_INIT(init_css_set.cgrp_links),
	.mg_src_preload_node	= LIST_HEAD_INIT(init_css_set.mg_src_preload_node),
	.mg_dst_preload_node	= LIST_HEAD_INIT(init_css_set.mg_dst_preload_node),
	.mg_node		= LIST_HEAD_INIT(init_css_set.mg_node),

	/*
	 * The following field is re-initialized when this cset gets linked
	 * in cgroup_init().  However, let's initialize the field
	 * statically too so that the default cgroup can be accessed safely
	 * early during boot.
	 */
	.dfl_cgrp		= &cgrp_dfl_root.cgrp,
};
```
