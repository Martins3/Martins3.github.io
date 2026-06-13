# sched 相关的问题

## 之前的几个问题
```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight; // 难道 bandwidth 使用的 ?
	struct rb_node			run_node;
	struct list_head		group_node; // task group ?
	unsigned int			on_rq; // why not boolean ?

  // @todo how runtime works ?
	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	u64				nr_migrations;
```
> 1. load 和 runnable_weight 之间的关系是什么 ?

## 参考下这个文章
https://github.com/tontinton/sched_animation

## 当时记录个问题
```c
/*
 * Pick up the highest-prio task:
 */
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{

	/*
	 * Optimization: we know that if all tasks are in the fair class we can
	 * call that function directly, but only if the @prev task wasn't of a
	 * higher scheduling class, because otherwise those loose the
	 * opportunity to pull in more work from other CPUs.
	 */
	if (likely((prev->sched_class == &idle_sched_class ||
		    prev->sched_class == &fair_sched_class) &&
		   rq->nr_running == rq->cfs.h_nr_running)) {

    // TODO 似乎 rq 和 sched_class 是分开的 ?
    // 不然这个参数根本没有必要使用rq
    // 但是 rq 中间持有 rbtree
		p = fair_sched_class.pick_next_task(rq, prev, rf);
```


## 还是这个问题

如何将 sched class 装换为
```txt
/*
 * The order of the sched class addresses are important, as they are
 * used to determine the order of the priority of each sched class in
 * relation to each other.
 */
#define SCHED_DATA				\
	STRUCT_ALIGN();				\
	__sched_class_highest = .;		\
	*(__stop_sched_class)			\
	*(__dl_sched_class)			\
	*(__rt_sched_class)			\
	*(__fair_sched_class)			\
	*(__martins3_sched_class)			\
	*(__idle_sched_class)			\
	__sched_class_lowest = .;
```

## 这个对比的很好了
如何评价华为称鸿蒙内核已超越 Linux 内核？ - 王飞的回答 - 知乎
https://www.zhihu.com/question/659531635/answer/3538601233

## 了解下 ionice 和 renice 这两个工具

## 似乎 isolcpus 已经被删除了
isolcpus=1-9 nohz_full=1-9 rcu_nocbs=1-9

## 关于 cpuset 的问题，似乎 isolcpus=2-7 rcu_nocbs=2-7 nohz_full=2-7 似乎第一个参数已经包含了后面的工作

## (pid) pid sid gid uid 之类到底都是啥 ?

```c
struct audit_context
...
	pid_t		    pid, ppid;
	kuid_t		    uid, euid, suid, fsuid;
	kgid_t		    gid, egid, sgid, fsgid;
	unsigned long	    personality;
	int		    arch;

	pid_t		    target_pid;
	kuid_t		    target_auid;
	kuid_t		    target_uid;
	unsigned int	    target_sessionid;
	u32		    target_sid;
...
```

https://lwn.net/Articles/893565/

```c
static inline bool thread_group_leader(struct task_struct *p)
{
	return p->exit_signal >= 0;
}
```
- [ ] exit_signal

- [ ] real_parent
```c
			list_add_tail(&p->sibling, &p->real_parent->children); // 这些成员的含义是什么 ?
```

## 在虚拟机观测到这个错误，这是什么原因
```txt
[322877.066129] ------------[ cut here ]------------
[322877.066143] cfs_rq->avg.load_avg || cfs_rq->avg.util_avg || cfs_rq->avg.runnable_avg
[322877.066176] WARNING: CPU: 28 PID: 187116 at kernel/sched/fair.c:3307 update_blocked_averages+0x6cd/0x720
[322877.066425] Workqueue:  0x0 (xprtiod)
[322877.066457] RIP: 0010:update_blocked_averages+0x6cd/0x720
[322877.066465] Code: 80 3f 5c 9b c6 05 ae 8b df 01 01 e8 19 67 aa 00 0f 0b e9 2b fa ff ff 48 c7 c7 68 43 5c 9b c6 05 90 8b df 01 01 e8 ff 66 aa 00 <0f> 0b 41 8b 87 38 01 00 00 e9 11 fc ff ff 48 c7 c7 e8 3e 5c 9b c6
[322877.066466] RSP: 0018:ffffb516057f7d28 EFLAGS: 00010092
[322877.066468] RAX: 0000000000000048 RBX: 0000000000000001 RCX: 0000000000000027
[322877.066470] RDX: ffff91cf6c918a08 RSI: 0000000000000001 RDI: ffff91cf6c918a00
[322877.066471] RBP: ffff91cf6c92bec0 R08: 0000000000000000 R09: ffffb516057f7b60
[322877.066472] R10: ffffb516057f7b58 R11: ffff91cf7ffb1fe8 R12: ffff91cf6c92b880
[322877.066474] R13: 00000000000000e0 R14: ffff91cf6c92c000 R15: ffff91cf6c92b740
[322877.066475] FS:  0000000000000000(0000) GS:ffff91cf6c900000(0000) knlGS:0000000000000000
[322877.066476] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[322877.066478] CR2: 00007f868c5d0038 CR3: 00000001677da003 CR4: 00000000001706e0
[322877.066481] DR0: 0000000000000000 DR1: 0000000000000000 DR2: 0000000000000000
[322877.066487] DR3: 0000000000000000 DR6: 00000000fffe0ff0 DR7: 0000000000000400
[322877.066491] Call Trace:
[322877.066518]  newidle_balance+0x18f/0x3d0
[322877.066533]  pick_next_task_fair+0x39/0x3b0
[322877.066537]  __schedule+0x16b/0x1500
[322877.066576]  ? xs_poll_check_readable+0x2e/0x60 [sunrpc]
[322877.066736]  ? xs_stream_data_receive_workfn+0x81/0x100 [sunrpc]
[322877.066756]  ? __cond_resched+0x16/0x40
[322877.066759]  schedule+0x44/0xa0
[322877.066761]  worker_thread+0xd0/0x3e0
[322877.066773]  ? process_one_work+0x390/0x390
[322877.066775]  kthread+0x127/0x150
[322877.066806]  ? set_kthread_struct+0x40/0x40
[322877.066809]  ret_from_fork+0x22/0x30
[322877.066832] ---[ end trace 5eec70ea6bc97993 ]---
[407944.729545] loop2: detected capacity change from 0 to 8366004
```

## kernel/taskstats.c 和 kernel/delayacct.c 是如何使用的
https://www.kernel.org/doc/html/latest/accounting/index.html

~/data/linux/tools/accounting 下的两个工具都使用的不正常

- https://www.uninformativ.de/blog/postings/2024-08-03/0/POSTING-en.html
- [What is the use of stop_sched_class in linux kernel](https://stackoverflow.com/questions/15399782/what-is-the-use-of-stop-sched-class-in-linux-kernel)
- https://github.com/CachyOS/linux-cachyos
    - 里面的各种介绍非常的诱人啊
- https://mp.weixin.qq.com/s/87pSYQBT_q8LIThOquLWKQ

## 这个文件是做啥的
kernel/latencytop.c

## 关键 TODO
- 将 Documentation 都浏览一下

## MLFQ : 多级反馈队列 是什么?

## [ ]  如何实现在 sysctl_sched_latency 时间内中所有的程序都可以运行的

## 似乎 soft irq 的 sched 都是 CPU 0 来执行的

```txt
- __softirqentry_text_start
   - 72.17% run_rebalance_domains
      - 72.06% _nohz_idle_balance
         - 50.68% update_nohz_stats
              update_blocked_averages
         - 17.60% rebalance_domains
            - 14.36% load_balance
               - 12.35% find_busiest_group
                    1.71% find_next_and_bit
                    1.71% idle_cpu
                    0.70% update_group_capacity
              0.71% __bitmap_and
           1.06% idle_cpu
           0.59% update_blocked_averages
```
因为 CPU 0 的时钟的中断最多吗?

但是主线上执行 softirq SCHED 所在的 CPU 不会那么集中到 CPU 0 上

## softirq 在那个 CPU 上运行有什么规律吗?
似乎默认都是在 CPU 0 上执行的

## 使用这个来分析一下 PREEMPT_LAZY
- https://mp.weixin.qq.com/s/y0uYzBkQm58tDZAAk_aeww

## 调查一下 kernel/taskstats.c 是做什么的

1. 这又是一个利用 netlink 吗 ?  genl 这一个库可以了解一下。
2. 用户层次的内容是什么 ?
    1. 用这些内容干什么 ? 诊断什么错误 ?
3. 怎么收集的内容，收集什么内容

相关文档:
1. Documentation
2. tools/accounting/getdelays.c

## 看看
https://mp.weixin.qq.com/s/pCg7XwSBIRktIF_27ZSSCg

## 很好
CFS与EEVDF - Mr.迷的文章 - 知乎
https://zhuanlan.zhihu.com/p/1890881623353974887

## 为什么 CPU core 总是在到处跑，上下文切换不是需要时间吗?

1. sysbench 在物理机上，会如何?
2. sysbench 在虚拟机中，虚拟机的 -smp 和物理机的 core 数量相同，会如何

## 这个好看
https://mp.weixin.qq.com/s/pCg7XwSBIRktIF_27ZSSCg


https://mp.weixin.qq.com/s/cO7YPXKsSv2wxBChFIlNaw

## 这个看看
https://mp.weixin.qq.com/s/g07Zsv1MwrUSJ-ETkfYvjQ


https://mp.weixin.qq.com/s/q946EW1AggkpyZATGD-jVg

https://mp.weixin.qq.com/s/Ly7BJdI4Zk5gR4nQesHvMA

https://mp.weixin.qq.com/s/qZe0UfzUZpOxczSQh-VNFw


SMP负载均衡调度 - 向大佬们学习的文章 - 知乎
https://zhuanlan.zhihu.com/p/1932364777734246676

https://github.com/masoncl/rsched

## 这个是做什么的?
```txt
@[
        check_preempt_wakeup_fair+5
        wakeup_preempt+101
        ttwu_do_activate+138
        try_to_wake_up+506
        kick_pool+96
        __queue_work+630
        mod_delayed_work_on+161
        kblockd_mod_delayed_work_on+27
        blk_mq_dispatch_plug_list+758
        blk_mq_flush_plug_list+61
        __blk_flush_plug+243
        io_schedule+65
        rq_qos_wait+188
        wbt_wait+163
        __rq_qos_throttle+36
        blk_mq_submit_bio+546
        __submit_bio+117
        __submit_bio_noacct+145
        xfs_buf_submit_bio+351
        xfs_buf_delwri_submit_nowait+182
        xfsaild_push+490
        xfsaild+180
        kthread+236
        ret_from_fork+49
        ret_from_fork_asm+26
]: 12
```

## config SCHED_PROXY_EXEC 是什么，找找内核文档看看


## 看看
https://mp.weixin.qq.com/s/I4pYwyHiUcHFlSQxuvR9dA


https://mp.weixin.qq.com/s/1Ryi1RNjPRSUb5KH-0BLuA


https://mp.weixin.qq.com/s/FcbT_DrLNTVqqe4_MvDWxQ

## 内部的这个文档先看懂了吧
https://docs.google.com/document/d/1uATZ3ItC0XAJGq95vCWA6DTMuaedvV0c2IwV9yLJiyo/edit?tab=t.0#heading=h.feb6l5x4y4vk

## 先测试一下 rt 的功能吧

## 调度系统处理大核和小核之类的代码是完全每看到再那里
可以找到对应的代码吗?

https://mp.weixin.qq.com/s/ndHO5kB2JX5u6mFm1XuYIw
https://mp.weixin.qq.com/s/kyj6MzN0ihyDtt2Bmfh8sg
https://mp.weixin.qq.com/s/mHkL5Bh5Te3DY1QMbj7asg

## schedstat 还可以做这个事情啊
<!-- d301ad3d-5c34-471d-b083-24fadfc9e1e9 -->
```txt
cpu ready 这个是 elf_vm_cpu_overall_steal_time_percent 指标，采集来源是宿主机上的 /proc/{}/schedstat 文件，和虚拟机内部的 steal time 属于同一件事的不同层面观测结果，预期是要对得上的
```

## 经典中的经典
https://uni.bluepuni.com/archives/the-linux-scheduler-a-decade-of-wasted-cores/

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
