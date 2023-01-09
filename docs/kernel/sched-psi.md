# psi

memory 记录有：
- psi_memstall_enter
- psi_memstall_leave

## 使用接口

- /proc/pressure/
- /sys/fs/cgroup/cg1/io.pressure


```c
/* Task state bitmasks */
#define TSK_IOWAIT	(1 << NR_IOWAIT)
#define TSK_MEMSTALL	(1 << NR_MEMSTALL)
#define TSK_RUNNING	(1 << NR_RUNNING)
#define TSK_MEMSTALL_RUNNING	(1 << NR_MEMSTALL_RUNNING)
```

## io 的时间统计
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

之后采样 current::in_iowait

## 为什么上下文切换的时候需要 psi
- psi_task_switch
  - psi_group_change
    - schedule_delayed_work : 启动触发 psi_avgs_work

每一个系统，或者 cgroup 中含有一个 psi_avgs_work

## [ ] 如何体现在 cgroup 上的?
