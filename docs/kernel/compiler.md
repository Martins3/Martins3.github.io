# Compiler for linux kernel

## gcc plugins
- https://lwn.net/Articles/691102/
- https://www.kernel.org/doc/html/v5.10/kbuild/gcc-plugins.html

## latent_entropy
- [ ] although I know how it works, but I can't run a demo
   - https://lwn.net/Articles/688492/
   - https://github.com/ephox-gcc-plugins/latent_entropy

## build kernel with clang
- https://www.kernel.org/doc/html/latest/kbuild/llvm.html
    - [ClangBuiltLinux](https://github.com/ClangBuiltLinux/tc-build)

- [使用 clang 的交叉编译](https://github.com/MaskRay/ccls/wiki/Example-Projects)
```plain
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -j10
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -k Image.gz modules // 好吧，-k Image.gz modules 是什么意思
```

也可以使用 gcc 来实现交叉编译 :
```plain
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make defconfig
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make
```

使用 LLVM 编译 x86
```plain
# Use clang 9 or newer.
make ARCH=x86_64 LLVM=1 defconfig
make ARCH=x86_64 LLVM=1 -j10
```

### Cross Compile standard MIPS
Tried, but faile to find the compiler

### Cross Compile Riscv
```plain
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
```
https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-introduction.html

## `__attribute__((destructor))`

https://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work

## `__ASSEMBLY__`

https://stackoverflow.com/questions/28924355/gcc-assembler-preprocessor-not-compatible-with-standard-headers

when reading `arch/x86/include/asm/idtentry.h`,
I can't find the caller of asm_common_interrupt.
There is only one, but ccls's highlight tell me that it will be compiled because of option `_ASSEMBLY__` not selected.
In fact, things doesn’t happened in that way.
ideentry will be include by normal C file and asm file,
when include by asm, `_ASSEMBLY__` will be selected.

## `__cacheline_aligned`

https://stackoverflow.com/questions/25947962/cacheline-aligned-in-smp-for-structure-in-the-linux-kernel

## asmlinkage
https://stackoverflow.com/questions/10459688/what-is-the-asmlinkage-modifier-meant-for

## 如何正确的切换 gcc 版本
https://badsimplicity.com/2019/07/10/gcc-9-and-ubuntu-kernel-error/

```sh
sudo apt install gcc-8 # 安装
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 1 # 添加 alternatives
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 2 # 添加 alternatives
sudo update-alternatives --config gcc # 选择

sudo update-alternatives --config cc # 设置默认 clang
sudo update-alternatives --config c++
```
类似可以选择默认的 clang 版本。

```sh
cd /usr/bin
sudo ln -f -s clang-11 clang
sudo ln -f -s clang++-11 clang++
sudo ln -f -s llvm-ar-11 llvm-ar
sudo ln -f -s llvm-as-11 llvm-as
sudo ln -f -s clangd-11  clangd
sudo ln -f -s clang-tidy-11 clang-tidy
```

## static key
- `sched_dynamic_update`
- 在函数 page_fixed_fake_head 有 static_branch_unlikely

## `__x86_return_thunk`
我们发现空函数实际上是跳转到 `__x86_return_thunk` 的
```txt
$ disass that
Dump of assembler code for function that:
0xffffffff81d7a8e0 <+0>: endbr64
0xffffffff81d7a8e4 <+4>: jmp 0xffffffff821f3544 <__x86_return_thunk>
End of assembler dump.

$ disass __x86_return_thunk
Dump of assembler code for function __x86_return_thunk:
   0xffffffff821f3604 <+5>:     ret
   0xffffffff821f3605 <+6>:     int3
```
1. endbr64 的作用 ： https://stackoverflow.com/questions/56905811/what-does-the-endbr64-instruction-actually-do
  - 处理安全问题。

2. `__x86_return_thunk`
```c
#if defined(CONFIG_RETHUNK) && !defined(__DISABLE_EXPORTS) && !defined(BUILD_VDSO)
#define RET	jmp __x86_return_thunk
#else /* CONFIG_RETPOLINE */
#ifdef CONFIG_SLS
#define RET	ret; int3
#else
#define RET	ret
#endif
#endif /* CONFIG_RETPOLINE */
```

```txt
config RETHUNK
	bool "Enable return-thunks"
	depends on RETPOLINE && CC_HAS_RETURN_THUNK
	select OBJTOOL if HAVE_OBJTOOL
	default y if X86_64
	help
	  Compile the kernel with the return-thunks compiler option to guard
	  against kernel-to-user data leaks by avoiding return speculation.
	  Requires a compiler with -mfunction-return=thunk-extern
	  support for full protection. The kernel may run slower.
```
也是安全问题。

具体细节有待深入。
