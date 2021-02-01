## ptrace

Usually, a large case structure that deals separately with each case (depending on the request
parameter) is employed for this purpose. I discuss only some important cases: `PTRACE_ATTACH` and
`PTRACE_DETACH`, `PTRACE_SYSCALL`, `PTRACE_CONT` as well as `PTRACE_PEEKDATA` and `PTRACE_POKEDATA`. 
The implementation of the remaining requests follows a similar pattern.

*All further tracing actions performed by the kernel are present in the signal handler code discussed in
Chapter 5*. When a signal is delivered, the kernel checks whether the `PT_TRACED` flag is set in the `ptrace`
field of `task_struct`. If it is, the state of the process is set to `TASK_STOPPED` (in `get_signal_to_deliver`
in `kernel/signal.c`) in order to interrupt execution. `notify_parent` with the `CHLD` signal is then used
to inform the tracer process. (The tracer process is woken up if it happens to be sleeping.) The tracer
process then performs the desired checks on the target process as specified by the remaining ptrace
options.

> 信号来自于 architecture 相关的代码: 比如 strace 的syscall 的，当设置 TIF 上的相关的flags，然后触发


1. 如何 install 一个 handler
2. 谁可以给 谁发送 消息 ? 没有限制的后果是什么 ? 
3. 一个正在执行的程序，靠检测什么东西来判断自己是否接收到了消息 ?
    1. 一个由于等待 signal 或者 各种蛇皮，sleep 之后，谁来唤醒我 ?
        1. 可能是 parent ?
        2. thread group leader ?
        3. init ?
        4. everyone ?

    2.
  
3. 接受到消息之后，处理消息的时机是什么 ?

4. 同时接收到多个消息，但是消息的处理只能一个个处理 ? 还是对于后面的进行忽视 ?
5. handler 能不能继承，当 fork 的时候 ?


