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
