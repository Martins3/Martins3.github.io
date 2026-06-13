# psi

## 先仔细看看文档
- https://docs.kernel.org/accounting/psi.html

> Monitors activate only when system enters stall state for the monitored psi metric
> and deactivates upon exit from the stall state.
> While system is in the stall state psi signal growth is monitored at a rate of 10 times per tracking window.
>
> The kernel accepts window sizes ranging from 500ms to 10s, therefore min monitoring update interval is 50ms and max is 1s.
> Min limit is set to prevent overly frequent polling.
> Max limit is chosen as a high enough number after which monitors are most likely not needed and psi averages can be used instead.

psi_task_switch
psi_group_change
psi_rtpoll_work : rt 是什么含义?
psi_cgroup_restart

## 使用接口

- /proc/pressure/
- /sys/fs/cgroup/cg1/io.pressure

## 代码分析
```c
/* Tracked task states */
enum psi_task_count {
	NR_IOWAIT,
	NR_MEMSTALL,
	NR_RUNNING,
	/*
	 * For IO and CPU stalls the presence of running/oncpu tasks
	 * in the domain means a partial rather than a full stall.
	 * For memory it's not so simple because of page reclaimers:
	 * they are running/oncpu while representing a stall. To tell
	 * whether a domain has productivity left or not, we need to
	 * distinguish between regular running (i.e. productive)
	 * threads and memstall ones.
	 */
	NR_MEMSTALL_RUNNING,
	NR_PSI_TASK_COUNTS = 4,
};
```
基本逻辑简单，检查这四个变量就可以找到标记的位置

基本的操作是在上下文的时间统计完成的:
- psi_task_switch
  - psi_group_change
    - schedule_delayed_work : 启动触发 psi_avgs_work

每一个系统，或者 cgroup 中含有一个 psi_avgs_work

### irq

仅仅在 psi_account_irqtime 中处理

其实就是借助 docs/kernel/sched-irq-time-accounting.md ，然后将之前 irq 占用的时间修改为 cgroup level 的

- [ ] /proc/stat 中加起来应该和 /proc/pressure/irq 的相等吧

赞同这里的说法，这个其实用途不大:
https://utcc.utoronto.ca/~cks/space/blog/linux/PSIIRQNumbersAndMeanings

### cpu

### io
```txt
#0  io_schedule_prepare () at kernel/sched/core.c:8777
#1  io_schedule () at kernel/sched/core.c:8810
#2  0xffffffff821ae17c in bit_wait_io (word=<optimized out>, mode=2) at kernel/sched/build_utility.c:4266
#3  0xffffffff821adb27 in __wait_on_bit (wq_head=wq_head@entry=0xffffffff82c06e88 <bit_wait_table+3336>, wbq_entry=wbq_entry@entry=0xffffc9004046bcf0, action=0xffffffff821ae170 <bit_wait_io>, mode=mode@entry=2) at kernel/sched/build_utility.c:4106
#4  0xffffffff821adc91 in out_of_line_wait_on_bit (word=word@entry=0xffff888100b4a000, bit=bit@entry=2, action=<optimized out>, mode=mode@entry=2) at kernel/sched/build_utility.c:4121
#5  0xffffffff81415c53 in wait_on_bit_io (bit=2, mode=2, word=0xffff888100b4a000) at ./include/linux/wait_bit.h:101
#6  0xffffffff814f1407 in wait_on_buffer (bh=0xffff888100b4a000) at ./include/linux/buffer_head.h:385
#7  journal_wait_on_commit_record (journal=0xffff888140d7e800, bh=0xffff888100b4a000) at fs/jbd2/commit.c:175
#8  jbd2_journal_commit_transaction (journal=journal@entry=0xffff888140d7e800) at fs/jbd2/commit.c:919
#9  0xffffffff814f80a8 in kjournald2 (arg=0xffff888140d7e800) at fs/jbd2/journal.c:210
#10 0xffffffff811556a4 in kthread (_create=0xffff88814066eb40) at kernel/kthread.c:376
#11 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```
io_schedule_prepare 中设置

```c
	current->in_iowait = 1;
```

之后在 psi_task_switch 采样 current::in_iowait

### mem

memory 记录有，这个需要在被各种地方调用:
- psi_memstall_enter
- psi_memstall_leave

其实也就是标记一下，之后统计:
```c
	current->in_memstall = 1;
```
## TODO
- [ ] 为了支持 cgroup ，需要哪些工作
- [ ] uswap 和 psi 会有干扰吗?
- [ ] 可以看懂这两个 fix 吗?
  - c6508124193d42bbc3224571eb75bfa4c1821fbb
  - 3840cbe24cf060ea05a585ca497814609f5d47d1

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
