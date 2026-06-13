# 中断是嵌套的吗?

- [x] 异常会自动屏蔽 interrupt 吗？
  - 应该是会的
- [ ] 真的会存在只是 disable 一个 number 的中断的操作吗？

```txt
Dump of assembler code for function common_interrupt:
   0xffffffff821dcf20 <+0>:     endbr64
   0xffffffff821dcf24 <+4>:     push   %r13
   0xffffffff821dcf26 <+6>:     push   %r12
   0xffffffff821dcf28 <+8>:     push   %rbp
   0xffffffff821dcf29 <+9>:     mov    %rdi,%rbp
   0xffffffff821dcf2c <+12>:    push   %rbx
   0xffffffff821dcf2d <+13>:    mov    %rsi,%rbx
   0xffffffff821dcf30 <+16>:    movzbl %bl,%r13d
   0xffffffff821dcf34 <+20>:    call   0xffffffff821e0130 <irqentry_enter>
   0xffffffff821dcf39 <+25>:    mov    $0xffffffff829d7857,%rdi
   0xffffffff821dcf40 <+32>:    mov    %eax,%r12d
   0xffffffff821dcf43 <+35>:    call   0xffffffff821e09c0 <__this_cpu_preempt_check>
   0xffffffff821dcf48 <+40>:    movb   $0x1,%gs:0x7de4f1f0(%rip)        # 0x2c140 <irq_stat>
   0xffffffff821dcf50 <+48>:    testb  $0x3,0x88(%rbp)
   0xffffffff821dcf57 <+55>:    jne    0xffffffff821dcf70 <common_interrupt+80>
   0xffffffff821dcf59 <+57>:    mov    $0xffffffff82a0db99,%rdi
   0xffffffff821dcf60 <+64>:    call   0xffffffff821e09c0 <__this_cpu_preempt_check>
   0xffffffff821dcf65 <+69>:    mov    %gs:0x7de4f2be(%rip),%al        # 0x2c22a <pcpu_hot+42>
   0xffffffff821dcf6c <+76>:    test   %al,%al
   0xffffffff821dcf6e <+78>:    je     0xffffffff821dcf96 <common_interrupt+118>
   0xffffffff821dcf70 <+80>:    call   0xffffffff81132e90 <irq_enter_rcu>
   0xffffffff821dcf75 <+85>:    mov    %r13d,%esi
   0xffffffff821dcf78 <+88>:    mov    %rbp,%rdi
   0xffffffff821dcf7b <+91>:    call   0xffffffff810ce440 <__common_interrupt>
   0xffffffff821dcf80 <+96>:    call   0xffffffff81132f10 <irq_exit_rcu>
   0xffffffff821dcf85 <+101>:   pop    %rbx
   0xffffffff821dcf86 <+102>:   mov    %r12d,%esi
   0xffffffff821dcf89 <+105>:   mov    %rbp,%rdi
   0xffffffff821dcf8c <+108>:   pop    %rbp
   0xffffffff821dcf8d <+109>:   pop    %r12
   0xffffffff821dcf8f <+111>:   pop    %r13
   0xffffffff821dcf91 <+113>:   jmp    0xffffffff821e01b0 <irqentry_exit>
   0xffffffff821dcf96 <+118>:   mov    $0xffffffff829d7857,%rdi
   0xffffffff821dcf9d <+125>:   movzbl %bl,%ebx
   0xffffffff821dcfa0 <+128>:   call   0xffffffff821e09c0 <__this_cpu_preempt_check>
   0xffffffff821dcfa5 <+133>:   mov    $0xffffffff82a0db99,%rdi
   0xffffffff821dcfac <+140>:   movb   $0x1,%gs:0x7de4f276(%rip)        # 0x2c22a <pcpu_hot+42>
   0xffffffff821dcfb4 <+148>:   call   0xffffffff821e09c0 <__this_cpu_preempt_check>
   0xffffffff821dcfb9 <+153>:   mov    %gs:0x7de4f25f(%rip),%r11        # 0x2c220 <pcpu_hot+32>
   0xffffffff821dcfc1 <+161>:   mov    %rsp,(%r11)
   0xffffffff821dcfc4 <+164>:   mov    %r11,%rsp
   0xffffffff821dcfc7 <+167>:   call   0xffffffff81132e90 <irq_enter_rcu>
   0xffffffff821dcfcc <+172>:   mov    %rbx,%rsi
   0xffffffff821dcfcf <+175>:   mov    %rbp,%rdi
   0xffffffff821dcfd2 <+178>:   call   0xffffffff810ce440 <__common_interrupt>
   0xffffffff821dcfd7 <+183>:   call   0xffffffff81132f10 <irq_exit_rcu>
   0xffffffff821dcfdc <+188>:   pop    %rsp
   0xffffffff821dcfdd <+189>:   mov    $0xffffffff829d7857,%rdi
   0xffffffff821dcfe4 <+196>:   call   0xffffffff821e09c0 <__this_cpu_preempt_check>
   0xffffffff821dcfe9 <+201>:   movb   $0x0,%gs:0x7de4f239(%rip)        # 0x2c22a <pcpu_hot+42>
   0xffffffff821dcff1 <+209>:   jmp    0xffffffff821dcf85 <common_interrupt+101>
End of assembler dump.
$ disass asm_common_interrupt
Dump of assembler code for function asm_common_interrupt:
   0xffffffff822013c0 <+0>:     endbr64
   0xffffffff822013c4 <+4>:     nop
   0xffffffff822013c5 <+5>:     nop
   0xffffffff822013c6 <+6>:     nop
   0xffffffff822013c7 <+7>:     cld
   0xffffffff822013c8 <+8>:     call   0xffffffff82201990 <error_entry>
   0xffffffff822013cd <+13>:    mov    %rax,%rsp
   0xffffffff822013d0 <+16>:    mov    %rsp,%rdi
   0xffffffff822013d3 <+19>:    mov    0x78(%rsp),%rsi
   0xffffffff822013d8 <+24>:    movq   $0xffffffffffffffff,0x78(%rsp)
   0xffffffff822013e1 <+33>:    call   0xffffffff821dcf20 <common_interrupt>
   0xffffffff822013e6 <+38>:    jmp    0xffffffff82201ad0 <error_return>
End of assembler dump.
```
上下文的保存，exception 也是调用 common_interrupt 的。


## 第一个函数
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

## 补充一个 exception 的例子
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

- irq_exit
  - `__irq_exit_rcu`

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

用户态中断存在两次装换？
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

## 当我已经确定中断不会嵌套的时候，但是看到这个函数

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

```txt
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

最终解释内容:
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

这里的回答是对的:
- https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html

这个 patch 也是说明曾经是支持的:
https://lwn.net/Articles/380937/

## 首先需要将 exception 整理下

double fault
triple fault

intel sdm 中的各种 nested exception 的描述
## 修改 DE 的 idt

```diff
diff --git a/arch/x86/kernel/idt.c b/arch/x86/kernel/idt.c
index f445bec516a0..89f65526ef51 100644
--- a/arch/x86/kernel/idt.c
+++ b/arch/x86/kernel/idt.c
@@ -29,10 +29,24 @@
 		.segment	= _segment,		\
 	}

+#define G2(_vector, _addr, _ist, _type, _dpl, _segment)	\
+	{						\
+		.vector		= _vector,		\
+		.bits.ist	= _ist,			\
+		.bits.type	= _type,		\
+		.bits.dpl	= _dpl,			\
+		.bits.p		= 0,			\
+		.addr		= _addr,		\
+		.segment	= _segment,		\
+	}
+
 /* Interrupt gate */
 #define INTG(_vector, _addr)				\
 	G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL0, __KERNEL_CS)

+#define INTG2(_vector, _addr)				\
+	G2(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL0, __KERNEL_CS)
+
 /* System interrupt gate */
 #define SYSG(_vector, _addr)				\
 	G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL3, __KERNEL_CS)
@@ -82,7 +96,7 @@ static const __initconst struct idt_data early_idts[] = {
  * set up TSS.
  */
 static const __initconst struct idt_data def_idts[] = {
-	INTG(X86_TRAP_DE,		asm_exc_divide_error),
+	INTG2(X86_TRAP_DE,		asm_exc_divide_error),
 	ISTG(X86_TRAP_NMI,		asm_exc_nmi, IST_INDEX_NMI),
 	INTG(X86_TRAP_BR,		asm_exc_bounds),
 	INTG(X86_TRAP_UD,		asm_exc_invalid_op),
```
会触发 double fault ，没有想象的 ev

## kvm exist
```diff
diff --git a/arch/x86/kvm/vmx/vmx.c b/arch/x86/kvm/vmx/vmx.c
index 47f899c23d35..b83fe9d8fe76 100644
--- a/arch/x86/kvm/vmx/vmx.c
+++ b/arch/x86/kvm/vmx/vmx.c
@@ -870,7 +870,7 @@ void vmx_update_exception_bitmap(struct kvm_vcpu *vcpu)
 	u32 eb;

 	eb = (1u << PF_VECTOR) | (1u << UD_VECTOR) | (1u << MC_VECTOR) |
-	     (1u << DB_VECTOR) | (1u << AC_VECTOR);
+	     (1u << DB_VECTOR) | (1u << AC_VECTOR) | (1u << DE_VECTOR);
 	/*
 	 * #VE isn't used for VMX.  To test against unexpected changes
 	 * related to #VE for VMX, intercept unexpected #VE and warn on it.
@@ -5371,6 +5371,10 @@ static int handle_exception_nmi(struct kvm_vcpu *vcpu)
 		if (handle_guest_split_lock(kvm_rip_read(vcpu)))
 			return 1;
 		fallthrough;
+	case DE_VECTOR:
+		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
+		kvm_queue_exception(vcpu, DE_VECTOR);
+		return 1;
 	default:
 		kvm_run->exit_reason = KVM_EXIT_EXCEPTION;
 		kvm_run->ex.exception = ex_no;
```

在 guest os 中触发，最后可以看到日志

如果将 `kvm_queue_exception` 去掉，那么:
1. guest 会永远停留在除法指令上
2. 可以观测到高频的 handle_exception_nmi

## 参考
- http://liujunming.top/2020/12/01/VT-x-Information-for-VM-Exits-During-Event-Delivery/

amd64/intel-sdm/v3-ch28.md 中的 28.2.4 Information for VM Exits During Event Delivery

## 内核处理 exception 的基本过程
arch/x86/include/asm/trapnr.h

一共就这么多的 exception 的:
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
