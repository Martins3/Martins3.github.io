# Linux TTY 子系统架构概览

本文基于本目录中的 `linux-tty-analysis.md`、`tty.md`、`pty-driver.md`、`console.md`、`serial.md`、`termios.md`、`sysfs.md`、`sysrq.md`、`qemu.md` 等笔记，整理 Linux 内核 TTY 子系统的整体结构。重点是建立可以迁移到不同内核版本的模型；具体字段和调用细节可能随内核版本变化。

## 1. 先建立一个正确的心智模型

TTY 不是某一种终端硬件，也不等同于 Shell、终端模拟器或控制台。它是内核提供的一套“面向字符终端的设备和协议框架”。同一套 TTY 接口，底层可以接 UART、伪终端、虚拟控制台、USB 转串口、virtio-console 或其他字符设备。

```text
用户态程序
  shell / vim / sshd / getty / minicom / terminal emulator
          |
          | read(), write(), ioctl(), poll()
          v
VFS 字符设备层
  tty_fops
          v
TTY 核心层
  tty_open() / tty_read() / tty_write() / tty_ioctl()
  设备生命周期、引用计数、会话和控制终端关联
          v
Line Discipline
  默认是 N_TTY
  规范模式、回显、信号、行缓冲、termios 相关处理
          v
TTY 驱动
  UART / PTY / VT / USB serial / hvc ...
  把字节连接到具体后端
          v
             物理或虚拟后端
       UART 硬件、framebuffer、PTY 对端、
       virtio 通道、USB 串口芯片等
```

Line Discipline 和 TTY 驱动不是并行的两个分支，而是同一个 TTY 数据通路上的两个连续层次。上图按“用户输出”方向绘制；输入方向则从后端进入 TTY 驱动，再经过接收缓冲和 Line Discipline，最后交给用户态。

可以把 TTY 拆成四个问题：

1. **谁拥有这个文件描述符？** 由 VFS 和 TTY 核心处理。
2. **输入是否按行缓冲、是否回显、Ctrl-C 是否产生信号？** 由 Line Discipline 处理。
3. **字节最终去哪里、从哪里来？** 由具体 TTY 驱动处理；它位于 Line Discipline 的下游（输出）或上游（输入）。
4. **字节中的 `ESC[...` 如何显示成颜色或光标移动？** 通常由用户态终端模拟器处理，不是 TTY 内核层绘制。

## 2. 主要概念和设备节点

| 概念 | 典型节点 | 作用 |
| --- | --- | --- |
| TTY | `/dev/ttyS0`、`/dev/pts/2`、`/dev/tty1` | 内核提供的终端字符设备接口 |
| UART 串口 | `/dev/ttyS*`、`/dev/ttyUSB*`、`/dev/ttyAMA*` | 连接真实 UART 或 USB 串口硬件 |
| VT/Virtual Console | `/dev/tty1` 到 `/dev/tty63` | 内核直接管理的虚拟控制台，可用 `Ctrl+Alt+F1` 等切换 |
| 当前 VT | `/dev/tty0` | 当前活动的虚拟控制台，不是固定的第 0 个 VT |
| 当前进程的控制终端 | `/dev/tty` | 由内核根据进程的 controlling terminal 解析 |
| PTY master | `/dev/ptmx` | 分配一对新的伪终端，并返回 master 文件描述符 |
| PTY slave | `/dev/pts/N` | 对 Shell、vim 等程序表现得像普通终端 |
| 系统控制台 | `/dev/console` | console 子系统选择的系统控制台入口，供内核日志和系统级程序使用 |

`/dev/console`、`/dev/tty0` 和 `/dev/tty` 不是同一个概念：前者属于 console 路由，第二个指当前活动 VT，第三个依赖打开它的进程。`/dev/pts/N` 也不是显示器，它只是 PTY 的 slave 端；真正绘制窗口的是用户态终端模拟器。

## 3. TTY 核心层

核心代码主要位于 `drivers/tty/tty_io.c`、`drivers/tty/tty_ioctl.c`、`drivers/tty/tty_buffer.c` 和相关头文件。

### 3.1 `struct tty_driver`

`tty_driver` 描述一类设备，而不是某一次打开操作。它包含：

- 驱动名称和设备节点前缀，例如 `serial`、`ttyS`、`pty_slave`；
- 主设备号、起始次设备号和设备数量；
- 设备类型，例如串口、PTY、系统设备；
- 初始 `termios`；
- 该类设备的 `tty_operations`。

驱动通常在初始化函数中调用 `tty_alloc_driver()`、设置字段、调用 `tty_set_operations()`，最后通过 `tty_register_driver()` 注册。`tty0tty.c` 中的 `tty0tty_init()` 是一个很小的示例。

### 3.2 `struct tty_struct`

`tty_struct` 表示一个运行中的 TTY 实例，通常在设备被打开时建立或取得。它关联：

- 所属 `tty_driver` 和 `tty_port`；
- 当前 `termios`；
- 当前 Line Discipline；
- 前台进程组、会话和 controlling terminal 状态；
- 输入/输出缓冲和各种状态位；
- 驱动私有数据 `driver_data`。

因此，`tty_driver` 是“设备类别”，`tty_struct` 是“这次实际使用的终端对象”。多个进程可能共享同一个 TTY 实例，具体引用和释放由 TTY 核心管理。

### 3.3 `struct tty_port`

`tty_port` 把端口的生命周期、打开等待、挂起/挂断以及接收缓冲等通用逻辑抽出来。现代驱动一般把它嵌入自己的端口结构，并通过 `tty_port_init()`、`tty_port_register_device()` 或类似接口接入 TTY 核心。

### 3.4 `struct tty_operations`

这是 TTY 核心向具体驱动分派操作的接口。常见回调包括：

- `open()`、`close()`：打开和关闭底层端口；
- `write()`：把用户态输出交给硬件或另一个虚拟端点；
- `write_room()`、`chars_in_buffer()`：报告发送缓冲状态；
- `set_termios()`：应用波特率、数据位、校验、流控等设置；
- `tiocmget()`、`tiocmset()`：读取或修改 RTS、CTS、DTR、DSR 等控制线；
- `ioctl()`：设备特有的控制命令；
- `throttle()`、`unthrottle()`、`stop()`、`start()`：流控和发送启停。

驱动不一定实现所有回调。没有实现的能力可能由 TTY 核心处理、返回不支持，或者表现为“接口存在但没有真实硬件语义”。

## 4. Line Discipline：TTY 的“字符处理层”

Line Discipline 位于 TTY 核心和具体驱动之间，默认 Line Discipline 是 `N_TTY`，实现主要在 `drivers/tty/n_tty.c`。

它处理的不是 UART 寄存器，而是字符流语义：

- **规范模式（canonical mode）**：输入按行组织，通常收到换行后，用户态 `read()` 才得到一行；
- **原始模式（raw mode）**：尽量逐字节传递，常用于串口程序、终端模拟器和交互式工具；
- **回显（echo）**：输入字符是否自动写回终端；
- **特殊字符**：`ERASE`、`KILL`、`EOF`、`VINTR`、`VSUSP` 等；
- **信号**：在 `ISIG` 打开时，Ctrl-C 可转换为 `SIGINT`，Ctrl-Z 可转换为 `SIGTSTP`；
- **软件流控**：`IXON`/`IXOFF`；
- **输入和输出转换**：例如回车换行处理。

`termios` 是用户态通过 `tcgetattr()`、`tcsetattr()` 操作的接口；内核侧对应 `struct ktermios` 和 `tty_ioctl.c` 中的 ioctl 处理。`c_iflag`、`c_oflag`、`c_cflag`、`c_lflag` 和 `c_cc` 分别描述输入、输出、控制、局部行为和特殊字符。

Line Discipline 可被替换，因此 TTY 不必总是使用 `N_TTY`。不过绝大多数 Shell、串口工具和 PTY 场景都依赖它。

## 5. 两条最重要的数据路径

### 5.1 输出路径

以 Shell 输出到图形终端为例：

```text
Shell 调用 write(1, ...)
        v
VFS -> tty_write()
        v
Line Discipline 的 write（通常是 n_tty_write）
        v
具体 TTY 驱动的 write
        v
PTY slave -> PTY master 的接收缓冲
        v
终端模拟器读取 PTY master
        v
解析 ANSI/VT 控制序列并绘制窗口
```

对于物理串口，最后几步会变成：

```text
TTY 驱动 -> UART 发送 FIFO/寄存器 -> 线路 -> 远端设备
```

发送中断、DMA、FIFO 和硬件流控属于 UART 驱动或硬件，不属于 `N_TTY`。

### 5.2 输入路径

输入方向通常是反过来的：

```text
键盘、UART、PTY master 或 USB 串口
        v
具体驱动接收字节
        v
tty_insert_flip_string*()
tty_flip_buffer_push()
        v
TTY 接收缓冲 / flush_to_ldisc
        v
Line Discipline receive_buf
        v
规范模式缓冲、回显、信号或 raw 数据
        v
阻塞的 read() 被唤醒并返回用户态
```

UART 驱动通常在中断或底半部中把硬件收到的字符放进 flip buffer。PTY master 写入时，也会通过类似的接收入口把字符送到 slave。`tty0tty.c` 的 `tty0tty_write()` 则直接找到对端 `tty_struct`，调用 `tty_insert_flip_string()` 和 `tty_flip_buffer_push()`，因此它不需要实现自己的 `.read()`。

### 5.3 用内核源码核对这两层的关系

在本地 Linux 6.6 内核源码中，可以直接从函数调用确认上面的模型：

用户态写入的完整主路径可以简化为：

```text
用户 write()
    |
    v
drivers/tty/tty_io.c: file_tty_write()
    |
    v
iterate_tty_write()
    |
    v
tty->ldisc->ops->write()
    |
    v
drivers/tty/n_tty.c: n_tty_write()
    |
    v
tty->ops->write()
    |
    v
具体 TTY driver
```

驱动接收数据的完整主路径可以简化为：

```text
硬件或虚拟后端
    |
    v
TTY driver
    |
    v
tty_insert_flip_string*()
tty_flip_buffer_push()
    |
    v
drivers/tty/tty_buffer.c: flush_to_ldisc()
    |
    v
drivers/tty/tty_port.c: tty_port_default_receive_buf()
    |
    v
tty_ldisc_receive_buf()
    |
    +--> ldisc->ops->receive_buf2()
    |       或
    +--> ldisc->ops->receive_buf()
    |
    v
当前 Line Discipline 接收缓冲（默认是 N_TTY）
    |
    v
drivers/tty/tty_io.c: tty_read()
    |
    v
iterate_tty_read()
    |
    v
ldisc->ops->read()
    |
    v
用户态 read()
```

PTY 的一对端点则通过下面的路径互相转发：

```text
端点 A 的 Line Discipline write
    |
    v
端点 A 的 tty->ops->write()
    |
    v
drivers/tty/pty.c: pty_write()
    |
    v
tty_insert_flip_string_and_push_buffer(端点 B->port, ...)
    |
    v
端点 B 的 flip buffer
    |
    v
端点 B 的 Line Discipline receive_buf*
    |
    v
端点 B 用户态 read()
```

- `drivers/tty/tty_io.c` 的 `tty_read()` 获取当前 TTY 的 `struct tty_ldisc`，然后通过 `iterate_tty_read()` 调用 `ld->ops->read`。因此，TTY 的用户态读取入口属于 Line Discipline，驱动没有通用的 `.read()` 回调。
- 同一文件中的 `file_tty_write()` 先检查 `tty->ops->write`，再获取 Line Discipline，并通过 `iterate_tty_write()` 调用 `ld->ops->write`。对默认 `N_TTY`，这个回调就是 `drivers/tty/n_tty.c` 的 `n_tty_write()`。
- `n_tty_write()` 在需要把数据真正发往设备时，调用 `tty->ops->write(tty, ...)`。因此写方向确实是 `Line Discipline -> TTY driver`，而不是两个分支同时处理。
- `drivers/tty/tty_buffer.c` 的 `flush_to_ldisc()` 消费 flip buffer；随后 `drivers/tty/tty_port.c` 的默认 `tty_port_default_receive_buf()` 调用 `tty_ldisc_receive_buf()`，后者最终调用 `receive_buf2` 或 `receive_buf`。因此输入方向是 `TTY driver -> flip buffer -> Line Discipline`。
- `drivers/tty/pty.c` 的 `pty_write()` 把数据写入对端的 `tty_port`，调用 `tty_insert_flip_string_and_push_buffer()`；这正是 PTY 将一个端点的驱动写操作变成另一个端点输入的实现。

`include/linux/tty_ldisc.h` 中还把 `struct tty_ldisc_ops` 的回调分成两组：上半部分标记为由 TTY 核心调用（`read`、`write`、`ioctl` 等），下半部分标记为由驱动侧调用（`receive_buf`、`write_wakeup` 等）。这个接口定义本身也说明了 Line Discipline 和 TTY driver 是上下游关系，只是数据方向相反。

## 6. 三类常见 TTY 驱动

### 6.1 UART/串口驱动

典型代码位于 `drivers/tty/serial/serial_core.c` 和 `drivers/tty/serial/8250/`。

串口驱动一般分两层：

- `serial_core` 提供通用的 TTY 接入、端口生命周期和串口操作抽象；
- 8250、16550 或 SoC 专用驱动实现寄存器、FIFO、中断、DMA 和硬件流控。

一个串口并不一定在模块加载时就申请 IRQ。很多驱动会在端口第一次打开时启动硬件、调用 `request_irq()`，这也是目录中记录的 `open -> uart_startup -> serial8250_do_startup -> request_irq` 调用链。

### 6.2 PTY 驱动

PTY 在 `drivers/tty/pty.c` 中实现，成对提供 master 和 slave：

```text
终端模拟器、ssh、容器 runtime
             |
             v
       PTY master fd
             <==== 内核 PTY 对 ==== >
       /dev/pts/N (slave)
             |
             v
       shell / vim / top
```

用户程序通过打开 `/dev/ptmx` 分配 master，再通过 `grantpt()`、`unlockpt()` 和 `ptsname()` 找到 slave。slave 对被控程序表现为普通 TTY，因而支持 `termios`、前台进程组、窗口大小、Ctrl-C 等终端语义。

终端模拟器本身通常是用户态程序：它从 master 读取字节，解析 VT100/ANSI 控制序列，然后用图形 API 绘制。颜色文档中的 `ESC[33m` 就是这种路径的例子。

### 6.3 VT/Virtual Console

VT 主要位于 `drivers/tty/vt/`，连接键盘输入、虚拟终端状态和 console 输出。它可以在没有图形桌面的情况下直接提供 `/dev/tty1` 等控制台，并通过 `Ctrl+Alt+Fn` 切换。

VT 和图形终端模拟器的关键区别是：VT 由内核直接管理虚拟控制台和显示输出，而图形终端通常通过 PTY 与 Shell 相连，绘制发生在用户态。

## 7. 会话、控制终端和登录流程

TTY 不只是字节通道，还参与 Unix 作业控制：

- 一个 session 可以拥有 controlling terminal；
- TTY 记录前台进程组；
- 只有前台进程组通常能无阻碍地读终端；
- Ctrl-C、Ctrl-Z 等由 Line Discipline 转换成信号并发给前台进程组；
- 窗口大小通过 `TIOCGWINSZ`、`TIOCSWINSZ` 传递给应用。

典型本地登录流程是：

```text
systemd 启动 getty@tty1.service 或 serial-getty@ttyS0.service
        v
agetty 打开 TTY，设置 termios，显示 login 提示
        v
login 验证用户并建立 session/controlling terminal
        v
Shell 继承 stdin/stdout/stderr
```

SSH 和容器交互终端只是把中间的输入输出搬到了网络或容器边界：

```text
SSH 客户端 / docker attach
        <=> 网络或 runtime
sshd / container runtime 持有 PTY master
        <=> 内核 PTY
Shell 持有 /dev/pts/N slave
```

如果 SSH 客户端传递 `TERM=xterm-ghostty`，远端程序还会根据 terminfo 查询终端能力。这个查询发生在用户态，与 TTY 内核传输字节的职责不同。

## 8. Console、`printk`、SysRq 和虚拟机串口

### 8.1 console 和普通 TTY 的区别

`console` 是内核日志和系统级输出的目标机制。启动参数中的 `console=tty0`、`console=ttyS0`、`console=hvc0` 会让相应的 console driver 注册为 printk 输出目标。

`/proc/consoles` 显示已经注册的 console 及其状态，`/sys/class/tty/console/active` 显示当前活动的 console。一个 console 设备同时也可能是普通 TTY，但“能接收 printk”是 console 子系统赋予它的额外角色。

### 8.2 SysRq

键盘 SysRq 通常先经过 input 子系统，再由 SysRq 处理器识别组合键；它不是普通 Shell 输入，也不必经过 `N_TTY` 才能触发。例如 `sysrq_handle_showregs()` 可以直接打印寄存器，`show-backtrace-all-active-cpus` 还会通过 NMI 请求其他 CPU 输出回溯。因此看到 SysRq 日志，并不代表字符一定走过了 PTY 或 Shell。

### 8.3 QEMU 和 virtio-console

QEMU 可以把 guest 的串口连接到 stdio、PTY、Unix socket、文件或其他 chardev。guest 内核看到的设备可能是 `/dev/ttyS0`，也可能是 `/dev/hvc0`；后者通常对应 hypervisor/virtio/hvc 通道。设备名字由 guest 内核驱动和设备树/ACPI/虚拟硬件共同决定，不能仅凭 QEMU 命令行中的“console”一词判断。

## 10. 如何阅读内核源码

建议按下面的顺序阅读，而不是一开始就钻进某个 UART 寄存器：

1. `drivers/tty/tty_io.c`：理解设备打开、读写、释放和 controlling terminal。
2. `include/linux/tty.h`、`include/linux/tty_driver.h`：确认核心对象和回调接口。
3. `drivers/tty/n_tty.c`：理解规范模式、回显、特殊字符和信号。
4. `drivers/tty/tty_ioctl.c`：把用户态 `termios`/TTY ioctl 和内核行为对应起来。
5. `drivers/tty/tty_buffer.c`：理解 flip buffer、接收缓冲和到 Line Discipline 的交接。
6. `drivers/tty/pty.c`：用最简单的虚拟驱动理解成对 TTY 的数据转发。
7. `drivers/tty/vt/`：理解键盘、VT 和内核 console 的组合。
8. `drivers/tty/serial/serial_core.c`、`drivers/tty/serial/8250/`：最后再进入真实硬件、中断和 FIFO。

目录中的 `tty/pty.c`、`pty-slave.c`、`tcgetattr.c` 和 `tcsetattr.c` 小程序适合配合上述路径做用户态实验；`tty0tty.c` 则展示了一个更简单的“成对虚拟串口”驱动如何使用 TTY 框架。

动态跟踪时，可以从这些函数入手：

- `tty_open()`、`tty_read()`、`tty_write()`：TTY 核心入口；
- `n_tty_read()`、`n_tty_write()`、`receive_chars()`：Line Discipline 和输入处理；
- `tty_insert_flip_string*()`、`tty_flip_buffer_push()`、`flush_to_ldisc()`：驱动到 Line Discipline 的接收路径；
- `uart_write()`、`serial8250_interrupt()`：典型 UART 发送和接收；
- PTY 驱动中的 master/slave write 回调：伪终端转发；
- `tty_ioctl()`：`TCGETS`、`TCSETS`、`TIOCGWINSZ`、`TIOCGPGRP` 等控制请求。


## 11. 常见误区

### TTY 就是终端窗口

不是。终端窗口是用户态 GUI 程序；TTY 是内核字符设备和终端语义接口。

### PTY slave 是显示器

不是。PTY slave 只是 Shell 看到的终端设备，输出由 PTY master 的持有者读取并解释。

### `termios` 会让内核理解所有 ANSI 控制序列

不会。`termios` 负责输入回显、规范模式、信号、流控和串口参数；ANSI/VT 控制序列通常由终端模拟器解析。


### 所有串口驱动都在模块加载时申请中断

不一定。许多 UART 驱动在端口首次打开时才完成 startup 和 IRQ 注册，目录中的 `serial8250` 调用链记录了这一点。

## 12. 总结

Linux TTY 的核心价值是提供稳定的终端抽象：上层程序只需要面对 `read`、`write`、`ioctl`、`termios` 和作业控制，下层可以自由替换为 UART、PTY、VT、USB 串口或 virtio-console。

最值得记住的两条路径是：

```text
输出：用户 write -> TTY 核心 -> Line Discipline -> TTY 驱动 -> 后端
输入：后端/驱动 -> flip buffer -> Line Discipline -> 用户 read
```

理解这两条路径后，`/dev/ttyS0`、`/dev/tty1`、`/dev/pts/N`、SSH、容器终端、QEMU serial console 和 `tty0tty` 都可以看成同一个框架下的不同后端，而不是互不相关的功能。

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
