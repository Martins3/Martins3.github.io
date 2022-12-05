# strace 基本使用

strace 可以监控的内容:
- 系统调用
- 信号
- process state

过滤掉特定的 syscall
```sh
#!/usr/bin/env bash
strace -e 'trace=!read,writev' tcpdump -A -s0 port 80
```

```sh
strace -A -o a.log ls
strace -p 2194,2195  # 同时跟踪多个进程
strace -p $(pidof dead_loop) # 利用 pidof 或者 pgrep
```

To trace child processes, -f option need to be specified [^1]
- `-b` syscall option can be used to instruct strace to detach process when specified syscall is executed. However, currently only execve is supported.
- `-D` option is used to detach strace and tracee processes. 使用此参数，tracee 不会作为 strace 的 children
```plain
strace -c ls # stat
strace -w -c ls # wall time
strace -c -S calls ls # sort by calls
```

-e expression 是一个强大的工具，可以控制输出的详细程度，约束到底跟踪哪一个东西，操控信号，注入 syscall 返回值

strace 执行的时候，可以修改执行的 trace 以及 修改指令执行的位置。

-I interruptible 用于屏蔽信号

strace -p <PID> attach 到一个进程上[^2]

[^1]: [strace little book](https://github.com/NanXiao/strace-little-book)

[^2]: https://stackoverflow.com/questions/7482076/how-does-strace-connect-to-an-already-running-process
