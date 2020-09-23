# 汇编
> TMD 本来以为自己学习过汇编，才发现，自己学习的是 16bit 的汇编，简直搞笑的不得了


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

## call convention
1. syscall ?
2. interrupt ? 
3. 一般的 ?
4. 32bit 和 64bit 区分 ?

```c
int callee(int, int, int);

int caller(void)
{
	return callee(1, 2, 3) + 5;
}   
```

使用 `-O0 -m32` 的来实现 32bit 的设置:

```
caller():
        push    ebp
        mov     ebp, esp
        sub     esp, 8
        sub     esp, 4
        push    3
        push    2
        push    1
        call    callee(int, int, int)
        add     esp, 16
        add     eax, 5
        leave
        ret
```

64bit
```
caller():
        push    rbp
        mov     rbp, rsp
        mov     edx, 3
        mov     esi, 2
        mov     edi, 1
        call    callee(int, int, int)
        add     eax, 5
        pop     rbp
        ret
```

问题:
1. leave 是什么意思 ?
2. m32 中间为什么需要 sub esp 和 add esp
    1. 而且两者的数量相加不一致啊 根据 [^1] 的描述，感觉不对啊


clang10 -m32 -O0 的结果:
```
caller():                             # @caller()
        push    ebp
        mov     ebp, esp
        sub     esp, 24
        mov     dword ptr [esp], 1
        mov     dword ptr [esp + 4], 2
        mov     dword ptr [esp + 8], 3
        call    callee(int, int, int)
        add     eax, 5
        add     esp, 24
        pop     ebp
        ret
```
1. 为什么 esp 需要下降 24，这是怎么计算的



➜  /tmp clang -c -o a.o  -O0 -m32 a.c && objdump -d a.o
```
00000000 <caller>:
   0:   55                      push   %ebp
   1:   89 e5                   mov    %esp,%ebp
   3:   53                      push   %ebx
   4:   83 ec 14                sub    $0x14,%esp
   7:   e8 00 00 00 00          call   c <caller+0xc>
   c:   58                      pop    %eax
   d:   81 c0 03 00 00 00       add    $0x3,%eax
  13:   8b 4d 08                mov    0x8(%ebp),%ecx
  16:   c7 04 24 01 00 00 00    movl   $0x1,(%esp)
  1d:   c7 44 24 04 02 00 00    movl   $0x2,0x4(%esp)
  24:   00
  25:   c7 44 24 08 03 00 00    movl   $0x3,0x8(%esp)
  2c:   00
  2d:   89 c3                   mov    %eax,%ebx
  2f:   89 4d f8                mov    %ecx,-0x8(%ebp)
  32:   e8 fc ff ff ff          call   33 <caller+0x33>
  37:   83 c0 05                add    $0x5,%eax
  3a:   03 45 08                add    0x8(%ebp),%eax
  3d:   83 c4 14                add    $0x14,%esp
  40:   5b                      pop    %ebx
  41:   5d                      pop    %ebp
  42:   c3                      ret
```
1. 这个结果明显不对啊，怎么可能解析出来两个 call 指令啊 !
    1. 到底是 gcc 的问题还是 objdump 的问题
    2. 







[^1]
Calling conventions describe the interface of called code:
- The order in which atomic (scalar) parameters, or individual parts of a complex parameter, are allocated
- How parameters are passed (pushed on the stack, placed in registers, or a mix of both)
- Which registers the called function must preserve for the caller (also known as: callee-saved registers or non-volatile registers)
- How the task of preparing the stack for, and restoring after, a function call is divided between the caller and the callee

Calling conventions, type representations, and name mangling are all part of what is known as an application binary interface (ABI).

> TODO



[^1]: [wiki](https://en.wikipedia.org/wiki/X86_calling_conventions)

