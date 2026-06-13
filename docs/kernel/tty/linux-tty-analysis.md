# Linux TTY 子系统深入分析

> 基于内核文档和源码的系统性技术分析

---

## 1. 高屋建瓴 - TTY 的本质

### 1.1 TTY 解决了什么问题？

**TTY**（TeleTYpewriter，电传打字机）最初是物理设备，用于用户与大型计算机交互。TTY 子系统解决的核心问题是：

1. **人机交互标准化**：提供统一的字符设备接口，屏蔽底层硬件差异
2. **会话管理**：支持多用户、多会话、前后台进程管理
3. **输入处理**：实现行编辑（回删、Ctrl-W 删除单词等）而无需用户态程序介入
4. **信号机制**：将特定按键组合（Ctrl+C、Ctrl+Z）转换为信号发送给进程组

> **关键洞察**：Line Discipline（线路规程）将字符编辑功能放在内核中，避免了每输入一个字符就进行内核/用户态上下文切换的开销。只有当用户按下 Enter 或缓冲区满时，才将数据传递给用户态程序。

### 1.2 TTY 的历史演变

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

### 1.3 TTY、Shell、Terminal Emulator、Console 的区别

| 概念 | 定义 | 例子 | 内核/用户态 |
|------|------|------|-----------|
| **TTY** | 字符设备接口，提供终端服务 | `/dev/ttyS0`, `/dev/pts/0` | 内核 |
| **Shell** | 命令行解释器 | bash, zsh, fish | 用户态 |
| **Terminal Emulator** | 图形化的终端模拟器 | alacritty, gnome-terminal | 用户态 |
| **Console** | 系统主控制台 | `/dev/console` | 内核 |
| **VT (Virtual Terminal)** | 虚拟终端 | `/dev/tty1` - `/dev/tty63` | 内核 |

**关系图**：
```
用户输入 → Keyboard → TTY Driver → Line Discipline → PTY Slave → Shell
                                           ↑                           ↓
Terminal Emulator ◄─────────────── PTY Master ◄────────────── 命令输出
```

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

### 2.2 核心数据结构

#### 2.2.1 `struct tty_struct` - TTY 实例

```c
struct tty_struct {
    int magic;              // TTY_STRUCT_MAGIC
    struct kref kref;       // 引用计数

    // 设备信息
    struct device *dev;     // 关联的设备
    struct tty_driver *driver;  // 所属的驱动
    const struct tty_operations *ops;  // 操作函数

    // 线路规程
    struct tty_ldisc *ldisc;    // 当前线路规程
    struct tty_ldisc_ops *ldisc_ops;

    // 缓冲区
    struct tty_port *port;      // TTY 端口

    // termios 设置
    struct ktermios termios;    // 当前终端设置
    struct ktermios termios_locked;

    // 进程组与会话
    pid_t pgrp;             // 前台进程组
    pid_t session;          // 会话 ID

    // 标志
    unsigned long flags;
    int count;              // 打开计数

    // 写缓冲
    struct tty_bufhead buf; // 写缓冲区

    // 窗口大小
    struct winsize winsize;

    // ... 更多字段
};
```

#### 2.2.2 `struct tty_driver` - TTY 驱动

```c
struct tty_driver {
    int magic;              // TTY_DRIVER_MAGIC
    struct kref kref;

    // 设备名称
    const char *driver_name;    // 驱动名称 (如 "serial")
    const char *name;           // 设备节点前缀 (如 "ttyS")
    int name_base;              // 起始编号

    // 设备号
    short major;            // 主设备号
    short minor_start;      // 起始次设备号
    short num;              // 支持的设备数量

    // 类型
    short type;             // TTY_DRIVER_TYPE_*
    short subtype;

    // 初始 termios
    struct ktermios init_termios;

    // 操作函数
    const struct tty_operations *ops;

    // 标志
    int flags;              // TTY_DRIVER_*

    // 内部链表
    struct tty_driver *next, *prev;
};
```

#### 2.2.3 `struct tty_operations` - 驱动操作

```c
struct tty_operations {
    // 生命周期
    int (*install)(struct tty_driver *driver, struct tty_struct *tty);
    int (*open)(struct tty_struct *tty, struct file *filp);
    void (*close)(struct tty_struct *tty, struct file *filp);
    void (*cleanup)(struct tty_struct *tty);

    // 数据读写
    ssize_t (*write)(struct tty_struct *tty, const u8 *buf, size_t count);
    int (*put_char)(struct tty_struct *tty, u8 ch);
    void (*flush_chars)(struct tty_struct *tty);
    unsigned int (*write_room)(struct tty_struct *tty);
    unsigned int (*chars_in_buffer)(struct tty_struct *tty);

    // 控制
    int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct tty_struct *tty, const struct ktermios *old);
    void (*throttle)(struct tty_struct *tty);
    void (*unthrottle)(struct tty_struct *tty);
    void (*stop)(struct tty_struct *tty);
    void (*start)(struct tty_struct *tty);

    // 硬件流控
    int (*tiocmget)(struct tty_struct *tty);
    int (*tiocmset)(struct tty_struct *tty, unsigned int set, unsigned int clear);

    // ... 更多
};
```

#### 2.2.4 `struct tty_ldisc` - 线路规程

```c
struct tty_ldisc_ops {
    // 生命周期
    int (*open)(struct tty_struct *tty);
    void (*close)(struct tty_struct *tty);

    // 数据处理
    ssize_t (*read)(struct tty_struct *tty, struct file *file,
                    u8 *buf, size_t nr, void **cookie, unsigned long offset);
    ssize_t (*write)(struct tty_struct *tty, struct file *file,
                     const u8 *buf, size_t nr);

    // 接收数据（从 driver 到 line discipline）
    void (*receive_buf)(struct tty_struct *tty, const u8 *cp,
                        const u8 *fp, size_t count);
    void (*write_wakeup)(struct tty_struct *tty);

    // ioctl
    int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);

    // 刷新
    void (*flush_buffer)(struct tty_struct *tty);

    // 名称和编号
    char *name;
    int num;
};

// 常用线路规程
#define N_TTY       0   // 默认，规范模式
#define N_SLIP      1   // SLIP 网络
#define N_MOUSE     2   // 鼠标
#define N_PPP       3   // PPP 网络
#define N_STRIP     4   // STRIP 网络
#define N_AX25      5   // AX.25 网络
// ...
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

### 2.4 关键组件详解

#### 2.4.1 Line Discipline (n_tty)

**核心职责**：
1. **行编辑**：处理退格、删除、Ctrl-W（删除单词）、Ctrl-U（删除行）
2. **回显控制**：是否回显输入字符（密码输入时关闭）
3. **规范模式 vs 原始模式**：
   - 规范模式（ICANON）：缓冲直到遇到换行
   - 原始模式：逐字符传递
4. **信号生成**：Ctrl+C → SIGINT，Ctrl+Z → SIGTSTP，Ctrl+\ → SIGQUIT
5. **流量控制**：Ctrl+S（停止输出），Ctrl+Q（恢复输出）

```c
// n_tty.c: 接收字符处理
static void n_tty_receive_char(struct tty_struct *tty, u8 c)
{
    // 处理特殊控制字符
    if (c == tty->termios.c_cc[VERASE]) {  // 退格
        // 处理退格逻辑
    } else if (c == tty->termios.c_cc[VWERASE]) {  // Ctrl-W
        // 删除单词逻辑 (处理 WERASE_CHAR)
    } else if (c == tty->termios.c_cc[VKILL]) {  // Ctrl-U
        // 删除整行
    } else if (c == tty->termios.c_cc[VINTR]) {  // Ctrl-C
        // 发送 SIGINT
    }
    // ...
}
```

#### 2.4.2 TTY Driver

**三种类型**：

| 类型 | 说明 | 驱动文件 | 设备节点 |
|------|------|----------|----------|
| **Console** | 系统主控制台 | vt.c, console.c | `/dev/console` |
| **Serial Port** | 串口设备 | 8250.c, serial_core.c | `/dev/ttyS*` |
| **PTY** | 伪终端 | pty.c | `/dev/ptmx`, `/dev/pts/*` |

#### 2.4.3 Terminal Emulator

**工作原理**：
1. 通过 `open("/dev/ptmx")` 获取 PTY master
2. `grantpt()` / `unlockpt()` 设置 slave 权限
3. `ptsname()` 获取 slave 设备名（如 `/dev/pts/2`）
4. `fork()` 子进程，`setsid()` 创建新会话
5. 子进程 `open(slave)`，执行 shell
6. 父进程通过 master fd 与 shell 通信
7. 处理 escape sequences（如 `\033[H\033[J` 清屏）

#### 2.4.4 Console

**多 Console 支持**：
```
内核启动参数: console=ttyS0 console=tty0
                    │              │
                    ▼              ▼
              Serial 输出      VGA 输出
                    │              │
                    └──────┬───────┘
                           ▼
                    /dev/console
                    (指向最后一个)
```

### 2.5 小结

- **分层设计**：TTY Core → Line Discipline → Driver → Hardware，每层职责明确
- **数据流向**：输出经过 Line Discipline 处理（行编辑、信号），输入经过缓冲和转发
- **抽象统一**：无论是物理串口、虚拟终端还是伪终端，都使用相同的 `tty_struct` 和操作接口

---

## 3. 实现细节 - 关键源码分析

### 3.1 TTY 核心层 (tty_io.c)

#### 3.1.1 设备打开流程

```c
// drivers/tty/tty_io.c
static int tty_open(struct inode *inode, struct file *filp)
{
    struct tty_struct *tty;
    int index = iminor(inode) - driver->minor_start;

    // 1. 查找或创建 tty_struct
    tty = tty_driver_lookup_tty(driver, inode, index);
    if (!tty) {
        tty = tty_init_dev(driver, index);
    }

    // 2. 调用线路规程的 open
    retval = tty_ldisc_open(tty);

    // 3. 调用驱动的 open
    if (tty->ops->open)
        retval = tty->ops->open(tty, filp);

    // 4. 设置 file->private_data
    filp->private_data = tty;

    return retval;
}
```

#### 3.1.2 写操作流程

```c
// drivers/tty/tty_io.c
static ssize_t tty_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct tty_struct *tty = file_tty(file);

    // 通过线路规程写入
    ld = tty_ldisc_ref_wait(tty);
    if (ld->ops->write)
        ret = ld->ops->write(tty, file, buf, count);
    tty_ldisc_deref(ld);

    return ret;
}

// 实际写操作
static inline ssize_t do_tty_write(
    ssize_t (*write)(struct tty_struct *, const u8 *, size_t),
    struct tty_struct *tty, const char __user *buf, size_t count)
{
    // 分配临时缓冲区
    char *kernel_buf = kmalloc(TTY_BUF_PAGE, GFP_KERNEL);

    // 循环写入
    while (count > 0) {
        size_t size = min(count, TTY_BUF_PAGE);
        copy_from_user(kernel_buf, buf, size);

        // 调用线路规程的 write
        ret = write(tty, kernel_buf, size);

        buf += ret;
        count -= ret;
    }

    kfree(kernel_buf);
    return total;
}
```

### 3.2 Line Discipline (n_tty.c)

#### 3.2.1 接收缓冲区处理

```c
// drivers/tty/n_tty.c
// 从 driver 接收数据
static void n_tty_receive_buf(struct tty_struct *tty,
                              const u8 *cp, const u8 *fp, size_t count)
{
    struct n_tty_data *ldata = tty->disc_data;

    // 处理每个字符
    while (count--) {
        u8 c = *cp++;
        u8 flag = fp ? *fp++ : TTY_NORMAL;

        // 处理错误标志
        if (flag == TTY_BREAK) {
            // 处理 break 信号
        } else if (flag == TTY_PARITY || flag == TTY_FRAME) {
            // 处理奇偶校验/帧错误
        }

        // 规范模式处理
        if (tty->termios.c_lflag & ICANON) {
            n_tty_receive_char_canon(tty, c);
        } else {
            n_tty_receive_char_raw(tty, c);
        }
    }

    // 唤醒等待读的进程
    wake_up_interruptible(&tty->read_wait);
}
```

#### 3.2.2 规范模式字符处理

```c
// 规范模式下处理单个字符
static void n_tty_receive_char_canon(struct tty_struct *tty, u8 c)
{
    struct n_tty_data *ldata = tty->disc_data;

    // 处理 ERASE (退格)
    if (c == ldata->termios.c_cc[VERASE]) {
        n_tty_erase_char(tty, 1);
        return;
    }

    // 处理 WERASE (Ctrl-W, 删除单词)
    if (c == ldata->termios.c_cc[VWERASE]) {
        n_tty_erase_word(tty);
        return;
    }

    // 处理 KILL (Ctrl-U, 删除整行)
    if (c == ldata->termios.c_cc[VKILL]) {
        n_tty_kill_line(tty);
        return;
    }

    // 处理 EOF (Ctrl-D)
    if (c == ldata->termios.c_cc[VEOF]) {
        // 设置 EOF 标志，但不存储字符
        return;
    }

    // 处理 EOL (行结束)
    if (c == '\n' || c == ldata->termios.c_cc[VEOL] ||
        c == ldata->termios.c_cc[VEOL2]) {
        // 标记行完成，可被读取
        set_bit(ldata->read_head % N_TTY_BUF_SIZE, ldata->read_flags);
    }

    // 处理信号字符
    if (c == ldata->termios.c_cc[VINTR]) {   // Ctrl-C
        tty_signal(tty, SIGINT);
        return;
    }
    if (c == ldata->termios.c_cc[VSUSP]) {   // Ctrl-Z
        tty_signal(tty, SIGTSTP);
        return;
    }

    // 普通字符：存储到读缓冲区
    put_tty_queue(c, ldata);

    // 回显（如果 ECHO 开启）
    if (L_ECHO(tty))
        n_tty_echo_char(tty, c);
}
```

#### 3.2.3 写操作（输出处理）

```c
// n_tty.c: 写操作
static ssize_t n_tty_write(struct tty_struct *tty, struct file *file,
                           const u8 *buf, size_t nr)
{
    const u8 *b = buf;

    // 处理 OPOST（输出处理）
    if (tty->termios.c_oflag & OPOST) {
        // 处理换行转换、制表符扩展等
        while (nr > 0) {
            char c = *b;

            // ONLCR: 将 \n 转换为 \r\n
            if (O_ONLCR(tty) && c == '\n') {
                process_output('\r', tty);
            }

            // OCRNL: 将 \r 转换为 \n
            if (O_OCRNL(tty) && c == '\r') {
                c = '\n';
            }

            process_output(c, tty);
            b++;
            nr--;
        }
    } else {
        // 原始输出
        process_output_block(tty, b, nr);
    }

    // 调用底层驱动的 write
    tty->ops->write(tty, buf, nr);

    return nr;
}
```

### 3.3 UART 驱动 (serial_core.c, 8250.c)

#### 3.3.1 UART 驱动结构

```c
// drivers/tty/serial/serial_core.c
// UART 操作结构
struct uart_ops {
    unsigned int (*tx_empty)(struct uart_port *);
    void (*set_mctrl)(struct uart_port *, unsigned int mctrl);
    unsigned int (*get_mctrl)(struct uart_port *);
    void (*stop_tx)(struct uart_port *);
    void (*start_tx)(struct uart_port *);
    void (*stop_rx)(struct uart_port *);
    void (*enable_ms)(struct uart_port *);
    void (*break_ctl)(struct uart_port *, int ctl);
    int (*startup)(struct uart_port *);
    void (*shutdown)(struct uart_port *);
    void (*flush_buffer)(struct uart_port *);
    void (*set_termios)(struct uart_port *, struct ktermios *new,
                        const struct ktermios *old);
    // ...
};

// 8250 UART 特定操作
static const struct uart_ops serial8250_pops = {
    .tx_empty   = serial8250_tx_empty,
    .get_mctrl  = serial8250_get_mctrl,
    .set_mctrl  = serial8250_set_mctrl,
    .stop_tx    = serial8250_stop_tx,
    .start_tx   = serial8250_start_tx,
    .stop_rx    = serial8250_stop_rx,
    .startup    = serial8250_startup,
    .shutdown   = serial8250_shutdown,
    .set_termios    = serial8250_set_termios,
    // ...
};
```

#### 3.3.2 串口写操作

```c
// drivers/tty/serial/8250/8250_port.c
static void serial8250_start_tx(struct uart_port *port)
{
    struct uart_8250_port *up = up_to_u8250p(port);

    // 启用发送中断
    if (!(up->ier & UART_IER_THRI)) {
        up->ier |= UART_IER_THRI;
        serial_port_out(port, UART_IER, up->ier);
    }
}

// 发送中断处理
static void serial8250_tx_chars(struct uart_8250_port *up)
{
    struct uart_port *port = &up->port;
    struct circ_buf *xmit = &port->state->xmit;
    int count;

    // 从循环缓冲区发送字符
    count = up->tx_loadsz;
    do {
        // 检查发送 FIFO 是否满
        if (!(serial_port_in(port, UART_LSR) & UART_LSR_THRE))
            break;

        // 发送字符
        serial_port_out(port, UART_TX, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;

        if (uart_circ_empty(xmit))
            break;
    } while (--count > 0);

    // 如果发送完成，禁用发送中断
    if (uart_circ_empty(xmit))
        serial8250_stop_tx(port);
}
```

#### 3.3.3 串口接收中断

```c
// 接收中断处理
static void serial8250_rx_chars(struct uart_8250_port *up, u8 lsr)
{
    struct uart_port *port = &up->port;
    u8 ch;
    char flag;

    do {
        // 读取接收到的字符
        ch = serial_port_in(port, UART_RX);
        flag = TTY_NORMAL;

        // 检查错误状态
        if (lsr & UART_LSR_BI) {  // Break
            flag = TTY_BREAK;
        } else if (lsr & UART_LSR_PE) {  // Parity Error
            flag = TTY_PARITY;
        } else if (lsr & UART_LSR_FE) {  // Framing Error
            flag = TTY_FRAME;
        } else if (lsr & UART_LSR_OE) {  // Overrun
            port->icount.overrun++;
        }

        // 将字符推送到 TTY 层
        uart_insert_char(port, lsr, UART_LSR_OE, ch, flag);

        lsr = serial_port_in(port, UART_LSR);
    } while (lsr & UART_LSR_DR);  // 数据就绪

    // 刷新到 line discipline
    tty_flip_buffer_push(&port->state->port);
}
```

### 3.4 PTY 实现 (pty.c)

#### 3.4.1 PTY 数据结构

```c
// drivers/tty/pty.c
struct pty_struct {
    struct tty_struct *tty;     // 关联的 tty
    struct tty_struct *link;    // 另一端的 tty（master <-> slave）

    // 对于 PTY master
    struct mutex mu;            // 互斥锁
    struct packetized_cnt *pckt;

    // 对于 PTY slave
    struct work_struct fold;    // 用于 SIGHUP
};

// PTY 操作
static const struct tty_operations ptm_unix98_ops = {
    .open = pty_open,
    .close = pty_close,
    .write = pty_write,
    .write_room = pty_write_room,
    .flush_buffer = pty_flush_buffer,
    .chars_in_buffer = pty_chars_in_buffer,
    .unthrottle = pty_unthrottle,
    .set_termios = pty_set_termios,
    .ioctl = pty_ioctl,
    // ...
};

static const struct tty_operations pty_unix98_ops = {
    .open = pty_open,
    .close = pty_close,
    .write = pty_write,
    .write_room = pty_write_room,
    .flush_buffer = pty_flush_buffer,
    .chars_in_buffer = pty_chars_in_buffer,
    .unthrottle = pty_unthrottle,
    .set_termios = pty_set_termios,
    .ioctl = pty_ioctl,
    // ...
};
```

#### 3.4.2 PTY 写操作（转发到另一端）

```c
// PTY 写操作：将数据从一端转发到另一端
static ssize_t pty_write(struct tty_struct *tty, const u8 *buf, size_t count)
{
    struct pty_struct *pty = tty->driver_data;
    struct tty_struct *to = pty->link;  // 另一端的 tty

    // 如果另一端已关闭，返回错误
    if (!to)
        return -EIO;

    // 将数据写入到另一端的缓冲区
    // 对于 master -> slave：数据进入 slave 的 read buffer
    // 对于 slave -> master：数据进入 master 的 read buffer
    return tty_insert_flip_string(to->port, buf, count);
}

// 唤醒读等待队列
static void pty_unthrottle(struct tty_struct *tty)
{
    struct pty_struct *pty = tty->driver_data;

    // 唤醒另一端的写等待
    if (pty->link)
        tty_wakeup(pty->link);
}
```

### 3.5 VT 实现 (vt/)

#### 3.5.1 VT 控制台结构

```c
// drivers/tty/vt/vt.c
struct vc_data {
    unsigned int vc_num;        // 控制台编号 (0-63)

    // 显示
    unsigned short *vc_screenbuf;   // 屏幕缓冲区
    unsigned int vc_screenbuf_size;
    unsigned int vc_cols;       // 列数
    unsigned int vc_rows;       // 行数

    // 光标位置
    unsigned int vc_x;          // 列
    unsigned int vc_y;          // 行

    // 颜色
    unsigned char vc_def_color;
    unsigned char vc_color;

    // 滚动
    unsigned int vc_top;        // 滚动区域顶部
    unsigned int vc_bottom;     // 滚动区域底部

    // 显示标志
    unsigned int vc_utf:1;
    unsigned int vc_disp_ctrl:1;
    unsigned int vc_toggle_meta:1;
    unsigned int vc_decckm:1;   // 光标键模式
    unsigned int vc_decawm:1;   // 自动换行

    // 与键盘的关系
    struct kbd_struct *vc_kbd;

    // 与 framebuffer 的关系
    struct fb_info *vc_fb_info;
};

// 当前激活的虚拟控制台
struct vc_data *vc_cons[MAX_NR_CONSOLES];
struct vc_data *fg_console;  // 前台控制台
```

#### 3.5.2 键盘事件处理

```c
// drivers/tty/vt/keyboard.c
// 键盘事件入口
void kbd_event(struct input_handle *handle, struct input_event *event)
{
    // 过滤事件类型
    if (event->type != EV_KEY)
        return;

    // 调用核心处理函数
    kbd_keycode(event->code, event->value, event->dev);
}

// 按键码处理
static void kbd_keycode(unsigned int keycode, int down, struct input_dev *dev)
{
    struct vc_data *vc = vc_cons[fg_console->vc_num];
    u16 keysym;
    u8 type;

    // 将扫描码转换为键码
    keysym = key_map[vc->vc_kbd->lockstate][keycode];
    type = KTYP(keysym);

    // 根据类型分发处理
    if (type == KT_LETTER || type == KT_LATIN) {
        // 字母/拉丁字符
        put_queue(vc, keysym & 0xff);
    } else if (type == KT_FN) {
        // 功能键 (F1-F12)
        fn_handler[keysym & 0xf](vc);
    } else if (type == KT_SPEC) {
        // 特殊键 (Enter, Backspace 等)
        k_spec(vc, keysym & 0xff, down);
    } else if (type == KT_CUR) {
        // 光标键 (方向键)
        // 发送 escape sequence
    } else if (type == KT_SHIFT) {
        // Shift/Ctrl/Alt
        // 更新修饰键状态
    }
    // ...
}

// 特殊键处理（SysRq 等）
static void k_spec(struct vc_data *vc, u8 value, int down)
{
    if (!down)
        return;

    switch (value) {
    case K_ENTER:  // Enter
        put_queue(vc, 13);
        if (vc->vc_decawm)
            cr(vc);
        lf(vc);
        break;
    case K_BS:     // Backspace
        bs(vc);
        break;
    case K_TAB:    // Tab
        put_queue(vc, 9);
        break;
    case K_CAPS:   // Caps Lock
        chg_vc_kbd_lock(kbd_table, VC_CAPSLOCK);
        break;
    case K_HOLD:   // Scroll Lock
        scrolllock(vc);
        break;
    // ... SysRq 处理
    }
}
```

### 3.6 关键函数调用链总结

| 操作 | 用户态 | VFS | TTY Core | Line Discipline | Driver | Hardware |
|------|--------|-----|----------|-----------------|--------|----------|
| **打开** | `open()` | → | `tty_open()` | `ldisc->open()` | `ops->open()` | - |
| **写** | `write()` | → | `tty_write()` | `n_tty_write()` | `uart_write()` | UART TX |
| **读** | `read()` | → | `tty_read()` | `n_tty_read()` | - | - |
| **接收** | - | - | - | `n_tty_receive_buf()` | IRQ handler | UART RX |
| **ioctl** | `ioctl()` | → | `tty_ioctl()` | `ldisc->ioctl()` | `ops->ioctl()` | - |

### 3.7 小结

- **TTY Core**：设备管理、数据路由、接口统一
- **n_tty**：行编辑、规范模式、信号生成
- **UART Driver**：硬件寄存器操作、中断处理
- **PTY**：主从设备对、数据转发
- **VT**：键盘输入处理、帧缓冲区显示

---

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

**内核配置**：
```
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
CONFIG_SERIAL_CORE=y
```

### 4.2 虚拟终端切换 (Ctrl+Alt+F1~F6)

**场景**：无图形界面时的多控制台

```bash
# 查看当前 VT
$ fgconsole
1

# 列出所有 VT
$ ls /dev/tty[0-9]*
/dev/tty0  /dev/tty1  /dev/tty2  /dev/tty3  /dev/tty4  /dev/tty5  /dev/tty6

# 向特定 VT 发送消息
$ sudo echo "hello tty2" > /dev/tty2

# 查看当前登录用户
$ who
martins3 tty1         2024-01-15 08:30
martins3 pts/0        2024-01-15 09:00 (192.168.1.100)
```

**内核配置**：
```
CONFIG_VT=y
CONFIG_VT_CONSOLE=y
CONFIG_VGA_CONSOLE=y
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

#### SysRq 命令

```
R - 打开键盘原始模式 (Raw)
E - 发送 SIGTERM 给所有进程 (tErminate)
I - 发送 SIGKILL 给所有进程 (kIll)
S - 同步所有挂载的文件系统 (Sync)
U - 以只读方式重新挂载所有文件系统 (Unmount)
B - 立即重启系统 (reBoot)

M - 显示内存信息
P - 显示当前寄存器/堆栈
T - 显示任务列表
W - 显示阻塞的任务
L - 显示所有 CPU 的堆栈回溯
```

**触发方式**：
```
物理机/BMC: Alt + SysRq + <key>
QEMU:       Ctrl+Alt+2 进入 monitor, 输入: sendkey alt-sysrq-<key>
串口:       发送 break 信号后按 <key>
/proc:      echo <key> | sudo tee /proc/sysrq-trigger
```

### 5.5 TTY 相关 sysfs 接口

```bash
# 查看 TTY 设备
$ ls /sys/class/tty/
console  ptmx  tty  tty0  tty1  tty2  ...  ttyS0  ttyS1  ...

# 查看 TTY 驱动
$ cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master

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

#### 实验 4：QEMU 串口调试

```bash
# 启动带串口的 QEMU
$ qemu-system-x86_64 \
    -kernel bzImage \
    -append "console=ttyS0,115200 nokaslr" \
    -serial mon:stdio \
    -display none

# 在另一个窗口连接
$ socat -,raw,echo=0 unix-connect:qemu-serial.sock
```

### 6.3 参考资料优先级

| 优先级 | 资料 | 说明 |
|--------|------|------|
| ★★★★★ | `drivers/tty/*.c` 源码 | 第一手资料，必读 |
| ★★★★★ | LWN TTY 系列文章 | 深度分析，高质量 |
| ★★★★☆ | LDD3 Chapter 18 | 经典教材，略有过时 |
| ★★★★☆ | The TTY Demystified | 概念讲解极佳 |
| ★★★☆☆ | Linux 手册页 (man tty) | 快速参考 |
| ★★★☆☆ | TLPI Chapter 62 | 用户态视角 |

### 6.4 关键源码文件索引

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

### 6.5 学习检查清单

- [ ] 理解 TTY 与 Shell、Terminal Emulator 的区别
- [ ] 能够解释 Line Discipline 的作用
- [ ] 理解 PTY Master/Slave 的工作原理
- [ ] 会使用 stty 设置串口参数
- [ ] 能够跟踪内核 TTY 调用链
- [ ] 理解 VT 切换原理
- [ ] 能够配置 QEMU 串口
- [ ] 会使用 SysRq 调试

### 6.6 小结

TTY 子系统是 Linux 内核中最古老但也最核心的部分之一。掌握它需要：

1. **理解历史**：从物理电传机到虚拟终端的演进
2. **分层思考**：Core → Line Discipline → Driver 的清晰分层
3. **动手实验**：通过 QEMU、ftrace 等工具验证理解
4. **阅读源码**：从 `tty_io.c` 和 `n_tty.c` 开始

---

## 附录：核心数据结构速查

### tty_struct 关键字段

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

### termios 标志位

```c
// c_iflag: 输入模式
IGNBRK  // 忽略 break
BRKINT  // break 产生 SIGINT
IGNPAR  // 忽略帧/奇偶错误
INPCK   // 启用输入校验
ISTRIP  // 剥离第 8 位
INLCR   // 将 NL 映射为 CR
IGNCR   // 忽略 CR
ICRNL   // 将 CR 映射为 NL
IXON    // 启用 XON/XOFF 输出流控
IXOFF   // 启用 XON/XOFF 输入流控

// c_oflag: 输出模式
OPOST   // 启用输出处理
ONLCR   // 将 NL 映射为 CR-NL
OCRNL   // 将 CR 映射为 NL
ONOCR   // 第 0 列不输出 CR
ONLRET  // NL 执行 CR 功能

// c_cflag: 控制模式
CSIZE   // 字符大小掩码
    CS5, CS6, CS7, CS8
CSTOPB  // 2 个停止位
CREAD   // 启用接收
PARENB  // 启用校验
PARODD  // 奇校验
HUPCL   // 最后关闭时挂起
CLOCAL  // 忽略调制解调器状态线

// c_lflag: 本地模式
ISIG    // 启用信号 (INTR, QUIT, SUSP)
ICANON  // 启用规范模式
ECHO    // 回显输入字符
ECHOE   // 回显擦除字符为 BS-SP-BS
ECHOK   // 在 KILL 字符后回显 NL
ECHONL  // 即使 ECHO 关闭也回显 NL
NOFLSH  // 信号产生时不刷新输入输出
IEXTEN  // 启用扩展实现定义函数
```

### 特殊控制字符 (c_cc)

| 索引 | 名称 | 默认 | 说明 |
|------|------|------|------|
| VINTR | Ctrl+C | `^C` | 发送 SIGINT |
| VQUIT | Ctrl+\ | `^\` | 发送 SIGQUIT |
| VERASE | Backspace | `^?` | 擦除前一个字符 |
| VKILL | Ctrl+U | `^U` | 擦除整行 |
| VEOF | Ctrl+D | `^D` | 文件结束 |
| VEOL | - | - | 行结束（备用）|
| VSTART | Ctrl+Q | `^Q` | 恢复输出 |
| VSTOP | Ctrl+S | `^S` | 停止输出 |
| VSUSP | Ctrl+Z | `^Z` | 发送 SIGTSTP |
| VWERASE | Ctrl+W | `^W` | 擦除单词 |

---

*文档生成日期：2026-02-25*
*基于 Linux Kernel 6.x 分析*

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
