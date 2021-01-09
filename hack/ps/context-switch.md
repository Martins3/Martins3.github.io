-> martins3 <-
# 深入理解 context switch

context switch 其实是一个很模糊的词汇，因为没有指定 switch 两端的 context 是什么。

下面从四个方面说明 context switch 的细节，并且从 MIPS 和 x64 的两种架构分别分析。

1. 函数
3. process
2. syscall
4. interrupt

## 函数的 context switch

## process 的 context switch

## syscall 的 context switch

## interrupt 的 context switch

reference
- https://os.phil-opp.com/cpu-exceptions/
- insides
- LoyenWang

[^1]: [System V Application Binary Interface](https://www.uclibc.org/docs/psABI-x86_64.pdf)


## context switch
**THIS IS WHAT TO READ BEFORE ANY FUTURE EXPLORATION OF CONTEXT SWITCH**

- [ ] [LoyenWang](https://www.cnblogs.com/LoyenWang/p/12386281.html)

- [ ]  那么 tlb flush 的操作在哪里，还是说支持了 vpid 之后可以不用 tlb flush
    - [ ]  那些位置需要 tlb flush ?

- [ ] `__schedule` ===> context_switch ===> `__switch_to_asm` ===> `__switch_to`

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

#### context switch switch_to_asm
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
inactive_task_frame 的 ret_addr 作爲返回地址。 inactive_task_frame::ret_addr 的賦值存在兩種情況:
1. fork : 在 copy_thread 中間賦值
2. 普通的 thread 切換 : 調用 `__switch_to_asm` 的時候，調用者保存的 eip 地址, 從而可以返回。



- [ ] Documentation/x86/entry_64.rst 是时候彻底搞清楚 entry_64 了啊 TODO 

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
3. 地址空间的切换在哪里 ?
    1. stack 的切换 => 整理到 stack blog => 因为 context_switch 的时候，发生在内核地址空间，其实只是需要进行，其实 rsp 的切换
- [ ] why `es` `ds` register has to be handled especially in `__switch_to` ?


#### context switch fpu

#### context switch stack

#### context switch TSS
- [ ] what's relation TSS and `thread_struct`

#### context switch TLS
process_64.c:`__switch_to` call `load_TLS` to load `ldt` from `thread_struct->tls_array` 
