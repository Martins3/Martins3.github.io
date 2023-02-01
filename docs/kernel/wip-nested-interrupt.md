# 中断是嵌套的吗?

- [ ] 异常会自动屏蔽 interrupt 吗？
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
