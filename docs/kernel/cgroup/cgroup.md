# cgroup

## 用户态简单介绍
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

- systemd-cgls : 从 systemd 的架构展示 cgroup 的控制关系。
- systemd-cgtop

## overview

- `css_task_iter_start`
```txt
#0  css_task_iter_start (it=0xffff88811ca47bd8, flags=3, css=0xffff888108a42000) at kernel/cgroup/cgroup.c:4925
#1  __cgroup_procs_start (s=0xffff888110735000, pos=0xffff888110735028, iter_flags=3) at kernel/cgroup/cgroup.c:5033
#2  0xffffffff814ddfed in kernfs_seq_start (sf=0xffff888110735000, ppos=0xffff888110735028) at fs/kernfs/file.c:160
#3  0xffffffff81460b08 in seq_read_iter (iocb=0xffffc90001843e98, iter=0xffffc90001843e70) at fs/seq_file.c:225
```


## 基础查询函数
- cgroup_get_from_fd
- cgroup_get_from_file
- cgroup_get_from_id
- cgroup_get_from_path

- task_cgroup_from_root
- cset_cgroup_from_root

## 创建
- cgroup_mkdir
    - cgroup_create
    - cgroup_apply_control_enable : 对于每一个层次 和 controller 创建
      - css_populate_dir : 填充文件, if `struct cgroup_subsys_state` doesn’t attach to any subsystem, only base file will be create.
      - css_create : 创建 css

- 因为 threaded mode 导致 css_set 和 cgroup 不是完全对应的，其实一个 cgroup 关联的 css 可能是从多个 cgroup 中找到的 subset 的
- find_css_set : 首先查询，如果无法查询到，那么就创建
```txt
#0  find_css_set (old_cset=old_cset@entry=0xffff888106ce3c00, cgrp=0xffff8881068f1000) at kernel/cgroup/cgroup.c:1208
#1  0xffffffff81230a58 in cgroup_migrate_prepare_dst (mgctx=mgctx@entry=0xffffc90001d5bd30) at kernel/cgroup/cgroup.c:2814
#2  0xffffffff81231b0b in cgroup_attach_task (dst_cgrp=dst_cgrp@entry=0xffff8881068f1000, leader=leader@entry=0xffff88810d20a300, threadgroup=threadgroup@entry=true) at kernel/cgroup/cgroup.c:2918
#3  0xffffffff81233f34 in __cgroup_procs_write (of=0xffff888105ebc900, buf=<optimized out>, threadgroup=threadgroup@entry=true) at kernel/cgroup/cgroup.c:5172
#4  0xffffffff81234007 in cgroup_procs_write (of=<optimized out>, buf=<optimized out>, nbytes=4, off=<optimized out>) at kernel/cgroup/cgroup.c:5185
#5  0xffffffff814de799 in kernfs_fop_write_iter (iocb=0xffffc90001d5bea0, iter=<optimized out>) at fs/kernfs/file.c:334
```

## proc

### [ ] /sys/kernel/cgroup/delegate
不太懂，delegate 是什么意思


### /proc/kpagecgroup
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

## v1 v2 的差别
### 如何切换 cgroup v2 来测试
检测当前是那个版本: https://kubernetes.io/docs/concepts/architecture/cgroups/

```sh
stat -fc %T /sys/fs/cgroup/
```
- tmpfs : v1
- cgroup2fs : v2

```sh
sudo grubby --update-kernel=ALL --args=systemd.unified_cgroup_hierarchy=1
```

老版本的 libcgroup 不能支持 cgroup v2 :
```txt
➜ sudo cgcreate -g cpu:A

[sudo] password for martins3:
cgcreate: libcgroup initialization failed: Cgroup is not mounted
```


可以用来搞清楚，那些代码是 v1 的，那些是 v2 的。

- [cgroupv2: Linux’s new unified control group system](https://chrisdown.name/talks/cgroupv2/cgroupv2-fosdem.pdf)

1. unified hierarchy: a process can belong only to a single subgroup.
  - All controller behaviors are hierarchical - if a controller is enabled on a cgroup, it affects all processes which belong to the cgroups consisting the inclusive sub-hierarchy of the cgroup.
  如图所示 ![loading](https://miro.medium.com/max/557/1*P7ZLLF_F4TMgGfaJ2XIfuQ.png)
2. 使用类型 BPF_PROG_TYPE_CGROUP_DEVICE 来控制 bpf 程序
3. You cannot attach a process to an **internal subgroup**，process 只能挂载到末端上。
4. All threads of a process belong to the same cgroup.


1. https://github.com/opencontainers/runc/blob/master/docs/cgroup-v2.md
  -  /sys/fs/cgroup/cgroup.controllers is present.

- [](https://lwn.net/Articles/679786/)

[Man cgroup(7)](https://man7.org/linux/man-pages/man7/cgroups.7.html)

[TLDR Understanding the new cgroups v2 API by Rami Rosen](https://medium.com/some-tldrs/tldr-understanding-the-new-control-groups-api-by-rami-rosen-980df476f633)



[kernel doc](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)


## 一些接口的使用说明
### memory.usage_in_bytes 和 memory.memsw.usage_in_bytes 区别
在 v1 中，memsw 包括 memory + swap

v2 中使用 `memory.swap.*` 来描述。

###  cgroup.controllers 和  cgroup.subtree_control
前者只读，后者控制，来控制一个 cgroup 中存在那些 controller

### cgroup.procs
v1 以前叫 tasks，后来都是

### cgroup.events
```txt
populated 0
frozen 0
```
1. populated : 描述一个 cgroup 中是否存在进程
2. frozen : @todo

可以通过 epoll 来监听这些文件，从而判断那些 cgroup 变为空了，内部靠 `cgroup_file_notify` 来实现。

### cgroup.misc
细节看 : https://lwn.net/Articles/856438/

处理一些 ASID 之类的分配:

```txt
/sys/fs/cgroup/misc.capacity
user.slice/misc.current
user.slice/misc.events
user.slice/misc.max
```


## reference

- [内核 v1](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/index.html)
- [内核 v2](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)
- [Man cgroup(7)](https://man7.org/linux/man-pages/man7/cgroups.7.html)
- [What’s new in control groups](https://man7.org/conf/lca2019/cgroups_v2-LCA2019-Kerrisk.pdf)
  - 分析 v2 中的特性，而且提到了一些高级特性，delegation 和 thread mode，这个 slides 常看常新。

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

## controller 的实现分析
- ./mm-memcontrol.md
- ./storage-blk-wbt.md
- ./sched-cgroup-cpuset.md
- ./sched-cgroup-cpu.md
- hugepage_cgroup
- blk-throttle.c
- mm/page_counter.c 是 memcontrol 和 hugepage_cgroup 的公共部分组件

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
