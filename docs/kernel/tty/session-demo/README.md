# 在本机观察 session、进程组和控制终端

## 本机快照

[完整快照](snapshot-2026-10-03.txt)采集于 2026-10-03 23:11:18 +08:00：

- 250 个非零 SID，其中 14 个关联控制终端，236 个没有控制终端。
- SID 0 下的 445 个内核任务单独列出，没有算入上述 session 数量。
- 这是 `ps`
  能看到的进程的采样，包含采集命令自身；进程会随时退出或创建，数量不是固定值，采集也不是原子的。
- 这里统计内核的 SID，与 `loginctl` 的登录会话、`uptime` 的 users 计数，以及
  tmux 自己的 session 概念不同。

14 个有控制终端的 session：

| SID     | TTY    | 前台 PGID | 前台程序                         |
| ------- | ------ | --------- | -------------------------------- |
| 18011   | tty2   | 18011     | gdm-wayland-ses、gnome-session-i |
| 270330  | pts/0  | 270330    | zsh                              |
| 301944  | pts/8  | 302102    | npm、node                        |
| 750052  | pts/2  | 750307    | tmux client                      |
| 750347  | pts/10 | 750347    | zsh                              |
| 753490  | pts/12 | 753885    | node、codex                      |
| 755648  | pts/13 | 755799    | node、codex                      |
| 3220900 | pts/1  | 4005532   | nvim                             |
| 3678140 | pts/4  | 3678900   | python3.13                       |
| 3818034 | pts/11 | 3818290   | node、codex                      |
| 3997992 | pts/3  | 4002624   | nvim                             |
| 4127488 | pts/5  | 4127867   | node、codex                      |
| 4133688 | pts/7  | 4133688   | zsh                              |
| 4134264 | pts/9  | 4134699   | node、codex                      |

最直接的例子是正在编辑文件的 nvim：

```text
SID 3220900，控制终端 /dev/pts/1
|
+-- PGID 3220900，后台
|   +-- zsh，PID 3220900
|
+-- PGID 4005532，前台（终端的 TPGID = 4005532）
    +-- nvim，PID 4005532，PPID 3220900
```

此时 shell 仍然存在，等待前台作业；终端由 nvim 所在进程组使用。退出或暂停 nvim
后，shell 会重新取得前台。

另一个 session 展示了一个前台作业里有多个进程：

```text
SID 753490，控制终端 /dev/pts/12
+-- PGID 753490，后台
|   +-- zsh，PID 753490
+-- PGID 753885，前台
    +-- node，PID 753885
    +-- codex，PID 753892，PPID 753885
```

进程父子关系可以跨 session。本机查到：

```text
tmux server，PID/SID/PGID = 485758，无控制终端
  创建子进程 -> zsh，PID/SID = 3220900，控制终端 pts/1
  创建子进程 -> zsh，PID/SID = 753490，控制终端 pts/12
  创建子进程 -> 其他 pane 的 shell，各自建立 session

ghostty，PID/SID = 270263，无控制终端
  创建子进程 -> zsh，PID/SID = 270330，控制终端 pts/0
```

这里箭头表示 PPID 关系，不表示属于同一个 session。终端模拟器可以持有 PTY
master，而自己没有控制终端。

当前 Codex 后端也是无控制终端的例子：SID 953507，leader 是 PID 953507 的
codex；node_repl 等子进程有不同 PGID，但仍保留 SID 953507。另一个观察是，前台
nvim（PID 4005532）的子进程 nvim（PID 4005533）已经有自己的 SID 4005533，TTY 为
`?`。

重新采集：

```bash
cd /home/martins3/data/vn/docs/kernel/tty/session-demo
./snapshot.py
```

脚本先列出所有非零 SID 的进程数、进程组数、TTY 和 leader，再按 SID → PGID → PID
展开有控制终端的进程。`?` 表示没有控制终端；`TPGID` 是终端的前台进程组
ID；`STAT` 的 `s` 表示 session leader，`+` 表示属于终端前台进程组。leader
已退出时，session 仍可能有成员。

Python 脚本只使用标准库和系统的 `ps` 命令。`Process` 给各列命名，
`read_processes()` 采集并解析记录，`print_summary()` 汇总 session，
`print_terminal_tree()` 展开进程组和进程；不需要临时文件。

## 实验一：fork、setsid、exec 到底改变什么

在自己的交互式终端中执行：

```bash
make
./session.out
```

阅读 [session.c](session.c) 的 `main()` 和 `show()`。程序不读键盘，会自动退出。

| 阶段               | SID / PGID           | 控制终端       | fd 0       |
| ------------------ | -------------------- | -------------- | ---------- |
| parent             | 从启动环境继承       | 原来的控制终端 | 原来的输入 |
| child after fork   | 和 parent 相同       | 继承           | 继承       |
| child after setsid | 两者都变成 child PID | 无             | 保留       |
| after exec         | 和 exec 前相同       | 无             | 保留       |
| parent unchanged   | 原值                 | 原值           | 原值       |

本机在一个临时 PTY 中实际运行的关键输出：

```text
parent               PID=803244 PPID=953507 SID=803244 PGID=803244
  fd 0 terminal: /dev/pts/14
  /dev/tty: available; foreground PGID=803244
child after fork     PID=803306 PPID=803244 SID=803244 PGID=803244
  fd 0 terminal: /dev/pts/14
  /dev/tty: available; foreground PGID=803244
child after setsid   PID=803306 PPID=803244 SID=803306 PGID=803306
  fd 0 terminal: /dev/pts/14
  /dev/tty: unavailable (No such device or address)
after exec           PID=803306 PPID=803244 SID=803306 PGID=803306
  fd 0 terminal: /dev/pts/14
  /dev/tty: unavailable (No such device or address)
```

重点：`setsid()` 后，fd 0 依然指向终端，但 `/dev/tty`
已打不开。终端文件描述符和控制终端关联是两回事。程序继续输出，也说明 `setsid()`
没有关闭 stdout。

再把 stdin 重定向：

```bash
./session.out < /dev/null
```

在交互式终端中，parent 的 `fd 0 terminal` 会变成 `not a terminal`，但 `/dev/tty`
仍然可用。这展示相反方向：stdin 不是终端，进程仍可以有控制终端。

如果从本来没有控制终端的环境启动，第一个 parent 的 `/dev/tty`
就不可用；此时依然能观察 SID/PGID 的变化。demo 先 fork 再 setsid，是为了确保调用
setsid 的进程不是进程组 leader。

## 实验二：后台读终端、fg、Ctrl+C

```bash
./job-control.out
```

这个实验全自动，在没有交互式终端的环境中也能运行。它新建私有
PTY，正常结束会回收子进程、关闭 PTY；十秒超时用于防止内部流程卡住。

```text
外层进程：持有 PTY master，扮演终端模拟器
    |
    +-- controller：forkpty 创建的 session leader，扮演 shell
        SID = controller PID，控制终端 = 新建的 PTY slave
        |
        +-- worker：同一个 SID，单独的 PGID，扮演作业
```

运行时依次验证：

1. controller 在前台，worker 在后台。worker 调用 `read(tty)`，内核发出
   `SIGTTIN`，将它暂停。controller 用 `waitpid(..., WUNTRACED)` 验证停止原因。
2. controller 调用 `tcsetpgrp()`，把 worker 的进程组设为前台，再发
   `SIGCONT`。外层向 PTY master 写入一行文字，worker 成功读到。
3. 外层向 master 写入字节 `0x03`，即 Ctrl+C。终端驱动产生 `SIGINT`，前台 worker
   终止，后台 controller 继续运行。
4. controller 回收 worker，再调用 `tcsetpgrp()` 把自己恢复为前台。

这里没有用 `kill(SIGINT)` 冒充 Ctrl+C，信号确实由终端驱动生成。`ISIG` 和 `VINTR`
在代码中明确设置。controller 忽略
`SIGTTOU`，因为它恢复前台时自身正处于后台；worker 使用默认信号处理。

阅读 [job-control.c](job-control.c)：`session_controller()`
管理前后台，`worker()` 尝试读终端，`main()` 从 master 端注入输入。初始 `SIGSTOP`
只用于同步，真正要观察的后台读停止原因是后面的 `SIGTTIN`，没有靠 sleep 猜时序。

实际运行结果包含：

```text
PASS: kernel stopped worker with SIGTTIN.
worker read: hello from PTY master
PTY master: writing byte 0x03 (Ctrl+C).
PASS: terminal delivered SIGINT to foreground worker; controller survived.
PASS: job control complete; worker reaped, private PTY will close.
```

## 用自己的 shell 手动感受一次

在启用了作业控制的交互式 bash/zsh 中，逐条执行：

```bash
cat &
jobs -l
fg
```

`cat &` 启动的后台 cat 会在读终端时收到 `SIGTTIN`，shell 通常显示
`Stopped (tty input)` 或类似信息；消息出现时间可能略晚。`fg`
恢复它到前台后，输入一行文字并回车，cat 会打印出来。

接着按 Ctrl+Z，cat 因 `SIGTSTP` 暂停，shell 重新显示提示符。再执行 `fg`，最后按
Ctrl+C，结束 cat，回到 shell。

在这个过程中，cat 的 SID、PGID 都没有改变；变化的是终端记录的前台 PGID，以及 cat
的运行/暂停状态。交互式 cat 可能显示两份文字，一份来自终端回显，一份来自 cat
输出。

## 已验证

- `make check`：两个 C 程序以 `-Wall -Wextra -Werror` 编译并运行，同时运行快照脚本，
  检查 Ruff lint 和格式。
- `session.out` 分别在无控制终端、带控制终端、带控制终端但 stdin
  重定向的环境中运行过。
- `job-control.out` 实际验证 `SIGTTIN`、前台读取和 Ctrl+C 产生的 `SIGINT`。

接口参考：[setsid(2)](https://man7.org/linux/man-pages/man2/setsid.2.html)、[tcsetpgrp(3)](https://man7.org/linux/man-pages/man3/tcsetpgrp.3.html)、[tty(4)](https://man7.org/linux/man-pages/man4/tty.4.html)。

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
