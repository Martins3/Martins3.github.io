# context switch

从这里开始: https://blog.codingconfessions.com/p/linux-context-switching-internals : 重点关注


## 如何进行切换?


- `__schedule`
  - `pick_next_task`
  - `context_switch`
    - `__switch_to_asm`
      - `__switch_to`
        - `switch_fpu_prepare`
        - `save_fsgs` : 将 prev task 的 fs(gs)index(base) 保存一下
        - `load_TLS` : 设置 next task 的 gdt 中间的 TLS
    	  - `savesegment(es, prev->es);`
		    - `loadsegment(es, next->es);`
	      - savesegment(ds, prev->ds);
		    - loadsegment(ds, next->ds);
        - `x86_fsgsbase_load`
        - `this_cpu_write(cpu_current_top_of_stack, task_top_of_stack(next_p));` // 可能是访问 TSS 过于费劲，所以将数值保存在此处 ?
        - `update_task_stack`


1. function
3. process
2. syscall
4. interrupt

| preserved registers               | scratch registers                         |
|-----------------------------------|-------------------------------------------|
| callee-saved                      | caller-saved                              |
| rbp, rbx, rsp, r12, r13, r14, r15 | rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11 |

![](https://os.phil-opp.com/cpu-exceptions/exception-stack-frame.svg)

Calling conventions specify the details of a function call. For example, they specify where function parameters are placed (e.g. in registers or on the stack) and how results are returned. On x86_64 Linux, the following rules apply for C functions (specified in the System V ABI):
- the first six integer arguments are passed in registers rdi, rsi, rdx, rcx, r8, r9
- additional arguments are passed on the stack
- results are returned in rax and rdx

```c
struct pt_regs {
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
 */
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long bp;
	unsigned long bx;
/* These regs are callee-clobbered. Always saved on kernel entry. */
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long ax;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
	unsigned long orig_ax;
/* Return frame for iretq */
	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;
/* top of stack page */
};
```

- [ ] 那么 tlb flush 的操作在哪里，还是说支持了 vpid 之后可以不用 tlb flush
    - [x] 那些位置需要 tlb flush ? (估计就是更新 page table 的时候吧!)
    - [ ] 那么总该存在一些关于 vpid 的代码吧!

- [ ] context switch 和 interrupt 的关系 ?
    - [ ] context_switch 为什么需要屏蔽中断
- [ ] 考虑一下多核的影响是什么 ?


The short list context switching tasks:
- Repointing the work space: Restore the stack (SS:SP)
- Finding the next instruction: Restore the IP (CS:IP)
- Reconstructing task state: Restore the general purpose registers
- Swapping memory address spaces: Updating page directory (CR3)
- ...and more: FPUs, OS data structures, debug registers, hardware workarounds, etc.

TODO
1. 调查一下第二种状况的原因，这不是 ucore 的方法
    1. interrupt 和 userspace return 分别对应什么
    2. 为什么有人会向其上插入这个 flags，在什么时机，在 scheduler_tick 吗 ? 这是唯一的时间点
2. wake up 的操作似曾相识
    1. 不是理解结束，而是插入 TIF_NEED_RESCHED flag
    2. 上面的第二条和第三条不应该合并为一条内容吗 ?

4. 代码的疑惑:
    1. tlb
    2. membarrier
    3. finish_task_switch : 过于综合
    4. prepare_task_switch


```c
/*
 * context_switch - switch to the new MM and the new thread's register state.
 */
static __always_inline struct rq *
context_switch(struct rq *rq, struct task_struct *prev,
	       struct task_struct *next, struct rq_flags *rf)
```

switch_to 被移动到 arch/x86/include/asm/switch_to.h 中间
```c
#define switch_to(prev, next, last)					\
do {									\
	((last) = __switch_to_asm((prev), (next)));			\
} while (0)
```

`__switch_to_asm` 在： arch/x86/entry/entry_64.S 中间，
```asm
SYM_FUNC_START(__switch_to_asm)
	/*
	 * Save callee-saved registers
	 * This must match the order in inactive_task_frame
	 */
	pushq	%rbp
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* switch stack */
	movq	%rsp, TASK_threadsp(%rdi)
	movq	TASK_threadsp(%rsi), %rsp

#ifdef CONFIG_STACKPROTECTOR
	movq	TASK_stack_canary(%rsi), %rbx
	movq	%rbx, PER_CPU_VAR(fixed_percpu_data) + stack_canary_offset
#endif

#ifdef CONFIG_RETPOLINE
	/*
	 * When switching from a shallower to a deeper call stack
	 * the RSB may either underflow or use entries populated
	 * with userspace addresses. On CPUs where those concerns
	 * exist, overwrite the RSB with entries which capture
	 * speculative execution to prevent attack.
	 */
	FILL_RETURN_BUFFER %r12, RSB_CLEAR_LOOPS, X86_FEATURE_RSB_CTXSW
#endif

	/* restore callee-saved registers */
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	popq	%rbp

	jmp	__switch_to
SYM_FUNC_END(__switch_to_asm)
```

注意，由於 `jmp __switch_to` 而 `__switch_to` 是 gcc 編譯的函數，沒有使用 call 調用，導致其 ret 會使用
prev task 的 ret_addr 作爲返回地址。所以，switch_to 的这个 macro 的返回值 last 就是 `__switch_to` 的返回值:
inactive_task_frame::ret_addr 的賦值存在兩種情況:
1. fork : 在 copy_thread 中間賦值
2. 普通的 thread 切換 : 調用 `__switch_to_asm` 的時候，利用 next task 保存的 eip 地址, 开始逐步返回发，从而 next task 的视角看到就是从 switch_to 函数逐步返回
  - next task 可以从 switch_to 逐步返回，进入正常的执行流程，前提是，在 next task 在其内核 stack 上构建了调用到 switch_to 的 stack 信息
  - 刚刚 fork 出来的新的进程，其想要被执行，也是从 switch_to 开始的, 想要回到用户空间，也需要在 stack 上放置一些信息，这就是  fork_frame 的由来了，具体在 copy_thread 中间，设置返回地址为 ret_from_fork

```c
/*
 * This is the structure pointed to by thread.sp for an inactive task.  The
 * order of the fields must match the code in __switch_to_asm().
 */
struct inactive_task_frame {
#ifdef CONFIG_X86_64
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
#else
	unsigned long flags;
	unsigned long si;
	unsigned long di;
#endif
	unsigned long bx;

	/*
	 * These two fields must be together.  They form a stack frame header,
	 * needed by get_frame_pointer().
	 */
	unsigned long bp;
	unsigned long ret_addr;
};

struct fork_frame {
	struct inactive_task_frame frame;
	struct pt_regs regs;
};
```


### fpu
- [ ] fpu 的 lazy 如何体现出来 ?

```c
/*
 * FPU state switching for scheduling.
 *
 * This is a two-stage process:
 *
 *  - switch_fpu_prepare() saves the old state.
 *    This is done within the context of the old process.
 *
 *  - switch_fpu_finish() sets TIF_NEED_FPU_LOAD; the floating point state
 *    will get loaded on return to userspace, or when the kernel needs it.
 *
 * If TIF_NEED_FPU_LOAD is cleared then the CPU's FPU registers
 * are saved in the current thread's FPU register state.
 *
 * If TIF_NEED_FPU_LOAD is set then CPU's FPU registers may not
 * hold current()'s FPU registers. It is required to load the
 * registers before returning to userland or using the content
 * otherwise.
 *
 * The FPU context is only stored/restored for a user task and
 * PF_KTHREAD is used to distinguish between kernel and user threads.
 */
static inline void switch_fpu_prepare(struct fpu *old_fpu, int cpu)
{
	if (static_cpu_has(X86_FEATURE_FPU) && !(current->flags & PF_KTHREAD)) {
		if (!copy_fpregs_to_fpstate(old_fpu))
			old_fpu->last_cpu = -1;
		else
			old_fpu->last_cpu = cpu;

		/* But leave fpu_fpregs_owner_ctx! */
		trace_x86_fpu_regs_deactivated(old_fpu);
	}
}
```


## 一个 process 的 context 到底持有什么?

```c
union thread_union {
	struct task_struct task;
#ifndef CONFIG_THREAD_INFO_IN_TASK
	struct thread_info thread_info;
#endif
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};
```

### thread_info
x64 和 arm64 的 latest kernel 的处理在两个 macro 的配置上完全相同，下面使用 x64 作为例子
```c
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

- x86-64 将 thread_info 放到了 task_struct 前面，是为了实现 current_thread_info() 方便
- flags 用于 `TIF_`, 比如 TIF_NEED_RESCHED 等

获取到 `pt_regs` 的过程
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

### thread_struct
一个 process 架构相关的内容放到这里

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
	 * IOPL. Privilege level dependent I/O permission which is
	 * emulated via the I/O bitmap to prevent user space from disabling
	 * interrupts.
	 */
	unsigned long		iopl_emul;

	unsigned int		iopl_warn:1;
	unsigned int		sig_on_uaccess_err:1;

	/*
	 * Protection Keys Register for Userspace.  Loaded immediately on
	 * context switch. Store it in thread_struct to avoid a lookup in
	 * the tasks's FPU xstate buffer. This value is only valid when a
	 * task is scheduled out. For 'current' the authoritative source of
	 * PKRU is the hardware itself.
	 */
	u32			pkru;

	/* Floating point and extended processor state */
	struct fpu		fpu;
	/*
	 * WARNING: 'fpu' is dynamically-sized.  It *MUST* be at
	 * the end.
	 */
};
```

- [ ]`tls_array`
- [ ] `fsbase` and `fsindex`

### tss_struct
```c
struct tss_struct {
	/*
	 * The fixed hardware portion.  This must not cross a page boundary
	 * at risk of violating the SDM's advice and potentially triggering
	 * errata.
	 */
	struct x86_hw_tss	x86_tss;

	struct x86_io_bitmap	io_bitmap;
} __aligned(PAGE_SIZE);
```

这不是之前想看的 stack :
```c
static inline void tss_setup_ist(struct tss_struct *tss)
{
	/* Set up the per-CPU TSS IST stacks */
	tss->x86_tss.ist[IST_INDEX_DF] = __this_cpu_ist_top_va(DF);
	tss->x86_tss.ist[IST_INDEX_NMI] = __this_cpu_ist_top_va(NMI);
	tss->x86_tss.ist[IST_INDEX_DB] = __this_cpu_ist_top_va(DB);
	tss->x86_tss.ist[IST_INDEX_MCE] = __this_cpu_ist_top_va(MCE);
	/* Only mapped when SEV-ES is active */
	tss->x86_tss.ist[IST_INDEX_VC] = __this_cpu_ist_top_va(VC);
}
```

### TSS
在 fix_processor_context 中，`set_tss_desc(cpu, &get_cpu_entry_area(cpu)->tss.x86_tss);`
从此之后，从此之后，总是直接修改 tss.x86_tss 来修改 kernel stack.

我感觉 : 显然 tr 只是一个 segment selector，用于指向 GDT 中间，TSS 的所在的位置,
但是并没有进行缓存，所以修改 tss.x86_tss 就可以实现修改 kernel stack 的目的.

> https://en.wikipedia.org/wiki/Task_state_segment
>
> The TSS may reside anywhere in memory. A segment register called the task register (TR) holds a segment selector that points to a valid TSS segment descriptor which resides in the GDT (a TSS descriptor may not reside in the LDT). Therefore, to use a TSS the following must be done by the operating system kernel:
> 1. Create a TSS descriptor entry in the GDT
> 2. Load the TR with the segment selector for that segment
> 3. Add information to the TSS in memory as needed

tr 寄存器从 gdt 获取 tss desc, 从 desc 中获取到 TSS 的地址，从 TSS 中间找到 stack 的地址

### fs gs
```txt
#0  native_swapgs () at ./arch/x86/include/asm/processor.h:551
#1  __rdgsbase_inactive () at arch/x86/kernel/process_64.c:193
#2  0xffffffff81036d08 in save_fsgs (
    task=0xffffffff83012940 <init_task>)
    at arch/x86/kernel/process_64.c:285
#3  __switch_to (prev_p=0xffffffff83012940 <init_task>,
    next_p=0xffff888004c63080)
    at arch/x86/kernel/process_64.c:627
#4  0xffffffff81f455d0 in context_switch (
    rf=0xffffc90000167e40,
    next=0xffffffff83012940 <init_task>,
    prev=0xffff888004c63080, rq=0xffff88803e630ac0)
    at kernel/sched/core.c:5369
```

为什么 swapgs 需要使用一个单独的指令来实现:
```c
static __always_inline void native_swapgs(void)
{
#ifdef CONFIG_X86_64
	asm volatile("swapgs" ::: "memory");
#endif
}
```

重新阅读了一些 amd sdm volume 2 section 4.5 的内容，segment register 中间存放是 segment selector 寄存器，而且缓存了 gdt 的内容，
在 64bit 的架构下，CS 的 Only the L (long), D (default operation size), and DPL (descriptor privilege-level) attributes
are recognized by 64-bit mode, 而  the contents of the ES, DS, and SS segment registers are ignored. FS 和 GS 寄存器的内容除了保存 segment selector 的内容外，
还提供了 fsbase, gsbase 的数值用于访问。

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


reference
- insides
- LoyenWang


- [ ] Documentation/x86/entry_64.rst 特别的说明了 swapgs 的问题

## 参考
- http://www.maizure.org/projects/evolution_x86_context_switch_linux/
- https://www.cnblogs.com/LoyenWang/p/12386281.html
- https://os.phil-opp.com/cpu-exceptions/

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
