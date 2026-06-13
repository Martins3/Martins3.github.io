# syscall restart 机制解析与笔记勘误

## 一、原笔记 docs/kernel/signal/syscall-restart.md 的内容概述

该笔记主要记录了 Linux 中系统调用被信号中断后的重启机制，分为以下几个方面：

1. **signal handler 导致的中断**：哪些 syscall 在设置 `SA_RESTART` 后可以自动重启，哪些（如带超时参数的）永远返回 `EINTR`
2. **stop signal 导致的中断**：`SIGSTOP`/`SIGTSTP` 停止进程后，通过 `SIGCONT` 恢复时，`restart_syscall` 如何重新执行被中断的调用
3. **`restart_syscall` 系统调用的作用**：专门用于重启需要调整时间参数的系统调用（如 `nanosleep`），会扣除已经流逝的时间
4. **内核中的错误码**：`ERESTARTSYS`、`ERESTARTNOHAND`、`ERESTARTNOINTR`、`ERESTART_RESTARTBLOCK` 的含义和区别
5. **内核实现**：`arch/x86/kernel/signal.c` 中信号处理后重启 syscall 的核心逻辑

---

## 二、原笔记中写错或表述不清的地方

### 1. "无论如何失败的，都是带有时间参数的"

**不完全准确。** `man signal(7)` 中列出的不会被 `SA_RESTART` 重启的接口，除了 `sleep/nanosleep/poll/select` 等带时间的，还包括从终端读取的 `read`、某些 `ioctl` 等。不全是带时间参数的。

### 2. "ERESTARTNOHAND 和 ERESTART_RESTARTBLOCK 这两个应该是一样的"

**这是明确错误。** 两者行为不同：

- `ERESTARTNOHAND`：如果没有信号处理程序就直接重启（通过回退指令指针）；如果有信号处理程序，返回 `-EINTR`
- `ERESTART_RESTARTBLOCK`：**必须通过 `sys_restart_syscall` 系统调用来重启**，用于需要调整参数的场景（如 `nanosleep` 要扣除已睡眠时间）。内核中两者虽然都在信号处理时可能被转为 `EINTR`，但重启路径完全不同

### 3. 内核内联函数 `restart_syscall()` 与系统调用 `sys_restart_syscall` 的混淆

笔记第 66 节标题是 "restart_syscall 系统调用"，讨论的是用户态看到的 `sys_restart_syscall`；但引用的代码却是内核中的内联函数：

```c
static inline int restart_syscall(void)
{
    set_tsk_thread_flag(current, TIF_SIGPENDING);
    return -ERESTARTNOINTR;
}
```

这是**两个完全不同的东西**：

- `sys_restart_syscall`：系统调用，用于重启被 stop signal 中断的 timed syscall
- `restart_syscall()` 内联函数：内核驱动代码中用来触发系统调用重启的辅助函数

笔记把两者混为一谈了。

### 4. `pause` 返回 `ERESTARTNOHAND` 的描述

笔记说"pause 如果没有 signal handler 那么就重放，否则直接结束"，这个描述基本正确，但紧接着说"这两个应该是一样的"就把 `ERESTART_RESTARTBLOCK` 的语义也带偏了。

### 5. 小 typo

"当看到nanosleep({5, 0},  立即ctrl-z" — 这句话缺了后半截，应该是"当看到 `nanosleep({5, 0}, ...)` 输出时立即 ctrl-z"。

---

## 三、为什么需要 syscall restart

当一个进程正在执行系统调用时，如果收到了信号（无论是 `SIGALRM`、`SIGUSR1` 等终止/通知信号，还是 `SIGSTOP`/`SIGTSTP` 等停止信号），内核必须先暂停当前系统调用，切换到用户态去执行信号处理函数。处理完信号后，内核面临一个问题：**刚才被中断的系统调用该怎么办？**

直接返回 `-EINTR` 给用户态固然是一种选择，但这会把负担转嫁给每个应用程序 —— 它们必须在每次系统调用后检查 `errno == EINTR` 并手动重试。而且很多系统调用本身就是幂等的（比如 `read`/`write` 从当前文件偏移继续），自动重启既安全又透明。

因此 Linux 设计了 **syscall restart** 机制，让内核在信号处理完成后自动重新执行被中断的系统调用，用户态对此无感知。但这又带来一个更复杂的问题：**对于 `nanosleep`、`poll`、`select` 等带时间参数的系统调用，简单回退指令指针重新执行，会导致已经流逝的时间被重复计算**（比如睡了 3 秒被中断，重新睡 5 秒就会变成总共睡 8 秒）。这就是 `restart_syscall` 系统调用存在的意义。

---

## 四、用户态代码如何处理

### 1. 大部分情况下：什么都不用做

如果使用 `sigaction` 并设置了 `SA_RESTART` 标志，内核会自动重启绝大多数可被中断的系统调用：

```c
struct sigaction sa;
sa.sa_flags = SA_RESTART;
sigaction(SIGUSR1, &sa, NULL);
```

### 2. 必须手动处理 EINTR 的场景

即使用户设置了 `SA_RESTART`，以下系统调用被信号中断后也**永远不会自动重启**，只会返回 `-EINTR`：

- 所有 sleep 类：`sleep`, `nanosleep`, `clock_nanosleep`
- 所有多路复用类：`select`, `pselect`, `poll`, `ppoll`, `epoll_wait`, `epoll_pwait`
- 从终端读取的 `read`
- `pause`, `wait`, `ioctl` 等

对于这些调用，用户态必须显式重试：

```c
while ((n = read(fd, buf, count)) < 0 && errno == EINTR) {
    /* retry */
}
```

glibc 提供了 `TEMP_FAILURE_RETRY` 宏来做这件事。

### 3. 被 stop signal 中断的场景

如果进程被 `Ctrl-Z`（`SIGTSTP`）停止，之后再 `fg` 恢复（`SIGCONT`），用户态完全不需要处理。内核会通过 `restart_syscall` 自动恢复被中断的定时系统调用。

---

## 五、内核代码如何处理

内核的实现分为两个层面：**系统调用返回时的标记**，以及**信号处理后的重启决策**。

### 1. 系统调用返回时的标记

内核中定义了四种内部错误码，系统调用被中断时返回其中之一：

| 错误码 | 含义 |
|--------|------|
| `-ERESTARTSYS` | 如果信号处理程序设置了 `SA_RESTART` 则重启，否则返回 `-EINTR` |
| `-ERESTARTNOINTR` | **总是重启**，不管有没有信号处理程序 |
| `-ERESTARTNOHAND` | 如果没有信号处理程序则重启，有则返回 `-EINTR` |
| `-ERESTART_RESTARTBLOCK` | 通过 `sys_restart_syscall` 系统调用来重启，主要用于需要调整时间参数的场景 |

例如 `pause()` 的实现：

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

### 2. 信号处理后的重启决策（架构相关代码）

以 x86 为例，在 `arch/x86/kernel/signal.c` 的 `arch_do_signal_or_restart()` 中，内核检查系统调用的返回值：

```c
if (syscall_get_nr(current, regs) != -1) {
    switch (syscall_get_error(current, regs)) {
    case -ERESTART_RESTARTBLOCK:
    case -ERESTARTNOHAND:
        regs->ax = -EINTR;  /* 有信号处理程序时转为 EINTR */
        break;

    case -ERESTARTSYS:
        if (!(ksig->ka.sa.sa_flags & SA_RESTART)) {
            regs->ax = -EINTR;
            break;
        }
        fallthrough;
    case -ERESTARTNOINTR:
        regs->ax = regs->orig_ax;  /* 恢复系统调用号 */
        regs->ip -= 2;             /* 回退指令指针到 syscall 指令 */
        break;
    }
}
```

**重启的本质**：将指令指针（`rip`/`ip`）回退 2 个字节（x86 上 `syscall` 指令的长度），并恢复原始系统调用号到 `rax`/`ax`。这样从信号处理返回用户态后，CPU 会重新执行 `syscall` 指令，再次陷入内核执行相同的系统调用。

### 3. stop signal + restart_syscall 的特殊路径

当进程因 `SIGSTOP` 被停止、又因 `SIGCONT` 恢复时，上述逻辑无法简单适用 —— 因为被 stop 时可能已经流逝了一部分时间。内核此时会：

1. 被中断的系统调用（如 `clock_nanosleep`）返回 `-ERESTART_RESTARTBLOCK`
2. 内核在进程恢复运行时，安排执行 `sys_restart_syscall`
3. `sys_restart_syscall` 根据内核记录的 `restart_block` 信息，调用对应的 restart 函数（如 `hrtimer_nanosleep_restart`），并**调整时间参数扣除已流逝的时间**

这就是为什么 `strace` 中你能看到：

```txt
--- SIGCONT ---
restart_syscall(<... resuming interrupted clock_nanosleep ...>)
```

### 4. 驱动/内核子系统中的主动触发

内核中还有一个内联函数 `restart_syscall()`（注意这不是系统调用）：

```c
static inline int restart_syscall(void)
{
    set_tsk_thread_flag(current, TIF_SIGPENDING);
    return -ERESTARTNOINTR;
}
```

驱动代码在长时间操作（如 `wait_event_interruptible`）中，如果需要让当前系统调用被信号中断后自动重启，可以返回这个函数的结果。

---

## 六、核心区别总结

- **`ERESTARTNOHAND` vs `ERESTART_RESTARTBLOCK`**：前者是"原地重启"（回退 IP），后者是"换道重启"（通过 `sys_restart_syscall` 重新进入内核并调整参数）。原笔记中把两者说成"一样"是**错误**的。
- **用户态视角**：要么依赖 `SA_RESTART`（透明），要么手动处理 `EINTR`。
- **内核视角**：通过四种 `ERESTART*` 错误码 + 架构相关的寄存器操作，决定是重启、转 `EINTR`、还是注入 `restart_syscall`。

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
