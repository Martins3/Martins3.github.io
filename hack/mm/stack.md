# linux 内存管理: stack


<!-- vim-markdown-toc GitLab -->

- [abstract](#abstract)
- [stack overflow attack](#stack-overflow-attack)
- [stack types](#stack-types)

<!-- vim-markdown-toc -->

- [ ] https://stackoverflow.com/questions/12911841/kernel-stack-and-user-space-stack

x86_64 also has a feature which is not available on i386, the ability to automatically switch to a new stack for designated events such as double fault or NMI, which makes it easier to handle these unusual events on x86_64. This feature is called the Interrupt Stack Table (IST). There can be up to 7 IST entries per CPU. The IST code is an index into the Task State Segment (TSS). The IST entries in the TSS point to dedicated stacks; each stack can be a different size.[^1]


如果是发生在用户态，那么是要进行两次 stack 切换的
https://unix.stackexchange.com/questions/491437/how-does-linux-kernel-switches-from-kernel-stack-to-interrupt-stack
- [ ] 代码上的证据在哪里 ?

## abstract 
不同的架构如何支持 stack 的 ?　使用的指令不同，在内核中间的配置不同 ?
1. x86-32
2. amd64
3. arm
4. mips

用户态的 stack:
1. 利用 brk 实现的吗 ?
2. stack 溢出攻击的原理
3. stack 和 calling convention 的效果
4. glibc 是如何帮助用户程序初始化 stack 的 ?
5. 用户程序的 argc 和 argv 以及 环境变量是放在 stack 的哪一个位置的 ?
6. 从编译器的角度，stack 是如何实现的 ? llvm 

内核态的 stack :
2. exception 的 stack 在哪里 ?
4. fork 的 flag 是如何处理 stack 的复制的 ?
    1. 能不能不进行复制 ? (应该没有这个选项吧)
    2. 复制的分为那几个步骤 ? 3. 也是 COW 吗 ?
5. stack 和 resource limit


1. stack 的增长和缩减的时机和方法，如果由于递归创建出来了一个巨大的 stack，然后逐级return 之后，这些 stack 的空间会被回收吗 ?
2. stack 和 一般的 anon mmap 的区别是什么 ? 或者说从内核的角度来说，
3. stack 是使用系统调用分配的 ? 还是 fork 的时候创建的 ? stack 对应的 vma 是什么创建的 ?


4. cat /proc/self/maps : vma 有名字吗，他怎么知道是 stack 
```
7ffc83f45000-7ffc83f67000 rw-p 00000000 00:00 0                          [stack]
```

## stack overflow attack
缓冲区在 stack 中间，如果让 stack 不可执行，那么不就结束了。


程序中发生函数调用时，计算机做如下操作：
- 首先把指令寄存器EIP（它指向当前CPU将要运行的 下一条指令的地址）中的内容压入栈，作为程序的返 回地址（下文中用RET表示）；
- 之后放入栈的是基址寄存器EBP，它指向当前函数栈 帧（stack frame） 的底部；
- 然后把当前的栈指针ESP拷贝到EBP，作为新的基地 址，
- 最后为本地变量的动态存储分配留出一定空间，并把 ESP减去适当的数值

提高成功率的两种方法 : 
1. 在 shellcode 前面添加填充代码
2. 在返回值附近重复跳转地址

## stack types
[^1]
第一个 stack : per thread stack
1. Like all other architectures, x86_64 has a kernel stack for every active thread.
While the thread is in user space the kernel stack is empty except for the `thread_info` structure at the bottom.
> TODO thread_info 到底是啥，真的是放到 stack 的末尾吗 ?
```c
#ifdef CONFIG_THREAD_INFO_IN_TASK
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;
#endif
```

第二个 stack : percpu stack



[^1]: [kernel doc : kernel stack](https://www.kernel.org/doc/html/latest/x86/kernel-stacks.html)
