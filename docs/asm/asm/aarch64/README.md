# https://mariokartwii.com/armv8/

yc 的讨论: https://news.ycombinator.com/item?id=39002028
## ch4 : Basic Registers
- r29 is known as the Frame Pointer (FP). r30 is known as the Link Register (LR)
- There is a unique register (that is to be used as a GPR when needed) known as the Zero Register. You cannot write to it. It always contains the value of zero.
- Those are the Program Counter (PC) and the Stack Pointer (SP). The PC is a read-only register. It simply keeps track of where the CPU is currently executing at within the program (Memory Address). The SP is used in conjunction with LR for subroutine use. SP will always contain a Memory Address.

FP(r29) SP(r31) 就不说了，LR 就是函数调用的时候存放 PC 到 LR(r30) 中
PC 是 r32

## ch6 : Instruction Format
- xD : 64bit
- wD : 32bit

扩展:
https://developer.arm.com/documentation/dui0801/l/Overview-of-AArch64-state/Predeclared-core-register-names-in-AArch64-state
>
> XZR
> WZR
>
> SP
> WSP
>
> - 64-bit RAZ/WI register. This name is the name for register 31 when it is used as the zero register in a 64-bit context.
> - 64-bit stack pointer. This name is the name for register 31 when it is used as the stack pointer in a 64-bit context.

https://stackoverflow.com/questions/61532867/in-arm64-assembly-code-when-is-register-31-xzr-versus-sp

- [ ] 什么叫做 register 31 是如何自动确定的?

扩展:
https://wiki.cdot.senecapolytechnic.ca/wiki/AArch64_Register_and_Instruction_Quick_Start
> Usage during syscall/function call:
> - r0-r7 are used for arguments and return values; additional arguments are on the stack
> - For syscalls, the syscall number is in r8
> - r9-r15 are for temporary values (may get trampled)
> - r16-r18 are used for intra-procedure-call and platform values (avoid)
> - The called routine is expected to preserve r19-r28 *** These registers are generally safe to use in your program.
> - r29 and r30 are used as the frame register and link register (avoid)

https://eclecticlight.co/wp-content/uploads/2021/06/armregisterarch.pdf


## ch11 : Pre and Post Index of Loads/Stores

Pre-Index Store:
```txt
str x0, [x1, #0x4]!
```

- x1 + 0x4 = Effective Address
- x0 (entire double-word) stored at Effective Address
- x1 is then incremented by 0x4, it now has a new value if the instruction gets executed again


Post-Index Store:

```txt
str x0, [x1], #0x4
```

- x1 = Effective Address
- x0 (entire double-word) stored at Effective Address
- x1 is then incremented by 0x4, it now has a new value if the instruction gets executed again

- 如果不想修改 x1 ，那么就不要用感叹号
```txt
str x0, [x1, #0x4]
```

## ch20 : Address Relative Loading

## ch21
- .s 和 .S 的区别 : .S 可以和 c/c++ 的库配合用，例如 main 入口和 glibc 库，而 .s 不可以

https://mariokartwii.com/armv8/

## ch22: Load & Store Pair

```txt
stp w1, w5, [x12]

Pretend w1 = 0x11112222
Pretend w5 = 0x000000A0
Pretend x12 = 0x40007FFC30
```
The above instruction will store 0x11112222 at 0x40007FFC30, and 0x000000A0 at 0x40007FFC34.

## ch24: Float Basics

- https://mariokartwii.com/armv8/ch24.html

- q0 thru q31 = quad precision; use all 128-bits of the FPR
- d0 thru d31 = double precision, use lower 64-bits of the FPR (upper 64 bits ignored)
- s0 thru s31 = single precision, use lower 32-bits of FPR (upper 96 bits ignored)
- h0 thru h31 = half precision, use lower 16-bits of FPR (upper 112 bits ignored)

> 还有很多就不看了

# asm_book

这本书应该在 github 上阅读还是 nice 的
https://github.com/pkivolowitz/asm_book#table-of-contents

## 其他

### wzr 就是 0
https://stackoverflow.com/questions/42788696/why-might-one-use-the-xzr-register-instead-of-the-literal-0-on-armv8

## TODO
### 理解下 gdb 中各个字段的含义
```txt
                 x0 0x0000000000000014                   x1 0x0000fffff7ffbb28                 x2 0x0000000000000000
                 x3 0x0000000000000000                   x4 0x0000ffffffffe48c                 x5 0x00000000004442b4
                 x6 0x000000000000000a                   x7 0x0000000000000001                 x8 0x0000000000000040
                 x9 0x0000000000000010                  x10 0x000000000000000a                x11 0x0000000000000000
                x12 0x00000000ffffffc8                  x13 0x0000ffffffffe620                x14 0x0000000000000000
                x15 0x000000000055acae                  x16 0x0000000000440018                x17 0x0000fffff7e0c1c0
                x18 0x0000000000003fff                  x19 0x0000ffffffffe7f8                x20 0x0000000000000001
                x21 0x000000000043fdf0                  x22 0x00000000004100b4                x23 0x0000ffffffffe808
                x24 0x0000fffff7ffbb30                  x25 0x0000000000000000                x26 0x0000fffff7ffc000
                x27 0x000000000043fdf0                  x28 0x0000000000000000                x29 0x0000ffffffffe690
                x30 0x00000000004101b8                   sp 0x00010001d0428d30                 pc 0x00000000004101b8
               cpsr [ EL=0 BTYPE=0 SSBS C Z ]          fpsr [ ]                              fpcr [ Len=0 Stride=0 RMode=0 ]
              tpidr 0x0000fffff7fff4a0               tpidr2 0x0000000000000000        pauth_dmask 0x007f000000000000
        pauth_cmask 0x007f000000000000
```

- [ ] 寻址方法总结一下
- [ ] 看看 arm 如何加载一个 64bit 立即数?
- [ ] simd 的测试
- [ ] cache tlb 指令的测试，这个真的可以用户态使用不会有问题吗?
- [ ] 内核中可以用 SIMD 吗? 似乎 copy from user 咋处理的
- [ ] 这个目录中的东西看看 : Documentation/arch/arm64/

## 这个看看，其中
https://devblogs.microsoft.com/oldnewthing/20220811-00/?p=106963

## ldur
- https://stackoverflow.com/questions/52894765/ldur-and-stur-in-arm-v8
- https://stackoverflow.com/questions/78823419/ldr-unsigned-offset-vs-ldur-unscaled

LDR 和 LDUR 是什么关系来着?

C3.2.2 Load/Store register (unscaled offset) 没看懂


## 如何知道那个 CPU 的版本是哪一个

现在是通过
https://en.wikipedia.org/wiki/Apple_M2

才知道是环境中是这个:
Instruction set	ARMv8.6-A

可以通过 cpuid.c 获取吗?

kernel 会知道那些指令有，那些没有吗?

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
