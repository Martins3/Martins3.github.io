# Understand Linux Kernel : Signals

## KeyNote
1. The `SIGKILL` and `SIGSTOP` 不可忽视
2. 相同类型的会被忽视，除非是 rt 类型的
3. 默认的 handler 到底在哪里 ?

4. 需要分析的代码:
  1. kill signal
  2. mask
  3. ptrace

Signals were introduced by the first Unix systems to allow interactions between User Mode processes;
the kernel also uses them to notify processes of system events.

## 1 The Role of Signals

Signals serve two main purposes:
• To make a process aware that a specific event has occurred
• To cause a process to execute a signal handler function included in its code

[](https://en.wikipedia.org/wiki/Signal_(IPC))
> 对于每一个信号的含义描述的更加清晰

An important characteristic of signals is that they may be sent at any time to a process whose state is usually unpredictable. Signals sent to a process that is not currently executing must be saved by the kernel until that process resumes execution.
**Blocking a signal (described later) requires that delivery of the signal be held off until
it is later unblocked, which exacerbates the problem of signals being raised before they can be delivered.**

Therefore, the kernel distinguishes two different phases related to signal transmission:
- Signal generation

  The kernel updates a data structure of the destination process to represent that a
  new signal has been sent.

- Signal delivery

  The kernel forces the destination process to react to the signal by changing its
  execution state, by starting the execution of a specified signal handler, or both.

Signals that have been generated but not yet delivered are called pending signals. At
any time, only one pending signal of a given type may exist for a process; additional
pending signals of the same type to the same process are not queued but simply discarded. Real-time signals are different, though: there can be several pending signals
of the same type.

**In general, a signal may remain pending for an unpredictable amount of time.** The following factors must be taken into consideration:
- Signals are usually delivered only to the currently running process (that is, to the current process).
- Signals of a given type may be selectively blocked by a process (see the later section “Modifying the Set of Blocked Signals”). In this case, the process does not
receive the signal until it removes the block.
- When a process executes a signal-handler function, it usually masks the corresponding signal—i.e., it automatically blocks the signal until the handler terminates. A signal handler therefore cannot be interrupted by another occurrence of
the handled signal, and the function doesn’t need to be reentrant.

Although the notion of signals is intuitive, the kernel implementation is **rather complex**. The kernel must:
- Remember which signals are blocked by each process.
- When switching from Kernel Mode to User Mode, check whether a signal for a
process has arrived. This happens at almost every timer interrupt (roughly every
millisecond).
- Determine whether the signal can be ignored. This happens when all of the following conditions are fulfilled:
  — The destination process is not traced by another process (the `PT_PTRACED` flag in the process descriptor ptrace field is equal to 0).
  — The signal is not blocked by the destination process.
  — The signal is being ignored by the destination process (either because the process explicitly ignored it or because the process did not change the default action of the signal and that action is “ignore”).
- Handle the signal, which may require switching the process to a handler function at any point during its execution and restoring the original execution context after the function returns.


#### 1.1 Actions Performed upon Delivering a Signal
There are three ways in which a process can respond to a signal:
1. Explicitly ignore the signal.
2. Execute the default action associated with the signal (see Table 11-1). This
action, which is predefined by the kernel, depends on the signal type and may be
any one of the following:
    1. Kill
    2. Terminate
    3. Dump
    4. Stop
    5. Continue
3. Catch the signal by invoking a corresponding signal-handler function.

The `SIGKILL` and `SIGSTOP` signals cannot be ignored, caught, or blocked, and their
default actions must always be executed. Therefore, `SIGKILL` and `SIGSTOP` allow a user
with appropriate privileges to terminate and to stop, respectively, every process,
regardless of the defenses taken by the program it is executing.

A signal is fatal for a given process if delivering the signal causes the kernel to kill the
process. The `SIGKILL` signal is always fatal; moreover, each signal whose default
action is “Terminate” and which is not caught by a process is also fatal for that process.
Notice, however, that a signal caught by a process and whose corresponding
signal-handler function terminates the process is not fatal, because the process chose
to terminate itself rather than being killed by the kernel.

#### 1.2 POSIX Signals and Multithreaded Applications

The POSIX 1003.1 standard has some stringent requirements for signal handling of
multithreaded applications:
- Signal handlers must be shared among all threads of a multithreaded application;
however, each thread must have its own mask of pending and blocked signals.
- The `kill()` and `sigqueue()` POSIX library functions (see the later section “System Calls Related to Signal Handling”) must send signals to whole multithreaded applications, not to a specific thread. The same holds for all signals
(such as `SIGCHLD`, `SIGINT`, or `SIGQUIT`) generated by the kernel.
- Each signal sent to a multithreaded application will be delivered to just one
thread, which is arbitrarily chosen by the kernel among the threads that are not
blocking that signal.
- If a fatal signal is sent to a multithreaded application, the kernel will kill all
threads of the application—not just the thread to which the signal has been
delivered.

Furthermore, a pending signal is private if it has been sent to a specific process; it is
shared if it has been sent to a whole thread group.

#### 1.3 Data Structures Associated with Signals
The most significant ones are shown in Figure 11-1.
![](../img/11-1.png)

* ***The signal descriptor and the signal handler descriptor***

In fact, as mentioned in the section “The clone(),
fork(), and vfork() System Calls” in Chapter 3, the signal descriptor is shared by all
processes belonging to the same thread group—that is, **all processes created by
invoking the clone() system call with the `CLONE_THREAD` flag set—thus the signal
descriptor includes the fields that must be identical for every process in the same
thread group.**
> 1. copy_sighand 和 copy_signal 可以验证以上说法，不过是否 copy sighand 是存在另外的 flag，而不是 CLONE_THREAD

> **此处介绍了 signal_struct 和 sighand_struct**

* ***The sigaction data structure***

Thus the `k_sigaction` structure simply reduces to a single `sa` structure of type
sigaction, which includes the following fields:

- sa_handler

  This field specifies the type of action to be performed; its value can be a pointer
  to the signal handler, SIG_DFL (that is, the value 0) to specify that the default
  action is performed, or SIG_IGN (that is, the value 1) to specify that the signal is
  ignored.

- **sa_flags**

  This set of flags specifies how the signal must be handled; some of them are
  listed in Table 11-6.

- sa_mask

  This `sigset_t` variable specifies the signals to be masked when running the signal handler.

Table 11-6. Flags specifying how to handle a signal
| Flag                     | Name Description                                                                                             |
|--------------------------|--------------------------------------------------------------------------------------------------------------|
| SA_NOCLDSTOP             | Applies only to SIGCHLD; do not send SIGCHLD to the parent when the process is stopped                       |
| SA_NOCLDWAIT             | Applies only to SIGCHLD; do not create a zombie when the process terminates                                |
| SA_SIGINFO               | Provide additional information to the signal handler (see the later section “Changing a Signal Action”)    |
| SA_ONSTACK               | Use an alternative stack for the signal handler (see the later section “Catching the Signal”)                |
| SA_RESTART               | Interrupted system calls are automatically restarted (see the later section “Reexecution of System Calls”) |
| SA_NODEFER, SA_NOMASK    | Do not mask the signal while executing the signal handler                                                    |
| SA_RESETHAND, SA_ONESHOT | Reset to default action after executing the signal handler                                                   |
> @todo 对于 sa_flags 缺乏深刻的理解

* ***The pending signal queues***
> sigqueue 挂到 sigpending 上，sigqueue 中间持有

#### 1.4 Operations on Signal Data Structures
> 介绍一堆辅助函数

## 2 Generating a Signal

When a signal is sent to a process, either from the kernel or from another process,
the kernel generates it by invoking one of the functions listed in Table 11-9.
All functions in Table 11-9 end up invoking the specific_send_sig_info() function
described in the next section.
When a signal is sent to a whole thread group, either from the kernel or from
another process, the kernel generates it by invoking one of the functions listed in
Table 11-10.

Table 11-9. Kernel functions that generate a signal for a process
| Name                 | Description                                                                                      |
|----------------------|--------------------------------------------------------------------------------------------------|
| send_sig()           | Sends a signal to a single process
| send_sig_info()      | Like send_sig(), with extended information in a siginfo_t structure
| force_sig()          | Sends a signal that cannot be explicitly ignored or blocked by the process
| force_sig_info()     | Like force_sig(), with extended information in a siginfo_t structure
| force_sig_specific() | Like force_sig(), but optimized for SIGSTOP and SIGKILL signals
| sys_tkill()          | System call handler of tkill() (see the later section “System Calls Related to Signal Handling”)
| sys_tgkill()         | System call handler of tgkill()

All functions in Table 11-9 end up invoking the `specific_send_sig_info()` function described in the next section.
> @todo 似乎并不是如此了 !

When a signal is sent to a whole thread group, either from the kernel or from
another process, the kernel generates it by invoking one of the functions listed in Table 11-10.

Table 11-10. Kernel functions that generate a signal for a thread group

| Name                  | Description                                                                                                                 |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------|
| send_group_sig_info() | Sends a signal to a single thread group identified by the process descriptor of one of its members                          |
| kill_pg()             | Sends a signal to all thread groups in a process group (see the section “Process Management” in Chapter 1) |
| kill_pg_info()        | Like kill_pg(), with extended information in a siginfo_t structure
| kill_proc()           | Sends a signal to a single thread group identified by the PID of one of its members
| kill_proc_info()      | Like kill_proc(), with extended information in a siginfo_t structure
| sys_kill()            | System call handler of kill() (see the later section “System Calls Related to Signal Handling”)
| sys_rt_sigqueueinfo() | System call handler of rt_sigqueueinfo()


#### 2.1 The specific_send_sig_info() Function
> skip
#### 2.2 The send_signal() Function
> skip
#### 2.3 The group_send_sig_info() Function
> skip

## 3 Delivering a Signal
We assume that the kernel noticed the arrival of a signal and invoked one of the
functions mentioned in the previous sections to prepare the process descriptor of the
process that is supposed to receive the signal.
But in case that process was not running on the CPU at that moment, the kernel deferred the task of delivering the signal.
We now turn to the activities that the kernel performs to ensure that pending signals of a process are handled.

As mentioned in the section “Returning from Interrupts and Exceptions” in
Chapter 4, the kernel checks the value of the `TIF_SIGPENDING` flag of the process
*before allowing the process to resume its execution in User Mode.* Thus, the kernel
checks for the existence of pending signals every time it finishes handling an interrupt or an exception.
> @todo 非常有意思! 什么叫做 before allowing the process to resume its execution in User Mode

To handle the nonblocked pending signals, the kernel invokes the `do_signal()` function, which receives two parameters:
1. regs

  The address of the stack area where the User Mode register contents of the current process are saved.

2. oldset

  The address of a variable where the function is supposed to save the bit mask
  array of blocked signals. It is NULL if there is no need to save the bit mask array.

/home/shen/Core/linux/arch/x86/kernel/signal.c
```c
/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
void do_signal(struct pt_regs *regs)
{
	struct ksignal ksig;

	if (get_signal(&ksig)) { // 感觉这才是主要的描述内容
		/* Whee! Actually deliver the signal.  */
		handle_signal(&ksig, regs);
		return;
	}

	/* Did we come from a system call? */
	if (syscall_get_nr(current, regs) >= 0) {
		/* Restart the system call - no handlers present */
		switch (syscall_get_error(current, regs)) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs->ax = regs->orig_ax;
			regs->ip -= 2;
			break;

		case -ERESTART_RESTARTBLOCK:
			regs->ax = get_nr_restart_syscall(regs);
			regs->ip -= 2;
			break;
		}
	}

	/*
	 * If there's no signal to deliver, we just put the saved sigmask
	 * back.
	 */
	restore_saved_sigmask();
}
```
> skip

#### 3.1 Executing the Default Action for the Signal
> skip 本 section 接下来的内容详细分析处理的过程，应该很有意思
> 如果没有 User Mode 就是没有信号机制了是不是 ? 应该是的，内核完全没有必要把事情搞得这么复杂!

## 4 System Calls Related to Signal Handling
> skip
