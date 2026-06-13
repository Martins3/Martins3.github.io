# syscall restart
<!-- f056d84c-9f8c-44dd-a294-8b010285fe4d -->

## Linux 默认不终止进程的信号

和记忆中的一样，默认的结果都是 TERM 或者 CORE ，是其他行为的如下:

在 man signal(7) 中的 Standard signals 中对于每一个信号
的行为都定义清晰的说明了。

在 Linux 中，大多数信号的默认行为是终止进程（`Term`）或终止并产生 core dump（`Core`）。
但以下信号的默认行为**不会导致进程直接退出**：

1. 默认忽略 (Ignore)

| 信号       | 说明                                    |
|------------|-----------------------------------------|
| `SIGURG`   | socket 上有紧急数据到达                 |
| `SIGWINCH` | 终端窗口大小改变                        |
| `SIGCHLD`   | Child stopped, terminated, or continued |

2. 默认停止进程 (Stop 和 Cont)

| 信号      | 说明                                     |
|-----------|------------------------------------------|
| `SIGSTOP` | 强制停止进程（**不可捕获、阻塞或忽略**） |
| `SIGTSTP` | 用户按下 Ctrl+Z 时产生                   |
| `SIGTTIN` | 后台进程试图读取终端输入                 |
| `SIGTTOU` | 后台进程试图写入终端输出                 |
| `SIGCONT` | 让已停止的进程恢复运行；如果进程已在运行则忽略 |

补充说明

- `SIGKILL` 和 `SIGSTOP` 是两个特殊的信号，既不能被捕获，也不能被阻塞或忽略。

## 总结

首先，我们考虑不注册 handler 的情况:
1. SIGWINCH 之类的自动忽略信号，如果不注册 handler ，就像是不存在一样
2. SIGTSTP 如果不注册，就和 SIGSTOP 一样， 相当于是 SA_RESTART 的 signal handler
	- SIGTTIN 和 SIGTTOU 的行为也是类似的，如何使用，参考 docs/kernel/signal/code/sigttin-sigtou.c
	- 所以，类似 read / write 之类的不用考虑处理 EINTR
3. 其他的，直接 core / term 了，就没什么需要考虑的了

如果考虑注册 handler 的情况，syscall 是否返回 -EINTR 取决于:
1. SA_RESTART 是否启用
2. syscall 的类型

如果是 signalfd 来接管信号的处理，就不存在这个问题了:
因为 signal handler 是打断了当前的 syscall 的执行的 thread ，然后去执行 handler。
signalfd 的处理是在单独的一个 thread 中来执行 handler ，既然不存在打断当前的 syscall ，
所以就不存在这些问题了

## 为什么这么设计?

或者说，为什么这些系统调用必须检查到 EINTR
1. select() poll() epoll_wait() pselect() ppoll() io_getevents()
2. sleep
	- 带 socket timeout 的 socket I/O : accept()、recv()、send()、connect() 如果设置了 SO_RCVTIMEO 或 SO_SNDTIMEO
	- nanosleep()、clock_nanosleep()、usleep()、sleep()

## restart syscall 是如何实现的

1.
使用 code/src/c/signal/syscall-restart.c 来理解
```txt
read(0, 0x7ffdd52571f0, 128)            = ? ERESTARTSYS (To be restarted if SA_RESTART is set)
--- SIGUSR1 {si_signo=SIGUSR1, si_code=SI_USER, si_pid=122040, si_uid=1000} ---
write(1, "Caught signal 10 (User defined s"..., 41Caught signal 10 (User defined signal 1)) = 41
rt_sigreturn({mask=[]})                 = 0
read(0
```

使用 strace 分析，发现这个 syscall 像是 signal handler 重新调用了。

2.
参考 https://phpor.net/blog/post/3709

执行 sleep 100000

回车之后，当看到nanosleep({5, 0},  立即ctrl-z，会发现程序 stop 了，
然后，输入 fg（切换后台任务到前台，继续运行），又一次看到了restart_syscall 了。

```txt
🤒  fg
[1]  + 350630 continued  strace sleep 10000
{tv_sec=9994, tv_nsec=953962062}) = ? ERESTART_RESTARTBLOCK (Interrupted by signal)
--- SIGCONT {si_signo=SIGCONT, si_code=SI_USER, si_pid=350513, si_uid=1000} ---
restart_syscall(<... resuming interrupted clock_nanosleep ...>
```

https://www.quora.com/What-are-the-use-cases-of-restart_syscall-system-call-on-Linux-Systems

似乎基本是可信的，就是 SIGCONT 结合 restart_syscall 使用


## restart_syscall 系统调用

为什么需要一个额外的 restart_syscall() 系统调用，显然，
当之前的系统调用结束之后，是不知道之前干过什么的，那么就直接发送的
一个 restart_syscall 系统调用，然后配合在内核记录下的信息，来继续
之前的系统调用了。

https://man7.org/linux/man-pages/man2/restart_syscall.2.html

> The restart_syscall() system call is used to restart certain
> system calls after a process that was stopped by a signal (e.g.,
> SIGSTOP or SIGTSTP) is later resumed after receiving a SIGCONT
> signal.  This system call is designed only for internal use by the
> kernel.

> restart_syscall() restarts the interrupted system call with a time
> argument that is suitably adjusted to account for the time that
> has already elapsed (including the time where the process was
> stopped by a signal).  Without the restart_syscall() mechanism,
> restarting these system calls would not correctly deduct the
> already elapsed time when the process continued execution.

也就是为了重新调用时间相关的系统调用的时候，再次调用的时候，时间参数是被调整过的。


这个函数，在内核中也是广泛使用的。
```c
static inline int restart_syscall(void)
{
	set_tsk_thread_flag(current, TIF_SIGPENDING);
	return -ERESTARTNOINTR;
}
```

### syscall 的 restart 就是由于 signal 导致的么？
是的，只有 signal 才会导致系统调用提前返回

### 当 signal 被 stop 的时候
```txt
 sudo cat /proc/1646434/stack
[sudo] password for martins3:
[<0>] do_signal_stop+0x182/0x210
[<0>] get_signal+0x25c/0x870
[<0>] arch_do_signal_or_restart+0x76/0x140
[<0>] exit_to_user_mode_loop+0x99/0x500
[<0>] do_syscall_64+0x211/0x6e0
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

## restart syscall 的内核实现
```txt
@[
        do_nanosleep+1
        hrtimer_nanosleep_restart+230
        do_syscall_64+116
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
很容易可以找到这个 set_restart_fn

### 睡眠醒过来之后，需要调用 signal_pending 检查
https://www.kernel.org/doc/html/latest/kernel-hacking/hacking.html#ioctls-not-writing-a-new-system-call

> After you slept you should check if a signal occurred:
> the Unix/Linux way of handling signals is to temporarily exit the system call
> with the -ERESTARTSYS error. The system call entry code will switch back to
> user context, process the signal handler and then your system call will be
> restarted (unless the user disabled that). So you should be prepared to process
> the restart, e.g. if you’re in the middle of manipulating some data structure.
>
> if (signal_pending(current))
>         return -ERESTARTSYS;

醒来之后，就需要检查一下 signal pending ，如果存在 pending 的 signal，之后重启该系统调用

### while 循环中应该 cond_resched() ，可以调用 signal_pending 来检查信号

## [ ]  几个 syscall restart 的错误处理

```c
/*
 * These should never be seen by user programs.  To return one of ERESTART*
 * codes, signal_pending() MUST be set.  Note that ptrace can observe these
 * at syscall exit tracing, but they will never be left for the debugged user
 * process to see.
 */
#define ERESTARTSYS	512
#define ERESTARTNOINTR	513
#define ERESTARTNOHAND	514	/* restart if no handler.. */
#define ERESTART_RESTARTBLOCK 516 /* restart by calling sys_restart_syscall */
```

1. ERESTARTNOHAND 和 ERESTART_RESTARTBLOCK

这两个应该是一样的，
pause 如果没有 signal handler 那么就重放，，否则直接结束。
```c
SYSCALL_DEFINE0(pause)
{
	while (!signal_pending(current)) {
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return -ERESTARTNOHAND;
}
```

alarmtimer_do_nsleep 这个就是 nanosleep 出现错误的时候的默认操作

如果被 stop ，那么可以恢复，如果收到其他的信号，那么一定会结束掉。

2. ERESTARTSYS : 如果信号处理程序设置了 SA_RESTART，则重启整个系统调用；否则返回 -EINTR 给用户空间

这个的使用场景就放多了，例如 eventfd_read

3. ERESTARTNOINTR : 总是重启系统调用，无论有没有信号处理程序或 SA_RESTART

这个就非常特殊了，即便是 man singal(7) 中也没有提到，因为用户态根本看不到这个事情。

本 restart_syscall() 封装，其调用位置非常多
```c
static inline int restart_syscall(void)
{
	set_tsk_thread_flag(current, TIF_SIGPENDING);
	return -ERESTARTNOINTR;
}
```

检查一下调用的位置，主要使用的地方是驱动和网络:
```sh
rg -l "restart_syscall\(\)"
```

```txt
block/blk-cgroup.c
net/dsa/dsa.c
arch/s390/kernel/process.c
net/bridge/br_sysfs_br.c
net/bridge/br_sysfs_if.c
net/ipv6/addrconf.c
mm/madvise.c
net/ipv4/devinet.c
drivers/base/core.c
drivers/infiniband/ulp/ipoib/ipoib_vlan.c
drivers/infiniband/ulp/ipoib/ipoib_cm.c
kernel/cgroup/cgroup-v1.c
drivers/thunderbolt/switch.c
drivers/thunderbolt/retimer.c
drivers/net/bonding/bond_options.c
drivers/net/usb/qmi_wwan.c
```

使用 syscount 分析，发现似乎记录是错误的:
```txt
[11:37:18]
SYSCALL                   COUNT
fstat                    442585
pread                    177869
read                      10821
access                     9922
gettid                     9868
epoll_wait                 9848
write                      6267
rt_sigprocmask             6100
newfstatat                 3511
openat                     2399
```
不用看了，这显然是一个 bug 了:

使用 strace 观察到的是
```txt
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
write(1, "7\n", 2)                      = ? ERESTARTNOINTR (To be restarted)
```

实现 restart 和用户态没有关系，在架构相关的代码中，例如
arch/x86/kernel/signal.c
```txt
@[
        arch_do_signal_or_restart+5
        exit_to_user_mode_loop+119
        do_syscall_64+689
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
所有的核心逻辑都是在:
```c
	/* Are we from a system call? */
	if (syscall_get_nr(current, regs) != -1) {
		/* If so, check system call restarting.. */
		switch (syscall_get_error(current, regs)) {
		case -ERESTART_RESTARTBLOCK:
		case -ERESTARTNOHAND:
			regs->ax = -EINTR;
			break;

		case -ERESTARTSYS:
			if (!(ksig->ka.sa.sa_flags & SA_RESTART)) {
				regs->ax = -EINTR;
				break;
			}
			fallthrough;
		case -ERESTARTNOINTR:
			regs->ax = regs->orig_ax;
			regs->ip -= 2;
			break;
		}
	}
```

## 阅读材料
- https://unix.stackexchange.com/questions/612506/is-it-safe-to-restart-system-calls
- https://stackoverflow.com/questions/13357019/how-to-know-if-a-linux-system-call-is-restartable-or-not
- https://unix.stackexchange.com/questions/612506/is-it-safe-to-restart-system-calls
- https://stackoverflow.com/questions/9576604/what-does-erestartsys-used-while-writing-linux-driver

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
