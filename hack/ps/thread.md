# thread_info, thread_struct, thread_union

- task_struct 放到 stack 之外了

## thread_info
x64 和 arm64 的 latest kernel 的处理在两个 macro 的配置上完全相同，下面使用 x64 作为例子
```c
#define CONFIG_THREAD_INFO_IN_TASK
// 也就是 task_struct 在 stack 上，但是 thread_info 不在 stack 上

union thread_union {
#ifndef CONFIG_ARCH_TASK_STRUCT_ON_STACK
	struct task_struct task;
#endif
#ifndef CONFIG_THREAD_INFO_IN_TASK
	struct thread_info thread_info; // 这一行是灰色的
#endif
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};


struct task_struct {
#ifdef CONFIG_THREAD_INFO_IN_TASK
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;
#endif
  // ...

	/* CPU-specific state of this task: */
	struct thread_struct		thread;

	/*
	 * WARNING: on x86, 'thread_struct' contains a variable-sized
	 * structure.  It *MUST* be at the end of 'task_struct'.
	 *
	 * Do not put anything below here!
	 */
}

struct thread_info {
	unsigned long		flags;		/* low level flags */
	unsigned long		syscall_work;	/* SYSCALL_WORK_ flags */
	u32			status;		/* thread synchronous flags */
};
```

- x86-64 将 thread_info 放到了 task_struct 前面, 在 ULK 的描述，以及 MIPS64 的目前的实现，thread_info 存在 preempt_count，但是 x86 引进进行更新（arch/x86/include/asm/preempt.h），在 LWN 上有文章描述
- flags 用于 `TIF_`, 比如 TIF_NEED_RESCHED 等

```c
#define task_pt_regs(p) \
	((struct pt_regs *)(THREAD_SIZE + task_stack_page(p)) - 1)

/*
 * When accessing the stack of a non-current task that might exit, use
 * try_get_task_stack() instead.  task_stack_page will return a pointer
 * that could get freed out from under you.
 */
static inline void *task_stack_page(const struct task_struct *task)
{
	return task->stack;
}

// x86 没有硬件支持，在 __switch_to 的时候维护这一个 percpu 硬件
static __always_inline struct task_struct *get_current(void)
{
	return this_cpu_read_stable(current_task);
}

#define current get_current()
```
