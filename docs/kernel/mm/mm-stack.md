# kernel stack

## 关键文档
https://www.kernel.org/doc/html/latest/arch/x86/kernel-stacks.html

x86_64 also has a feature which is not available on i386,
the ability to automatically switch to a new stack for designated events such
as double fault or NMI, which makes it easier to handle these unusual events on x86_64.
This feature is called the Interrupt Stack Table (IST).
There can be up to 7 IST entries per CPU. The IST code is an index into the Task State Segment (TSS).
The IST entries in
the TSS point to dedicated stacks; each stack can be a different size.


## stack 的使用
### 软中断

> [!NOTE]
> 参考 Deepseeek ，有待验证

软中断（Softirq）是中断处理的“下半部”（Bottom Half），它的栈使用情况比较特殊，分为两种情况：
情况A（最常见）：当一个硬件中断处理程序（Hard IRQ handler）执行完毕返回时，内核会检查是否有待处理的软中断。如果有，它会立即在同一个CPU上、继续使用当前的中断栈来执行软中断处理函数。这样做效率很高，避免了不必要的栈切换。
情况B（高负载时）：如果软中断的负载非常高，不能在中断返回路径上处理完，那么剩余的软中断任务会被一个名为 ksoftirqd 的内核线程接管。ksoftirqd 是一个普通的内核线程，所以当它运行时，它使用的是它自己的进程内核栈。
所以，软中断要么使用中断栈，要么使用**ksoftirqd 线程的内核栈**。

1. 在 `invoke_softirq` 中，会根据 `CONFIG_HAVE_IRQ_EXIT_ON_IRQ_STACK` 分析到底是使用

#### config HAVE_IRQ_EXIT_ON_IRQ_STACK

现在，可以买的到硬件都是 CONFIG_HAVE_IRQ_EXIT_ON_IRQ_STACK ，不去考虑那么多，
也就是 invoke_softirq 继续调用 __do_softirq 就是在 irq stack 上的。
```txt
config HAVE_IRQ_EXIT_ON_IRQ_STACK
	bool
	help
	  Architecture doesn't only execute the irq handler on the irq stack
	  but also irq_exit(). This way we can process softirqs on this irq
	  stack instead of switching to a new one when we call __do_softirq()
	  in the end of an hardirq.
	  This spares a stack switch and improves cache usage on softirq
	  processing.
```

```c
static inline void invoke_softirq(void)
{
	if (!force_irqthreads() || !__this_cpu_read(ksoftirqd)) {
#ifdef CONFIG_HAVE_IRQ_EXIT_ON_IRQ_STACK
		/*
		 * We can safely execute softirq on the current stack if
		 * it is the irq stack, because it should be near empty
		 * at this stage.
		 */
		__do_softirq();
#else
		/*
		 * Otherwise, irq_exit() is called on the task stack that can
		 * be potentially deep already. So call softirq in its own stack
		 * to prevent from any overrun.
		 */
		do_softirq_own_stack();
#endif
	} else {
		wakeup_softirqd();
	}
}
```

#### do_softirq_own_stack

do_softirq -> do_softirq_own_stack ，之前一直没有理解 do_softirq_own_stack 的注释的意思，
这个也是 softirq 导致什么 stack 上工作的干扰，其实原理很简单。
- 首先，do_softirq 是在 task context 上调用的，当需要打开 bh 的时候，会来调用 do_softirq
- 此时，irqstack 显然不会正在被使用，因为当前在 task context ，所有的中断都执行完成之后，才可能切换到 task context 中
- 还是由于为了防止出现 stack overflow ，所以使用 irqstack 而非继续在 task 的 stack 上执行
```c
#ifndef CONFIG_PREEMPT_RT
/*
 * Macro to invoke __do_softirq on the irq stack. This is only called from
 * task context when bottom halves are about to be reenabled and soft
 * interrupts are pending to be processed. The interrupt stack cannot be in
 * use here.
 */
#define do_softirq_own_stack()                      \
{                                   \
    __this_cpu_write(hardirq_stack_inuse, true);            \
    call_on_irqstack(__do_softirq, ASM_CALL_ARG0);          \
    __this_cpu_write(hardirq_stack_inuse, false);           \
}

#endif
```

- [ ] 还是和 preempt 有关的，而且是关闭的时候才是如此。

### 硬中断
percpu 一个的

#### 初始化
aarch64 中 CONFIG_IRQSTACKS 默认打开的:

arch/arm64/kernel/irq.c
```c
static void __init init_irq_stacks(void)
{
	int cpu;
	unsigned long *p;

	for_each_possible_cpu(cpu) {
		p = arch_alloc_vmap_stack(IRQ_STACK_SIZE, early_cpu_to_node(cpu));
		per_cpu(irq_stack_ptr, cpu) = p;
	}
}
```

在 x86 中

arch/x86/kernel/irq.c
```txt
DEFINE_PER_CPU_CACHE_HOT(struct irq_stack *, hardirq_stack_ptr);
```


#### 在用户态触发中断会切换两次 stack ?
- [ ] https://unix.stackexchange.com/questions/491437/how-does-linux-kernel-switches-from-kernel-stack-to-interrupt-stack?rq=1

- 这个回答说，如果发生在用户态，会进行两次 stack 切换，我是不信的

从 call_on_irqstack_cond 注释看，如果从用户态进入，task stack 本来就是空的，
所以直接使用 task stack 就可以了

```c
#define call_on_irqstack(func, asm_call, argconstr...)			\
	call_on_stack(__this_cpu_read(pcpu_hot.hardirq_stack_ptr),	\
		      func, asm_call, argconstr)
```

```c
/*
 * Macro to invoke system vector and device interrupt C handlers.
 */
#define call_on_irqstack_cond(func, regs, asm_call, constr, c_args...)	\
{									\
	/*								\
	 * User mode entry and interrupt on the irq stack do not	\
	 * switch stacks. If from user mode the task stack is empty.	\
	 */								\
	if (user_mode(regs) || __this_cpu_read(pcpu_hot.hardirq_stack_inuse)) { \
		irq_enter_rcu();					\
		func(c_args); 						\
		irq_exit_rcu();						\
	} else {							\
		/*							\
		 * Mark the irq stack inuse _before_ and unmark _after_	\
		 * switching stacks. Interrupts are disabled in both	\
		 * places. Invoke the stack switch macro with the call	\
		 * sequence which matches the above direct invocation.	\
		 */							\
		__this_cpu_write(pcpu_hot.hardirq_stack_inuse, true);	\
		call_on_irqstack(func, asm_call, constr);		\
		__this_cpu_write(pcpu_hot.hardirq_stack_inuse, false);	\
	}								\
}
```

似乎可以通过看 backtrace 中是否有 call_on_irqstack 来确定是个中断当时
是在内核态触发的还是用户态的触发的:


不过需要注意，这个 call_on_irq_stack 是 arm 环境的函数:

```txt
@[
        md_wakeup_thread+0
        end_sync_read+152
        bio_endio+388
        blk_mq_end_request_batch+608
        nvme_pci_complete_batch+100
        nvme_irq+128
        __handle_irq_event_percpu+148
        handle_irq_event+84
        handle_fasteoi_irq+168
        handle_irq_desc+60
        generic_handle_domain_irq+36
        gic_handle_irq+84
        call_on_irq_stack+36 (call_on_irqstack)
        do_interrupt_handler+136
        el1_interrupt+52
        el1h_64_irq_handler+24
        el1h_64_irq+108
        __pi_memcmp+116
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 184
```

这个是 x86 的，从这里可以看到中断的时候，当时正好是用户态还是内核态:
```txt
[
        handle_irq_event+5
        handle_edge_irq+159
        __common_interrupt+77
        common_interrupt+128
        asm_common_interrupt+38
        pv_native_safe_halt+15
        default_idle+19
        default_idle_call+122
        do_idle+480
        cpu_startup_entry+41
        rest_init+348
        start_kernel+1753
        x86_64_start_reservations+36
        x86_64_start_kernel+196
        common_startup_64+318
]: 4305
@[
        handle_irq_event+5
        handle_edge_irq+159
        __common_interrupt+77
        common_interrupt+66
        asm_common_interrupt+38
]: 9764
```

可以解释 backtrace 为什么可以将中断的打印出来了。

差不多了，也是清楚的。

- TSS 可以找到内核 stack, 那么靠什么找到 IRQ stack ?


### exception
当前进程上下文中



### 用户态的 stack

- fork 的时候，内核态必然对应的创建一个 stack ，
用户态的 stack 取决于是 CLONE_VM 这个 flag ，要么完全复制，要么需要提供给 clone 系统调用一个新的区域作为
child 的 stack

- [ ] stack 的增长和缩减的时机和方法，如果由于递归创建出来了一个巨大的 stack，然后逐级 return 之后，这些 stack 的空间会被回收吗 ?
2. 用户态的 stack 为什么使用 brk() 系统调用，而不是 mmap ?
3. stack 是使用系统调用分配的 ? 还是 fork 的时候创建的 ? stack 对应的 vma 是什么创建的 ?

- [ ] 用户程序的 argc 和 argv 以及 环境变量是放在 stack 的哪一个位置的 ?

## 和 context switch 的关系
1. 程序启动的参数放在哪里？什么栈 环境变量存放的位置在什么地方(CSAPP 提到过，PA 试验, 进程初始化的过程是什么)
2. 进程切换的时候，stack 是如何切换的
3. 当 syscall 的时候，stack 如何切换，和上面的切换有什么区别和不同之处
  - 到底切换 stack 麻烦的在于，函数执行需要 stack 的支持，但是切换 stack 是通过调用函数实现的，实现切换的过程真空期如何处理。

## 问题

### vma 的名称
cat /proc/self/maps : vma 有名字吗，他怎么知道是 stack
```plain
7ffc83f45000-7ffc83f67000 rw-p 00000000 00:00 0                          [stack]
```

### stack overflow attack
缓冲区在 stack 中间，如果让 stack 不可执行，那么不就结束了。

程序中发生函数调用时，计算机做如下操作：
- 首先把指令寄存器 EIP（它指向当前 CPU 将要运行的 下一条指令的地址）中的内容压入栈，作为程序的返回地址（下文中用 RET 表示）；
- 之后放入栈的是基址寄存器 EBP，它指向当前函数栈 帧（stack frame） 的底部；
- 然后把当前的栈指针 ESP 拷贝到 EBP，作为新的基地 址，
- 最后为本地变量的动态存储分配留出一定空间，并把 ESP 减去适当的数值

提高成功率的两种方法 :
1. 在 shellcode 前面添加填充代码
2. 在返回值附近重复跳转地址

### task_struct 是放到 stack 上的吗?
<!-- dc112fe5-fbd5-40de-9af8-28134d4c3588 -->

不是

从 thread_union 看，似乎 task_struct 在 stack 上，检查代码，这个可能只是 init 线程临时使用的:
```c
union thread_union {
	struct task_struct task;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};
```

通过 mm/stack.c 中，可以看到 stack 和 task_struct 是可以分离的
```c
struct task_struct {
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;
  // ...
  void				*stack;
  // ...
  /* CPU-specific state of this task: */
	struct thread_struct		thread;
}
```

从 alloc_thread_stack_node 中 stack 是这样分配的:
```c
	/*
	 * Allocated stacks are cached and later reused by new threads,
	 * so memcg accounting is performed manually on assigning/releasing
	 * stacks to tasks. Drop __GFP_ACCOUNT.
	 */
	stack = __vmalloc_node(THREAD_SIZE, THREAD_ALIGN,
				     THREADINFO_GFP & ~__GFP_ACCOUNT,
				     node, __builtin_return_address(0));

```

而 task_struct 是通过 slab 机制分配的
```c
static inline struct task_struct *alloc_task_struct_node(int node)
{
	return kmem_cache_alloc_node(task_struct_cachep, GFP_KERNEL, node);
}
```
所以，可以通过 /sys/kernel/slab/task_struct 获取 task_struct 分配的详细信息。

## 获取 task_struct

之前一些误导，似乎 task_struct 需要借助其他的方法才可以获取到，但是实际上，很简单:
```c
static __always_inline struct task_struct *get_current(void)
{
	if (IS_ENABLED(CONFIG_USE_X86_SEG_SUPPORT))
		return this_cpu_read_const(const_current_task);

	return this_cpu_read_stable(current_task);
}

#define current get_current()
```

## VMAP_STACK
```txt
config VMAP_STACK
	default y
	bool "Use a virtually-mapped stack"
	depends on HAVE_ARCH_VMAP_STACK
	depends on !KASAN || KASAN_HW_TAGS || KASAN_VMALLOC
	help
	  Enable this if you want the use virtually-mapped kernel stacks
	  with guard pages.  This causes kernel stack overflows to be
	  caught immediately rather than causing difficult-to-diagnose
	  corruption.

	  To use this with software KASAN modes, the architecture must support
	  backing virtual mappings with real shadow memory, and KASAN_VMALLOC
	  must be enabled.
```


## shadow stack

-cpu host 直通的时候 guest os 还是没有

但是 host 上是有这个的:

```txt
CET_SS: CET shadow stack                 = false
```


似乎是 2023 才进入的 feature
```txt
History:        #0
Commit:         18e66b695e787374ca762ecdeaa1ab5e3772af94
Author:         Rick Edgecombe <rick.p.edgecombe@intel.com>
Author Date:    Tue 13 Jun 2023 08:10:32 AM CST
Committer Date: Wed 12 Jul 2023 05:12:18 AM CST

x86/shstk: Add Kconfig option for shadow stack

Shadow stack provides protection for applications against function return
address corruption. It is active when the processor supports it, the
kernel has CONFIG_X86_SHADOW_STACK enabled, and the application is built
for the feature. This is only implemented for the 64-bit kernel. When it
is enabled, legacy non-shadow stack applications continue to work, but
without protection.

Since there is another feature that utilizes CET (Kernel IBT) that will
share implementation with shadow stacks, create CONFIG_CET to signify
that at least one CET feature is configured.

Co-developed-by: Yu-cheng Yu <yu-cheng.yu@intel.com>
Signed-off-by: Yu-cheng Yu <yu-cheng.yu@intel.com>
Signed-off-by: Rick Edgecombe <rick.p.edgecombe@intel.com>
Signed-off-by: Dave Hansen <dave.hansen@linux.intel.com>
Reviewed-by: Borislav Petkov (AMD) <bp@alien8.de>
Reviewed-by: Kees Cook <keescook@chromium.org>
Acked-by: Mike Rapoport (IBM) <rppt@kernel.org>
Tested-by: Pengfei Xu <pengfei.xu@intel.com>
Tested-by: John Allen <john.allen@amd.com>
Tested-by: Kees Cook <keescook@chromium.org>
Link: https://lore.kernel.org/all/20230613001108.3040476-7-rick.p.edgecombe%40intel.com
```

原来，VM_SHADOW_STACK 是这个含义啊:
```c
pte_t pte_mkwrite(pte_t pte, struct vm_area_struct *vma)
{
	if (vma->vm_flags & VM_SHADOW_STACK)
		return pte_mkwrite_shstk(pte);

	pte = pte_mkwrite_novma(pte);

	return pte_clear_saveddirty(pte);
}
```

https://docs.kernel.org/next/x86/shstk.html

SDM vol3 26.4.3 Shadow-Stack Updates

非常奇怪，即便是将 -cpu host ，l1 中也是看不到 CET 的了

## CONFIG_DEBUG_STACK_USAGE
<!-- 52a9cdb1-02a5-44e0-ba87-973a47134f6b -->

观察到虚拟机才有这个，在 nixos 的物理机中从来没有这个问题
```txt
[   10.053291] 10.0.0.2-manage (2288) used greatest stack depth: 12176 bytes left
[   10.107265] mount.nfs (2281) used greatest stack depth: 10576 bytes left
[  260.502203] rg (4669) used greatest stack depth: 10552 bytes left
```
原因是打开了 CONFIG_DEBUG_STACK_USAGE

## TODO
call_on_irqstack_cond 中的 stack 测试一下

控制 virtio dummy 的中断亲和性，似乎不难，但是到此为止

## 应该是无论是在用户态还是内核态，中断的时候 stack 都是 CPU 对应的 stack

map_irq_stack 中设置 stack 的位置是:

```txt
$1 = (void *) 0xffffc90000000000
```
0xffffc900000c8f58 <---- interrupt
0xffffc90000d63d30 <---- exception

0xffffc90001253f18 <----- syscall 的

对于 double fault 之类的，stack 在其他的位置:
https://www.kernel.org/doc/Documentation/x86/kernel-stacks

> Like the split thread and interrupt stacks on i386, this gives more room
>  for kernel interrupt processing without having to increase the size
>  of every per thread stack.

用户态中断存在两次转换？
> https://unix.stackexchange.com/questions/491437/how-does-linux-kernel-switches-from-kernel-stack-to-interrupt-stack
> 这个回答应该是误导人的吧?
> 分析 asm_common_interrupt 以及 asm_exc_page_fault 中，都是直接就开始上下文保存

https://stackoverflow.com/questions/38360312/how-does-linux-kernel-switch-between-user-mode-and-kernel-mode-stack
- 在 TSS 中保存了用户态进程的 kernel stack，当出现 exception 的时候，自动使用该 stack

总结：
- TSS : thread kernel stack；
- IST : interrupt stack；
- 内核和用户态使用的 stack 不做区分；
- exception 应该是 kernel stack，因为 syscall 就是这个 stack 的。

## signal handler 的 stack
man signal(7)

 A  process can change the disposition of a signal using sigaction(2) or signal(2).

- [x] By default, a signal handler is invoked on the normal process stack.
It is possible to arrange that the signal handler uses an alternate stack;
see sigaltstack(2) for a discussion of how to do this and when it might be useful.

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
