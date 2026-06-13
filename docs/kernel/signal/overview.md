# signal

## 内核基本实现
`task_struct` 中的信号字段可分为两类：

| 字段                         | 共享/私有                                | 作用                                                |
|------------------------------|------------------------------------------|-----------------------------------------------------|
| `struct signal_struct signal`                     | 指向 thread group 共享的 `signal_struct` | 包含 `shared_pending`、进程组信息、资源限制、统计等 |
| `struct sighand_struct __rcu sighand`                    | 指向共享的 `sighand_struct`              | 包含信号 handler 表 `action[]`、保护锁 `siglock`    |
| `sigset_t blocked`                    | 每个线程私有                             | 当前被屏蔽（mask）的信号集合                        |
| ` sigset_t real_blocked`               | 每个线程私有                             | 额外的临时屏蔽，通常用于 sigsuspend                 |
| `saved_sigmask`              | 每个线程私有                             | `set_restore_sigmask()` 保存的旧 mask               |
| ` struct sigpending pending` | 每个线程私有                             | 本线程独有的 pending 信号队列                       |

关键结论：

- 同一线程组内，所有线程的 `signal` 指向同一个 `signal_struct`，因此共享 `shared_pending`。
- 同一线程组内，所有线程的 `sighand` 通常指向同一个 `sighand_struct`，因此共享信号 handler。
- 但每个线程的 `blocked` 和 `pending` 是独立的。

```c
struct signal_struct {
    // ...
    struct sigpending shared_pending;   // 线程组共享 pending
    struct list_head thread_head;       // 线程组链表
    pid_t pgrp;                         // process group ID
    pid_t session;                      // session ID
    struct rlimit rlim[RLIM_NLIMITS];   // 资源限制
    // cputimer, stats, coredump 状态等
};
```

重点：

- `shared_pending`：发给整个线程组（`PIDTYPE_TGID`）的信号挂在这里。
- `pgrp` / `session`：用于 job control 和 `killpg`。
- 它没有自己的锁，因为共享 `signal_struct` 必然共享 `sighand_struct`，锁 `sighand->siglock` 就够了。

sighand_struct :

```c
struct sighand_struct {
    spinlock_t      siglock;            // 信号子系统核心自旋锁
    refcount_t      count;              // 引用计数
    wait_queue_head_t signalfd_wqh;     // signalfd 等待队列
    struct k_sigaction action[_NSIG];   // 每个信号的 handler 配置
};
```


## 注意，信号和 fpu 也是有关系的
arch/x86/kernel/fpu/signal.c

## 3. 标准信号与实时信号

### 3.1 标准信号（standard signals）

- 不排队：同一信号在阻塞期间产生多次，只保留一个 pending 实例；解除阻塞后只交付一次。
- 后续到达的同种标准信号不会覆盖已 pending 的 `siginfo_t`，进程收到的是第一个实例的信息。

### 3.2 实时信号（real-time signals）

- 可排队：产生多少次，最终就要处理多少次。
- 通过 `sigqueue(3)` 发送时可附带一个整型或指针值。
- 按到达顺序保证交付。
- 未处理的默认动作是终止接收进程。
- 注意：rt signal 的 "real-time" 体现在规范扩展（排队、附带数据、有序交付），并非实时性。

## TODO
- You don't need Kafka: Building a message queue with only two UNIX signals
	- https://news.ycombinator.com/item?id=45650178

- windows 等价的机制是什么?

别人其实已经仔细研究过 signal 对于多线程的问题的
- https://stackoverflow.com/questions/11679568/signal-handling-with-multiple-threads-in-linux
- https://unix.stackexchange.com/questions/225687/what-happens-to-a-multithreaded-linux-process-if-it-gets-a-signal
- [Should you be scared of Unix signals?](https://news.ycombinator.com/item?id=37899098)

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
