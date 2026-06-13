# sys.c
1. pid
2. rlimit
3. prctl

## KeyNote
- [](https://medium.com/hungys-blog/linux-kernel-process-99629d91423c)

The field parent in task_struct usually matches the process descriptor pointed by real_parent.
- real_parent: Points to the process descriptor of the process that created P or to the descriptor of process 1 (init) if the parent process no longer exists.
- parent: Points to the current parent of P (this is the process that must be signaled when the child process terminates, i.e. SIGCHLD). It may occasionally differ from real_parent in some cases, such as when another process issues a ptrace() system call requesting that it be allowed to monitor P.

## Question
pid.c 中间的疑惑:
1. 为什么将 pid->tasks[PIDTYPE_PID] 总是单独说明 ?
    1. 既然总是被 struct task->thread_pid 替代，为什么不直接将其删除掉 ?
2. 如果 attach 真的是将 task 加入到 pid hlist 的唯一方法，change_pid 的含义到底是什么 ?
    1. 比如 clone ，难道不是自动加入到 thread group 中间吗 ?
3. pid_task 的实现 :
    1. 由于初始化的时候，第一个才是该 pid 的代表，所以各种，find_task 之类的函数的实现非常科学
5. 一个 task 一共可以持有 5 个 pid 指针 : thread_pid 和 signal
  1. 同时，pid 中间可以持有四种 task

6. 到底为什么 task 需要共享 pid ?
    1. tgid pgid sid 的存在, 之所以存在 PIDTYPE_PID ，是因为 thread_pid 需要使用
    2. @todo 找到加入 thread group 的方法

## prctl : process control 各种蛇皮业务的处理者
A process can set and reset the `keep_capabilities` flag by means of the Linux-specific prctl() system call.

- [](https://unix.stackexchange.com/questions/250153/what-is-a-subreaper-process)

- [](https://stackoverflow.com/questions/284325/how-to-make-child-process-die-after-parent-exits)
