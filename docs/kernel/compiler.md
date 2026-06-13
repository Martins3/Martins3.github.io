# 谈谈 kernel 构建的基本技术

## gcc plugins
- https://lwn.net/Articles/691102/
- https://www.kernel.org/doc/html/v5.10/kbuild/gcc-plugins.html

## GUN extension
- https://news.ycombinator.com/item?id=40340248

## latent_entropy
- [ ] although I know how it works, but I can't run a demo
   - https://lwn.net/Articles/688492/
   - https://github.com/ephox-gcc-plugins/latent_entropy

## build kernel with clang
- https://www.kernel.org/doc/html/latest/kbuild/llvm.html
    - [ClangBuiltLinux](https://github.com/ClangBuiltLinux/tc-build)
      - 他在这里记录很多 Issue : https://github.com/ClangBuiltLinux/linux/issues

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
```sh
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

## dwarf vs frame pointer
https://news.ycombinator.com/item?id=34789247

## `__init`

初始化的代码只会运行一次，因此可以之后释放掉:
https://stackoverflow.com/questions/8832114/what-does-init-mean-in-the-linux-kernel-code

## 看看这个文章，分析下 likely 实现
- https://johnnysswlab.com/how-branches-influence-the-performance-of-your-code-and-what-can-you-do-about-it/
	- 里面说不要用 switch case ，这是我没想到的 ，但是说编译器已经会自动优化 switch case 了

## Profile-guided Optimization
- https://github.com/h0tc0d3/linux_pgo : 在内核中使用 Profile-guided Optimization
- https://doc.rust-lang.org/rustc/instrument-coverage.html

## 也可以参考下
- How to build highly-debuggable C++ binaries
  - https://dhashe.com/how-to-build-highly-debuggable-c-binaries.html
    - https://news.ycombinator.com/item?id=41074703

## 如果可以让 mold link linux kernel ，那是极好的
https://github.com/rui314/mold/issues/563

https://mp.weixin.qq.com/s/ZEwph6tg8U0yGbV75VpAZw


## 希望不要如何激进的 inline

- https://stackoverflow.com/questions/41638178/is-building-the-linux-kernel-with-fno-inline-supported
  - 增加 -fno-inline 不行的

Makefile 增加这个:

```txt
KBUILD_CFLAGS	+= -fno-inline-small-functions
```

验证一下吧

## static key

配套测试:
/home/martins3/data/vn/code/src/m/static_key.c

用户态的解决办法:
https://users.rust-lang.org/t/announcing-static-keys-reimplementaion-of-linux-kernels-static-keys-mechanism-for-rust-userland/115060

https://lwn.net/Articles/979683/



### 使用案例

#### x86 apic

如果检查修改，直接看 	apic_update_callback 的调用就可以了
例如 kvm_setup_pv_ipi

```c
#define apic_update_callback(_callback, _fn) {					\
		__x86_apic_override._callback = _fn;				\
		apic->_callback = _fn;						\
		static_call_update(apic_call_##_callback, _fn);			\
		pr_info("APIC: %s() replaced with %ps()\n", #_callback, _fn);	\
}
```

问题的关键在于:
- update_static_calls : 就是通过全部变量 apic 中注册的 hook 来更新 static call 调用的函数
- restore_override_callbacks : 将 __x86_apic_override 中更新到 apic 中，不过为什么有这个不清楚

其实 static key 是简单，这是 __x86_apic_override 的出现把逻辑变的很绕了。

### 理解一下这个例子
看来不仅可以更新函数指针，还可以更新

static_branch_enable
```c
static inline void perf_event_task_sched_in(struct task_struct *prev,
					    struct task_struct *task)
{
	if (static_branch_unlikely(&perf_sched_events))
		__perf_event_task_sched_in(prev, task);

	if (__perf_sw_enabled(PERF_COUNT_SW_CPU_MIGRATIONS) &&
	    task->sched_migrated) {
		__perf_sw_event_sched(PERF_COUNT_SW_CPU_MIGRATIONS, 1, 0);
		task->sched_migrated = 0;
	}
}
```

### 通过这个理解一些实现细节
```txt
config HAVE_PREEMPT_DYNAMIC_CALL
	bool
	depends on HAVE_STATIC_CALL
	select HAVE_PREEMPT_DYNAMIC
	help
	  An architecture should select this if it can handle the preemption
	  model being selected at boot time using static calls.

	  Where an architecture selects HAVE_STATIC_CALL_INLINE, any call to a
	  preemption function will be patched directly.

	  Where an architecture does not select HAVE_STATIC_CALL_INLINE, any
	  call to a preemption function will go through a trampoline, and the
	  trampoline will be patched.

	  It is strongly advised to support inline static call to avoid any
	  overhead.

config HAVE_PREEMPT_DYNAMIC_KEY
	bool
	depends on HAVE_ARCH_JUMP_LABEL
	select HAVE_PREEMPT_DYNAMIC
	help
	  An architecture should select this if it can handle the preemption
	  model being selected at boot time using static keys.

	  Each preemption function will be given an early return based on a
	  static key. This should have slightly lower overhead than non-inline
	  static calls, as this effectively inlines each trampoline into the
	  start of its callee. This may avoid redundant work, and may
	  integrate better with CFI schemes.

	  This will have greater overhead than using inline static calls as
	  the call to the preemption function cannot be entirely elided.
```


### 参考
https://www.zhihu.com/question/471637144/answer/3377224126

## 看看
https://pvs-studio.com/en/blog/posts/0397/

## 简单看看
https://zhuanlan.zhihu.com/p/1904189249164678881?share_code=ThxjUwNH9bSq&utm_psn=1916847358697767282

## 这里面的所有的 flags 都整理一下
```txt
    "command": "ccache gcc -Wp,-MMD,./..vmlinux.export.o.d -nostdinc -I./arch/x86/include -I./arch/x86/include/generated -I./include -I./include -I./arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/compiler-version.h -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h -D__KERNEL__ -Werror -std=gnu11 -fshort-wchar -funsigned-char -fno-common -fno-PIE -fno-strict-aliasing -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=branch -fno-jump-tables -m64 -falign-jumps=1 -falign-loops=1 -mno-80387 -mno-fp-ret-in-387 -mpreferred-stack-boundary=3 -mskip-rax-setup -march=x86-64 -mtune=generic -mno-red-zone -mcmodel=kernel -mstack-protector-guard-reg=gs -mstack-protector-guard-symbol=__ref_stack_chk_guard -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -mindirect-branch-cs-prefix -mfunction-return=thunk-extern -fno-jump-tables -fpatchable-function-entry=16,16 -fno-delete-null-pointer-checks -O2 -fno-allow-store-data-races -fstack-protector-strong -ftrivial-auto-var-init=zero -fno-stack-clash-protection -pg -mrecord-mcount -mfentry -DCC_USING_FENTRY -fmin-function-alignment=16 -fstrict-flex-arrays=3 -fno-strict-overflow -fno-stack-check -fconserve-stack -fno-builtin-wcslen -Wall -Wextra -Wundef -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Werror=strict-prototypes -Wno-format-security -Wno-trigraphs -Wno-frame-address -Wno-address-of-packed-member -Wmissing-declarations -Wmissing-prototypes -Wframe-larger-than=2048 -Wno-main -Wno-dangling-pointer -Wvla-larger-than=1 -Wno-pointer-sign -Wcast-function-type -Wno-array-bounds -Wno-stringop-overflow -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wenum-conversion -Wunused -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-packed-not-aligned -Wno-format-overflow -Wno-format-truncation -Wno-stringop-truncation -Wno-override-init -Wno-missing-field-initializers -Wno-type-limits -Wno-shift-negative-value -Wno-maybe-uninitialized -Wno-sign-compare -Wno-unused-parameter -g -gdwarf-5 -DGCC_PLUGINS    -DKBUILD_MODFILE='\"/.vmlinux.export\"' -DKBUILD_BASENAME='\".vmlinux.export\"' -DKBUILD_MODNAME='\".vmlinux.export\"' -D__KBUILD_MODNAME=kmod_.vmlinux.export -c -o .vmlinux.export.o .vmlinux.export.c",
```

先是所有的 warning :

-Wall
-Wextra
-Wundef
-Werror=implicit-function-declaration
-Werror=implicit-int
-Werror=return-type
-Werror=strict-prototypes
-Wno-format-security
-Wno-trigraphs
-Wno-frame-address
-Wno-address-of-packed-member
-Wmissing-declarations
-Wmissing-prototypes
-Wframe-larger-than=2048
-Wno-main
-Wno-dangling-pointer
-Wvla-larger-than=1
-Wno-pointer-sign
-Wcast-function-type
-Wno-array-bounds
-Wno-stringop-overflow
-Wno-alloc-size-larger-than
-Wimplicit-fallthrough=5
-Werror=date-time
-Werror=incompatible-pointer-types
-Werror=designated-init
-Wenum-conversion
-Wunused
-Wno-unused-but-set-variable
-Wno-unused-const-variable
-Wno-packed-not-aligned
-Wno-format-overflow
-Wno-format-truncation
-Wno-stringop-truncation
-Wno-override-init
-Wno-missing-field-initializers
-Wno-type-limits
-Wno-shift-negative-value
-Wno-maybe-uninitialized
-Wno-sign-compare
-Wno-unused-parameter

## 其实这个可以从浅到深写点东西的

首先是使用，最后到一个大的角度，

## docs/kernel/lpc/2025.md 中记录了 iwyu 在内核中的使用的探索

## clang-tidy 可以用吗?

其实现在 clang 就可以报错，为什么还需要一个额外的 clang-tidy 啊

## 两个常用问题
```txt
[466129.927345]  <TASK>
[466129.927348]  show_stack+0x52/0x5c
[466129.927357]  dump_stack_lvl+0x4a/0x63
[466129.927363]  dump_stack+0x10/0x16
[466129.927366]  ubsan_epilogue+0x9/0x49
[466129.927370]  __ubsan_handle_shift_out_of_bounds.cold+0x61/0xef
[466129.927378]  ? generic_write_end+0xf9/0x160
[466129.927384]  __radix_tree_lookup.cold+0x1f/0x5a
[466129.927388]  ? ext4_da_write_end+0x77/0x210
[466129.927394]  radix_tree_lookup+0xd/0x20
[466129.927400]  balance_dirty_pages_ratelimited+0x17c/0x3d0
[466129.927408]  generic_perform_write+0x140/0x1f0
[466129.927412]  ext4_buffered_write_iter+0xac/0x180
[466129.927417]  ext4_file_write_iter+0x43/0x60
[466129.927420]  new_sync_write+0x114/0x1a0
[466129.927425]  vfs_write+0x1d5/0x270
[466129.927429]  ksys_write+0x67/0xf0
[466129.927432]  __x64_sys_write+0x19/0x20
[466129.927436]  do_syscall_64+0x5c/0xc0
[466129.927441]  ? syscall_exit_to_user_mode+0x27/0x50
[466129.927446]  ? __x64_sys_recvmsg+0x1d/0x30
[466129.927452]  ? do_syscall_64+0x69/0xc0
[466129.927455]  ? exit_to_user_mode_prepare+0x37/0xb0
[466129.927460]  ? syscall_exit_to_user_mode+0x27/0x50
[466129.927464]  ? __x64_sys_sendto+0x24/0x30
[466129.927467]  ? do_syscall_64+0x69/0xc0
[466129.927470]  ? syscall_exit_to_user_mode+0x27/0x50
[466129.927474]  ? __x64_sys_recvmsg+0x1d/0x30
[466129.927478]  ? do_syscall_64+0x69/0xc0
[466129.927481]  ? syscall_exit_to_user_mode+0x27/0x50
```
> [!NOTE]
> 参考神奇海螺的意见，有待验证

- 有的 backtrace 中含有 ? : 是推测结果，但是什么时候会做推测结果，不清楚
- `__radix_tree_lookup.cold` 中的 cold 的含义

GCC / Clang 在优化时，会把极少执行的代码路径拆出来，形成一个单独的函数，目的只有一个
优化 I-cache 和分支预测
- 热路径（hot path）
- 连续
- cache 友好
- 分支预测稳定
冷路径（cold path）
- error handling
- BUG/WARN
- UBSAN/KASAN
- unlikely 分支

## 有趣的
https://harshanu.space/en/tech/ccc-vs-gcc/

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
