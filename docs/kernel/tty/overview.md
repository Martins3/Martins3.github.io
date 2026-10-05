# tty

```
drivers/tty/
├── tty_io.c              # TTY 核心层
├── tty_ioctl.c           # ioctl 处理
├── tty_ldisc.c           # 线路规程管理
├── n_tty.c               # 默认线路规程 (N_TTY)
├── pty.c                 # 伪终端实现
├── vt/                   # 虚拟终端
│   ├── vt.c              # VT 核心
│   ├── keyboard.c        # 键盘处理
│   ├── console.c         # 控制台
│   └── vc_screen.c       # 屏幕缓冲
├── serial/               # 串口驱动
│   ├── serial_core.c     # 串口核心
│   └── 8250/             # 8250 UART 驱动
│       └── 8250.c
└── hvc/                  # Hypervisor 控制台

include/linux/tty.h       # TTY 核心头文件
include/linux/tty_driver.h # 驱动头文件
include/linux/tty_ldisc.h # 线路规程头文件
```

## 层次结构

### 1.  vfs 层
```c
static const struct file_operations tty_fops = {
	.read_iter	= tty_read,
	.write_iter	= tty_write,
	.splice_read	= copy_splice_read,
	.splice_write	= iter_file_splice_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
	.show_fdinfo	= tty_show_fdinfo,
};

static const struct file_operations console_fops = {
	.read_iter	= tty_read,
	.write_iter	= redirected_tty_write,
	.splice_read	= copy_splice_read,
	.splice_write	= iter_file_splice_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
};

static const struct file_operations hung_up_tty_fops = {
	.read_iter	= hung_up_tty_read,
	.write_iter	= hung_up_tty_write,
	.poll		= hung_up_tty_poll,
	.unlocked_ioctl	= hung_up_tty_ioctl,
	.compat_ioctl	= hung_up_tty_compat_ioctl,
	.release	= tty_release,
	.fasync		= hung_up_tty_fasync,
};
```

### 2. line desp

```txt
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
```

n_tty 最通用的实现了:
```c
static struct tty_ldisc_ops n_tty_ops = {
	.owner		 = THIS_MODULE,
	.num		 = N_TTY,
	.name            = "n_tty",
	.open            = n_tty_open,
	.close           = n_tty_close,
	.flush_buffer    = n_tty_flush_buffer,
	.read            = n_tty_read,
	.write           = n_tty_write,
	.ioctl           = n_tty_ioctl,
	.set_termios     = n_tty_set_termios,
	.poll            = n_tty_poll,
	.receive_buf     = n_tty_receive_buf,
	.write_wakeup    = n_tty_write_wakeup,
	.receive_buf2	 = n_tty_receive_buf2,
	.lookahead_buf	 = n_tty_lookahead_flow_ctrl,
};
```

### 3. tty driver

1. uart
2. pty
3. usb
4. vt

## 的确可以挖掘下
tty_nr = new_encode_dev(tty_devnum(sig->tty));
为什么是 sighand 中挂 tty ，有意思的

```txt
struct signal_struct {
```

中持有 tty_struct

所以，这是基本上每一个 task 都是有一个的 !?

```c
struct tty_struct {
    // 设备
    struct tty_driver *driver;
    const struct tty_operations *ops;
    int index;                  // 次设备号索引

    // 线路规程
    struct tty_ldisc *ldisc;

    // 进程组
    pid_t pgrp;                 // 前台进程组
    pid_t session;              // 会话 ID

    // 终端设置
    struct ktermios termios;
    struct winsize winsize;     // 窗口大小

    // 标志
    unsigned long flags;
    int count;                  // 打开引用计数

    // 端口（硬件相关）
    struct tty_port *port;
};
```

tty_driver 是什么层次?


这个结果难以理解，首先，为什么没有 VT ，
如何理解 console 在其中的作用?
```txt
enum tty_driver_type {
	TTY_DRIVER_TYPE_SYSTEM,
	TTY_DRIVER_TYPE_CONSOLE,
	TTY_DRIVER_TYPE_SERIAL,
	TTY_DRIVER_TYPE_PTY,
	TTY_DRIVER_TYPE_SCC,
	TTY_DRIVER_TYPE_SYSCONS,
};

enum tty_driver_subtype {
	SYSTEM_TYPE_TTY = 1,
	SYSTEM_TYPE_CONSOLE,
	SYSTEM_TYPE_SYSCONS,
	SYSTEM_TYPE_SYSPTMX,

	PTY_TYPE_MASTER = 1,
	PTY_TYPE_SLAVE,

	SERIAL_TYPE_NORMAL = 1,
};
```

        2.2.1 `struct tty_struct` - TTY 实例
        2.2.2 `struct tty_driver` - TTY 驱动
        2.2.3 `struct tty_operations` - 驱动操作
        2.2.4 `struct tty_ldisc` - 线路规程

### 6.3 参考资料优先级

有待扩展:
- LWN TTY 系列文章       | 深度分析，高质量   |
- LDD3 Chapter 18        | 经典教材，略有过时 |
- The TTY Demystified    | 概念讲解极佳       |
- Linux 手册页 (man tty) | 快速参考           |
- TLPI Chapter 62        | 用户态视角         |

## 最后，说明一下如下内容

说明一下当键盘按下一个键，到屏幕显示结果的过程
1. 中断
2. 字符设备
3. tty 等等

## tty_operations

```txt
static const struct tty_operations hvc_ops = {
	.install = hvc_install,
	.open = hvc_open,
	.close = hvc_close,
	.cleanup = hvc_cleanup,
	.write = hvc_write,
	.hangup = hvc_hangup,
	.unthrottle = hvc_unthrottle,
	.write_room = hvc_write_room,
	.chars_in_buffer = hvc_chars_in_buffer,
	.tiocmget = hvc_tiocmget,
	.tiocmset = hvc_tiocmset,
#ifdef CONFIG_CONSOLE_POLL
	.poll_init = hvc_poll_init,
	.poll_get_char = hvc_poll_get_char,
	.poll_put_char = hvc_poll_put_char,
#endif
};
```

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_openat
        - __se_sys_openat
          - __do_sys_openat
            - do_sys_open
              - do_sys_openat2
                - do_file_open
                  - path_openat
                    - do_open
                      - vfs_open
                        - do_dentry_open
                          - chrdev_open
                            - tty_open
                              - hvc_open


### tty 是控制层，然后后端是传输层

其他典型通路如下：

   TTY 设备                  tty_operations    下一层                         实际传输/输出
  ━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   普通 UART，如 ttyS*       uart_ops          uart_write()、uart_port.ops    8250、PL011 等硬件驱动，通过 MMIO/PIO/FIFO/IRQ 发送
  ────────────────────────  ────────────────  ─────────────────────────────  ─────────────────────────────────────────────────────
   VT，如 tty1               con_ops           do_con_write()、vc->vc_sw      VGA、framebuffer、dummy console 等显示后端
  ────────────────────────  ────────────────  ─────────────────────────────  ─────────────────────────────────────────────────────
   USB serial，如 ttyUSB*    serial_ops        usb_serial_driver->write()     USB bulk/control URB 和具体 USB-serial 芯片
  ────────────────────────  ────────────────  ─────────────────────────────  ─────────────────────────────────────────────────────
   RPMsg TTY                 rpmsg_tty_ops     rpmsg_trysend()                RPMsg endpoint
  ────────────────────────  ────────────────  ─────────────────────────────  ─────────────────────────────────────────────────────
   PTY                       pty_ops           对端 TTY 的 flip buffer        内核内两个 TTY 之间的软件传输
  ────────────────────────  ────────────────  ─────────────────────────────  ─────────────────────────────────────────────────────
   GSM 07.10 多路复用        gsmtty_ops        DLCI FIFO、调度和封装          再通过底层串口/TTY 发送

  普通串口的分层很清楚：

  • TTY 接口：drivers/tty/serial/serial_core.c:2668
  • uart_write() 先写入发送 FIFO：drivers/tty/serial/serial_core.c:606
  • __uart_start() 再调用具体 UART 驱动的 port->ops->start_tx()：drivers/tty/serial/serial_core.c:137

  VT 也有类似的第二层，但它不是“数据传输层”，而是“显示输出后端”：

  con_ops.write()
    -> do_con_write()
    -> vc->vc_sw->con_putcs()
    -> vga_con / dummy_con / newport_con / ...

  对应 drivers/tty/vt/vt.c:3657、drivers/tty/vt/vt.c:3912 和 include/linux/console.h:83。

  USB serial 也是标准的“通用 TTY ops + 设备驱动 ops”：

  serial_ops.write()
    -> serial_write()
    -> port->serial->type->write()
    -> 具体 USB serial 芯片驱动

  见 drivers/usb/serial/usb-serial.c:361 和 drivers/usb/serial/usb-serial.c:1285。

  需要区分的是：

  • tty_operations：TTY 设备对 TTY core 暴露的操作接口。
  • tty_port_operations：主要负责 port 生命周期、activate、shutdown、DTR/RTS 等管理，不是主要数据传输接口。
  • line discipline，如 N_TTY、N_GSM：位于用户和 tty_operations 之间，负责规范模式、回显、协议封装等。
  • struct console / struct consw：内核 console 和显示后端接口，和 /dev/tty* 的 TTY ops 是另一套接口。

  所以结论是：hvc_ops -> hv_ops -> virtio 是一个典型模式；UART、USB serial、RPMsg、GSM mux 等都有类似的“TTY 抽象层 + 实际后端”结构。VT
  也有第二层，但其后端是显示设备，而不是字节传输设备。

### uart / vt / usb 都是存在多个后端的可能性

```c
struct uart_ops {
	unsigned int	(*tx_empty)(struct uart_port *);
	void		(*set_mctrl)(struct uart_port *, unsigned int mctrl);
	unsigned int	(*get_mctrl)(struct uart_port *);
	void		(*stop_tx)(struct uart_port *);
	void		(*start_tx)(struct uart_port *);
	void		(*throttle)(struct uart_port *);
	void		(*unthrottle)(struct uart_port *);
	void		(*send_xchar)(struct uart_port *, char ch);
	void		(*stop_rx)(struct uart_port *);
	void		(*start_rx)(struct uart_port *);
	void		(*enable_ms)(struct uart_port *);
	void		(*break_ctl)(struct uart_port *, int ctl);
	int		(*startup)(struct uart_port *);
	void		(*shutdown)(struct uart_port *);
	void		(*flush_buffer)(struct uart_port *);
	void		(*set_termios)(struct uart_port *, struct ktermios *new,
				       const struct ktermios *old);
	void		(*set_ldisc)(struct uart_port *, struct ktermios *);
	void		(*pm)(struct uart_port *, unsigned int state,
			      unsigned int oldstate);
	const char	*(*type)(struct uart_port *);
	void		(*release_port)(struct uart_port *);
	int		(*request_port)(struct uart_port *);
	void		(*config_port)(struct uart_port *, int);
	int		(*verify_port)(struct uart_port *, struct serial_struct *);
	int		(*ioctl)(struct uart_port *, unsigned int, unsigned long);
#ifdef CONFIG_CONSOLE_POLL
	int		(*poll_init)(struct uart_port *);
	void		(*poll_put_char)(struct uart_port *, unsigned char);
	int		(*poll_get_char)(struct uart_port *);
#endif
};
```

所以，这些驱动的层次跟多的将相同的后端归类一样



## 流程
### 2.3.1 输出路径（Shell → 屏幕）

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


## 关键结构体的疑问

1. struct termios

```txt
struct ktermios {
	tcflag_t c_iflag;		/* input mode flags */
	tcflag_t c_oflag;		/* output mode flags */
	tcflag_t c_cflag;		/* control mode flags */
	tcflag_t c_lflag;		/* local mode flags */
	cc_t c_line;			/* line discipline */
	cc_t c_cc[NCCS];		/* control characters */
	speed_t c_ispeed;		/* input speed */
	speed_t c_ospeed;		/* output speed */
};

```
2. struct tty_driver


可以分成三个层次理解：一组设备的驱动、某个终端的状态、一次打开的文件对象。

 对象                 描述什么                           典型内容
━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 struct tty_driver    由同一个驱动管理的一组 TTY 设备    名字、主次设备号范围、设备数量、操作函数、默认终端配置
───────────────────  ─────────────────────────────────  ────────────────────────────────────────────────────────
 struct tty_struct    某一个具体终端的运行状态           终端配置 termios、行规程 ldisc、窗口大小、等待队列等
───────────────────  ─────────────────────────────────  ────────────────────────────────────────────────────────
 struct file          一次打开所产生的文件对象           打开标志、文件操作、指向 TTY 的私有数据

例如，一个串口驱动管理 /dev/ttyS0、/dev/ttyS1：

tty_driver（管理 ttyS 这一组设备）
    ├── tty_struct（ttyS0）
    │       ↑           ↑
    │     file A      file B
    │    进程 A 打开   进程 B 打开
    │
    └── tty_struct（ttyS1）
            ↑
          file C

两个结构体的关系。

### 添加上 port 之后的结果

现在可以把四个对象放在一起：

 对象          负责什么
━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 tty_driver    管理一组设备，提供共同的驱动操作
────────────  ─────────────────────────────────────────────────────
 tty_port      一个具体端口的缓冲、激活/关闭、载波检测等
────────────  ─────────────────────────────────────────────────────
 tty_struct    这个终端当前的终端语义：termios、行规程、窗口大小等
────────────  ─────────────────────────────────────────────────────
 file          一次打开产生的文件对象

以 /dev/ttyS0 为例：

tty_driver（串口驱动）
    │
    └── tty_port（ttyS0 的端口）
            ↕
        tty_struct（ttyS0 的终端运行状态）
            ↑           ↑
          file A      file B

为什么已经有 tty_struct，还需要 tty_port？主要是职责和生命周期不同。

tty_port 通常随着设备建立，即使没有进程打开，端口对象也可以存在。tty_struct 则可能在首次打开时建立，在最后关闭后释
放。之后再次打开，可以为同一个 tty_port 建立新的 tty_struct。


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
