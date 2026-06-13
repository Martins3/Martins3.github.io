## jobctl
<!-- bd6518c9-f086-4c44-bc1f-a28424f3010c -->

我在分析 signal 的时候，观察到这些字段:

task->jobctl

```c
#define JOBCTL_STOP_DEQUEUED	(1UL << JOBCTL_STOP_DEQUEUED_BIT)
#define JOBCTL_STOP_PENDING	(1UL << JOBCTL_STOP_PENDING_BIT)
#define JOBCTL_STOP_CONSUME	(1UL << JOBCTL_STOP_CONSUME_BIT)
#define JOBCTL_TRAP_STOP	(1UL << JOBCTL_TRAP_STOP_BIT)
#define JOBCTL_TRAP_NOTIFY	(1UL << JOBCTL_TRAP_NOTIFY_BIT)
#define JOBCTL_TRAPPING		(1UL << JOBCTL_TRAPPING_BIT)
#define JOBCTL_LISTENING	(1UL << JOBCTL_LISTENING_BIT)
#define JOBCTL_TRAP_FREEZE	(1UL << JOBCTL_TRAP_FREEZE_BIT)
#define JOBCTL_PTRACE_FROZEN	(1UL << JOBCTL_PTRACE_FROZEN_BIT)
```
结合 codex 的分析，简单来说就是，当 thread 被 pstrace 的时候，有一系列的状态变化，
这些状态就是记录在 task->jobctl 中的

## codex 的分析
第一层是用户语义：shell 需要管理“作业”。

例如：

sleep 100 | cat

这不是一个进程，而是一组进程。用户按 Ctrl-Z，shell 期望整个 pipeline 停住；执行 fg，整个
pipeline 回到前台；执行 bg，整个 pipeline 继续后台跑。这个语义靠单个 task 的普通 signal
pending 状态表达不了，所以 Unix 抽象出了：

- session
- process group
- foreground process group
- controlling terminal
- stop/continue signal
- parent 通过 waitpid() 观察 stopped/continued 状态

这就是 POSIX job control 解决的问题：shell/终端以“进程组/作业”为单位管理前台、后台、停止、
继续。

第二层是 Linux 内核实现：task->jobctl 解决“停止/陷入/继续”这类状态机不好塞进普通信号或调度状态的问题。

普通信号机制只回答“某个 signal 是否 pending”。调度状态只回答“这个 task 当前能不能运行”。但
job control 需要更多信息，比如：

- 这个 task 是否应该参与一次 group stop：JOBCTL_STOP_PENDING
- stop signal 是哪个：JOBCTL_STOP_SIGMASK
- 这个 stop 是否已经被取出处理：JOBCTL_STOP_DEQUEUED
- group stop 计数是否要消费：JOBCTL_STOP_CONSUME
- ptrace 是否要求进入 trap stop：JOBCTL_TRAP_STOP
- ptracer 是否正在等待 traced 状态：JOBCTL_TRAPPING
- task 是真的 group-stopped，还是 ptrace-stopped：JOBCTL_STOPPED / JOBCTL_TRACED

所以 jobctl 不是给用户态直接用的接口，而是内核用来维护这些复杂语义的状态字。

核心原因是：“停止一个任务”不是简单地把 task state 设成 stopped。它还涉及：

- 一个线程组/进程组里的所有线程是否都停住了
- 是否只通知父进程一次
- SIGCONT 和 stop signal 竞争时谁赢
- SIGSTOP、SIGTSTP、SIGTTIN、SIGTTOU 规则不同
- ptrace stop 和 job-control stop 不能混淆
- cgroup freezer 也想让任务停下来，但不能伪装成用户发了 SIGSTOP

因此可以这样理解：

job control 是用户可见的作业管理语义；
task->jobctl 是内核为了正确实现这种语义，以及复用类似“让任务进入受控停止点”的机制，而维护
的一组内部状态位。

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
