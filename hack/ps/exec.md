## exec
说实话，都是 快速的阅读 程序员的自我修养 ，现在对于 elf 格式各种的恐惧，但是静态链接怕个屁。

- [ ]  chapter 27 28 和 ulk execution program 那个可以一起看
- [ ] /home/maritns3/core/vn/kernel/plka/syn/fs/binfmt_elf.md 之前的总结

我想知道的问题 :
- [ ] 执行 exec 的进程空间是如何被释放以及替换的 ? 
- [ ] elf 格式解析了，靠什么将代码加载进来的
- [ ] 什么内容不会被继承 ?

- [x] 那么 pc 数值是怎么设置的，pc 数值显然只能设置为 glibc 提供的入口，如果使用是 glibc 作为动态链接库，但是加载动态链接库是不是 loader 的事情吗 ?
  - interrupter 也就是 loader 实际上第一行被执行的代码

- [ ] vdso 是怎么放进去的 ?

```c
/*
 * Create a new mm_struct and populate it with a temporary stack
 * vm_area_struct.  We don't have enough context at this point to set the stack
 * flags, permissions, and offset, so we use temporary values.  We'll update
 * them later in setup_arg_pages().
 */
static int bprm_mm_init(struct linux_binprm *bprm)
```

```c
static struct linux_binfmt elf_format = {
	.module		= THIS_MODULE,
	.load_binary	= load_elf_binary,
	.load_shlib	= load_elf_library,
	.core_dump	= elf_core_dump,
	.min_coredump	= ELF_EXEC_PAGESIZE,
};
```

- [ ] envp

- do_execveat_common
  - alloc_bprm
  - bprm_stack_limits
  - copy_string_kernel : Copy and argument/environment string from the kernel to the processes stack.
  - copy_strings : 'copy_strings()' copies argument/environment strings from the old processes's memory to the new process's stack.
    - bprm_execve
      - unshare_files
      - prepare_bprm_creds
      - do_open_execat : 打开文件
      - sched_exec : execve() is a valuable balancing opportunity, because at this point the task has the smallest effective memory and cache footprint.
      - exec_binprm : **TODO** 原来 search_binary_handler 是递归的，现在的处理被做成循环，但是为什么要递归，循环为什么是 5 次数，不清楚啊 !
        - search_binary_handler
          - struct linux_binfmt::load_binary
            - load_elf_phdrs
            - parse_elf_properties
            - begin_new_exec
              - de_thread : Make this the only thread in the thread group
              - exec_mmap
                - exec_mm_release
                - activate_mm
              - unshare_sighand
            - setup_new_exec
            - setup_arg_pages
            - for(i = 0, elf_ppnt = elf_phdata; i < elf_ex->e_phnum; i++, elf_ppnt++) : 将 image load 到空间中去
              - elf_map
              - set_brk
            - load_elf_interp : 将 /lib64/ld-linux-x86-64.so.2 加载进去
            - create_elf_tables
            - start_thread
        - ptrace_event : If the current program is being ptraced, a SIGTRAP signal is sent to it after a successful execve().
          - send_sig(SIGTRAP, current, 0);

- [ ] de_thread 的实现非常诡异啊

## syscalls
270	n64	kexec_load			sys_kexec_load
320	common	kexec_file_load		sys_kexec_file_load

57	n64	execve			sys_execve
316	n64	execveat			sys_execveat

```c
SYSCALL_DEFINE3(execve,
		const char __user *, filename,
		const char __user *const __user *, argv,
		const char __user *const __user *, envp)
{
	return do_execve(getname(filename), argv, envp);
}

SYSCALL_DEFINE5(execveat,
		int, fd, const char __user *, filename,
		const char __user *const __user *, argv,
		const char __user *const __user *, envp,
		int, flags)
{
	int lookup_flags = (flags & AT_EMPTY_PATH) ? LOOKUP_EMPTY : 0;

	return do_execveat(fd,
			   getname_flags(filename, lookup_flags, NULL),
			   argv, envp, flags);
}
```
两个系统调用没有什么区别，只是搜索方式即将执行的文件方式不同而已

[^1]: https://lwn.net/Articles/630727/
[^2]: https://lwn.net/Articles/631631/ : 应该算是分析的即为详细了吧!
