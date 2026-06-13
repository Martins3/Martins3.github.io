# idt
可以首先读读 [Writing an OS in Rust : CPU Exceptions](https://os.phil-opp.com/cpu-exceptions/) 来复习一下 x86 中断。
```c
/**
 * idt_setup_apic_and_irq_gates - Setup APIC/SMP and normal interrupt gates
 */
void __init idt_setup_apic_and_irq_gates(void)
{
  int i = FIRST_EXTERNAL_VECTOR;
  void *entry;

  idt_setup_from_table(idt_table, apic_idts, ARRAY_SIZE(apic_idts), true);

  for_each_clear_bit_from(i, system_vectors, FIRST_SYSTEM_VECTOR) {
    entry = irq_entries_start + 8 * (i - FIRST_EXTERNAL_VECTOR);
    set_intr_gate(i, entry);
}
```

在 idtentry.h 中间:
```asm
/*
 * ASM code to emit the common vector entry stubs where each stub is
 * packed into 8 bytes.
 *
 * Note, that the 'pushq imm8' is emitted via '.byte 0x6a, vector' because
 * GCC treats the local vector variable as unsigned int and would expand
 * all vectors above 0x7F to a 5 byte push. The original code did an
 * adjustment of the vector number to be in the signed byte range to avoid
 * this. While clever it's mindboggling counterintuitive and requires the
 * odd conversion back to a real vector number in the C entry points. Using
 * .byte achieves the same thing and the only fixup needed in the C entry
 * point is to mask off the bits above bit 7 because the push is sign
 * extending.
 */
  .align 8
SYM_CODE_START(irq_entries_start)
    vector=FIRST_EXTERNAL_VECTOR // FIRST_EXTERNAL_VECTOR 是 0x20
    .rept NR_EXTERNAL_VECTORS    //
  UNWIND_HINT_IRET_REGS
0 :
  .byte 0x6a, vector            // 这里就是装配
  jmp asm_common_interrupt
  nop
  /* Ensure that the above is 8 bytes max */
  . = 0b + 8
  vector = vector+1
    .endr
SYM_CODE_END(irq_entries_start)
```
通过 `.rept` 上面的代码自动生成了很多类似的代码，大致如下:
```asm
pushq 0x20
jmp asm_common_interrupt
pushq 0x21
jmp asm_common_interrupt
pushq 0x22
jmp asm_common_interrupt
...
```

#### jump to C function

在 idtentry.h 中定义了:
```c
/* Device interrupts common/spurious */
DECLARE_IDTENTRY_IRQ(X86_TRAP_OTHER,  common_interrupt);
```

idtentry.h 会分别被 c 源文件和 asm 源文件 include，所以其定义也分别有两种
```c
#define DECLARE_IDTENTRY_IRQ(vector, func)        \
  asmlinkage void asm_##func(void);       \
  asmlinkage void xen_asm_##func(void);       \
  __visible void func(struct pt_regs *regs, unsigned long error_code)

/* Entries for common/spurious (device) interrupts */
#define DECLARE_IDTENTRY_IRQ(vector, func)        \
  idtentry_irq vector func
```

在 `arch/x86/entry/entry_64.S` 中间定义了 `idtentry_irq`，下面分析其是如何被一步步展开的:
```asm
.macro idtentry_irq vector cfunc
  idtentry \vector asm_\cfunc \cfunc has_error_code=1
.endm
```

```asm
.macro idtentry vector asmsym cfunc has_error_code:req
SYM_CODE_START(\asmsym)
  idtentry_body \cfunc \has_error_code
SYM_CODE_END(\asmsym)
.endm
```

```asm
.macro idtentry_body cfunc has_error_code:req

  call  error_entry // 其中的 error_entry 是用于保存 pt_regs 的上下文的

  // 将 rsp 数值传递给 rdi
  // 根据 x86 abi 的标准, rdi 就是第二个参数，rsi 是第一个参数
  // pt_regs 正好是 x86 放到 stack 的内容
  movq  %rsp, %rdi /* pt_regs pointer into 1st argument*/

  .if \has_error_code == 1
    movq  ORIG_RAX(%rsp), %rsi  /* get error code into 2nd argument*/
  .endif

  // 当然这个函数就是 common_interrupt 了
  call  \cfunc

  jmp error_return
.endm
```

第二个参数，也即是 irq number 是从 `(void *)pt_regs + ORIG_RAX` 获取的，这就是在 irq_entries_start 中生成的，并且通过 `.byte 0x6a, vector` 放到 stack 上的。
```c
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
#define ORIG_RAX 120
```
将上面所有的整合起来，就相当于生成了:
```asm
asm_common_interrupt
  call  error_entry
  movq  %rsp, %rdi /* pt_regs pointer into 1st argument*/
  movq  ORIG_RAX(%rsp), %rsi  /* get error code into 2nd argument*/
  call  common_interrupt
  jmp error_return
```
`common_interrupt` 是一般设备中断的入口, 例如 ipi 以及 timer 等中断的走的入口不同。


## 整理一下这里的吧

Vectors	usually	IRQ#+	32[^6]:
– Below	32	reserved	for	non-maskable	intr	&	excepWons
– Maskable	interrupts	can	be	assigned	as	needed
– Vector	128	used	for	syscall
– Vectors	251-255	used	for	IPI

- [ ] 简单验证一下 IRQ#+32 ，也就是在 idt 中间存在偏移量

IDT:	gate	descriptors,	one	per	vector[^6]
– Address	of	handler
– Current	Privilege	Level	(CPL)
– Descriptor	Privilege	Level	(DPL)
– Gates	(slightly	different	ways	of	entering	kernel)
    - Task	gate:	includes	TSS	to	transfer	to	(not	used	by Linux)
    - Interrupt	gate:	disables	further	interrupts
    - Trap	gate:	further	interrupts	sWll	allowed

- [ ] CPL 和 DPL : DPL 不总是内核态吗 ? 难道还可以在用户态执行 handler ?
    - [ ] 是不是可以根据 CPL 的不同，操作不同，比如切换到内核态的 stack
- [ ] Interrupt gate 和 Trap gate 的唯一区别就是是否 disable interrupt 吗 ？


- arch/x86/include/asm/idtentry.h
- arch/x86/kernel/irq.c

- [x] DEFINE_IDTENTRY_IRQ : interesting, wrap interrupt handler with `irq_enter_rcu`, `irq_exit_rcu` and `irqentry_exit`

```c
/*
 * common_interrupt() handles all normal device IRQ's (the special SMP
 * cross-CPU interrupts have their own entry points).
 */
DEFINE_IDTENTRY_IRQ(common_interrupt)
```

- [x] How exception are set
```c
/*
 * Dummy trap number so the low level ASM macro vector number checks do not
 * match which results in emitting plain IDTENTRY stubs without bells and
 * whistels.
 */
#define X86_TRAP_OTHER		0xFFFF

/* Simple exception entry points. No hardware error code */
DECLARE_IDTENTRY(X86_TRAP_DE,		exc_divide_error);
DECLARE_IDTENTRY(X86_TRAP_OF,		exc_overflow);
DECLARE_IDTENTRY(X86_TRAP_BR,		exc_bounds);
DECLARE_IDTENTRY(X86_TRAP_NM,		exc_device_not_available);
DECLARE_IDTENTRY(X86_TRAP_OLD_MF,	exc_coproc_segment_overrun);
DECLARE_IDTENTRY(X86_TRAP_SPURIOUS,	exc_spurious_interrupt_bug);
DECLARE_IDTENTRY(X86_TRAP_MF,		exc_coprocessor_error);
DECLARE_IDTENTRY(X86_TRAP_XF,		exc_simd_coprocessor_error);

/* 32bit software IRET trap. Do not emit ASM code */
DECLARE_IDTENTRY_SW(X86_TRAP_IRET,	iret_error);

/* Simple exception entries with error code pushed by hardware */
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_TS,	exc_invalid_tss);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_NP,	exc_segment_not_present);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_SS,	exc_stack_segment);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_GP,	exc_general_protection);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_AC,	exc_alignment_check);

/* Raw exception entries which need extra work */
DECLARE_IDTENTRY_RAW(X86_TRAP_UD,		exc_invalid_op);
DECLARE_IDTENTRY_RAW(X86_TRAP_BP,		exc_int3);
DECLARE_IDTENTRY_RAW_ERRORCODE(X86_TRAP_PF,	exc_page_fault);
```

```c
/*
 * The default IDT entries which are set up in trap_init() before
 * cpu_init() is invoked. Interrupt stacks cannot be used at that point and
 * the traps which use them are reinitialized with IST after cpu_init() has
 * set up TSS.
 */
static const __initconst struct idt_data def_idts[] = {
	INTG(X86_TRAP_DE,		asm_exc_divide_error),
	INTG(X86_TRAP_NMI,		asm_exc_nmi),
	INTG(X86_TRAP_BR,		asm_exc_bounds),
	INTG(X86_TRAP_UD,		asm_exc_invalid_op),
	INTG(X86_TRAP_NM,		asm_exc_device_not_available),
	INTG(X86_TRAP_OLD_MF,		asm_exc_coproc_segment_overrun),
	INTG(X86_TRAP_TS,		asm_exc_invalid_tss),
	INTG(X86_TRAP_NP,		asm_exc_segment_not_present),
	INTG(X86_TRAP_SS,		asm_exc_stack_segment),
	INTG(X86_TRAP_GP,		asm_exc_general_protection),
	INTG(X86_TRAP_SPURIOUS,		asm_exc_spurious_interrupt_bug),
	INTG(X86_TRAP_MF,		asm_exc_coprocessor_error),
	INTG(X86_TRAP_AC,		asm_exc_alignment_check),
	INTG(X86_TRAP_XF,		asm_exc_simd_coprocessor_error),

#ifdef CONFIG_X86_32
	TSKG(X86_TRAP_DF,		GDT_ENTRY_DOUBLEFAULT_TSS),
#else
	INTG(X86_TRAP_DF,		asm_exc_double_fault),
#endif
	INTG(X86_TRAP_DB,		asm_exc_debug),

#ifdef CONFIG_X86_MCE
	INTG(X86_TRAP_MC,		asm_exc_machine_check),
#endif

	SYSG(X86_TRAP_OF,		asm_exc_overflow),
#if defined(CONFIG_IA32_EMULATION)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_compat),
#elif defined(CONFIG_X86_32)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_32),
#endif
};
```

## x86 vector
arch/x86/kernel/irqinit.c

with irq domain, we can map *linux irq* and *hw irq*,
`vector_irq_t` maps interrupt number ==> irq_desc, used in `common_interrupt`

```c
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
	[0 ... NR_VECTORS - 1] = VECTOR_UNUSED,
};
```

- [x] how interrupt are seted in idt
    - **idt_setup_apic_and_irq_gates**
- [ ] how interrupt are set in vector_irq
  - [ ] lapic_online : irq and desc relation already setup, *find* out when they are setup.

assign_irq_vector ==> assign_vector_locked

此外，arch_early_irq_init 中间初始化的默认 fwnode 以及 irq_domain 都是叫做 vector 的:
```c
int __init arch_early_irq_init(void)
{
	struct fwnode_handle *fn;

	fn = irq_domain_alloc_named_fwnode("VECTOR");
	BUG_ON(!fn);
	x86_vector_domain = irq_domain_create_tree(fn, &x86_vector_domain_ops,
						   NULL);
	BUG_ON(x86_vector_domain == NULL);
	irq_set_default_host(x86_vector_domain);

	BUG_ON(!alloc_cpumask_var(&vector_searchmask, GFP_KERNEL));

	/*
	 * Allocate the vector matrix allocator data structure and limit the
	 * search area.
	 */
	vector_matrix = irq_alloc_matrix(NR_VECTORS, FIRST_EXTERNAL_VECTOR,
					 FIRST_SYSTEM_VECTOR);
	BUG_ON(!vector_matrix);

	return arch_early_ioapic_init();
}
```


## 特权级切换相关内容

收集一下各种架构对应的代码
```c
/*
 * user_mode(regs) determines whether a register set came from user
 * mode.  On x86_32, this is true if V8086 mode was enabled OR if the
 * register set was from protected mode with RPL-3 CS value.  This
 * tricky test checks that with one comparison.
 *
 * On x86_64, vm86 mode is mercifully nonexistent, and we don't need
 * the extra check.
 */
static inline int user_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_32
	return ((regs->cs & SEGMENT_RPL_MASK) | (regs->flags & X86_VM_MASK)) >= USER_RPL;
#else
	return !!(regs->cs & 3);
#endif
}
```

## 如何理解和 idt 和 ist 关系?
在 356628-complex-shadow-stack-updates-2.pdf 中描述的

Since delivery of event E uses the IST, it switches shadow stacks.


ist 和 tss 实现新的stack 切换 和 irq_stack_union等初始化关系是什么？ 对于某些 interrupt 通过ist和TSS机制，ist是TSS 的index， TSS 中间存储具体的stack位置，当出现某些int, 自动跳转到对应的stack 中间，采用percpu 初始化这些stack 只是由于每一个cpu 都可以中断了

## 这两个数据结构什么关系?
```c
struct idt_data {
	unsigned int	vector;
	unsigned int	segment;
	struct idt_bits	bits;
	const void	*addr;
};

struct gate_struct {
	u16		offset_low;
	u16		segment;
	struct idt_bits	bits;
	u16		offset_middle;
#ifdef CONFIG_X86_64
	u32		offset_high;
	u32		reserved;
#endif
} __attribute__((packed));
```

idt_data 和 gate_desc 的关系是什么 ? 应该是保存的信息是相同的，但是idt_data 的内存布局更好，容易赋值和操作而已。

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
