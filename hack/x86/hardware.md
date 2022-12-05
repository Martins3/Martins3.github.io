# x86

<!-- vim-markdown-toc GitLab -->

  - [long mode](#long-mode)
  - [control register](#control-register)
      - [cr0](#cr0)
  - [gdt idt](#gdt-idt)
  - [fpu](#fpu)
      - [fpu signal](#fpu-signal)
      - [fpu xstate](#fpu-xstate)
  - [apic](#apic)
  - [io apic](#io-apic)
  - [question](#question)
- [关于amd64 架构的疑问](#关于amd64-架构的疑问)
  - [tss](#tss)
- [要不要学一波汇编语言](#要不要学一波汇编语言)

<!-- vim-markdown-toc -->
## long mode

https://wki.osdev.org/Setting_Up_Long_Modei

## control register
https://e.wikipedia.org/wiki/Control_registern

https://wiki.odev.org/CPU_Registers_x86-64#IA32_EFERs : x86 到底有多少 register 

#### cr0
[wiki](https://en.wikipedia.org/wiki/Control_register#CR0)
```c
/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1u << 1)
#define CR0_EM (1u << 2)
#define CR0_TS (1u << 3)
#define CR0_ET (1u << 4)
#define CR0_NE (1u << 5)
#define CR0_WP (1u << 16)
#define CR0_AM (1u << 18)
#define CR0_NW (1u << 29)
#define CR0_CD (1u << 30)
#define CR0_PG (1u << 31)
```

## gdt idt


## fpu

arch/x86/include/asm/fpu/types.h
```c
/*
 * This is our most modern FPU state format, as saved by the XSAVE
 * and restored by the XRSTOR instructions.
 *
 * It consists of a legacy fxregs portion, an xstate header and
 * subsequent areas as defined by the xstate header.  Not all CPUs
 * support all the extensions, so the size of the extended area
 * can vary quite a bit between CPUs.
 */
struct xregs_state {
	struct fxregs_state		i387;
	struct xstate_header		header;
	u8				extended_state_area[0];
} __attribute__ ((packed, aligned (64)));

/*
 * This is a union of all the possible FPU state formats
 * put together, so that we can pick the right one runtime.
 *
 * The size of the structure is determined by the largest
 * member - which is the xsave area.  The padding is there
 * to ensure that statically-allocated task_structs (just
 * the init_task today) have enough space.
 */
union fpregs_state {
	struct fregs_state		fsave; // legacy x87 FPU state format : FSAVE FRSTOR
	struct fxregs_state		fxsave; // legacy fx SSE/MMX FPU state format, as saved by FXSAVE and restored by the FXRSTOR instructions.
	struct swregs_state		soft; // Software based FPU emulation state
	struct xregs_state		xsave;
	u8 __padding[PAGE_SIZE];
};
```

#### fpu signal
I guess it's mainly used for enter user mode and return from user mode whne handling sigaction

#### fpu xstate
- [ ] why `copy_xstate_to_kernel` and `copy_user_to_xstate` are so complex ?


## apic

## io apic

## question
> 关于需要硬件的问题:
> 1. context switch 的硬件支持，以前使用TSS 为什么现在不使用了
> 2. exception interrupt syscall 需要的支持
> 3. io 映射， inb outb 的实现 ?
> 4. 用户和内核，用户和用户之间如何实现的保护的
> 5. 原子操作指令(应该很简单)
> 6. 内存屏障指令


# 关于amd64 架构的疑问
1. cs gs fs 三个segment 寄存器的作用是什么 ?
2. gdt  tss  idt 各自运行机制是什么
3. msr 寄存器等作用


4. swapfs 等各种特权指令
> 直接参考v3 中的特权指令




> 需要最终可以看的懂 entry_64.S(32 到 64 的切换) 和 head_64.S 中间的所有的内容即可。
```
 * entry.S contains the system-call and fault low-level handling routines.
 *
 * Some of this is documented in Documentation/x86/entry_64.txt
```


## [tss](https://en.wikipedia.org/wiki/Task_state_segment)
The TSS may reside anywhere in memory. A segment register called the task register (TR) holds a segment selector that points to a valid TSS segment descriptor which resides in the GDT (a TSS descriptor may not reside in the LDT). Therefore, to use a TSS the following must be done by the operating system kernel:

- Create a TSS descriptor entry in the GDT
- Load the TR with the segment selector for that segment
- Add information to the TSS in memory as needed

For security purposes, the TSS should be placed in memory that is accessible only to the kernel.


The x86-64 architecture does not support hardware task switches. However the TSS can still be used in a machine running in the 64 bit extended modes. In these modes the TSS is still useful as it stores:

_ The *stack pointer addresses* for each privilege level.
_ Pointer Addresses for the Interrupt Stack Table (The inner-level stack pointer section above, discusses the need for this).
_ Offset Address of the *IO permission bitmap*.

Also, the task register is expanded in these modes to be able to hold a 64-bit base address.

> 所以，TSS 记录了在 kernel stack 和 interrupt stack 的地址
> kernel stack 每一个 process 都是有一个，所以在进行 context switch 的时候，需要将 TSS 也进行更新

# 要不要学一波汇编语言
https://software.intel.com/en-us/articles/introduction-to-x64-assembly intel 入学手册，感觉没有必要。
