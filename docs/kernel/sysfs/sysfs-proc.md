# proc fs

## 文档
- https://www.kernel.org/doc/html/latest/filesystems/proc.html

## 初始化流程
- proc_root_init

## /proc/$pid

```txt
 attr
 autogroup
 auxv
 cgroup
 clear_refs
 cmdline
 comm
 coredump_filter
 cpuset
 cwd -> /proc/323954
 environ
 exe -> /usr/bin/zsh
 fd
 fdinfo
 gid_map
 idle_pages
 io
 limits
 loginuid
 map_files
 maps
 mem
 mountinfo
 mounts
 mountstats
 net
 ns
 numa_maps
 oom_adj
 oom_score
 oom_score_adj
 pagemap
 patch_state
 personality
 projid_map
 root -> /
 sched
 schedstat
 sessionid
 setgroups
 smaps
 smaps_rollup
 stack
 stat
 statm
 status
 swap_pages
 syscall
 task
 timers
 timerslack_ns
 uid_map
 wchan
```
- status : 这个比较麻烦了 proc_pid_status


- statm : 内存相关统计，参考 task_statm
- setgroups : 和 config USER_NS 有关
- autogroup : sched_autogroup_show
- syscall :  proc_pid_syscall 记录当前进行的系统调用的参数
- loginuid : audit 机制
- personality : 就是展示 task_struct::personality

- mounts
- mountinfo  : 似乎是 mounts 的升级版，似乎主要是容器的需求


## /proc/fs 基本代码分析

关键的几个文件:
- base.c : 进程相关
- array.c : 和 process 相关
- task_mmu.c
  - 实现 /proc/1493/pagemap : 参考 https://www.kernel.org/doc/Documentation/vm/pagemap.txt
- generic.c : proc 文件系统的维护，创建目录之类的
  - 使用红黑树来存放 dentry : pde_subdir_insert
- /proc/self 是如何实现的 : fs/proc/self.c::proc_self_get_link 中，也就是实现一个软链接即可

看一个经典的路径吧:
```txt
#0  proc_pid_lookup (dentry=dentry@entry=0xffff88810577f680, flags=flags@entry=17) at fs/proc/base.c:3436
#1  0xffffffff814c44b1 in proc_root_lookup (dir=0xffff88810caa0048, dentry=0xffff88810577f680, flags=17) at fs/proc/root.c:324
#2  0xffffffff81439e81 in __lookup_slow (name=name@entry=0xffffc90001df3dd0, dir=dir@entry=0xffff88810c9315c0, flags=flags@entry=17) at fs/namei.c:1686
#3  0xffffffff8143e165 in lookup_slow (flags=17, dir=0xffff88810c9315c0, name=0xffffc90001df3dd0) at fs/namei.c:1703
#4  walk_component (nd=0xffffc90001df3dc0, flags=0) at fs/namei.c:1994
#5  0xffffffff8143e3da in link_path_walk (name=0xffff88810b8d002b "oom_score_adj", name@entry=0xffff88810b8d0020 "/proc/self/oom_score_adj", nd=nd@entry=0xffffc90001df3dc0) atfs/namei.c:2318
#6  0xffffffff8143ec13 in link_path_walk (nd=0xffffc90001df3dc0, name=0xffff88810b8d0020 "/proc/self/oom_score_adj") at ./include/linux/err.h:36
#7  path_openat (nd=nd@entry=0xffffc90001df3dc0, op=op@entry=0xffffc90001df3edc, flags=flags@entry=65) at fs/namei.c:3711
#8  0xffffffff81440d06 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff88810b8d0000, op=op@entry=0xffffc90001df3edc) at fs/namei.c:3742
#9  0xffffffff81427aea in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90001df3f18) at fs/open.c:1356
#10 0xffffffff81427fe7 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1372
#11 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1388
#12 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1383
#13 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1383
#14 0xffffffff822a0f0c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001df3f58) at arch/x86/entry/common.c:50
#15 do_syscall_64 (regs=0xffffc90001df3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#16 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#17 0x0000000000000000 in ?? ()
```

## /proc/sys

- 文档 : https://www.kernel.org/doc/html/latest/admin-guide/sysctl/index.html
  -  https://www.kernel.org/doc/Documentation/sysctl/kernel.txt

例如 /proc/sys/dev/scsi/logging_level 是可以通过 sysctl -q -w dev.scsi.logging_level=0 来操作的

sysctl 来操作的似乎都是 proc 下的这些接口，但是如果是 sys 下的接口，例如类似这种
/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/scsi_disk/7:0:0:0/max_retries
似乎不可以操作的，通过 sysctl 操作都是放到 /proc/sys 下面的

才意识到 ctl_table 就是这个目录下的
提供了 sysctl 的操作

```txt
/proc/sys/dev
├── hpet
│   └── max-user-freq
├── i915
│   ├── oa_max_sample_rate
│   └── perf_stream_paranoid
├── mac_hid
│   ├── mouse_button2_keycode
│   ├── mouse_button3_keycode
│   └── mouse_button_emulation
├── scsi
│   └── logging_level
├── tty
│   ├── ldisc_autoload
│   └── legacy_tiocsti
└── xe
    └── observation_paranoid
```
似乎这个东西就是不可以替代的，通过 sysfs 的配置都是和具体的 kobj 关联的
而 /proc/sys 下可以配置全局内容。

## /proc/devices

- fs/proc/devices.c
  - `chrdev_show`
  - `blkdev_show`

## /proc/kcore

2. dynamic kernel image : 128T

```plain
➜  .SpaceVim.d git:(master) ✗ l /proc/kcore
.r-------- root root 128 TB Tue Mar 17 18:47:48 2020   kcore
```

## /proc/sysrq-trigger

https://www.kernel.org/doc/html/latest/admin-guide/sysrq.html

### g : Used by kgdb (kernel debugger)

### b : 直接 reboot

这个不会清理 cache 的，最后调用到这里:

```c
struct machine_ops machine_ops __ro_after_init = {
	.power_off = native_machine_power_off,
	.shutdown = native_machine_shutdown,
	.emergency_restart = native_machine_emergency_restart,
	.restart = native_machine_restart,
	.halt = native_machine_halt,
#ifdef CONFIG_KEXEC_CORE
	.crash_shutdown = native_machine_crash_shutdown,
#endif
};
```

如果是 `shutdown now` 会存在这个调用

```txt
#0  native_machine_power_off () at arch/x86/kernel/reboot.c:737
#1  0xffffffff8117884e in __do_sys_reboot (magic1=-18751827, magic2=<optimized out>, cmd=1126301404, arg=0x7fcbe9d289e0) at kernel/reboot.c:753
#2  0xffffffff8227db9f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#3  do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#4  0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 理解 hugetlb_sysctl_handler_common

```c
static int hugetlb_sysctl_handler_common(bool obey_mempolicy,
			 struct ctl_table *table, int write,
			 void *buffer, size_t *length, loff_t *ppos)
{
	struct hstate *h = &default_hstate;
	unsigned long tmp = h->max_huge_pages;
	int ret;

	if (!hugepages_supported())
		return -EOPNOTSUPP;

	ret = proc_hugetlb_doulongvec_minmax(table, write, buffer, length, ppos,
					     &tmp);
	if (ret)
		goto out;

	if (write)
		ret = __nr_hugepages_store_common(obey_mempolicy, h,
						  NUMA_NO_NODE, tmp, *length);
out:
	return ret;
}
```

## sysctl

1. 所以和fs/sysfs的关系是什么，一个实现/proc 一个实现 /sys/ 的吗 ?
2. 前面2000行都是各种定义蛇皮表格! @todo 通过表格，但是如何形成 fs 的 tree ?
3. handler 最后是如何被调用的呢 ? 或者向 /proc/sys/vm/compact_memory 中间写入1，那么最后如何实现将handler 调用，以及!

```c
int __init sysctl_init(void)
{
	struct ctl_table_header *hdr;

  // TODO sysctl_base_table 中间的子项目不知道对应的内容，至少不是/proc
	hdr = register_sysctl_table(sysctl_base_table);
	kmemleak_not_leak(hdr);
	return 0;
}
// fs/proc/proc_sysctl.c 中间，可以确定这个文件只是proc/sys 的部分内容
// 但是我ls 出来的比 sysctl_base_table 的内容更多啊! 比如缺少关键的net !
// TODO 而且有的tbale 是空的 !
// net 具体的内容被放到网络中间了 !
int __init proc_sys_init(void)
{
	struct proc_dir_entry *proc_sys_root;

	proc_sys_root = proc_mkdir("sys", NULL);
	proc_sys_root->proc_iops = &proc_sys_dir_operations;
	proc_sys_root->proc_fops = &proc_sys_dir_file_operations;
	proc_sys_root->nlink = 0;

	return sysctl_init();
}
```

> 还包含一些字符串，数值，bitmap 等各种io的处理
```c
// 一个proc_handler 可能用于 /proc/sys/kernel/hostname 之类的保存一个全局变量的之类的操作吧 !
typedef int proc_handler (struct ctl_table *ctl, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos);

int proc_dostring(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	if (write)
		proc_first_pos_non_zero_ignore(ppos, table);

	return _proc_do_string((char *)(table->data), table->maxlen, write,
			       (char __user *)buffer, lenp, ppos);
}

int proc_douintvec(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp, loff_t *ppos)
```

### sysctl 的基本操作

重新加载配置文件
```txt
sudo sysctl -p
```

## sysctl
理解 /proc/sys 目录 ，利用 m/sysctl.c 来理解下

- [ ] /etc/sysctl.d

/proc/sys/kernel
```txt
@[
    __register_sysctl_table+5
    neigh_sysctl_register+286
    devinet_sysctl_register+85
    inetdev_init+237
    inetdev_event+777
    notifier_call_chain+90
    register_netdevice+1675
    __tun_chr_ioctl+3364
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    __register_sysctl_table+5
    __devinet_sysctl_register+203
    devinet_sysctl_register+148
    inetdev_init+237
    inetdev_event+777
    notifier_call_chain+90
    register_netdevice+1675
    __tun_chr_ioctl+3364
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    __register_sysctl_table+5
    neigh_sysctl_register+286
    addrconf_sysctl_register+90
    ipv6_add_dev+743
    addrconf_notify+430
    notifier_call_chain+90
    register_netdevice+1675
    __tun_chr_ioctl+3364
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    __register_sysctl_table+5
    __addrconf_sysctl_register+227
    addrconf_sysctl_register+150
    ipv6_add_dev+743
    addrconf_notify+430
    notifier_call_chain+90
    register_netdevice+1675
    __tun_chr_ioctl+3364
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 2
```

不过这个到底在搞什么东西啊
```txt
🧀  sudo trace '__register_sysctl_table "%s", arg2'
[sudo] password for martins3:
PID     TID     COMM            FUNC             -
8106    8106    msedge          __register_sysctl_table kernel
22647   22647   slack           __register_sysctl_table user
22651   22651   slack           __register_sysctl_table user
22647   22647   slack           __register_sysctl_table user
22647   22647   slack           __register_sysctl_table kernel
22647   22647   slack           __register_sysctl_table net/netfilter
22647   22647   slack           __register_sysctl_table net/netfilter/nf_log
22647   22647   slack           __register_sysctl_table net/core
22647   22647   slack           __register_sysctl_table net/ipv4/conf/all
22647   22647   slack           __register_sysctl_table net/ipv4/conf/default
22647   22647   slack           __register_sysctl_table net/ipv4
22647   22647   slack           __register_sysctl_table net/core
22647   22647   slack           __register_sysctl_table net/ipv4
22647   22647   slack           __register_sysctl_table net/ipv4/route
22647   22647   slack           __register_sysctl_table net/mptcp
22647   22647   slack           __register_sysctl_table net/ipv4
22647   22647   slack           __register_sysctl_table net/unix
22647   22647   slack           __register_sysctl_table net/ipv4
22647   22647   slack           __register_sysctl_table net/ipv6
22647   22647   slack           __register_sysctl_table net/ipv6/conf/all
22647   22647   slack           __register_sysctl_table net/ipv6/conf/default
22647   22647   slack           __register_sysctl_table net/ipv6
22647   22647   slack           __register_sysctl_table net/ipv6
22647   22647   slack           __register_sysctl_table net/ipv6/route
22647   22647   slack           __register_sysctl_table net/ipv6/icmp
22647   22647   slack           __register_sysctl_table net/netfilter
22647   22647   slack           __register_sysctl_table net/netfilter
22647   22647   slack           __register_sysctl_table net/bridge
22647   22647   slack           __register_sysctl_table net/ipv4/neigh/lo
22647   22647   slack           __register_sysctl_table net/ipv4/conf/lo
22647   22647   slack           __register_sysctl_table net/ipv6/neigh/lo
22647   22647   slack           __register_sysctl_table net/ipv6/conf/lo
22653   22653   slack           __register_sysctl_table user
22655   22655   slack           __register_sysctl_table kernel
23010   23010   msedge          __register_sysctl_table user
23025   23025   msedge          __register_sysctl_table user
23010   23010   msedge          __register_sysctl_table user
23010   23010   msedge          __register_sysctl_table kernel
23010   23010   msedge          __register_sysctl_table net/netfilter
23010   23010   msedge          __register_sysctl_table net/netfilter/nf_log
23010   23010   msedge          __register_sysctl_table net/core
23010   23010   msedge          __register_sysctl_table net/ipv4/conf/all
23010   23010   msedge          __register_sysctl_table net/ipv4/conf/default
23010   23010   msedge          __register_sysctl_table net/ipv4
23010   23010   msedge          __register_sysctl_table net/core
23010   23010   msedge          __register_sysctl_table net/ipv4
23010   23010   msedge          __register_sysctl_table net/ipv4/route
23010   23010   msedge          __register_sysctl_table net/mptcp
23010   23010   msedge          __register_sysctl_table net/ipv4
23010   23010   msedge          __register_sysctl_table net/unix
23010   23010   msedge          __register_sysctl_table net/ipv4
23010   23010   msedge          __register_sysctl_table net/ipv6
23010   23010   msedge          __register_sysctl_table net/ipv6/conf/all
23010   23010   msedge          __register_sysctl_table net/ipv6/conf/default
23010   23010   msedge          __register_sysctl_table net/ipv6
23010   23010   msedge          __register_sysctl_table net/ipv6
23010   23010   msedge          __register_sysctl_table net/ipv6/route
23010   23010   msedge          __register_sysctl_table net/ipv6/icmp
23010   23010   msedge          __register_sysctl_table net/netfilter
23010   23010   msedge          __register_sysctl_table net/netfilter
23010   23010   msedge          __register_sysctl_table net/bridge
23010   23010   msedge          __register_sysctl_table net/ipv4/neigh/lo
23010   23010   msedge          __register_sysctl_table net/ipv4/conf/lo
23010   23010   msedge          __register_sysctl_table net/ipv6/neigh/lo
23010   23010   msedge          __register_sysctl_table net/ipv6/conf/lo
23027   23027   msedge          __register_sysctl_table user
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
8106    8106    msedge          __register_sysctl_table kernel
```

似乎是用来构建这个 table 出来的
```txt
🧀  ls
anycast_delay           interval_probe_time_ms  retrans_time
app_solicit             locktime                retrans_time_ms
base_reachable_time     mcast_resolicit         ucast_solicit
base_reachable_time_ms  mcast_solicit           unres_qlen
delay_first_probe_time  proxy_delay             unres_qlen_bytes
gc_stale_time           proxy_qlen
ipv4/neigh/lo🔒 🔥
🧀  pwd
/proc/sys/net/ipv4/neigh/lo
```

原来 sched 下的 sysctl table 移动到了 debugfs 了

```txt
commit 3b87f136f8fccddf7da016ab7d04bb3cf9b180f0
Author: Peter Zijlstra <peterz@infradead.org>
Date:   Thu Mar 25 11:31:20 2021 +0100

    sched,debug: Convert sysctl sched_domains to debugfs

    Stop polluting sysctl, move to debugfs for SCHED_DEBUG stuff.

    Signed-off-by: Peter Zijlstra (Intel) <peterz@infradead.org>
    Reviewed-by: Dietmar Eggemann <dietmar.eggemann@arm.com>
    Reviewed-by: Valentin Schneider <valentin.schneider@arm.com>
    Tested-by: Valentin Schneider <valentin.schneider@arm.com>
    Link: https://lkml.kernel.org/r/YHgB/s4KCBQ1ifdm@hirez.programming.kicks-ass.net
```

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
