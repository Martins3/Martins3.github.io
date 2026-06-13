# interrupt, execption , softirq 和 nmi 谁可以打断谁

## 中断的简单流程

完整的 x86 外部中断入口链路是这样的：

1. CPU 硬件自动执行

当中断到达时，CPU 硬件（微架构层面）自动完成：
- 压栈 RFLAGS
- 压栈 CS
- 压栈 RIP
- 清除 IF 位（关中断）
- 根据 IDT 找到对应条目，跳转

这里简化了，详细参考: https://os.phil-opp.com/cpu-exceptions/

2. IDT Stub（软件的第一行）

IDT 指向的不是 asm_common_interrupt，而是 irq_entries_start 中为每个 vector 生成的 stub（arch/x86/include/asm/idtentry.h）：

```asm
  SYM_CODE_START(irq_entries_start)
      vector=FIRST_EXTERNAL_VECTOR
      .rept NR_EXTERNAL_VECTORS
     ENDBR
     .byte   0x6a, vector        # pushq $vector  (作为 error_code)
     jmp   asm_common_interrupt
     ...
     vector = vector+1
      .endr
```

每个外部中断 vector 有自己独立的 stub，第一行可执行指令是这里的 endbr64，然后 pushq $vector，再 jmp asm_common_interrupt。

3. asm_common_interrupt（你反汇编看到的）

```txt
$ disass asm_common_interrupt
Dump of assembler code for function asm_common_interrupt:
   0xffffffff822013c0 <+0>:     endbr64
   0xffffffff822013c4 <+4>:     nop
   0xffffffff822013c5 <+5>:     nop
   0xffffffff822013c6 <+6>:     nop
   0xffffffff822013c7 <+7>:     cld
   0xffffffff822013c8 <+8>:     call   0xffffffff82201990 <error_entry> # 切换栈、保存寄存器等
   0xffffffff822013cd <+13>:    mov    %rax,%rsp
   0xffffffff822013d0 <+16>:    mov    %rsp,%rdi # pt_regs 作为第1个参数
   0xffffffff822013d3 <+19>:    mov    0x78(%rsp),%rsi # vector 作为第2个参数
   0xffffffff822013d8 <+24>:    movq   $0xffffffffffffffff,0x78(%rsp)
   0xffffffff822013e1 <+33>:    call   0xffffffff821dcf20 <common_interrupt> # C 函数
   0xffffffff822013e6 <+38>:    jmp    0xffffffff82201ad0 <error_return>
End of assembler dump.
```

## exception 的基本过程
arch/x86/include/asm/trapnr.h
```c
#define X86_TRAP_DE		 0	/* Divide-by-zero */
#define X86_TRAP_DB		 1	/* Debug */
#define X86_TRAP_NMI		 2	/* Non-maskable Interrupt */
#define X86_TRAP_BP		 3	/* Breakpoint */
#define X86_TRAP_OF		 4	/* Overflow */
#define X86_TRAP_BR		 5	/* Bound Range Exceeded */
#define X86_TRAP_UD		 6	/* Invalid Opcode */
#define X86_TRAP_NM		 7	/* Device Not Available */
#define X86_TRAP_DF		 8	/* Double Fault */
#define X86_TRAP_OLD_MF		 9	/* Coprocessor Segment Overrun */
#define X86_TRAP_TS		10	/* Invalid TSS */
#define X86_TRAP_NP		11	/* Segment Not Present */
#define X86_TRAP_SS		12	/* Stack Segment Fault */
#define X86_TRAP_GP		13	/* General Protection Fault */
#define X86_TRAP_PF		14	/* Page Fault */
#define X86_TRAP_SPURIOUS	15	/* Spurious Interrupt */
#define X86_TRAP_MF		16	/* x87 Floating-Point Exception */
#define X86_TRAP_AC		17	/* Alignment Check */
#define X86_TRAP_MC		18	/* Machine Check */
#define X86_TRAP_XF		19	/* SIMD Floating-Point Exception */
#define X86_TRAP_VE		20	/* Virtualization Exception */
#define X86_TRAP_CP		21	/* Control Protection Exception */
#define X86_TRAP_VC		29	/* VMM Communication Exception */
#define X86_TRAP_IRET		32	/* IRET Exception */
```

所有的 exception 的处理过程都是在:
- arch/x86/kernel/idt.c
- arch/x86/kernel/traps.c

流程也是一样的，只是会直接跳转到提前定义好的函数中，可以看 def_idts :
```c
static const __initconst struct idt_data def_idts[] = {
```

```txt
Dump of assembler code for function asm_exc_page_fault:
   0xffffffff82201260 <+0>:     endbr64
   0xffffffff82201264 <+4>:     nopl   (%rax)
   0xffffffff82201267 <+7>:     cld
   0xffffffff82201268 <+8>:     call   0xffffffff82201990 <error_entry>
   0xffffffff8220126d <+13>:    mov    %rax,%rsp
   0xffffffff82201270 <+16>:    mov    %rsp,%rdi
   0xffffffff82201273 <+19>:    mov    0x78(%rsp),%rsi
   0xffffffff82201278 <+24>:    movq   $0xffffffffffffffff,0x78(%rsp)
   0xffffffff82201281 <+33>:    call   0xffffffff821dfda0 <exc_page_fault>
   0xffffffff82201286 <+38>:    jmp    0xffffffff82201ad0 <error_return>
```

```txt
两者都可能在栈上留一个 8 字节的值（位于 pt_regs->orig_ax 位置）：
┌──────────────────────────────┬───────────────────────┬─────────────────────────┐
│ 类型                         │ 该位置的值            │ 含义                    │
├──────────────────────────────┼───────────────────────┼─────────────────────────┤
│ 无错误码异常（如 #DE, #NMI） │ $-1                   │ 标记"无 syscall 可重启" │
├──────────────────────────────┼───────────────────────┼─────────────────────────┤
│ 有错误码异常（如 #PF, #GP）  │ 硬件压入的 Error Code │ 描述异常原因            │
├──────────────────────────────┼───────────────────────┼─────────────────────────┤
│ 外部中断                     │ vector 号             │ 由 stub push 的向量号   │
└──────────────────────────────┴───────────────────────┴─────────────────────────┘
```

## 异常可以屏蔽中断吗?

直到我写这个文章的时候，我的一般印象是 "既然 page fault 处理过程中都可以睡眠，显然需要打开中断"，
但是实际上比我想象有趣:

| 阶段                                  | 中断状态   | 说明                                           |
|---------------------------------------|------------|------------------------------------------------|
| 进入 `asm_exc_page_fault`             | ❌ 关      | 中断门硬件自动清除 `EFLAGS.IF`                 |
| 内核态缺页处理 (`do_kern_addr_fault`) | ❌ 关      | 内核路径不能睡眠，全程保持关闭                 |
| 用户态正常缺页 (`do_user_addr_fault`) | ✅ 开      | `local_irq_enable()`，允许缺页处理期间响应中断 |
| `handle_page_fault` 返回前            | ❌ 关      | `local_irq_disable()`，确保异常返回时中断必关  |
| `iret` 返回用户/内核态                | 恢复原状态 | 硬件从栈上恢复原始 `EFLAGS.IF`                 |


## 屏蔽中断

- disable_irq() :  确保中断 handler 完全停止，比如卸载驱动前
- local_irq_disable() : 临界区保护，短时间关闭本 CPU 所有中断

## 中断可以嵌套?
```c
noinstr irqentry_state_t irqentry_enter(struct pt_regs *regs)
{
	irqentry_state_t ret = {
		.exit_rcu = false,
	};

	if (user_mode(regs)) {
		irqentry_enter_from_user_mode(regs);
		return ret;
	}

	/*
	 * If this entry hit the idle task invoke ct_irq_enter() whether
	 * RCU is watching or not.
	 *
	 * Interrupts can nest when the first interrupt invokes softirq
	 * processing on return which enables interrupts.
	 *
	 * Scheduler ticks in the idle task can mark quiescent state and
	 * terminate a grace period, if and only if the timer interrupt is
	 * not nested into another interrupt.
	 *
	 * Checking for rcu_is_watching() here would prevent the nesting
	 * interrupt to invoke ct_irq_enter(). If that nested interrupt is
	 * the tick then rcu_flavor_sched_clock_irq() would wrongfully
	 * assume that it is the first interrupt and eventually claim
	 * quiescent state and end grace periods prematurely.
	 *
	 * Unconditionally invoke ct_irq_enter() so RCU state stays
	 * consistent.
	 *
	 * TINY_RCU does not support EQS, so let the compiler eliminate
	 * this part when enabled.
	 */
```

> Interrupts can nest when the first interrupt invokes softirq processing on return which enables interrupts.

虽然没有十足的证据，但是基本可以确定，是在 softirq 的时候才会打开中断的。

感觉回顾一下 do_softirq 和 raise_softirq 大约就可以理解了吧!

```c
static inline void __irq_exit_rcu(void)
{
#ifndef __ARCH_IRQ_EXIT_IRQS_DISABLED
  local_irq_disable();
#else
  lockdep_assert_irqs_disabled();
#endif
  account_hardirq_exit(current);
  preempt_count_sub(HARDIRQ_OFFSET);
  if (!in_interrupt() && local_softirq_pending())
    invoke_softirq();

  tick_irq_exit();
}
```

### 为什么需要 local_irq_disable

一种解释是，
在 `__handle_irq_event_percpu` 中存在这个判断，也就是说，有的中断处理函数会犯错，
但是大多数函数不会如此，否则，bottom half 和 top half 的意义在什么地方！

```c
		if (WARN_ONCE(!irqs_disabled(),"irq %u handler %pS enabled interrupts\n",
			      irq, action->handler))
			local_irq_disable();
```

__ARCH_IRQ_EXIT_IRQS_DISABLED 在 arm 中定义了，但是在 x86 没有定义，也就是说，
x86 可能会有到 `__irq_exit_rcu` 的时候，中断是打开的。

使用 ftrace_function 来跟踪 irq_exit_rcu ，虽然的确是在进入 __irq_exit_rcu 之前，
中断已经是关闭的，所以不太清楚中断到底什么时候会打开。
```txt
   rs:main Q:Reg-3135    [098] D.h.. 956086.831771: irq_exit_rcu <-sysvec_apic_timer_interrupt
          <idle>-0       [050] dNh1. 956086.470324: irq_exit_rcu <-sysvec_call_function_single
```

### in_interrupt 的判断
invoke_softirq -> __do_softirq ，说明上面的 in_interrupt() 结果一般为否，
但是，如果中断是屏蔽的情况，

irq_enter_rcu 中调用 `__irq_enter_raw`，导致 preempt_count_add(HARDIRQ_OFFSET) 判断为真，
但是 invoke_softirq 上面马上做了一个 preempt_count_sub(HARDIRQ_OFFSET);

这里这里的逻辑很合理，就是如果 HARDIRQ_OFFSET 被去掉之后，就不是 in_hardirq 了，
然后就可以执行 softirq ，如果有嵌套中断，即便是 preempt_count_sub(HARDIRQ_OFFSET)
当前还是 in_hardirq ，那么就不可以执行中断。

## 中断其实是不可以嵌套的
```c
/*
 * We put the hardirq and softirq counter into the preemption
 * counter. The bitmask has the following meaning:
 *
 * - bits 0-7 are the preemption count (max preemption depth: 256)
 * - bits 8-15 are the softirq count (max # of softirqs: 256)
 *
 * The hardirq count could in theory be the same as the number of
 * interrupts in the system, but we run all interrupt handlers with
 * interrupts disabled, so we cannot have nesting interrupts. Though
 * there are a few palaeontologic drivers which reenable interrupts in
 * the handler, so we need more than one bit here.
 *
 *         PREEMPT_MASK:	0x000000ff
 *         SOFTIRQ_MASK:	0x0000ff00
 *         HARDIRQ_MASK:	0x000f0000
 *             NMI_MASK:	0x00f00000
 * PREEMPT_NEED_RESCHED:	0x80000000
 */
#define PREEMPT_BITS	8
#define SOFTIRQ_BITS	8
#define HARDIRQ_BITS	4
#define NMI_BITS	4
```
其实这里也总结了，softirq hardirq 和 preempt 是否可以被嵌套。
preempt 嵌套指的是，被打断之后，继续被打断

这两个回答都是错误的:
- https://stackoverflow.com/questions/34527763/linux-nested-interrupts
- https://stackoverflow.com/questions/5934402/can-an-interrupt-handler-be-preempted

这个是对的:
- https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html

patch 也是说明曾经是支持的: https://lwn.net/Articles/380937/

## 首先需要将 exception 整理下

double fault
triple fault

intel sdm 中的各种 nested exception 的描述

## 参考
http://liujunming.top/2020/12/01/VT-x-Information-for-VM-Exits-During-Event-Delivery/
amd64/intel-sdm/v3-ch28.md 中的 28.2.4 Information for VM Exits During Event Delivery


## exception 和中断的嵌套
<!-- b7b8a70c-3dc6-4c37-b2f4-c1c807ca1f13 -->

配套实验 : vn/code/src/m/exception.c

1. exception 嵌套 : 可以，然后就是 double fault 了。
2. 中断嵌套 : 不可以
3. 中断中有 exception : hrtimer +
4. exception 中触发中断 : 可以的，在触发 page fault 的时候，就是普通的 process 上下文，那个 process 被打断，
就是那个 process 的上下文。
```txt
@[
        handle_userfault+5
        __handle_mm_fault+1370
        handle_mm_fault+279
        do_user_addr_fault+707
        exc_page_fault+116
        asm_exc_page_fault+38
]: 6

# tracer: function
#
# entries-in-buffer/entries-written: 0/0   #P:32
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
  userfault2.out-1364258 [012] ..... 188446.936791: handle_userfault <-__handle_mm_fault
  userfault2.out-1364258 [012] ..... 188446.942796: handle_userfault <-__handle_mm_fault
  userfault2.out-1364258 [012] ..... 188446.946505: handle_userfault <-__handle_mm_fault
```
内核中显然是可以触发 exceptions 的，例如访问用户态的地址，copy to user 的时候，触发 page fault 。
exception 的触发过程总是走到内核中的，但是在内核中会继续来判断，该告诉 user 还是该 panic

除零或除法溢出
```txt
[186611.998670] traps: a.out[1344026] trap divide error ip:40
11d6 sp:7ffc09be10b0 error:0 in a.out[11d6,401000+1000]
```

nmi 可以穿透一切，制作一个 hardlockup 的 bascktrace ，也许可以看到 nmi -> hardirq -> softirq 的三个 backtrace

## 既然 exception 一件稀松平常的事情，那么是如何识别 double fault 的

看上去在内核中，page fault 的处理路径也没什么特殊的东西，那么 CPU 是需要知道当前正在处理 execption 吗？

需要有什么标记吗?

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
