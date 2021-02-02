# linux process management
1. 进程的生命周期
  1. fork
  2. exit
  3. exec
2. scheduler
  1. context switch
  2. cfs
    1. load_avg
    2. task group
    3. bandwidth
    4. load balance
    5. numa balance
    6. autogroup
  3. preempt
3. signal


首先收集各种问题吧:
- [ ] try to wake up 的复杂程度还是超过想象

- [ ] exit 的 find real parent 的问题

- [ ] signal 中间的 jobctl

- [ ] ptrace 的实现

```c
static inline bool thread_group_leader(struct task_struct *p)
{
	return p->exit_signal >= 0;
}
```
- [ ] exit_signal

- [ ] workqueue 的重新总结, 从而进入到 software 层次的 interrupt 中

- [ ] kernel/smp.c 的实现，如何 for each cpu ?

- [ ] real_parent ?

```c
			list_add_tail(&p->sibling, &p->real_parent->children); // 这些成员的含义是什么 ?
```

- [ ] kernel_exec

## TIF_NEED_RESCHED
Four reference of TIF_NEED_RESCHED
```c
#define tif_need_resched() test_thread_flag(TIF_NEED_RESCHED)

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline void clear_tsk_need_resched(struct task_struct *tsk)
{
	clear_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline int test_tsk_need_resched(struct task_struct *tsk)
{
	return unlikely(test_tsk_thread_flag(tsk,TIF_NEED_RESCHED));
}
```

`set_tsk_need_resched` call site:
1. rcu
2. resched_curr


- [ ] TIF_NEED_FPU_LOAD
