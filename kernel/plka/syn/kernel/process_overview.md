# 进程管理
> 基于章节2
> 2.1 - 2.4 处理process的生老病死
> 2.5 - 2.8 scheduler 的内容

## fork
clone(2), execve(2), vfork(2) 以及 kernel thread

1. vfork() is a special case of clone(2).  It is used to create new processes without copying the page tables of the parent process.  It may be useful in performance-sensitive applications where a child is created which then immediately issues an execve(2).
2. Unlike fork(2), clone() allows the child process to share parts of its execution context with the calling process, such as the virtual address space, the table of file descriptors, and the table of signal handlers.  (Note that on this manual page, "calling process" normally corresponds to "parent process".  But see the description of CLONE_PARENT below.)
3. One use of clone() is to implement threads: multiple flows of control in a program that run concurrently in a shared address space.

all syscall converge to one : `_do_fork`

fork 完成的事情非常简单：拷贝 wakeup
> 拷贝的内容比想象的多，和init类似，都是众多内容的合集，按照书上的内容走吧!

dup_task_struct : 分配current 的进程控制块以及复制 task_struct 和 kernel stack

`thread_info`在内核stack的底部，stack 的顶端依旧放置trapframe 吧!

1. execve exit wait daemon namespace

## relation between process
```c
struct task_struct {
	/* PID/PID hash table linkage. */
	struct pid			*thread_pid;
	struct hlist_node		pid_links[PIDTYPE_MAX];
	struct list_head		thread_group;
	struct list_head		thread_node;
```

## 补充
#### process group and session
https://stackoverflow.com/questions/6548823/use-and-meaning-of-session-and-process-group-in-unix

## TODO later
1. binfmt_elf.md doc 

## TODO
1. kill syscall 的实现 (应该是信号机制中间吧 !)
2. chapter 27 28 和 ulk execution program 那个可以一起看
3. tlpi 29 ~ 33 之间的 thread 实现内核的支持是什么 ?

4. 我需要 context switch 的绝对细节
5. thread_struct 的具体内容是什么 ?


https://stackoverflow.com/questions/9305992/if-threads-share-the-same-pid-how-can-they-be-identified
课代表总结：
用户看到的是tgid: thread group id
kernel看到的是pid

> thread group id,  process group , session group 是什么关系


1. group 机制
2. 写一个总结 tlpi 的内容
3. 进入内核 Exec 的内容


```c
			list_add_tail(&p->sibling, &p->real_parent->children); // 这些成员的含义是什么 ?
			list_add_tail_rcu(&p->tasks, &init_task.tasks);
```

