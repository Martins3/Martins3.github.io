## termios
`termios` 是 POSIX 标准中用于控制终端（TTY）设备 I/O 行为的接口，主要针对串行端口（如 UART、USB 转串口）以及伪终端（pty）等字符设备。以下几类项目通常**需要使用 `termios`**：

### 1. **串口通信程序**
- 与嵌入式设备、传感器、调制解调器、工业控制器等通过串口通信。
- 需要配置波特率、数据位、停止位、校验位（如 115200 8N1）。
- 示例：用 C 语言通过 `/dev/ttyUSB0` 控制 Arduino 或 STM32。

### 2. **终端模拟器（Terminal Emulator）**
- 如 `xterm`、`Alacritty`、`tmux`、`screen` 等需设置本地终端为 raw 模式或 cbreak 模式。
- 禁用回显（echo）、行缓冲，以便实现按键即时响应。

### 3. **伪终端（PTY）管理工具**
- 实现 `ssh`、`docker exec`、`script` 等工具时，需创建和配置 master/slave pty。
- 控制从端（slave）的终端行为，如回车换行处理、信号生成（Ctrl+C → SIGINT）。

### 4. **系统级调试或监控工具**
- 通过串口调试 Linux 内核（如通过串口 console 获取 oops 信息）。
- 需设置串口为非规范模式（non-canonical mode）以逐字节读取。

### 5. **自定义 REPL 或交互式解释器**
- 如嵌入式 shell、自研命令行工具，需要精细控制输入行为（如不回显密码、处理方向键）。

### 6. **低层网络或协议仿真（通过串口或 loopback TTY）**
- 某些旧协议（如 PPP over serial）仍需串口配置。
- 自定义协议仿真器可能使用 PTY 对（如 `pty` + `socat`）进行测试。

---

### 典型 `termios` 操作包括：
- 使用 `tcgetattr()` / `tcsetattr()` 获取/设置终端属性。
- 设置 `c_cflag`（如 `B115200`, `CS8`, `CLOCAL`, `CREAD`）。
- 设置 `c_lflag`（如禁用 `ICANON` 和 `ECHO` 进入 raw 模式）。
- 控制输入/输出时序（`VTIME`, `VMIN`）。


`termios` 是 POSIX 标准定义的一套**接口规范**，其“源码”并非单一文件，而是由 **C 标准库（如 glibc）** 提供用户空间的封装函数（如 `tcgetattr`, `tcsetattr`），而底层功能则由 **Linux 内核** 实现。因此，它的实现分布在两个地方：

---

### 一、用户空间：glibc（或 musl 等 C 库）
这些库提供符合 POSIX 的 `termios.h` 头文件和函数封装。


### 二、内核空间：Linux 内核
真正的终端设备行为（如串口、pty 的设置）由内核实现，主要集中在：

- **核心 TTY 子系统**：
  - 源码路径：`drivers/tty/`
    - 通用 TTY 核心：`drivers/tty/tty_ioctl.c`（处理 `TCGETS`/`TCSETS` 等 ioctl）
    - 串口驱动：`drivers/tty/serial/`
    - 伪终端（pty）：`drivers/tty/pty.c`

- **关键 ioctl 命令处理**：
  - `TCGETS` / `TCSETS` / `TCSETSW` / `TCSETSF` 等命令在 `tty_ioctl.c` 中实现，操作 `struct ktermios`（内核版 termios）。

- **架构/平台相关部分**（如串口硬件寄存器配置）：
  - 如 `drivers/tty/serial/8250/`（传统 UART）
  - 或 `drivers/tty/serial/omap-serial.c`（SoC 串口）

关键文件：
- `include/uapi/asm-generic/termios.h`（用户空间可见的结构定义）
- `include/linux/tty.h`（内核内部 TTY 结构）
- `drivers/tty/tty_ioctl.c`（ioctl 实现）

---

### 总结
| 组件       | 作用                     | 源码位置示例                     |
|------------|--------------------------|----------------------------------|
| glibc      | 提供 `tcgetattr` 等函数  | glibc 源码中的 `termios/` 目录   |
| Linux 内核 | 实现底层 TTY 控制逻辑    | `drivers/tty/`, `tty_ioctl.c`    |
| 用户头文件 | 提供 `struct termios`    | `/usr/include/termios.h`         |

如果你是 Linux 内核工程师（如记忆所示），你可能更关注 **内核中的实现**，特别是 `tty_ioctl.c` 中 `tty_mode_ioctl()` 和 `n_tty_ioctl_helper()` 等函数如何处理 `termios` 的设置。

需要我展示 `tcsetattr` 如何从 glibc 调用到内核的完整路径吗？

## 似乎 api 就是这些
```txt
   - tcgetattr(fd, &termios) - Gets terminal attributes (using TCGETS/TCGETS2)
   - tcsetattr(fd, action, &termios) - Sets terminal attributes (using TCSETS/TCSETS2)
   - tcflush(fd, queue_selector) - Flushes data (using TCFLSH)
   - tcflow(fd, action) - Controls flow (using TCXONC)
   - tcdrain(fd) - Waits for output to drain (using TCSBRK)
   - tcsendbreak(fd, duration) - Sends break signal (using TCSBRK/TCSBRKP)
   - tcgetpgrp(fd) - Gets foreground process group (using TIOCGPGRP)
   - tcsetpgrp(fd, pgrp) - Sets foreground process group (using TIOCSPGRP)
   - tcgetsid(fd) - Gets session ID (using TIOCGSID)
```

其实，最精确的实现在 drivers/tty/tty_io.c 中

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
