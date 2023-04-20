# wait syscall
wait 和 exit 都在 kernel/exit.c 下

## 阅读 man
> The wait() system call suspends execution of the calling thread until one of its children terminates.  The call wait(&wstatus)  is
> equivalent to:
>
> > waitpid(-1, &wstatus, 0);
>
> waitid()
>
> > The waitid() system call (available since Linux 2.6.9) provides more precise control over which child state changes to wait for.
>
> The wait3() and wait4() system calls are similar to waitpid(2), but additionally re‐
> turn  resource  usage  information  about  the  child in the structure pointed to by
> rusage.
>
> Other than the use of the rusage argument, the following wait3() call:
>
> > wait3(wstatus, options, rusage);
>
> is equivalent to:
>
> > waitpid(-1, wstatus, options);
>
> Similarly, the following wait4() call:
>
> > wait4(pid, wstatus, options, rusage);
>
> is equivalent to:
>
> > waitpid(pid, wstatus, options);

和 fork 系列类似，三个是逐级递增的，wait / waitpid / wait
```c
/* Wait for a child matching PID to die.
   If PID is greater than 0, match any process whose process ID is PID.
   If PID is (pid_t) -1, match any process.
   If PID is (pid_t) 0, match any process with the
   same process group as the current process.
   If PID is less than -1, match any process whose
   process group is the absolute value of PID.
   If the WNOHANG bit is set in OPTIONS, and that child
   is not already dead, return (pid_t) 0.  If successful,
   return PID and store the dead child's status in STAT_LOC.
   Return (pid_t) -1 for errors.  If the WUNTRACED bit is
   set in OPTIONS, return status for stopped children; otherwise don't.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern __pid_t waitpid (__pid_t __pid, int *__stat_loc, int __options);
```
- [x] If  a  child  has  **already** changed state, then these calls return immediately.
  - 只要发生过状态转变, 而不是在调用函数之后进行这些转变

- [x] WNOWAIT     Leave the child in a waitable state; a later wait call can be used to again retrieve the child status information.
  - [x]  WNOWAIT 的作用应该是, 之后表示还需要获取信息的，不要清空信息，否则默认清空(tlpi 找到佐证)

## 试图读读代码

- waitid
  - kernel_waitid : 根据 idtype 获取正确的 struct pid, 组装 struct wait_opts
    - do_wait : 将当前 current 加入 wait_queue, 等待 child 调用 `__wake_up_parent`
      - `add_wait_queue(&current->signal->wait_chldexit, &wo->child_wait);`
        - [x] 难道说，`current->signal->wait_chldexit` 上可以放置多个 `wo->child_wait`（因为不会，只是为了利用 wait queue 的这个机制)
      - do_wait_thread : 首先检查一次有没有符合要求的，如果没有，那么就利用 wait_queue 睡眠，直到 child 通知自己
        - wait_consider_task
          - eligible_child : 检查这个 child 是不是应该被 wait 的
          - wait_task_zombie : 用于 WNOHANG 和 EXITED
          - wait_task_stopped
          - wait_task_continued

- [ ] 比如一个 child exit 了，怎么知道那些 parent 都在等待, 让后去 wake up 他们?

- do_notify_parent : Let a parent know about the death of a child. For a stopped/continued status change, use do_notify_parent_cldstop instead.

- do_notify_parent_cldstop


```c
void __wake_up_parent(struct task_struct *p, struct task_struct *parent)
{
	__wake_up_sync_key(&parent->signal->wait_chldexit,
			   TASK_INTERRUPTIBLE, p);
}
```
