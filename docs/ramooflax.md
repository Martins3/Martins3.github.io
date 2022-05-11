# Bare Mental Binary Translator

- [ ] 测试运行一下吧，或者整理为一个类型的代码都可以尝试一下的

- [ ] ramooflax 是怎么处理网卡的 ?
  - 疑问在于，网卡是 ramooflax 的调试组件，同时这个网卡还会在启动的被使用
  - [ ] ramooflax 如何知道其 mmio 空间

## 结论
- [x] 为什么我们需要重新处理网卡，如果不是标准的，那岂不是就需要重新转发了
  - 因为曾经硬件没有保证 cache 一致性

## 可以借鉴的代码
- [ ] captive
   - IncludeOS 中间应该有类似的东西, 不然 C++ 的 new 都是需要自己重写的

- [x] 找到更多的 IncludeOS, 那种可以直接在硬件上运行的那种
    - 笑死，根本没有[^1]

#### ramooflax
似乎我们也是可以划分为三种结构，loader, setup 和 tcg

- [ ] 系统如何启动的 ?
  - [ ] 系统如何实现基本的初始化的
- [ ] 如何进行调试的 ?

1. entry.S 前面应该有什么东西吧 ?
  - [ ] 至少需要有一个模式跳转之类的吧, 看看 loader 是怎么搞的

在 /home/maritns3/core/ld/ramooflax/setup/src/core/init.c 中间装载的 `static info_data_t __info;`

而 info_data_t 就是各种系统初始化的时候完成的:
```c
static info_data_t __info;
```

是怎么和 grub 打交道的 ?
显然，我们用的也是 grub 的呀!



```c
#define __mbh__                 __attribute__ ((section(".mbh"),aligned(4)))
```

```ld
OUTPUT_FORMAT("elf32-i386","elf32-i386","elf32-i386");
OUTPUT_ARCH("i386")

ENTRY(entry)

SECTIONS
{
   . = 0x200000;
   __kernel_start__ = .;

   .mbh       . : { *(.mbh) . = ALIGN(4);           }
   .text      . : { *(.text)                        }
   .rodata      : { *(.rodata)                      }
   .data        : { *(.data)                        }
   .bss         : { *(.bss COMMON)                  }
   /DISCARD/  	: { *(.note* .indent .comment)      }
}
```

https://en.wikipedia.org/wiki/Multiboot_specification

那么 init 的参数
```c
void __regparm__(1) init(mbi_t *mbi)
```

```asm
/*
** - make us uninterruptible
** - set initial stack for loader
** - clear eflags
** - init loader with grub multiboot info
*/
entry:
        cli
        movl    $__kernel_start__, %esp
        pushl   $0
        popf
        movl    %ebx, %eax
        jmp     init
```
我们知道 %ebx 是这个东西，而且 mbi_t 显然是实现构造好的


- [ ] 这个命令的效果是什么?
```sh
make INSTOOL=tools/installer_qemu.sh install
```




读读文章：
1. The objective is to virtualize already installed operating systems on physical dedicated machine.

- [ ] 最后是怎么切换到 already installed os 上的 ?

2. This allows virtualization, and so analysis, of operating systems running
in their native environment more specifically regarding devices which are
hardly emulated by common existing virtualization solutions.
The idea is to boot the hypervisor from an external storage media (USB
key), and once the hypervisor has been initialized, to tell the BIOS (now
virtualized) to boot the already installed operating system.

- [ ] 岂不是将 BIOS 放到虚拟机中间运行吗?

- [ ] Loader 可以被 Grub 检测到，怎么实现的?

- [ ] 观测一下其中探测物理内存的方法

Once the VMM initialized, the setup installs in conventional memory
the `int 0x19` instruction and starts VMM execution.

- [ ] int 0x19 是做什么的?

The proxy mode is used to intercept, log and emulate MSRs accesses for instance.
The cpuid instruction is managed this way by default because the hypervisor needs to hide some features to the VM.

The setup finishes its execution by installing the first VM instructions in
conventional memory: int 0x16 and int 0x19.

The first one is a BIOS service which allows to wait for a keystroke. The second one tells the
BIOS to load the bootsector of its first bootable device which uses to be
an hard drive where the native operating system is already installed.
By doing this, we take benefit of existing BIOS features (devices access
like USB, SATA, . . . ). The hypervisor seamlessly virtualizes real mode
code whether it is BIOS or not.

## 想法
- 应该首先放到虚拟机中间测试才对的啊，按道理如果可以在 bm 上跑起来，那么必然需要在虚拟机上跑起来的
  - [ ] 首先写一个 unikernel 出来，让 loongarch 的 qemu 可以运行才可以
  - [ ] 制作一个 C 语言的版本的 InlcudeOS，是不是会轻松很多 ?
    - [ ] 既然 captive / includeos 都是支持 c++ 的，而且不知道以后会不会使用 LLVM 的啊!

- [x] includeos 的 hypervisor 在什么地方 ?
  - 是 qemu : https://github.com/includeos/vmrunner/blob/master/bin/boot
  - 如果，加入，让 unikernel 支持了各种 posix 系统调用, 那么将整个 qemu 放到上面也不是不可能的操作

[^1]: https://github.com/cetic/unikernels
