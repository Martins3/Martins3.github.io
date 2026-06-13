# fork

## 首先学会用户态的使用
- man clone(2) : clone, __clone2, clone3
- man vfork(2) : 是一个特殊的 clone(2)，当使用 vfork 的时候，page table 不会被拷贝。
- man fork(2)
- man unshare(2)
- man set_tid_address(2)

## 整体流程

大多数代码都是在 kernel/fork.c 中，大约存在 3000 行左右

- kernel_clone
  - copy_process
      - dup_task_struct
        - alloc_thread_stack_node : 拷贝内核态的 stack
      - sched_fork : 初始化和 scheduler 相关的配置
      - copy_fs
      - copy_mm
        - copy_mm
          - dup_mm
            - allocate_mm
            - mm_init
              - init_new_context
              - mm_alloc_pgd
            - dup_mmap
              - vm_area_dup
              - anon_vma_fork
              - copy_page_range
                - copy_p4d_range
                - copy_pud_range
                - copy_pmd_range
                - copy_pte_range
                  - copy_nonpresent_pte :  copy swap entry, migration entry and device entry
                  - copy_nonpresent_pte
                    - copy_present_page
  - wake_up_new_task

单独拎出来地址空间的拷贝:
- `copy_process` : 进行各种 flags 检查，防止出现互相冲突的 flag，利用 flag 确定到底什么需要进行拷贝，当 flag 中含有 CLONE_VM 的时候，会利用 copy_mm 进行进程地址空间的拷贝。
- `copy_mm` : 初始化 `mm_struct` 等
- `dup_mmap` : 逐个 `vm_area` 拷贝，并且构建 `vm_area` 属性
- `copy_page_range` => `copy_p4d_range` => `copy_pud_range` => copy_pmd_range => copy_pte_range => copy_one_pte : 逐级的进行页表拷贝



## 几个问题
- kthread 的产生方法和用户态进程的产生方法有何不同?
  - 没啥不同，都是调用 kernel_clone 而已
- task_struct::stack 和 task_struct::thread_info 的作用
- signal 在 fork 下的处理?
- fork 的 page table 拷贝位置

## exit

> Man `exit_group`
>
>   This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.

- exit_group
  - do_group_eixt : 其他人已经发送了通知 ?
  - zap_other_threads : 我来通知大家
  - **do_exit** : 将当前进程回收
    - ptrace_event(PTRACE_EVENT_EXIT, code);
    - exit_signals(tsk); 释放和 signal 相关的资源
      - `__cleanup_sighand`
    - exit_notify
      - forget_original_parent : 正如注释所说 1. 将 child 放到 init 上 2. (POSIX 3.2.2.2) 上要求的 SIGHUP  和 SIGCONT
        - find_child_reaper : 找到 init reaper
          - `struct task_struct *reaper = pid_ns->child_reaper;`  : 由于 namespace 的出现，所以不一定是 init
          - find_alive_thread : 如果是 namespace 中间的 init 要挂掉了，那么找一个 init 在一个 thread group  的其他的 thread 来作为 reaper
          - zap_pid_ns_processes : 如果 namespace 的 init 的整个 thread group 都没了，那么整个 namespace 被回收掉
        - find_new_reaper : 注释说的很清楚，首先找 thread group 内部的，其次 prctl, 最后 find_child_reaper 获取的 init
          - reparent_leader : (POSIX 3.2.2.2) 的实现
        - release_task : 如果当时正在 trace 其他的进程，那么需要被 trace 的进程需要被 kill
    - exit_mm : 诸如此类的内容还有很多
    - do_task_dead : 将当前 schedule 出去，然后之后 parent 来回收剩下的资源

## difference between exit and exit_group
- `__do_sys_exit`
  - **do_eixt**

所以区别就是 : exit_group 会发送 KILL 信号给整个 process group 的开始结束
```c
/*
 * Nuke all other threads in the group.
 */
int zap_other_threads(struct task_struct *p)
{
	struct task_struct *t = p;
	int count = 0;

	p->signal->group_stop_count = 0;

	while_each_thread(p, t) {
		task_clear_jobctl_pending(t, JOBCTL_PENDING_MASK);
		count++;

		/* Don't bother with already dead threads */
		if (t->exit_state)
			continue;
		sigaddset(&t->pending.signal, SIGKILL);
		signal_wake_up(t, 1);
	}

	return count;
}
```

## vfork
> the parent is suspended until the child terminates or calls exec()

具体来说，通过 wait_for_vfork_done() 来实现

> As with fork(2), the child process created by vfork() inher‐
its  copies  of  various  of the caller's process attributes
(e.g., file descriptors, signal  dispositions,  and  current
working  directory);

> Until that  point,  the  child  shares  all memory with its parent, including the
stack.

```c
SYSCALL_DEFINE0(vfork)
{
	struct kernel_clone_args args = {
		.flags		= CLONE_VFORK | CLONE_VM,
		.exit_signal	= SIGCHLD,
	};

	return kernel_clone(&args);
}
```

CLONE_VM 表示共享地址空间，其他的 flags 都没有，表示其他的各种资源都是拷贝的。

- [ ] 这里我不是很懂，既然总是要替换掉的，为什么还需要拷贝过来，这有意义吗、
- [x] vfork 如何不拷贝页表的
  - 因为 vfork 采用的是 CLONE_VM ，共享地址空间，然后直接替换原来的，所以无须拷贝。
  换言之，只要是没有 CLONE_VM 的都是会拷贝 page table 的，如果创建的都是线程，也是共享 page table 的。
- [ ] 听上去不错，为什么现在的内核中都不去使用这个东西
执行:
```sh
sudo bpftrace -e "kprobe:__do_sys_vfork { @[kstack] = count(); }"
```
完全找不到输出，其实只是这个系统调用没有人用而已，因为大伙用的 clone(2) 来替代 vfork 的。

## 扩展
- https://news.ycombinator.com/item?id=48425528 : 2026-06-07 的讨论
- https://www.microsoft.com/en-us/research/publication/a-fork-in-the-road/
- https://news.ycombinator.com/item?id=30502392
- https://thume.ca/2020/04/18/telefork-forking-a-process-onto-a-different-computer/

### fork 设计问题

- [Is it safe to fork from within a thread?](https://stackoverflow.com/questions/6078712/is-it-safe-to-fork-from-within-a-thread)

- [ ] pthread_atfork : 在 multithread 的情况下进行 fork 的时候，整个地址空间都会拷贝，但是存在一些恶心的事情
  - The problem is that fork() only copies the calling thread, and any mutexes held in child threads will be forever locked in the forked child. The pthread solution was the pthread_atfork() handlers. [^1]
  - [x] 在 multithread 中间执行 exec 的时候，也是存在很诡异的问题，其他的 threads 的地址空间被删除了，所以只有当 thread_group 只有一个 thread 才可以，细节在 de_thread 中间


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
