# syscall

## syscall 的流程
### 进入 syscall 之前 : glibc
### 进入 syscall 中

### syscall 返回

## syscall 的优化
- [ ] int 0x80 对比 fast system call, 它做了更多的工作， 是什么，为什么要做

- int 0x80 / iret
- 32 bit 下引入的 fast system call：sysenter / sysexit（自 Intel Pentium II 始），syscall / sysret（自 AMD K6 始）4
- 64 bit 下的 syscall / sysret

- [ ] 利用 glibc 确认在现在的机器上，使用的是哪一个技术
- [ ] 使用 asm 分别使用一下在机器上调用一次

### sysenter / sysexit

### vsyscall
- 将 vsyscall 映射到固定的位置为什么存在潜在的风险

```c
#include <stdio.h>
#include <time.h>

typedef time_t (*time_func)(time_t *);

int main(int argc, char *argv[]) {
  time_t tloc;
  int retval = 0;

  time_func func = (time_func)0xffffffffff600000;

  retval = func(&tloc);
  if (retval < 0) {
    perror("time_func");
    return -1;
  }
  printf("%ld\n", tloc);

  return 0;
}
```


### vdso
- [ ] x86 中 gettimeofday 在 vdso 中如何生成的

将 vdso dump 出来
https://kernel.googlesource.com/pub/scm/linux/kernel/git/luto/misc-tests/+/5655bd41ffedc002af69e3a8d1b0a168c22f2549/dump-vdso.c

## 整理这个 issue
https://github.com/Martins3/Martins3.github.io/issues/8

## `CONFIG_COMPAT` 到底是为了兼容什么

```c
#ifdef CONFIG_COMPAT
```

## 关键参考
- [ ] [x86 架构下 Linux 的系统调用与 vsyscall, vDSO](https://vvl.me/2019/06/linux-syscall-and-vsyscall-vdso-in-x86)
- [ ] https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-1.html : kernel inside 分析
- [ ] https://lwn.net/Articles/604515/ : Anatomy of a system call, part 2 : 分析 32bit 系统调用
- [ ] https://www.binss.me/blog/the-analysis-of-linux-system-call/ : 分析 32bit 系统调用
- [ ] https://lwn.net/Articles/615809/ : vdso
- [ ] https://www.linuxjournal.com/content/creating-vdso-colonels-other-chicken : 甚至需要自己创建出来一个 vdso

## syscall 列表整理
- https://news.ycombinator.com/item?id=41018135

## syscall 的直接封装
https://news.ycombinator.com/item?id=42022088

# kernel inside 中的笔记
> 汇编例子，可以用来重新分析 程序员的自我修养 System V Application Binary Interface

https://en.wikipedia.org/wiki/Model-specific_register

通过 MSR 存储了 syscall 的入口地址

`/home/shen/linux/arch/x86/kernel/cpu/common.c`
```c

	wrmsr(MSR_STAR, 0, (__USER32_CS << 16) | __KERNEL_CS); // WDNMD 为什么又有CS寄存器啊!
	if (static_cpu_has(X86_FEATURE_PTI))
		wrmsrl(MSR_LSTAR, SYSCALL64_entry_trampoline);
	else
		wrmsrl(MSR_LSTAR, (unsigned long)entry_SYSCALL_64); // 这不是vector table 的入口，是syscall 函数的入口
```
> 如何使用寄存器存储 syscall 入口地址，那是不是 syscall 不用走 idt 哪一条路
> idt 中间的 exception 的分析，似乎是没有参数的吧!
> syscall 的参数传递

> 后面还分析一下: DEFINE_SYSCALL 之类的东西，很基础

The first model specific register - MSR_STAR contains 63:48 bits of the user code segment. These bits will be loaded to the CS and SS segment registers for the sysret instruction which provides functionality to return from a system call to user code with the related privilege. Also the MSR_STAR contains 47:32 bits from the kernel code that will be used as the base selector for CS and SS segment registers when user space applications execute a system call.
> @todo  这两个宏，`__USER32_CS` 和 `__KERNEL_CS`

## How does the Linux kernel handle a system call

```c
asmlinkage const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
	/*
	 * Smells like a compiler bug -- it doesn't work
	 * when the & below is removed.
	 */
	[0 ... __NR_syscall_max] = &sys_ni_syscall,
#include <asm/syscalls_64.h>
};
```

> wrmsr 指令是 ?

> @todo 跳过 32 位兼容设置


entry_SYSCALL_64 在 entry_64.S 中间定义，是所有 syscall 的入口，但是是如何进入到 entry_SYSCALL_64 中间的。

```c
	/* IRQs are off. */
	movq	%rax, %rdi
	movq	%rsp, %rsi
	call	do_syscall_64		/* returns with IRQs disabled */

__visible void do_syscall_64(unsigned long nr, struct pt_regs *regs)
```

> @todo swapgs 是只有 syscall 才如何使用，还是所有都是如此

> 关于 entry_SYSCALL_64 的分析可以看看, that what you are looking for !

## vsyscalls and vDSO
Mapping of the vsyscall page occurs in the `map_vsyscall` function that is defined in the `arch/x86/entry/vsyscall/vsyscall_64.c` source code file.


```c
void __init map_vsyscall(void)
{
	extern char __vsyscall_page;
	unsigned long physaddr_vsyscall = __pa_symbol(&__vsyscall_page);

	if (vsyscall_mode != NONE) {
		__set_fixmap(VSYSCALL_PAGE, physaddr_vsyscall,
			     PAGE_KERNEL_VVAR);
		set_vsyscall_pgtable_user_bits(swapper_pg_dir);
	}

	BUILD_BUG_ON((unsigned long)__fix_to_virt(VSYSCALL_PAGE) !=
		     (unsigned long)VSYSCALL_ADDR);
}
```

vsyscall 含有安全问题，调用 vsyscall 不会更快，而是更加缓慢

The main difference between the vsyscall and vDSO mechanisms is that vDSO maps memory pages into each process in a shared object form, but vsyscall is static in memory and has the same address every time.

Implementation of the vDSO is similar to `vsyscall`.


```c
static int __init init_vdso(void)
{
	init_vdso_image(&vdso_image_64);

#ifdef CONFIG_X86_X32_ABI
	init_vdso_image(&vdso_image_x32);
#endif

	/* notifier priority > KVM */
	return cpuhp_setup_state(CPUHP_AP_X86_VDSO_VMA_ONLINE,
				 "x86/vdso/vma:online", vgetcpu_online, NULL);
}
subsys_initcall(init_vdso);
```

Our kernel is configured for the x86 architecture or for the x86_64 and compatibility mode, we will have ability to call a system call with the int 0x80 interrupt, if compatibility mode is enabled, we will be able to call a system call with the native syscall instruction or sysenter instruction in other way:
> syscall , int 80 和 sysenter 之间的区别是什么 ?

As we can understand from the name of the vdso_image structure, it represents image of the vDSO for the certain mode of the system call entry

> 似乎介绍的很简单，但是实际上关键的内容丧失了。
> 关键的问题:
> 1. 所以什么样的 syscall 可以通过 vdso 简化，什么样的不可以使用
> 2. 为什么 vdso 可以防止攻击，而 vsyscall 不可以
> 3. vdso 是动态链接库加载和映射的典范吧!

## How does the Linux kernel run a program
> we will come back soon, 讲解了一些非常有意思的东西，但是我们想知道 是从 idt 到达 syscall 的吗 ? 应该是不可能的!

> 本 section 讲解了 execvp 的实现，一下是关于自己对于其的设想

1. 初始化 mm_struct，但是此时并不可以初始化 vm_area, vm_area 的信息需要从 分析可执行文件(大小，起始位置之类的)
0. 将可执行文件读取到内存中间
1. 请求 ld 的协作处理动态链接库
2. 分配 stack
3. 将用户参数和环境变量复制到 stack 的顶部
4. 初始化 context 环境，比如将 rip 之类的放到 stack 的顶端，形成一个 context_struct，当被选中之后, 就像是该进程从打断之中恢复一样
6. execvp 实现 img 替换，重新初始化 heap 和 stack , 所以需要处理原来进程对应 heap stack 需要被释放，原来的父子关系需要被保留下来。但是原来的 IPC 机制内容都会被取消掉。

> `strace ls` 指令的时候，为什么第一条 syscall 不是 fork


```c
static int __do_execve_file(int fd, struct filename *filename, // 分析的源头
			    struct user_arg_ptr argv,
			    struct user_arg_ptr envp,
			    int flags, struct file *file)

  // some check

	retval = unshare_files(&displaced); // @todo We need to call this function to eliminate potential leak of the execve'd binary's file descriptor.

	retval = exec_binprm(bprm); //
      --> static int exec_binprm(struct linux_binprm *bprm)
          --> int search_binary_handler(struct linux_binprm *bprm)
              --> static int load_elf_library(struct file *file)
                  --> void start_thread(struct pt_regs *regs, unsigned long new_ip, unsigned long new_sp)
                      --> static void start_thread_common(struct pt_regs *regs, unsigned long new_ip, unsigned long new_sp, unsigned int _cs, unsigned int _ss, unsigned int _ds)

  // execve finisehed it's work and do some clean up !
```

> start_thread_common


##  How does the open system call work

## Limits on resources in Linux



## 补充
https://en.wikibooks.org/wiki/X86_Assembly/Interfacing_with_Linux
> syscall 和 int 使用指令居然是不同的

Operating systems can use both paging and segmentation to implement protected memory models.
Segment descriptors provide the necessary memory protection and privilege checking for segment
accesses. By setting segment-descriptor fields appropriately, operating systems can enforce access
restrictions as needed
> segmentation 其作用不是

根据 Amd64 manual V3 的内容，描述和作者内容一致，而且可以确定，syscall 不会经过 idt

## 小实验，我的机器中，到底在调什么 syscall

openEuler 的虚拟机:
```txt
🤒  sudo syscount -i 3
Tracing syscalls, printing top 10... Ctrl+C to quit.
[23:05:05]
SYSCALL                   COUNT
read                       2260
futex                      1203
epoll_pwait                 485
poll                        218
getrusage                   185
write                       184
recvmsg                      91
openat                       49
close                        46
newfstatat                   46
```

nixos 的物理机中:
```txt
[23:02:30]
SYSCALL                   COUNT
ppoll                     12028
ioctl                     12009
futex                      4442
write                       534
read                        333
poll                        281
recvmsg                     252
epoll_pwait                 231
wait4                       195
nanosleep                   188
```

原来是 qemu 啊:
```txt
🧀  sudo bpftrace -e "kfunc:vmlinux:__x64_sys_ppoll { @[comm] = count(); }"
Attaching 1 probe...
^C

@[sudo]: 4
@[sshd]: 134
@[ssh]: 215
@[qemu-system-x86]: 65740
```

## 这个项目
https://github.com/hrw/syscalls-table

## 这个工具好哇
ausyscall --dump

## 工具
https://github.com/facebookexperimental/reverie

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
