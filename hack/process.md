# 进程管理

<!-- vim-markdown-toc GitLab -->

- [introduction](#introduction)
- [preemption](#preemption)
    - [preempt count](#preempt-count)
    - [preempt locking](#preempt-locking)
    - [preempt notes](#preempt-notes)
- [context switch](#context-switch)
    - [context switch fpu](#context-switch-fpu)
    - [context switch stack](#context-switch-stack)
    - [context switch TSS](#context-switch-tss)
    - [context switch TLS](#context-switch-tls)
- [vfork](#vfork)
- [TLS](#tls)
- [thread_struct](#thread_struct)
- [signal](#signal)
    - [send signal](#send-signal)
    - [wait signal](#wait-signal)
    - [do signal](#do-signal)
- [idle](#idle)
    - [ptrace](#ptrace)
- [workqueue](#workqueue)
- [waitqueue](#waitqueue)
- [design](#design)
- [fork](#fork)
    - [copy_process](#copy_process)
    - [stack's copy](#stacks-copy)
- [stack](#stack)
    - [x86 stack](#x86-stack)
- [exec](#exec)
- [exit.c](#exitc)
    - [do_exit](#do_exit)
- [pidfd](#pidfd)
- [pid](#pid)
    - [getpid](#getpid)
- [thread_info](#thread_info)
    - [TIF](#tif)
      - [TIF_NEED_RESCHED](#tif_need_resched)
      - [TIF_NEED_FPU_LOAD](#tif_need_fpu_load)
- [green thread](#green-thread)
- [cpp thread keyword](#cpp-thread-keyword)
- [cpu](#cpu)
- [syscall](#syscall)
    - [x86](#x86)
- [daemon](#daemon)
- [user group](#user-group)
- [smp](#smp)
- [`__schedule`](#__schedule)
- [TODO](#todo)

<!-- vim-markdown-toc -->

用户需要怎样执行程序: (调查一下所有相关的 syscall 都是什么东西)
1. 容器: cgroup 和 namespace
2. 多用户 : 调度器
    1. 不用的场景存在不同的 exec
3. 启动 : fork 
    1. 第一个进程启动 : ?
    2. 启动第一个用户进程 : ?
    3. Posix 规定的 25个 flags
4. 执行 : exec 
4. 暂停 : yield
5. 死亡 : kill exit
6. 交流 : signal 


进入调度器之前 : 
1. exit.c 
2. pidfd
3. pidnamespace : 据说 namespace 下，可以没有 pid = 1
.....
n. preemption !


## introduction

| 主要内容       | 一句话说明要点                                                                                                     | 涉及的文件 | 关键的函数          |
|----------------|--------------------------------------------------------------------------------------------------------------------|------------|---------------------|
| 进程的层级关系 | thread group, process group 和 session, 逐级包含的关系，其中 thread group 用于pthread 线程的实现，后者主要用于终端 | fork.c     | fork.c:copy_process |
| 创建进程       | clone fork, vfork 全部汇集到 `_do_fork`, 其参数决定了复制的内容                                                    | fork.c     |
| 销毁进程       |
| 进程通信       | IPC 和 signal :
| 进程调度       |
| 进程地址空间   | brk


## preemption
- [x] So what's the difference between preempt and disable interrupt ?

this [ans](https://stackoverflow.com/questions/9473301/are-there-any-difference-between-kernel-preemption-and-interrupt)
summaries incisively, and we can check the comment of `__schedule`


[^7]
However, in nonpreemptive kernels, the current process cannot be replaced unless it is about to switch
to User Mode.

Therefore, the main characteristic of a preemptive kernel is that a process running in
Kernel Mode can be replaced by another process while in the middle of a kernel
function.

[^7]:
it is greater than zero when any of the following cases occurs:
1. The kernel is executing an interrupt service routine.
2. The deferrable functions are disabled (always true when the kernel is executing a
softirq or tasklet).
3. The kernel preemption has been explicitly disabled by setting the preemption
counter to a positive value.

huxueshi: this is correct


- [x] I can't find anything about CONFIG_PREEMPT_NONE when tracing `scheduler_tick` ?

task_tick_fair => entity_tick => check_preempt_tick

As `__schedule`'s comment says, scheduler_tick only set TIF_NEED_RESCHED flags on the thread.

- [x] check_preempt_tick and check_preempt_wakeup

former used by timer, latter by kernel code like ttwu
```c
const struct sched_class fair_sched_class = {
	.check_preempt_curr	= check_preempt_wakeup,
```

在 64bit 中间 kmap 和 kmap_atomic 的区别:

1. 为什么这保证了 atomic ?
2. 什么情况需要 atomic ?
```c
static inline void *kmap_atomic(struct page *page)
{
	preempt_disable();
	pagefault_disable();
	return page_address(page);
}
```

- [ ] In fact there are three types of preemption !

detail `kernel/Kconfig.preempt`
- No Forced Preemption (Server)
- Voluntary Kernel Preemption (Desktop) `CONFIG_PREEMPT_VOLUNTARY`
- Preemptible Kernel (Low-Latency Desktop) `CONFIG_PREEMPT`

This's only place that `CONFIG_PREEMPT_VOLUNTARY` used !
```c
#ifdef CONFIG_PREEMPT_VOLUNTARY
extern int _cond_resched(void);
# define might_resched() _cond_resched()
#else
# define might_resched() do { } while (0)
#endif
```

#### preempt count
`CONFIG_PREEMPT` will select `CONFIG_PREEMPT_COUNT`

https://lwn.net/Articles/831678/ : valuable 

- [ ] why preempt need a counter ?

- [ ] I need a more clear and strong reason : why if the process disasble interrupt, it shouldn't be preempted ?


- [ ] preempt.h : where preempt meets need_resched

- [ ] how `__preempt_count` enable ?

- [ ] how to explain `preemptible()` ?

```c
#define preemptible()	(preempt_count() == 0 && !irqs_disabled())
```

- [ ] set_preempt_need_resched
```c
/*
 * We fold the NEED_RESCHED bit into the preempt count such that
 * preempt_enable() can decrement and test for needing to reschedule with a
 * single instruction.
 *
 * We invert the actual bit, so that when the decrement hits 0 we know we both
 * need to resched (the bit is cleared) and can resched (no preempt count).
 */

static __always_inline void set_preempt_need_resched(void)
{
	raw_cpu_and_4(__preempt_count, ~PREEMPT_NEED_RESCHED);
}
```

- [ ] when will preempt happens ?
```c
asmlinkage __visible void __sched notrace preempt_schedule(void)
```


#### preempt locking
https://www.kernel.org/doc/html/latest/locking/preempt-locking.html

https://stackoverflow.com/questions/18254713/why-linux-disables-kernel-preemption-after-the-kernel-code-holds-a-spinlock




#### preempt notes

https://stackoverflow.com/questions/5283501/what-does-it-mean-to-say-linux-kernel-is-preemptive
> If the system allows that task to be preempted while it is running kernel code, then we have what is called a "preemptive kernel." 

https://stackoverflow.com/questions/49414559/linux-kernel-why-preemption-is-disabled-when-use-per-cpu-variable
> There are preemption points throughout the kernel (might_sleep). Forcible preemption = each interrupt (including timer interrupts when time slices run out) is a possible preemption point. 
> 
> *when you finish your business and about to go back to user space your time slice is checked and if its done you go to another process instead of back to user space, so Im trying to understand how this dynamic work now*

really interesting : if one process is about to switch to user space and it's time slice is used up, just switch to another process.





## context switch
2. vdso 是怎么回事 ?
3. x86 中间 TSS 是如何参与的，其他的架构又是如何设计的 ?
4. 那些令人窒息的 try to wake up 是干嘛的 ? wakeup 需要那么复杂吗 ? 
5. context switch 和 interrupt 的关系 ?
    1. context_switch 为什么需要屏蔽中断
6. 考虑一下多核的影响是什么: 

7. 检查不同的 context_switch 的代码实现 !


context switch 需要考虑的事情:
1. 上下文的种类 : 用户进程 内核线程 interrupt context, exception context (@todo 存在这个东西，需要单独说明吗 ?)
    1. 感觉需要划分为两个种类，第一种，context_switch 的，都是发生在内核中间的，没有地址空间的切换，stack 指针切换一下基本可以维持生活，rip 都是不用切换的
    2. syscall : 需要切换 rip 和 rsp 的，同时需要切换地址空间
    3. interrupt : 切换 rip rsp
    4. exception : 需要区分发生的时间和位置
2. 造成上下文切换的原因 : 
    1. interrupt，exception (注意 : interrupt 和 exception 都是可以出现在内核和用户态的)，syscall 也是其中一部分吧!
    2. sleep yield lock 等等各种原因 进程休眠，可能是用户进程，可能是内核。


context switch 需要完成那些东西的切换:
1. 硬件资源 : tlb, mmu, cache, register file (cache 真的需要切换吗 ?)
2. 
3. 反思 : 两个线程之间切换 和 两个进程之间切换，替换的内容不同的。

那些部分是软件完成的，那些是需要硬件的支持:

[^1] 的笔记整理:
其中论述的核心在 switch_to 这个 macro 

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
3. preemption 的区别，似乎可以切换的时间点不同而已 ?

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
	prepare_switch_to(next);					\
									\
	((last) = __switch_to_asm((prev), (next)));			\
} while (0)
```
1. prepare_switch_to : TODO 看注释，VMAP_STACK 是干什么的
2. switch_to 的三个参数 ?

`__switch_to_asm` 在：
arch/x86/entry/entry_64.S 中间，

Documentation/x86/entry_64.rst 是时候彻底搞清楚 entry_64 了啊 TODO 

关于汇编的问题:
1. 混合编译
```c
struct task_struct *__switch_to_asm(struct task_struct *prev,
				    struct task_struct *next);
```

```
/*
 * %rdi: prev task
 * %rsi: next task
 */
SYM_FUNC_START(__switch_to_asm)
```
// TODO
1. 为什么其中的，两个参数就是使用 rdi 和 rsi 传递的，哪里
2. 汇编语言中的宏如何定义啊
3. 在 qemu 中间运行汇编还是在 真正的机器上运行，分别需要什么样子的准备 ?

// TODO
关于 switch_to_asm 的更多问题:
1. pushq 和 popq 是发生在不同的 stack 上，当一个 stack 被结束之后，首先将几个寄存器 push 进去
2. 然后进入到 `__switch_to`，完全看不懂啊
3. 地址空间的切换在哪里 ?
    1. stack 的切换 => 整理到 stack blog => 因为 context_switch 的时候，发生在内核地址空间，其实只是需要进行，其实 rsp 的切换
    2. 
3. 为什么不用切换 eip ?
    1. 如果所有的函数，
    2. 那么

- [ ] why `es` `ds` register has to be handled especially in `__switch_to` ?

#### context switch fpu

#### context switch stack

#### context switch TSS
- [ ] what's relation TSS and `thread_struct`

#### context switch TLS
process_64.c:`__switch_to` call `load_TLS` to load `ldt` from `thread_struct->tls_array` 

## vfork
- [ ] it's easy, it's simple, but it's necessary to understand clearly.


## TLS
// TODO
https://chao-tic.github.io/blog/2018/12/25/tls
https://wiki.osdev.org/Thread_Local_Storage

`arch/x86/kernel/tls.c`

- [ ] syscall `get_thread_area` and `set_thread_area`, there is no complete example for it.


[this](https://stackoverflow.com/questions/34243432/how-are-ldt-and-gdt-used-differently-in-intel-x86)
answered the difference between GDT and LDT

## thread_struct
1. `tls_array`
2. `fsbase` and `fsindex`

[^7]
At every process switch, the hardware context of the process being replaced must be
saved somewhere. It cannot be saved on the TSS, as in the original Intel design,
because Linux uses a single TSS for each processor, instead of one for every process.

Thus, each process descriptor includes a field called thread of type thread_struct, in
which the kernel saves the hardware context whenever the process is being switched
out. As we’ll see later, this data structure includes fields for most of the CPU registers, except the general-purpose registers such as eax, ebx, etc., which are stored in
the Kernel Mode stack.


## signal
1. 还是先把信号的机制的 tlpi 看完看懂，写代码，做习题完成在看吧，其实挺重要的
2. fork 是如何处理 signal 共享的 ?

为什么需要信号机制:
1. 提供给用户操作进程的状态的接口，kill 等
2. 内核之间的通信
2. debug

需要的分析的细节:
2. generic_file_buffered_read 中间会调用 fatal_signal_pending，那么各种检查函数的要求是什么 ?

Man 分析的很清楚[^2] :
1. 发送
2. 接收
3. 屏蔽或者延迟信号的执行

- 当程序被 interrupt，或者 sleep 到来的信号，如何处理 ?
    1. sleep 会被唤醒, 参考 complete_signal 中的 signal_wake_up
    2. interrupt : 完成 interrupt 或者 exception 的时候

用户的 handler 如何注册的 ?

- [ ] 执行完成 handler 是在用户态 还是 内核态，如果是内核态，没有安全，如果用户态，这个上下文如何构造 ?
- [x] 当用户的 handler 执行完成之后，用户靠什么回到内核，此外，用户程序要是拒绝回到内核怎么办
  - [ ] 好吧，CNM, 居然在 x86/kernel/signal 中间



实际上的代码在 kernel/entry/
irqentry_exit_to_user_mode 和 syscall_exit_to_user_mode

#### send signal
没想到吧，这其实发送 signal 的

do_send_sig_info : 万恶之源

`__send_signal`

#### wait signal

#### do signal
exit_to_usermode_loop => do_signal() => handle_signal()

![](http://liujunming.top/images/2018/12/76.png)

- [x] sigreturn rt_sigreturn

- https://man7.org/linux/man-pages/man2/sigreturn.2.html
- https://stackoverflow.com/questions/31267825/rt-sigreturn-and-linux-kernels



## idle
1. fork_idle 的含义 ?
    - copy_process 对于 pid=0 的判断


#### ptrace 
https://github.com/x64dbg/x64dbg


`_do_fork` 最后的 ptrace 部分:

```c
	/* forking complete and child started to run, tell ptracer */
	if (unlikely(trace))
		ptrace_event_pid(trace, pid);

	if (clone_flags & CLONE_VFORK) {
		if (!wait_for_vfork_done(p, &vfork))
			ptrace_event_pid(PTRACE_EVENT_VFORK_DONE, pid);
	}
```
> fork 工作完成之后，通知一下 tracer 

signal.c :
```
ptrace_notify 
  ptrace_stop
    do_notify_parent_cldstop
```

## workqueue
workqueue 不应该被放到 interrupt 中间，毕竟 page writeback 就是使用 workqueue 维持生活的

## waitqueue
1. https://github.com/cirosantilli/linux-kernel-module-cheat/blob/master/kernel_modules/wait_queue.c : 还是要好好分析一下其中的代码呀 !



do_wait() : 每个 `task_struct->signal->wait_chldexit` 上放置 wait queue
```c
  // TODO child_wait_callback 函数调用的时机 : 元素加入 还是 元素离开
  // child_wait_callback 会唤醒 current
	init_waitqueue_func_entry(&wo->child_wait, child_wait_callback);
	wo->child_wait.private = current; // 用于唤醒
	add_wait_queue(&current->signal->wait_chldexit, &wo->child_wait);

 // 最终去掉，如果捕获了多个 thread
 remove_wait_queue
```

wake up : do_notify_parent_cldstop 和 do_notify_parent


## design
1. 阅读一个那个 fork paper 

如何创建第一个线程进程之类的问题理解了可以加深对于操作系统的代码的理解，关键的是:

1. 其实我对于 exec 的设计非常不满 ?
    1. 当使用 exec 的时候，总是 fork 然后 exec 的
    2. 那么，那么为什么不去创建一个新的 syscall 直接创建一个
        1. 为了维护 parent 和 child 的关系 ?

## fork
1. posix spawn 到底是靠什么实现的, 比如这里有一个用户库: https://github.com/rtomayko/posix-spawn
    1. 安装 : https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it
    2. 检查代码 : sysdeps/posix/spawni.c 中间的 `__spawni` 函数, 靠 vfork 维持生活
2. https://lwn.net/Articles/360556/ 说明了各种 fork + exec 模式

`_do_fork` : 万恶之源

check 一下这个阅读:
https://stackoverflow.com/questions/4856255/the-difference-between-fork-vfork-exec-and-clone?rq=1

#### copy_process

#### stack's copy

- [ ] 两个 thread 共享地址空间，如何防止 stack 互相覆盖 ?


- [ ] 更多关于 pt_regs 的作用，应该是可以作为 syscall 的保存，似乎也可以作为 ptrace ? please read it and search it
https://stackoverflow.com/questions/52765688/in-linux-where-is-user-mode-stack-stored
https://stackoverflow.com/questions/33104091/how-are-system-calls-stored-in-pt-regs

- [ ] task_pt_regs 的原理
- [ ] task_stack_page : 才发现 `task->stack` 是用于指向 stack 的指针，这个变量的 ref 并不多，好好 check 一下.
  - [ ] fork 的时候，如何从 parent 的拷贝 stack 的
  - [ ] fork 的返回值为什么是两个的原因定位一下代码

## stack
notes from questions [^6]:

- [x] when is `task_stack` allocated and destroied ?

```c
static unsigned long *alloc_thread_stack_node(struct task_struct *tsk, int node)

static void release_task_stack(struct task_struct *tsk)
```
it's simple, allocate page and assign to `task->stack`

- [x] why `task_struct` has to access `stack` ?

because kernel put `task_pt_regs` on the stack

```c
static inline void *task_stack_page(const struct task_struct *task)
{
  return task->stack;
}


#ifndef current_pt_regs
#define current_pt_regs() task_pt_regs(current)
#endif

/*
 * unlike current_pt_regs(), this one is equal to task_pt_regs(current)
 * on *all* architectures; the only reason to have a per-arch definition
 * is optimisation.
 */
#ifndef signal_pt_regs
#define signal_pt_regs() task_pt_regs(current)
#endif

#define task_pt_regs(task) \
({									\
	unsigned long __ptr = (unsigned long)task_stack_page(task);	\
	__ptr += THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;		\
	((struct pt_regs *)__ptr) - 1;					\
})
```

- [x] current_top_of_stack()

stored in the 'soft' TSS

```c
static inline unsigned long current_top_of_stack(void)
{
	/*
	 *  We can't read directly from tss.sp0: sp0 on x86_32 is special in
	 *  and around vm86 mode and sp0 on x86_64 is special because of the
	 *  entry trampoline.
	 */
	return this_cpu_read_stable(cpu_current_top_of_stack);
}

/*
 * per-CPU TSS segments. Threads are completely 'soft' on Linux,
 * no more per-task TSS's. The TSS size is kept cacheline-aligned
 * so they are allowed to end up in the .data..cacheline_aligned
 * section. Since TSS's are completely CPU-local, we want them
 * on exact cacheline boundaries, to eliminate cacheline ping-pong.
 */
__visible DEFINE_PER_CPU_PAGE_ALIGNED(struct tss_struct, cpu_tss_rw) = {
	.x86_tss = {
		/*
		 * .sp0 is only used when entering ring 0 from a lower
		 * privilege level.  Since the init task never runs anything
		 * but ring 0 code, there is no need for a valid value here.
		 * Poison it.
		 */
		.sp0 = (1UL << (BITS_PER_LONG-1)) + 1,

		/*
		 * .sp1 is cpu_current_top_of_stack.  The init task never
		 * runs user code, but cpu_current_top_of_stack should still
		 * be well defined before the first context switch.
		 */
		.sp1 = TOP_OF_INIT_STACK,

#ifdef CONFIG_X86_32
		.ss0 = __KERNEL_DS,
		.ss1 = __KERNEL_CS,
#endif
		.io_bitmap_base	= IO_BITMAP_OFFSET_INVALID,
	 },
};
```
check the code in the `__switch_to`
```c
	/*
	 * Switch the PDA and FPU contexts.
	 */
	this_cpu_write(current_task, next_p);
	this_cpu_write(cpu_current_top_of_stack, task_top_of_stack(next_p));
```

- [ ] so, what's difference between sp0 and sp1 ?


#### x86 stack
some x86 kernel notes:[^5]

*Switching to the kernel interrupt stack is done by software based on a per CPU interrupt nest counter. This is needed because x86-64 “IST” hardware stacks cannot nest without races.*

*x86_64 also has a feature which is not available on i386, the ability to automatically switch to a new stack for designated events such as double fault or NMI, which makes it easier to handle these unusual events on x86_64. This feature is called the Interrupt Stack Table (IST). There can be up to 7 IST entries per CPU. The IST code is an index into the Task State Segment (TSS). The IST entries in the TSS point to dedicated stacks; each stack can be a different size.*

- [ ] fine, too many difficult material related with **IDT**, **IST**, **TSS**
  - [ ] may review the article later


## exec
说实话，都是 快速的阅读 程序员的自我修养 ，现在对于 elf 格式始终。。。。

## exit.c
2. wait kill zombie orphan yeild 的代码的验证
3. waitid 的含义, 回答此问题 : https://stackoverflow.com/questions/13853524/why-doesnt-waitid-block-until-child-terminates


waitid -> kernel_waitid -> .... -> do_wait -> ...
-> wait_task_stopped
-> wait_task_continued

do_wait : 构建 workqueue，call do_wait_thread

#### do_exit
process 的各种资源的回收，最后通过 do_task_dead 将自己 schedule 出去法

其中 exit_mm 释放 process 持有的各种 prcess

TODO:
1. do_exit 是用户 process 结束方法，那么 kernel thread 是如何结束的 ?

## pidfd
https://man7.org/linux/man-pages/man2/pidfd_open.2.html

rust 封装:
https://github.com/pop-os/pidfd

## pid
当我们发现
1. pid 在 namespace 的层次结构，需要在上层每一个都需分配一个 pid，
2. thread group, process group, session group 
3. task_struct::pid, task_struct::tgid 是顶层 namespace 的对应 pid tgid 的快捷表示

那么剩下的都很简单了:


```c
struct task_struct {
	pid_t				pid;
	pid_t				tgid;

	/* PID/PID hash table linkage. */
	struct pid			*thread_pid;
	struct hlist_node		pid_links[PIDTYPE_MAX];
	struct list_head		thread_group;
	struct list_head		thread_node;
```

```c
/*
 * struct upid is used to get the id of the struct pid, as it is
 * seen in particular namespace. Later the struct pid is found with
 * find_pid_ns() using the int nr and struct pid_namespace *ns.
 */

struct upid {
	int nr;
	struct pid_namespace *ns;
};

struct pid
{
	refcount_t count;
	unsigned int level;
	spinlock_t lock;
  // 多个类型的 pid
	/* lists of tasks that use this pid */
	struct hlist_head tasks[PIDTYPE_MAX];
	struct hlist_head inodes;
	/* wait queue for pidfd notifications */
	wait_queue_head_t wait_pidfd;
	struct rcu_head rcu;
	struct upid numbers[1];
};

```

从 alloc_pid 中
1. pid 和 thread_pid 的关系
2. pid 和 tgid


```c
static inline void
init_task_pid(struct task_struct *task, enum pid_type type, struct pid *pid)
{
	if (type == PIDTYPE_PID)
		task->thread_pid = pid;
	else
		task->signal->pids[type] = pid;
}
```


attach_pid : 通过 pid 可以找到这个数值关联的所有的 thread
init_task_pid : 让 task 知道其所在的 pid 中

```c
static inline void
init_task_pid(struct task_struct *task, enum pid_type type, struct pid *pid)
{
	if (type == PIDTYPE_PID)
		task->thread_pid = pid;
	else
		task->signal->pids[type] = pid;
}

/*
 * attach_pid() must be called with the tasklist_lock write-held.
 */
void attach_pid(struct task_struct *task, enum pid_type type)
{
	struct pid *pid = *task_pid_ptr(task, type);
	hlist_add_head_rcu(&task->pid_links[type], &pid->tasks[type]);
}
```


```c
struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
  // task_pid(tsk) : task_struct->thread_pid
	return ns_of_pid(task_pid(tsk));
}

static inline struct pid_namespace *ns_of_pid(struct pid *pid)
{
	struct pid_namespace *ns = NULL;
	if (pid)
    // pid 是跟随 thread 的，通过 level 就可以知道其 ns
		ns = pid->numbers[pid->level].ns;
	return ns;
}

// ns 控制了一个 pid 空间，idr 加速访问
struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
{
	return idr_find(&ns->idr, nr);
}
```

#### getpid
下面跟踪 pid 的实现:

```c
/**
 * sys_getpid - return the thread group id of the current process
 *
 * Note, despite the name, this returns the tgid not the pid.  The tgid and
 * the pid are identical unless CLONE_THREAD was specified on clone() in
 * which case the tgid is the same in all threads of the same group.
 *
 * This is SMP safe as current->tgid does not change.
 */
SYSCALL_DEFINE0(getpid)
{
	return task_tgid_vnr(current);
}

static inline pid_t task_tgid_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_TGID, NULL);
}


pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns)
{
	pid_t nr = 0;

	rcu_read_lock();
	if (!ns)
		ns = task_active_pid_ns(current); // 获取到 ns
	nr = pid_nr_ns(rcu_dereference(*task_pid_ptr(task, type)), ns); // 通过 task->thread_pid 获取 thread 对应的 pid
	rcu_read_unlock();

	return nr;
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns)
{
	struct upid *upid;
	pid_t nr = 0;

	if (pid && ns->level <= pid->level) {
		upid = &pid->numbers[ns->level]; // 获取到该 level 上的
		if (upid->ns == ns)
			nr = upid->nr;
	}
	return nr;
}

static struct pid **task_pid_ptr(struct task_struct *task, enum pid_type type)
{
	return (type == PIDTYPE_PID) ?
		&task->thread_pid :
		&task->signal->pids[type];
}

```

## thread_info
`thread_info` is used to locate `task_struct`
https://unix.stackexchange.com/questions/22372/what-is-the-need-of-the-struct-thread-info-in-locating-struct-task-struct

in `asm/thread_info.h`, `TIF_NEED_RESCHED` are defined

- [x] do we need `thread_info` to access `task_struct` ?

no, we can access `task_struct` directly
```c
struct task_struct;

DECLARE_PER_CPU(struct task_struct *, current_task);

static __always_inline struct task_struct *get_current(void)
{
	return this_cpu_read_stable(current_task);
}
```

#### TIF
in `arch/x86/include/asm/thread_info.h`

##### TIF_NEED_RESCHED
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


##### TIF_NEED_FPU_LOAD



## green thread
https://github.com/google/marl : 为什么用户层可以控制 scheduler ?

## cpp thread keyword
如何实现的 ?


## cpu
https://superuser.com/questions/1217454/how-do-you-control-thread-affinity-across-multiple-processes-on-linux
1. cat /proc/cpuinfo |grep -P 'processor|physical id|core id' 中间的各种 id 的含义是什么 ?
2. 更进一步，/proc/cpuinfo 中的每一个含义是什么，数据从哪里导出的

## syscall
gcc a.c -v 可以获取很多东西:
```
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/lto-wrapper
Target: x86_64-pc-linux-gnu
Configured with: /build/gcc/src/gcc/configure --prefix=/usr --libdir=/usr/lib --libexecdir=/usr/lib --mandir=/usr/share/man --infodir=/usr/share/info --with-bugurl=https://bugs.archlinux.org/ --enable-languages=c,c++,ada,fortran,go,lto,objc,obj-c++,d --with-isl --with-linker-hash-style=gnu --with-system-zlib --enable-__cxa_atexit --enable-cet=auto --enable-checking=release --enable-clocale=gnu --enable-default-pie --enable-default-ssp --enable-gnu-indirect-function --enable-gnu-unique-object --enable-install-libiberty --enable-linker-build-id --enable-lto --enable-multilib --enable-plugin --enable-shared --enable-threads=posix --disable-libssp --disable-libstdcxx-pch --disable-libunwind-exceptions --disable-werror gdc_include_dir=/usr/include/dlang/gdc
Thread model: posix
Supported LTO compression algorithms: zlib zstd
gcc version 10.1.0 (GCC)
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/cc1 -quiet -v a.c -quiet -dumpbase a.c -mtune=generic -march=x86-64 -auxbase a -version -o /tmp/ccTZJNTI.s
GNU C17 (GCC) version 10.1.0 (x86_64-pc-linux-gnu)
	compiled by GNU C version 10.1.0, GMP version 6.2.0, MPFR version 4.0.2, MPC version 1.1.0, isl version isl-0.21-GMP

GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
ignoring nonexistent directory "/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../x86_64-pc-linux-gnu/include"
#include "..." search starts here:
#include <...> search starts here:
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include
 /usr/local/include
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include-fixed
 /usr/include
End of search list.
GNU C17 (GCC) version 10.1.0 (x86_64-pc-linux-gnu)
	compiled by GNU C version 10.1.0, GMP version 6.2.0, MPFR version 4.0.2, MPC version 1.1.0, isl version isl-0.21-GMP

gGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
Compiler executable checksum: 8cbeeaf463513b43dc3c4cd12e0bccb6
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 as -v --64 -o /tmp/ccVXGSbG.o /tmp/ccTZJNTI.s
GNU assembler version 2.34.0 (x86_64-pc-linux-gnu) using BFD version (GNU Binutils) 2.34.0
COMPILER_PATH=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/
LIBRARY_PATH=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/:/lib/../lib/:/usr/lib/../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../:/lib/:/usr/lib/
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/collect2 -plugin /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/liblto_plugin.so -plugin-opt=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/lto-wrapper -plugin-opt=-fresolution=/tmp/ccXugFlJ.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr --hash-style=gnu -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/Scrt1.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/crti.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/crtbeginS.o -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0 -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib -L/lib/../lib -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../.. /tmp/ccVXGSbG.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/crtendS.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/crtn.o
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
```
我们会发现，直接 ld `gcc -c a.c`产生的.o 是没用的，但是如果使用上面结果的倒数第二行的内容，
将 /tmp/ 的 .o 替换为 `gcc -c a.c` 产生.o 就可以了, 参考 [^22]
但是如果是 nasm 编译的，ld 就不需要任何特别的参数，可能是因为不需要依赖外部库吧! 
参考 compiler/programmer/extra/write.asm

关于 syscall 的参数传递，可以 system V abi 的内容: [^23] [^24] (其实只有 100 多页)

函数的 convention 和 syscall 的 convention 不同，所以需要调整一下:
https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/x86_64/syscall.S

#### x86
https://david942j.blogspot.com/2018/10/note-learning-kvm-implement-your-own.html : 通过 cs 来区分 dpl
https:/www.felixcloutier.com/x86/syscall/
From the pseudo code of sysret you can see it sets attributes of cs and ss explicitly:

TODO 分别找到 legacy 和 sysenter 的注册位置(初始化 handler) 的方法.

[^3] 放到 insides 部分记录吧!


syscall_64.c:sys_call_table : 持有所有的 syscall

x86/common.c:do_syscall_64 : 从 sys_call_table 中简单的选择函数，进行调用
  1. syscall_enter_from_user_mode  
  2. syscall_exit_from_user_mode

entry_64.S : entry_SYSCALL_64 是 syscall 的入口，这个入口的初始化 x86/kernel/cpu/common.c:syscall_init 中

这个教程很新，一共三个，非常适合阅读:
https://lwn.net/Articles/604287/ 而 [^4] 全面，但是有的没有讲解清楚.


TOOD :
1. `__kernel_vsyscall` 还是调用 sysenter 之类的，那么有什么意义啊
2. 为什么 syscall, sysenter, int 0x80 的性能区别的原因是什么 ?


## daemon
systemctl 利用 /etc/init.d/cassandra 的来启动，还是很有意思的


## user group
了解设计思想，然后阅读代码

## smp
kernel/smp.c

smp_call_function_many 之类的函数可以看看


## `__schedule`
```c
/*
 * __schedule() is the main scheduler function.
 *
 * The main means of driving the scheduler and thus entering this function are:
 *
 *   1. Explicit blocking: mutex, semaphore, waitqueue, etc.
 *
 *   2. TIF_NEED_RESCHED flag is checked on interrupt and userspace return
 *      paths. For example, see arch/x86/entry_64.S.
 *
 *      To drive preemption between tasks, the scheduler sets the flag in timer
 *      interrupt handler scheduler_tick().
 *
 *   3. Wakeups don't really cause entry into schedule(). They add a
 *      task to the run-queue and that's it.
 *
 *      Now, if the new task added to the run-queue preempts the current
 *      task, then the wakeup sets TIF_NEED_RESCHED and schedule() gets
 *      called on the nearest possible occasion:
 *
 *       - If the kernel is preemptible (CONFIG_PREEMPTION=y):
 *
 *         - in syscall or exception context, at the next outmost
 *           preempt_enable(). (this might be as soon as the wake_up()'s
 *           spin_unlock()!)
 *
 *         - in IRQ context, return from interrupt-handler to
 *           preemptible context
 *
 *       - If the kernel is not preemptible (CONFIG_PREEMPTION is not set)
 *         then at the next:
 *
 *          - cond_resched() call
 *          - explicit schedule() call
 *          - return from syscall or exception to user-space
 *          - return from interrupt-handler to user-space
 *
 * WARNING: must be called with preemption disabled!
 */
static void __sched notrace __schedule(bool preempt)
```

## TODO
https://phoenixnap.com/kb/create-a-sudo-user-on-debian : 首先搞清楚这种简单的文章


疑问: 
1. ucore lab1 的附加题的说明，实现地址空间切换的方法
    1. 似乎调整一下一个寄存器的属性就可以了，那么，syscall 是如何调整这些属性的
2. 似乎虚拟机利用中间层次的 ring  ?
3. 是什么表示当前是内核态还是用户态呀 ?

1. 第一个用户进程是如何产生的 ?
    1. idle 和 init 进程如何产生的 ?
3. IPC 和 signal 各自的适用范围是什么 ?
4. preemption 
5. 为什么需要维持 parent child 的树状关系 ?
    1. exit 回收资源 ?
    2. 部分信号机制总是只是出现在 child 和 parent 
6. 一个 thread group 都是 group leader 的 children 吗 ?



[^22]: https://stackoverflow.com/questions/14163208/how-to-link-c-object-files-with-ld
[^23]: https://stackoverflow.com/questions/18133812/where-is-the-x86-64-system-v-abi-documented
[^24]: https://uclibc.org/docs/psABI-x86_64.pdf
[^1]: [blog : Evolution of the x86 context switch in Linux](http://www.maizure.org/projects/evolution_x86_context_switch_linux/)
[^2]: https://man7.org/linux/man-pages/man7/signal.7.html 
[^3]: https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-2.html
[^4]: https://blog.packagecloud.io/eng/2016/04/05/the-definitive-guide-to-linux-system-calls/#64-bit-f
[^5]: https://www.kernel.org/doc/html/latest/x86/kernel-stacks.html
[^6]: https://stackoverflow.com/questions/61886139/why-thread-info-should-be-the-first-element-in-task-struct
[^7]: Understanding Linux Kernel
