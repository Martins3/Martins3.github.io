
| 概念 | 定义 | 例子 | 内核/用户态 |
|------|------|------|-----------|
| **TTY** | 字符设备接口，提供终端服务 | `/dev/ttyS0`, `/dev/pts/0` | 内核 |
| **Shell** | 命令行解释器 | bash, zsh, fish | 用户态 |
| **Terminal Emulator** | 图形化的终端模拟器 | alacritty, gnome-terminal | 用户态 |
| **Console** | 系统主控制台 | `/dev/console` | 内核 |
| **VT (Virtual Terminal)** | 虚拟终端 | `/dev/tty1` - `/dev/tty63` | 内核 |


## 2. 架构原理 - TTY 核心设计

### 2.1 TTY 子系统分层架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         用户态 (User Space)                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐ │
│  │  Shell/Bash  │  │     vim      │  │   Terminal   │  │  stty/minicom    │ │
│  │              │  │              │  │  Emulator    │  │                  │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘ │
└─────────┼─────────────────┼─────────────────┼───────────────────┼───────────┘
          │                 │                 │                   │
          │ write()/read()  │                 │                   │
          │                 │                 │                   │
┌─────────▼─────────────────▼─────────────────▼───────────────────▼───────────┐
│                         VFS 层                                              │
│                      (tty_fops)                                             │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  struct file_operations tty_fops = {                                   │ │
│  │      .read = tty_read,                                                 │ │
│  │      .write = tty_write,                                               │ │
│  │      .unlocked_ioctl = tty_ioctl,                                     │ │
│  │  };                                                                    │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      TTY Core Layer                                         │
│                   (drivers/tty/tty_io.c)                                    │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  - 设备管理 (tty_open, tty_release)                                    │ │
│  │  - 数据路由 (tty_read, tty_write)                                      │ │
│  │  - ioctl 处理 (TIOCGWINSZ, TCGETS, TCSETS)                            │ │
│  │  -  line discipline 切换 (set_ldisc)                                            │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                   Line Discipline Layer                                     │
│                 (drivers/tty/n_tty.c)                                       │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  N_TTY (默认线路规程):                                                 │ │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                │ │
│  │  │ 输入处理    │───►│ 行编辑      │───►│ 规范模式    │                │ │
│  │  │ (回显控制)  │    │ (退格/Ctrl-W)│   │ (缓冲直到\n)│                │ │
│  │  └─────────────┘    └─────────────┘    └─────────────┘                │ │
│  │                                                                         │ │
│  │  信号生成: Ctrl+C → SIGINT, Ctrl+Z → SIGTSTP                           │ │
│  │  特殊字符: ERASE, KILL, EOF, EOL 等                                    │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      TTY Driver Layer                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │   UART Driver   │  │   PTY Driver    │  │      VT Driver              │  │
│  │ (serial_core.c) │  │   (pty.c)       │  │   (vt/, keyboard.c)         │  │
│  │                 │  │                 │  │                             │  │
│  │ - 8250/16550    │  │ - PTY Master    │  │ - 虚拟控制台                │  │
│  │ - ttyS0~ttyS31  │  │ - PTY Slave     │  │ - Ctrl+Alt+Fn               │  │
│  │ - IRQ 处理      │  │ - /dev/pts/*    │  │ - Framebuffer               │  │
│  └────────┬────────┘  └────────┬────────┘  └─────────────┬───────────────┘  │
│           │                    │                         │                  │
│           ▼                    ▼                         ▼                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │   Hardware      │  │   Pseudo        │  │      Hardware               │  │
│  │   (Serial Port) │  │   Terminal      │  │   (VGA + Keyboard)          │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```


### 2.3 数据流向分析

#### 2.3.1 输出路径（Shell → 屏幕）

```
┌──────────┐    write()     ┌──────────┐
│   Shell  │───────────────►│   VFS    │
└──────────┘                └────┬─────┘
                                 │
                                 ▼
┌──────────┐  tty_write()  ┌──────────┐
│ tty_io.c │◄──────────────│ tty_fops │
│          │               │          │
│ do_tty_  │  n_tty_write()│          │
│  write() │──────────────►│          │
└────┬─────┘               └──────────┘
     │
     ▼
┌──────────┐  uart_write() ┌──────────┐
│ n_tty.c  │──────────────►│ uart_ops │
│ process_ │               │          │
│ _output_ │               │          │
│  block() │               │          │
└────┬─────┘               └────┬─────┘
     │                          │
     │                          ▼
     │                    ┌──────────┐
     │                    │serial8250│
     │                    │_start_tx()│
     │                    └────┬─────┘
     │                         │
     ▼                         ▼
┌──────────┐            ┌──────────┐
│ PTY:     │            │ Hardware │
│ pty_write│            │  UART    │
└────┬─────┘            │ 芯片     │
     │                  └──────────┘
     ▼
┌──────────┐
│ Terminal │
│ Emulator │
│ (Screen) │
└──────────┘
```

#### 2.3.2 输入路径（键盘 → Shell）

```
硬件中断路径:
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│ Keyboard │────►│ i8042    │────►│ atkbd_   │────►│ input_   │
│ 硬件     │ IRQ │ interrupt│     │ receive_ │     │ event()  │
└──────────┘     └──────────┘     │ byte()   │     └────┬─────┘
                                  └──────────┘          │
                                                        ▼
软件处理路径:                                  ┌──────────┐
                                               │ kbd_event│
                                               │ (vt/)    │
                                               └────┬─────┘
                                                    │
                                                    ▼
┌──────────┐     ┌──────────┐     ┌──────────┐  ┌──────────┐
│   tty_   │◄────│ n_tty_   │◄────│ tty_flip_│◄─│ line     │
│ receive_ │     │ receive_ │     │ buffer_  │  │ discipline│
│  buf()   │     │  _buf()  │     │  push()  │  │ 处理     │
└────┬─────┘     └──────────┘     └──────────┘  └──────────┘
     │
     ▼
┌──────────┐     ┌──────────┐
│  信号生成 │     │ 用户输入  │
│ Ctrl+C  │     │ 缓冲区   │
│ → SIGINT │     │          │
└──────────┘     └────┬─────┘
                      │
                      ▼
               ┌──────────┐
               │   read() │
               │   Shell  │
               └──────────┘
```


## 4. 使用场景 - 实际应用

### 4.1 物理串口调试 (/dev/ttyS0)

**场景**：服务器 BMC 调试、嵌入式开发

```bash
# 查看可用串口
$ dmesg | grep tty
[    0.123456] serial8250: ttyS0 at I/O 0x3f8 (irq = 4, base_baud = 115200) is a 16550A
[    0.123789] serial8250: ttyS1 at I/O 0x2f8 (irq = 3, base_baud = 115200) is a 16550A

# 使用 minicom 连接串口
$ sudo minicom -D /dev/ttyS0 -b 115200

# 使用 picocom（更轻量）
$ picocom -b 115200 /dev/ttyS0

# 直接使用 stty 设置参数
$ stty -F /dev/ttyS0 115200 cs8 -cstopb -parenb
$ echo "test" > /dev/ttyS0
$ cat /dev/ttyS0
```

### 4.2 虚拟终端切换 (Ctrl+Alt+F1~F6)

**场景**：无图形界面时的多控制台

```bash
# 查看当前 VT
$ fgconsole
1
```


### 4.3 伪终端 (Terminal Emulator, tmux, ssh)

**场景**：图形界面终端、远程登录、终端复用

```bash
# 查看当前伪终端
$ tty
/dev/pts/2

# 查看所有 pts
$ ls /dev/pts/
0  1  2  3  ptmx

# 查看 pts 使用情况
$ lsof /dev/pts/*

# SSH 连接后的 pts
$ ssh user@remote
$ tty
/dev/pts/1

# 使用 tmux
$ tmux new -s mysession
$ tty
/dev/pts/5
```

**PTY 内部工作机制**：
```
Terminal Emulator (alacritty)
    │
    │ open(/dev/ptmx) → master fd
    │
    ▼
PTY Master (/dev/ptm/*)  ←──────→  PTY Slave (/dev/pts/N)
                                        │
                                        │ fork/exec shell
                                        ▼
                                    Shell (bash)
```

### 4.4 QEMU 中的 TTY

**场景**：虚拟机串口调试、控制台重定向

```bash
# 基本串口配置
$ qemu-system-x86_64 \
    -serial stdio \
    -kernel vmlinuz \
    -append "console=ttyS0,115200"

# 多串口配置
$ qemu-system-x86_64 \
    -serial mon:stdio \
    -serial file:serial.log \
    -serial pipe:/tmp/mypipe \
    -serial pty

# virtio-console（更高性能）
$ qemu-system-x86_64 \
    -device virtio-serial-pci \
    -chardev stdio,id=char0 \
    -device virtconsole,chardev=char0 \
    -append "console=hvc0"

# 使用 unix socket 连接串口
$ qemu-system-x86_64 \
    -chardev socket,id=ser0,path=/tmp/serial.sock,server=on,wait=off \
    -serial chardev:ser0

# 从主机连接 QEMU 串口
$ socat -,raw,echo=0 unix-connect:/tmp/serial.sock
```

**QEMU 串口后端类型**：

| 后端 | 说明 | 示例 |
|------|------|------|
| `stdio` | 标准输入输出 | `-serial stdio` |
| `file` | 输出到文件 | `-serial file:serial.log` |
| `pipe` | 命名管道 | `-serial pipe:/tmp/pipe` |
| `pty` | 伪终端 | `-serial pty` |
| `socket` | Unix/TCP socket | `-serial tcp::4444,server=on` |
| `telnet` | Telnet 协议 | `-serial telnet::4444,server=on` |

### 4.5 BMC 和 IPMI 串口重定向

**场景**：服务器远程管理、无网络时的紧急访问

```
┌──────────────┐      IPMI      ┌──────────────┐
│   管理员工具  │ ◄════════════► │    BMC       │
│  (ipmitool)  │                │ (基板管理器)  │
└──────────────┘                └──────┬───────┘
                                       │
                                       │ Serial over LAN (SOL)
                                       ▼
                               ┌──────────────┐
                               │   Server     │
                               │   Serial     │
                               │   Console    │
                               └──────────────┘
```

```bash
# 使用 ipmitool SOL 连接
$ ipmitool -I lanplus -H <BMC_IP> -U <USER> -P <PASS> sol activate

# 配置 SOL
$ ipmitool sol set enabled true 1
$ ipmitool sol set baud-rate 115200 1

# 查看 SOL 状态
$ ipmitool sol info 1
```

### 4.6 容器中的 TTY

**场景**：Docker/Podman 容器交互

```bash
# 交互式容器（-t 分配伪终端，-i 交互）
$ docker run -it ubuntu bash

# 后台启动，之后 attach
$ docker run -dt --name mycontainer ubuntu sleep 3600
$ docker attach mycontainer

# 查看容器的 TTY
$ docker exec mycontainer tty
/dev/pts/0

# 在主机上查看
$ ls -la /proc/$(docker inspect -f '{ {.State.Pid}}' mycontainer)/fd/
lrwx------ 1 root root 64 Jan 15 10:00 0 -> /dev/pts/0
lrwx------ 1 root root 64 Jan 15 10:00 1 -> /dev/pts/0
lrwx------ 1 root root 64 Jan 15 10:00 2 -> /dev/pts/0
```

### 4.7 小结

| 场景 | 设备类型 | 典型设备 | 主要工具 |
|------|----------|----------|----------|
| 物理串口调试 | UART | `/dev/ttyS0`, `/dev/ttyUSB0` | minicom, picocom, screen |
| 虚拟终端 | VT | `/dev/tty1` - `/dev/tty63` | Ctrl+Alt+Fn |
| 图形终端 | PTY | `/dev/pts/*` | alacritty, gnome-terminal |
| 远程登录 | PTY over Network | `/dev/pts/*` | ssh, telnet |
| 终端复用 | PTY 管理 | `/dev/pts/*` | tmux, screen |
| 虚拟机 | Serial/PTY/Virtio | `/dev/ttyS0`, `/dev/hvc0` | QEMU, minicom |
| 服务器管理 | SOL | `/dev/ttyS0` | ipmitool |

---

## 5. 常见问题 - 调试技巧

### 5.1 TTY 设置与 termios

#### stty 常用命令

```bash
# 查看当前 TTY 设置
$ stty -a
speed 38400 baud; rows 73; columns 284; line = 0;
intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
eol2 = <undef>; swtch = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt = ^R;
werase = ^W; lnext = ^V; discard = ^O; min = 1; time = 0;
-parenb -parodd -cmspar cs8 -hupcl -cstopb cread -clocal -crtscts
-ignbrk brkint ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl ixon -ixoff
-iuclc -ixany -imaxbel iutf8
opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0
isig icanon iexten echo echoe echok -echonl -noflsh -xcase -tostop -echoprt
echoctl echoke -flusho -extproc

# 设置原始模式（用于串口通信）
$ stty -F /dev/ttyS0 raw -echo

# 设置波特率
$ stty -F /dev/ttyS0 115200 cs8 -cstopb -parenb

# 禁用行缓冲（逐字符读取）
$ stty -icanon min 1 time 0

# 恢复默认设置
$ stty sane
```

#### 编程设置 termios

```c
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// 设置串口为原始模式
int setup_serial(const char *device, int baudrate) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return -1;

    struct termios tty;
    tcgetattr(fd, &tty);

    // 设置原始模式
    cfmakeraw(&tty);

    // 设置波特率
    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    // 8N1
    tty.c_cflag &= ~PARENB;  // 无校验
    tty.c_cflag &= ~CSTOPB;  // 1 停止位
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 数据位
    tty.c_cflag |= CREAD | CLOCAL;  // 启用接收，忽略控制线

    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}
```

### 5.2 串口调试

#### minicom 配置

```bash
# 启动 minicom
$ sudo minicom -D /dev/ttyUSB0 -b 115200

# 进入配置菜单 (Ctrl+A, O)
# - Serial port setup
# - Save setup as dfl

# 常用快捷键
Ctrl+A, Q  # 退出
Ctrl+A, X  # 重置
Ctrl+A, L  # 开启日志
```

#### picocom（更简洁）

```bash
# 启动 picocom
$ picocom -b 115200 /dev/ttyUSB0

# 带日志
$ picocom -b 115200 --logfile serial.log /dev/ttyUSB0

# 退出 (Ctrl+A, Ctrl+X)
```

#### screen

```bash
# 使用 screen 连接串口
$ screen /dev/ttyUSB0 115200

# 退出 (Ctrl+A, k, y)
```

### 5.3 控制台日志

#### 内核启动参数

```
# 串口控制台
console=ttyS0,115200n8

# 多控制台（日志输出到多个设备）
console=tty0 console=ttyS0,115200

# 早期打印（用于启动早期调试）
earlyprintk=serial,ttyS0,115200

# 禁止 printk 到控制台
dmesg -D          # 禁用
```

#### dmesg 和 printk

```bash
# 查看内核日志
$ dmesg | grep tty

# 实时监控
$ dmesg -w

# 清空缓冲区
$ sudo dmesg -C

# 设置控制台日志级别
$ cat /proc/sys/kernel/printk
4       4       1       7
# (current, default, minimum, boot-default)

$ echo 8 | sudo tee /proc/sys/kernel/printk  # 显示所有消息
```

#### 查看当前控制台

```bash
# 查看激活的控制台
$ cat /sys/class/tty/console/active
tty0 ttyS0

# 查看所有注册的 console
$ cat /proc/consoles
tty0                 -WU (EC p  )    4:1
ttyS0                -W- (E  p a)    4:64

# 说明：
# -W : 可写
# U  : 正在使用 (used)
# E  : 启用
# p  : 可以作为 printk 目标
# a  : 可以作为 boot console
```

### 5.4 SysRq 魔术键

#### 启用 SysRq

```bash
# 临时启用
$ echo 1 | sudo tee /proc/sys/kernel/sysrq

# 永久启用（写入 sysctl.conf）
$ echo "kernel.sysrq = 1" | sudo tee -a /etc/sysctl.conf
```

### 5.5 TTY 相关 sysfs 接口

```bash
# 查看 TTY 设备
$ ls /sys/class/tty/
console  ptmx  tty  tty0  tty1  tty2  ...  ttyS0  ttyS1  ...

# 查看串口信息
$ cat /proc/tty/driver/serial
serinfo:1.0 driver revision:
0: uart:16550A port:000003F8 irq:4 tx:12345 rx:67890 CTS|DSR
1: uart:16550A port:000002F8 irq:3 tx:0 rx:0

# 查看 VT 控制台
$ cat /sys/class/vtconsole/vtcon*/name
(S) dummy device
(M) frame buffer device

# PTY 限制
$ cat /proc/sys/kernel/pty/max
4096
$ cat /proc/sys/kernel/pty/nr
10
```

### 5.6 常见问题排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 串口无输出 | 波特率不匹配 | `stty -F /dev/ttyS0` 检查 |
| | 线缆问题 | 更换 null-modem 线 |
| | 流控问题 | 禁用流控 `stty -crtscts` |
| 中文乱码 | 编码问题 | 设置 UTF-8 `stty iutf8` |
| | 终端模拟器设置 | 检查 locale |
| Ctrl+S 后无响应 | XOFF 流量控制 | 按 Ctrl+Q 恢复 |
| SSH 断开后程序停止 | SIGHUP 信号 | 使用 `nohup` 或 `tmux` |
| 串口数据丢失 | 缓冲区溢出 | 提高进程优先级，优化读取 |
| | 波特率过高 | 降低波特率 |
| 无法打开 /dev/ttyS0 | 权限不足 | `sudo usermod -a -G dialout $USER` |
| | 设备被占用 | `lsof /dev/ttyS0` 查看 |

### 5.7 小结

- **调试工具**：minicom/picocom（串口）、stty（参数设置）、dmesg（内核日志）
- **SysRq**：系统级调试魔术键，关键时刻救命
- **sysfs/proc**：了解系统 TTY 状态的重要接口
- **常见问题**：波特率、流控、权限是串口调试的三大坑

---

## 6. 学习路径 - 系统掌握

### 6.1 推荐阅读顺序

```
Level 1: 基础概念
├── 阅读本文档第 1、2 章
├── 理解 TTY、PTY、VT 的区别
└── 实践：使用 stty、查看 /dev/tty*

Level 2: 用户态编程
├── 学习 termios API
├── 编写简单的串口通信程序
├── 理解 PTY 的工作原理
└── 实践：实现自己的 mini terminal emulator

Level 3: 内核架构
├── 阅读 drivers/tty/tty_io.c
├── 理解 tty_struct、tty_driver、tty_operations
├── 阅读 n_tty.c 理解线路规程
└── 实践：使用 ftrace 跟踪 tty_write 调用链

Level 4: 驱动开发
├── 学习 serial_core.c
├── 阅读 8250.c 作为具体例子
├── 理解中断处理、DMA
└── 实践：在 QEMU 中实现自己的简单 tty driver

Level 5: 高级主题
├── VT 子系统 (vt/, fbcon/)
├── PTY 完整实现 (pty.c)
├── Console 多路复用
└── 实践：分析 tmux/screen 源码
```

### 6.2 关键实验验证

#### 实验 1：观察 PTY 创建过程

```bash
# 终端 1：监控 pts 目录
$ watch -n 0.5 'ls -la /dev/pts/'

# 终端 2：打开新终端窗口
# 观察 pts 目录变化

# 使用 strace 跟踪
$ strace -e open,openat,ioctl -o pty_trace.log xterm
# 分析 pty_trace.log 中的 /dev/ptmx 操作
```

#### 实验 2：跟踪内核 TTY 调用链

```bash
# 使用 ftrace 跟踪 tty_write
$ sudo trace-cmd record -p function_graph -g tty_write -g n_tty_write \
    -g uart_write -g pty_write

# 或者使用 bpftrace
$ sudo bpftrace -e '
fentry:tty_write {
    printf("tty_write: pid=%d, comm=%s\n", pid, comm);
}
fentry:n_tty_write {
    printf("  -> n_tty_write\n");
}
fentry:pty_write {
    printf("  -> pty_write\n");
}
'
```

#### 实验 3：理解 Line Discipline

```c
// test_ldisc.c: 测试不同线路规程
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("/dev/tty", O_RDWR);
    struct termios tty;

    tcgetattr(fd, &tty);

    // 关闭规范模式
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tcsetattr(fd, TCSANOW, &tty);

    printf("非规范模式：按 q 退出\n");
    char c;
    while (read(fd, &c, 1) == 1 && c != 'q') {
        printf("收到字符: %d ('%c')\n", c, c);
    }

    // 恢复
    tty.c_lflag |= ICANON | ECHO;
    tcsetattr(fd, TCSANOW, &tty);

    return 0;
}
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
