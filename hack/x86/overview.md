# x86

<!-- vim-markdown-toc GitLab -->

- [TODO](#todo)
- [syntax](#syntax)
- [long mode](#long-mode)
- [control register](#control-register)
    - [cr0](#cr0)
- [gdt idt](#gdt-idt)
- [fpu](#fpu)
    - [fpu signal](#fpu-signal)
    - [fpu xstate](#fpu-xstate)

<!-- vim-markdown-toc -->

## TODO
2. 可以参考的资源:

* [The NASM Manual](https://www.nasm.us/doc/)
* [Compiler Explorer](https://godbolt.org/)
* [System V Application Binary Interface](https://www.uclibc.org/docs/psABI-x86_64.pdf)
* [The Intel Processor Manuals](https://software.intel.com/en-us/articles/intel-sdm)
* [The AMD Processor Manuals](https://developer.amd.com/resources/developer-guides-manuals/)
* [Agner Fog's optimization guides](https://www.agner.org/optimize/)
* [x86 and amd64 instruction reference](https://www.felixcloutier.com/x86/)

3. https://asmjit.com/ : 神奇的内联汇编

4. exit.c:do_exit 的 set_fs

5. check code in uaccess.c
  - for example, understand `get_fs`

- [ ] https://www.kernel.org/doc/html/latest/x86/index.html 
- [ ] http://infophysics.net/att0.pdf
- [ ] https://docs.oracle.com/cd/E19253-01/817-5477/817-5477.pdf


## syntax
1. ~/Core/x86-64-assembly 似乎非常不错:

配合答案使用:
https://github.com/0xAX/asm
也许可以写一个简化版的，感觉应该非常不错吧!


需要回答的问题:
1. global 是否存在对称的关键字
2. 定义 static 变量的方法

3. 宏
4. inlcude
5. section 的定义

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

