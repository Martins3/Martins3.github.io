## pty driver
<!-- 20f3996d-a2d2-48cb-8f65-f4266109cffe -->

(chatgpt 大师，你真的太强了)

这是一个很好的问题，**PTY（pseudo terminal）用户态 driver** 本质上就是一个：

> 在用户态 **模拟一个 tty 设备的一端或两端**，并负责 **数据转发 + 线路规约 + 控制信号处理** 的程序。

我分 4 个层次讲：
- PTY 的内核模型
- 一个最小可用的“用户态 PTY driver”框架
- 真正工程级要补齐哪些能力
- 业界最典型的用户态 PTY 项目

## 一、PTY 在内核里的真实结构（你在用户态“驱动”的是什么）

在 Linux 内核中，PTY 是 **成对出现的字符设备**：

```
master  <======>  slave
  |                   |
  |               /dev/pts/N
  |
  用户态程序控制
```

| 角色         | 作用                |
| ---------- | ----------------- |
| **master** | 用户程序控制端（你“驱动”的入口） |
| **slave**  | 对进程来说就是“真终端”      |

使用方式本质是：

```text
你在用户态操作 master fd
内核 TTY 子系统负责：
- 行规（ICANON）
- 回显（ECHO）
- 信号（Ctrl-C -> SIGINT）
- termios
```


## 三、工程级 PTY Driver 需要补齐的“驱动能力”

真实项目中一个 PTY driver 一般需要处理：

### I/O 多路复用（必须）

```c
epoll(master, stdin, socket, ...)
```

否则无法做：

* 远程终端
* 多路复用
* 终端转发

### termios 透明转发（核心）

```c
struct termios t;
tcgetattr(slave_fd, &t);
tcsetattr(slave_fd, TCSANOW, &t);
```

必须正确处理：

| 能力         | 说明       |
| ---------- | -------- |
| ICANON     | 行缓存      |
| ECHO       | 回显       |
| ISIG       | Ctrl+C/Z |
| VTIME/VMIN | 非阻塞读取    |

### 控制消息与 ioctl 透传

必须支持：

```c
TIOCGWINSZ / TIOCSWINSZ   // 窗口尺寸
TIOCSTI                // 注入字符
TIOCGPGRP              // 前台进程组
```

否则：

* vim
* tmux
* top
  都会行为异常。

---

### Hangup / SIGHUP 处理（类驱动语义）

```c
close(master);  // slave 会收到 SIGHUP
```

用于模拟：

* 终端断线
* ssh 断连
* 控制台重置

---

### Baudrate / 流控（串口模拟场景）

很多 PTY driver 会模拟串口：

```c
B9600 / B115200
CRTSCTS
IXON/IXOFF
```

---

## 四、典型“用户态 PTY Driver”项目（真实工程级）

下面这些都是**标准级实现范本**：

> 世界上用得最多的 PTY 用户态 driver

功能：

* pty 分配
* 远程 shell 映射
* 网络 ↔ pty 双向转发
* window resize 透传

源码位置：

```text
openssh/sshpty.c
openssh/session.c
```

这是 **最教科书级 PTY driver 实现** ✅

tmux 和 screen

> 多路终端复用器 = 高级 PTY driver

能力：

* 一个 slave 对应多个显示终端
* 输入分发
* 窗口 resize 同步
* 挂后台不断线

源码：

```text
tmux/tty.c
tmux/server-client.c
```

如果你要写“多路复用 PTY”，强烈参考 tmux


4. socat

```bash
socat PTY,raw,echo=0 PTY,raw,echo=0
```
相当于创建了一个：

```
pts/3 <====> pts/4
```

4. qemu
```text
qemu/chardev/char-pty.c
```


4. container / CRI 终端 attach
```
docker attach
kubectl exec -it
```

所有的实现本质上都是：

```
socket <-> pty(master)
```

### gdb / expect / automation framework

expect 本质就是：

```text
脚本 <-> PTY <-> 被控程序
```

用于：

* 自动化交互
* 终端模拟测试
* fuzz 终端程序

## 五、如果你要“真正写一个工程级 PTY Driver”，推荐三种架构

### ✅ 架构 1：最经典模型（ssh / docker）

```
网络 socket
     ⇅
 [ user PTY driver ]
     ⇅
    pty master <==> kernel tty <==> shell
```

适合：

* 远程终端
* 容器控制台
* 云平台 serial console

### ✅ 架构 2：虚拟串口仿真模型（QEMU / socat）

```
ttyUSB 模拟
   ⇅
 [ user pty driver ]
   ⇅
  虚拟设备
```

适合：

* 串口服务器
* 网络设备仿真
* IoT 调试

### ✅ 架构 3：安全隔离终端（sandbox）

```
非可信程序 <-> pty <-> 过滤器 <-> 真实终端
```

可实现：

* 命令过滤
* ANSI 转义清洗
* 键盘注入防护
* 敏感输出遮挡

## 六、如果你需要：我可以直接给你一个“工程级模板”

我可以直接给你：

✅ 一个完整的：

* epoll 驱动框架
* termios 透明处理
* 窗口 resize 处理
* socket↔pty 桥接
* 自动回收子进程


## ptmx 的功能
```txt
lrwxrwxrwx     - root 15 Dec 22:15   ptmx -> ../../devices/virtual/tty/ptmx
```

/dev/tty/ptmx 是伪终端主设备（PTY master multiplexer）
内核驱动在 drivers/tty/pty.c 中

```txt
open("/dev/ptmx")  → 得到 master fd
grantpt()          → 设置 slave 权限
unlockpt()         → 解锁 slave
ptsname()          → 得到 /dev/pts/N
```

## 这居然是一个内核模块
https://github.com/freemed/tty0tty

## 现在想想，其实 pueue 也是类似的工作

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
