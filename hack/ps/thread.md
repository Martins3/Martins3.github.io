# thread_info, thread_struct, thread_union

- [ ] 为什么需要将 thread_info 放到 task_struct 之外 ?
  - task_struct 放到 stack 之外


## thread_info

```c
union thread_union {
#ifndef CONFIG_ARCH_TASK_STRUCT_ON_STACK
	struct task_struct task;
#endif
#ifndef CONFIG_THREAD_INFO_IN_TASK
	struct thread_info thread_info;
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
- flags 用于 `TIF_`


#### TIF_NEED_RESCHED
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

## thread_struct

```c
struct thread_struct {
	/* Cached TLS descriptors: */
	struct desc_struct	tls_array[GDT_ENTRY_TLS_ENTRIES];
#ifdef CONFIG_X86_32
	unsigned long		sp0;
#endif
	unsigned long		sp;
#ifdef CONFIG_X86_32
	unsigned long		sysenter_cs;
#else
	unsigned short		es;
	unsigned short		ds;
	unsigned short		fsindex;
	unsigned short		gsindex;
#endif

#ifdef CONFIG_X86_64
	unsigned long		fsbase;
	unsigned long		gsbase;
#else
	/*
	 * XXX: this could presumably be unsigned short.  Alternatively,
	 * 32-bit kernels could be taught to use fsindex instead.
	 */
	unsigned long fs;
	unsigned long gs;
#endif

	/* Save middle states of ptrace breakpoints */
	struct perf_event	*ptrace_bps[HBP_NUM];
	/* Debug status used for traps, single steps, etc... */
	unsigned long           virtual_dr6;
	/* Keep track of the exact dr7 value set by the user */
	unsigned long           ptrace_dr7;
	/* Fault info: */
	unsigned long		cr2;
	unsigned long		trap_nr;
	unsigned long		error_code;
#ifdef CONFIG_VM86
	/* Virtual 86 mode info */
	struct vm86		*vm86;
#endif
	/* IO permissions: */
	struct io_bitmap	*io_bitmap;

	/*
	 * IOPL. Priviledge level dependent I/O permission which is
	 * emulated via the I/O bitmap to prevent user space from disabling
	 * interrupts.
	 */
	unsigned long		iopl_emul;

	unsigned int		sig_on_uaccess_err:1;

	/* Floating point and extended processor state */
	struct fpu		fpu;
	/*
	 * WARNING: 'fpu' is dynamically-sized.  It *MUST* be at
	 * the end.
	 */
};
```

## thread_struct
- [ ]`tls_array`
- [ ] `fsbase` and `fsindex`

[^7]
At every process switch, the hardware context of the process being replaced must be
saved somewhere. It cannot be saved on the TSS, as in the original Intel design,
because Linux uses a single TSS for each processor, instead of one for every process.

Thus, each process descriptor includes a field called thread of type `thread_struct`, in
which the kernel saves the hardware context whenever the process is being switched
out. As we’ll see later, this data structure includes fields for most of the CPU registers,
except the general-purpose registers such as eax, ebx, etc., which are stored in
the Kernel Mode stack.

总结 : thread_struct 是保存 task_struct 中间用于保存 hardware context 的部分。

- [ ] 估计相关代码都是 context switch


[^7]: Understanding Linux Kernel
