# pty

- PTY：Pseudo Terminal，伪终端，通常泛指整套伪终端机制。
- PTS：Pseudo Terminal Slave，伪终端从端，对应 /dev/pts/N。
- PTMX：Pseudo Terminal Multiplexer，伪终端多路复用器；打开 /dev/ptmx 会创建一对新的 master/slave。

必须成人 pty 不是一个很容易理解的机制，非常的违背直觉。

我打开了一个 ghostty ，我可以里面敲各种命令。

在我的设想中，数据的处理流程是这样的，

1. 用户输入
2. ghostty 接受到字符串，fork 程序，获取结果
3. ghostty 展示出来

## 1. ghostty 和 bash 如何通信?

很明显，bash 才是真的交互对象，ghostty 需要将结果都传递到 bash 中去，然后接受 bash 的输出，在展示出来。
这是两个 process ，他们需要互相通信。Linux 中，进程通信的方法有非常多，Unix domain socket 是一个非常方便的方法，
性能不算太差，

终端 A：
```txt
socat UNIX-LISTEN:/tmp/serial-demo.sock STDIO,raw,echo=0,escape=0x1d
```
终端 B：
```txt
socat STDIO,raw,echo=0,escape=0x1d UNIX-CONNECT:/tmp/serial-demo.sock
```

连接后，两边输入的字符会立即显示到对方，按 Ctrl-] 退出。
这里的 /tmp/serial-demo.sock 是 socket 文件。socat 手册 (https://man7.org/linux/man-pages/man1/socat.1.html)

但是，你会发现测试数据收发、命令交互，这样可以。

- 测试真实终端行为，例如 termios、终端 ioctl、行规程：socket 无法替代 PTY。
- 只是想避免手动找 /dev/pts/N：可以在之前的 PTY 参数中加入 link=/tmp/demo-tty，客户端打
  开 /tmp/demo-tty。这只是创建固定名称的软链接，底层仍然使用 /dev/pts/N。

这里的 STDIO,raw 仍在配置你当前终端；替换成 socket 的是两个 socat 之间的连接。


###
把终端 A 的 STDIO 换成启动 Bash，就能在终端 B 执行命令。 命令会以终端 A 的当前用户身份在本机运行。

终端 A，启动服务：


  socat UNIX-LISTEN:/tmp/serial-demo.sock,mode=0600 \
      EXEC:'bash --noprofile --norc -i',pty,setsid,ctty,stderr

 socat UNIX-LISTEN:/tmp/serial-demo.sock,mode=0600 \
      EXEC:'bash --noprofile --norc',stderr



  终端 B，连接：

  socat STDIO,raw,echo=0,escape=0x1d \
      UNIX-CONNECT:/tmp/serial-demo.sock

socat STDIO UNIX-CONNECT:/tmp/serial-demo.sock


  连接后会看到 Bash 提示符，可以测试：

  pwd
  tty
  ls
  sleep 100

  运行 sleep 100 后按 Ctrl-C 可以中断它；输入 exit 正常退出，或按 Ctrl-] 断开连接。

  服务端的几个参数分别是：

  - pty：为 Bash 创建伪终端。
  - setsid：让子进程创建新会话。
  - ctty：把伪终端设为子进程的控制终端，支持交互 shell 的作业控制。
  - stderr：把错误输出也传给客户端。socat 手册 (https://man7.org/linux/man-pages/man1/socat.1.html)

  此时数据路径为：

  终端 B ↔ socat ↔ Unix socket ↔ socat ↔ PTY ↔ Bash

  这里不用手动指定 /dev/pts/N，但底层仍创建了 PTY，在 Bash 中执行 tty 就能看到。若你仍希望完全不创建新的 PTY，可以把终端 A
  的第二行改为：

      EXEC:'bash --noprofile --norc',stderr

  这样也能逐行执行命令、返回结果，但没有正常的交互提示符、终端编辑和作业控制。

## 除了 ctrl c 可以处理外，还可以做什么?


• PTY 的核心作用是：让程序面对一个具有终端语义的设备，而不只是能读写字节的管道或 socket。 除了 Ctrl-C，主要还提供这些能力：

   能力                具体表现
  ━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   行缓冲和基本编辑    canonical 模式下，输入先保存在内核中，按回车后程序才读到；支持退格、Ctrl-U 清除当前行
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   输入回显            输入的字符自动显示；输入密码时，程序可以通过关闭 ECHO 隐藏字符
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   特殊字符处理        Ctrl-D 表示输入结束的语义，Ctrl-Z 触发挂起，Ctrl-\ 触发退出信号；这些行为可以配置
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   作业控制            配合控制终端和前台进程组，让 Bash 的 fg、bg、挂起和恢复正常工作
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   终端尺寸            程序可以查询行列数；尺寸变化时，前台进程组可收到 SIGWINCH
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   输入输出转换        根据配置处理回车、换行，以及 Ctrl-S / Ctrl-Q 软件流控
  ──────────────────  ───────────────────────────────────────────────────────────────────────────────────────
   终端接口            提供 isatty()、tcgetattr()、tcsetattr() 等接口，让程序识别和配置终端

  拿你刚才的实验来说，有几个区别特别容易观察。

  密码输入：程序通过 termios 关闭 PTY 的回显，用户仍然能输入，但屏幕上不会显示密码。普通 socket 没有这个终端属性。

  挂起和恢复：执行 sleep 100，按 Ctrl-Z，Bash 会重新显示提示符；输入 fg，它又回到前台。这依赖终端的前台进程组机制，以及
  shell 对作业的管理。

  全屏程序：Vim、top 等程序会查询终端尺寸，并把终端切换到逐字符输入模式，这样按键不必等回车才生效。不过，PTY 本身不负责画界
  面、颜色和移动光标，这些由终端模拟器解释程序输出的转义序列完成。

  还有两个容易混淆的地方：

  - 历史记录、Tab 补全、左右移动光标，通常由 Bash 的 Readline 等用户态代码实现。PTY 提供它们需要的逐字符输入和终端控制接口。
  - 窗口尺寸不会自动穿过 socket 传播。你刚才那组 socat 命令没有实现尺寸同步；SSH 这类程序会专门传递尺寸变化，再更新远端
    PTY。

  所以完整的交互体验来自三者配合：PTY 提供终端机制，shell/应用处理交互逻辑，终端模拟器负责显示和键盘输入。


### window size 的过程

可以用一个具体场景理解：你正在终端里运行 Vim，然后把终端窗口从 80 列拉宽到 120 列。Vim 需要知道这个变化，才能重新排版。

这里有两个独立的机制：查询当前尺寸，以及通知尺寸发生变化。

1. “行列数”是内核保存的一份终端属性

每个 TTY 的 tty_struct 中都有 winsize，其中主要是：

struct winsize {
    unsigned short ws_row;     /* 行数 */
    unsigned short ws_col;     /* 列数 */
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

例如 ws_row = 24、ws_col = 80，表示显示区域能容纳 24 行、每行 80 个字符单元。

程序通过下面的调用查询：

struct winsize ws;
ioctl(tty_fd, TIOCGWINSZ, &ws);

你执行：

stty size

就能看到当前终端的行数和列数。接口文档 (https://man7.org/linux/man-pages/man2/TIOCSWINSZ.2const.html)

2. 谁告诉内核“窗口变大了”？

对于终端模拟器里的 PTY，通常是终端模拟器自己，比如 Kitty、WezTerm。

它收到图形窗口的尺寸变化事件后，根据字体和显示区域计算新的行列数，然后对 PTY 调用：

ioctl(master_fd, TIOCSWINSZ, &new_size);

内核不会自己观察屏幕窗口。它需要用户态程序把尺寸告诉它。

完整过程是：

你拖动终端窗口
    ↓
终端模拟器计算新的行列数
    ↓
ioctl(PTY master, TIOCSWINSZ, ...)
    ↓
内核记录新尺寸，并向前台进程组发送 SIGWINCH
    ↓
Vim 收到通知，再通过 TIOCGWINSZ 查询尺寸
    ↓
Vim 按新尺寸重新绘制界面

SIGWINCH 可以记成 window change。这个信号本身不携带新的行列数，只表示“尺寸变了，去查询”。

3. 为什么是“前台进程组”？

因为一个终端里可以存在很多进程，但当前与用户交互的通常只有一个作业：

Bash 提示符等待输入时：前台进程组是 Bash 所在的组

Bash 启动 Vim 后：     前台进程组切换为 Vim 所在的组

因此，运行 Vim 时，内核会通知 Vim 所在的前台进程组。对于一个前台管道作业，同组的多个进程都可能收到信号。

这里的“前台”指的是 TTY 的作业控制状态，不是桌面上哪个窗口被鼠标选中。

4. 对应的内核实现

你本地源码中：

- /home/martins3/data/kernel/linux/drivers/tty/tty_io.c 的 tiocgwinsz()：把 tty->winsize 复制给查询者。
- 同文件的 tiocswinsz()：读取用户提供的新尺寸，调用驱动的 resize 回调。
- /home/martins3/data/kernel/linux/drivers/tty/pty.c 的 pty_resize()：检查尺寸是否变化，获取两端的前台进程组，调用
  kill_pgrp(..., SIGWINCH, ...)，并同步两端的 winsize。

联系你之前的 socat 实验：终端 B 的窗口与服务端 Bash 的 PTY 是两个不同的终端。 那组 socat 命令只转发字节，没有转发窗口尺寸
变化，所以拖动终端 B，不会自动更新 Bash 那个 PTY 的尺寸。SSH 则专门实现了这条通知和同步路径。

## 这个需要继续调查可以实现什么功能
3. **输入处理**：实现行编辑（回删、Ctrl-W 删除单词等）而无需用户态程序介入

## TTY 的历史演变

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Phase 1: 物理电传打字机 (1960s-1970s)                                        │
│  ┌─────────┐      Serial      ┌───────────────┐                              │
│  │ 用户终端 │  ═══════════════► │ 大型机 (Unix) │                              │
│  │(Teletype)│      RS-232      │               │                              │
│  └─────────┘                   └───────────────┘                              │
│                              硬件实现: UART 芯片                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                      ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│  Phase 2: 物理终端 + 视频终端 (1970s-1980s)                                   │
│  ┌─────────┐      Serial      ┌───────────────┐     ┌─────────────┐          │
│  │ 视频终端 │  ═══════════════► │   Line        │────►│   Shell     │          │
│  │ (VT100)  │                  │  Discipline   │     │  (用户态)    │          │
│  └─────────┘                  │  (n_tty)      │     └─────────────┘          │
│                               └───────────────┘                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│  Phase 3: 虚拟终端 VT (1990s-现在)                                            │
│  ┌──────────┐                ┌───────────────┐     ┌─────────────┐           │
│  │ 键盘+显卡 │ ─────────────► │ Virtual Term. │────►│   Shell     │           │
│  │(本地物理) │                │  (/dev/ttyN)  │     │             │           │
│  └──────────┘                └───────────────┘     └─────────────┘           │
│                               绕过 X11，直接使用 frame buffer                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                      ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│  Phase 4: 伪终端 PTY (现代图形界面时代)                                        │
│  ┌─────────────┐             ┌───────────────┐     ┌─────────────┐           │
│  │ Terminal    │             │  PTY Master   │     │  PTY Slave  │           │
│  │ Emulator    │◄═══════════►│  (如 pts/0)   │◄────│  (/dev/pts/N)│           │
│  │(alacritty)  │             │               │     │             │           │
│  └─────────────┘             └───────────────┘     │  Shell/vim  │           │
│                                                    └─────────────┘           │
│  终端模拟器在用户态实现，PTY 在内核中实现，完全兼容 TTY 接口                      │
└─────────────────────────────────────────────────────────────────────────────┘
```
(这个是真的吗?)

Phase 2 和 phase 3 的区别是什么?

一个是键盘接受中断，然后到 tty 机制中去，然后写入到 frame buffer 中去

一个是 serial 接受中断，然后还是写入 serial 中去

TODO 这里的 backtrace 都可以看看

## ghostty -> zsh -> tmux

这是有多个 tty 和 pts ，那么特殊符号是被谁劫持的?

## pty 太神奇了

来连接 pty 的方法:
```txt
# 这时候你立刻在另一个终端执行：
screen /dev/pts/12 115200
# 或者
minicom -D /dev/pts/12
```

## 思考一下在 bmc 界面中，使用 aspeed 显卡，然后使用 vim 的结果是什么

也就是清屏的操作现在是在驱动来实现了，
可以分别找一下，在 alacritty 和 驱动中实现这个的证据


## 有趣的观察，标准输入输出在 systemd 中切换了

systemd 中程序

```txt
total 0
dr-x------ 2 root root  0 Jun  3 15:41 .
dr-xr-xr-x 9 root root  0 Jun  3 15:41 ..
lr-x------ 1 root root 64 Jun  3 15:41 0 -> /dev/null
lrwx------ 1 root root 64 Jun  3 15:41 1 -> 'socket:[37045]'
lrwx------ 1 root root 64 Jun  3 15:41 2 -> 'socket:[37045]'
lrwx------ 1 root root 64 Jun  3 15:41 7 -> 'socket:[25320]'
```

普通模式运行:

```txt
lrwx------ - martins3  4 Jun 12:18  0 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  1 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  2 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  7 -> socket:[11477421]
```

## 我是没想到，原来 /dev/pts 也有文件系统的定义
fs/devpts/inode.c
```c
static struct file_system_type devpts_fs_type = {
	.name		= "devpts",
	.mount		= devpts_mount,
	.kill_sb	= devpts_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};
```

## mesg 工具
https://www.man7.org/linux/man-pages/man1/mesg.1.html

mesg 用来控制其他用户能否向你的终端写入消息，例如通过 write 或 talk。手册说明
(https://www.man7.org/linux/man-pages/man1/mesg.1.html)

 命令      作用
━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 mesg      查看当前状态：is y 或 is n
────────  ────────────────────────────
 mesg y    允许接收消息
────────  ────────────────────────────
 mesg n    禁止接收消息

它的实现是修改终端设备文件的组写权限；通常对应
/dev/pts/N。因此，它控制的是终端写入权限，不是聊天软件或桌面通知开关。自 util-linux 2.41 起，只修改 group
权限。DESCRIPTION (https://www.man7.org/linux/man-pages/man1/mesg.1.html#DESCRIPTION)

两个容易忽略的细节：

• 操作对象是 标准错误输出 stderr 所关联的终端。
• 退出码 0 表示允许，1 表示禁止，>1 才表示出错；没有关联终端时返回 2，默认不打印警告，可用 mesg -v
  查看。手册说明 (https://www.man7.org/linux/man-pages/man1/mesg.1.html)


### write 配合使用
```txt
write -h

Usage:
 write [options] <user> [<ttyname>]

Send a message to another user.

Options:
 -h, --help     display this help
 -V, --version  display version

For more details see write(1).
```

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
