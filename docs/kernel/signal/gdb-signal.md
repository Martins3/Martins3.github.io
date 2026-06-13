## gdb 和 signal fd 的诡异故事
<!-- 8baa3556-ec1b-48f6-a39e-08f744cfdc02 -->

使用 docs/kernel/signal/code/ 下两个小 demo :
- gdb-sigint.c :  注册了 signal handler ，但是不会执行，程序不会接受到任何的消息
- gdb-sigfd.c : gdb 中 ctrl-c ，会监听到信号
- gdb-stop.c : 发现，如果 gdb 中 ctrl-c ，然后恢复，epoll 会返回为 EINTR

所以，我们有两个问题:
1. 为什么 signalfd 可以接受到信号?
2. 为什么 STOP 可以感知到，但是 SIGINT 不会感知到?

## 结论 by codex
(2026-05-22 ，这个东西我不是完全确认，这个至少需要和内核中的实现对照一下)

这里不是一条“信号必然进入用户态 handler”的路径，而是三条不同路径混在一起了:

1. 普通未阻塞的 `SIGINT` 在 GDB 下会先进入 ptrace 的 signal-delivery-stop，GDB 默认停住程序，但不把 `SIGINT` 注入给 tracee，所以用户态 handler 不执行。
2. `signalfd` demo 先把 `SIGINT` block 住，信号不会进入普通 delivery 路径，而是留在 pending 集合中，之后由 `signalfd` 消费。
3. `SIGSTOP` 不能被 catch、block、ignore。它不会调用用户态 handler，但会让进程停止，并可能打断正在阻塞的 syscall，例如 `epoll_wait`，所以恢复后可以看到 `EINTR`。

一句话概括:

> `SIGINT` 被 GDB 当成调试控制信号消费了；`signalfd` 通过 block 改变了信号路径；`SIGSTOP` 不走用户 handler，但会改变调度和阻塞 syscall 状态。

### gdb-sigint: SIGINT handler 为什么不执行

`gdb-sigint.c` 注册了 `SIGINT` handler:

```c
sa.sa_handler = handler;
sa.sa_flags = SA_RESTART;
sigaction(SIGINT, &sa, NULL);
```

如果程序直接在 shell 中运行，按 `Ctrl-C` 时终端驱动会给前台进程组发送 `SIGINT`，进程收到后会进入普通 signal delivery，最后执行用户态 handler。

但是程序在 GDB 下运行时，`SIGINT` 的路径变了:

```txt
Ctrl-C
  -> SIGINT generated
  -> tracee 进入 signal-delivery-stop
  -> GDB 被 waitpid 唤醒
  -> GDB 根据自己的 signal policy 决定是否注入 SIGINT
```

GDB 默认对 `SIGINT` 的处理通常是:

```txt
stop  = yes
print = yes
pass  = no
```

也就是 GDB 会停住程序并打印提示，但不会把 `SIGINT` 继续传给被调试进程。因此:

- handler 不会执行
- 如果没有注册 handler，程序也不会因为默认 `SIGINT` 动作退出
- `SA_RESTART` 在这里不是关键，因为根本没有执行用户态 handler

可以在 GDB 中确认:

```gdb
info signals SIGINT
```

如果显式允许 GDB 传递 `SIGINT`:

```gdb
handle SIGINT pass
```

或者手动注入:

```gdb
signal SIGINT
```

这时 `gdb-sigint.c` 的 handler 才会执行。

### gdb-sigfd: signalfd 为什么可以收到 SIGINT

`gdb-sigfd.c` 的关键不是 `signalfd` 本身，而是它先把 `SIGINT` block 了:

```c
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
pthread_sigmask(SIG_BLOCK, &mask, NULL);

sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
```

被 block 的信号不会立刻 delivery 给用户态 handler，而是先进入 pending signal 集合:

```txt
SIGINT generated
  -> SIGINT 当前被 block
  -> 不进入普通 signal delivery
  -> 不调用 handler
  -> 标记为 pending
  -> signalfd 可读
  -> read(signalfd) 取出 signalfd_siginfo
```

这也解释了为什么它看起来和 `gdb-sigint.c` 行为不同。`signalfd` 不是“抢赢了 GDB”，而是 `pthread_sigmask(SIG_BLOCK)` 让 `SIGINT` 暂时不走普通 delivery 路径。信号还在进程的 pending 集合里，`signalfd` 只是提供了一个 fd 形式的消费接口。

ptrace 文档中也有对应语义: tracee 被 tracing 时，信号 delivery 前会先停给 tracer；但是如果信号当前被 block，signal-delivery-stop 不会发生，直到信号被 unblock。`SIGSTOP` 是例外，因为它不能被 block。

### gdb-stop: STOP 为什么可以通过 EINTR 被感知

`gdb-stop.c` 阻塞在:

```c
epoll_wait(epfd, events, MAX_EVENTS, -1);
```

`SIGSTOP` 不能被 catch、block、ignore，所以程序没有机会注册 handler 来处理它。它的效果是让整个线程组进入 stopped 状态。恢复时，Linux 上某些阻塞 syscall 会以 `-1/EINTR` 返回，即使没有任何用户态 handler 被调用。

路径可以理解为:

```txt
epoll_wait 正在 TASK_INTERRUPTIBLE 睡眠
  -> SIGSTOP / ptrace attach stop / GDB interrupt 让 tracee stop
  -> 内核唤醒可中断睡眠
  -> syscall 被提前结束
  -> tracee 恢复执行
  -> epoll_wait 返回 -1, errno = EINTR
```

这不是程序“捕获了 SIGSTOP”。程序能观察到的只是 stop/resume 对阻塞 syscall 的副作用。

`man signal(7)` 对 Linux 的这个行为有专门描述: 在没有 signal handler 的情况下，进程被 stop signal 停住然后由 `SIGCONT` 恢复后，某些阻塞接口也可能失败并返回 `EINTR`，其中包括 `epoll_wait` / `epoll_pwait`。

`ptrace` 还有一个相关副作用: `PTRACE_ATTACH` 会给 tracee 发送 `SIGSTOP`。调试器通常会 suppress 这个 `SIGSTOP`，避免把一个假的 stop 信号真正注入给程序。但即使 signal injection 被 suppress，被中断的 syscall 仍然可能出现一次 stray `EINTR`。`ptrace(2)` 明确把 `epoll_wait` 列为这类历史问题的典型例子。

所以这类代码不能写成:

```c
int nr = epoll_wait(epfd, events, MAX_EVENTS, -1);
assert(nr == 1);
```

而应该显式处理 `EINTR`:

```c
int nr;

do {
	nr = epoll_wait(epfd, events, MAX_EVENTS, -1);
} while (nr < 0 && errno == EINTR);
```

### 对比

| 场景 | 信号是否 block | GDB/ptrace 是否先看到 | 用户 handler 是否执行 | 用户程序能看到什么 |
| --- | --- | --- | --- | --- |
| `gdb-sigint.c` | 否 | 是 | 默认不执行，GDB suppress 了 | GDB 停住程序 |
| `gdb-sigfd.c` | 是 | 通常不会立刻 signal-delivery-stop | 不执行 | `signalfd` 可读 |
| `gdb-stop.c` | `SIGSTOP` 不能 block | GDB/ptrace stop 或真实 stop 都会影响任务状态 | 不可能执行 | `epoll_wait` 可能返回 `EINTR` |

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
