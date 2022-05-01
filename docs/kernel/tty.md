## tty

运行一个简单[^1]的 Hello World ，CPU 的大多数时间在干什么，使用 [perf](../tips-reading-kernel.md) 可以看一下:

![](./img/printf.svg)

除了 Hello World 程序之外，还发现 alacritty 和 tmux

层次结构:
```c
static const struct file_operations tty_fops = {
	.llseek		= no_llseek,
	.read_iter	= tty_read,
	.write_iter	= tty_write,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
	.show_fdinfo	= tty_show_fdinfo,
};
```

```c
static const struct tty_operations ptm_unix98_ops = {
	.lookup = ptm_unix98_lookup,
	.install = pty_unix98_install,
	.remove = pty_unix98_remove,
	.open = pty_open,
	.close = pty_close,
	.write = pty_write,
	.write_room = pty_write_room,
	.flush_buffer = pty_flush_buffer,
	.unthrottle = pty_unthrottle,
	.ioctl = pty_unix98_ioctl,
	.compat_ioctl = pty_unix98_compat_ioctl,
	.resize = pty_resize,
	.cleanup = pty_cleanup,
	.show_fdinfo = pty_show_fdinfo,
};
```

- [ ] `tty_io.c` 在搞什么 ?

## [x] `hack/lab/tty/gen_rand.c` 对于 tty 输出就是当前的输出
https://unix.stackexchange.com/questions/60641/linux-difference-between-dev-console-dev-tty-and-dev-tty0

## [ ] 还有很多事情没有分析，比如 reverse shell

## [x] 在 emulator 上输出一个字符，那么这个字符最后是如何传输给上面的 bash 的

从 tty 的角度理解一下也可以的

## 理解 line decspline 一下
- 无法理解 line decspline 的问题

在内核中，是 `WERASE_CHAR`

## [x] tmux 是如何实现的
https://www.quora.com/How-do-I-understand-the-tmux-design-architecture-and-internals

## 这应该完全理解 pty 的所有的知识的地方
https://www.uninformativ.de/blog/postings/2018-02-24/0/POSTING-en.html
- 似乎 master 和 slave 就是不存在这种问题的


关于 tty 的疑问:
- tmux 是如何实现的?
- pty 和 pts 是什么 ?
- 和 ssh 如何实现的啊 ?
- 在 /dev/ 为什么存在那么多个 tty1/2/3
- cttyhack
- vim 中为什么可以创建一个 terminal 啊
- 让我奇怪的地方在于，类似

## 一些有趣的实验

### 利用 tty 在当前 terminal 上回显
```txt
➜  vn git:(master) ✗ tty
/dev/pts/2

➜  vn git:(master) ✗ strace tty
ioctl(0, TCGETS, {B38400 opost isig icanon echo ...}) = 0
fstat(0, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
readlink("/proc/self/fd/0", "/dev/pts/2", 4095) = 10
stat("/dev/pts/2", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
write(1, "/dev/pts/2\n", 11/dev/pts/2
)            = 11
close(1)                                = 0
close(2)                                = 0
exit_group(0)                           = ?
+++ exited with 0 +++
➜  vn git:(master) ✗ l /proc/self/fd/0
lrwx------ maritns3 maritns3 64 B Sat Apr 30 13:10:54 2022  /proc/self/fd/0 ⇒ /dev/pts/2
➜  vn git:(master) ✗ l /proc/self/fd/1
lrwx------ maritns3 maritns3 64 B Sat Apr 30 13:11:05 2022  /proc/self/fd/1 ⇒ /dev/pts/2
➜  vn git:(master) ✗ l /proc/self/fd
dr-x------ maritns3 maritns3  0 B Sat Apr 30 13:11:16 2022  .
dr-xr-xr-x maritns3 maritns3  0 B Sat Apr 30 13:11:16 2022  ..
lrwx------ maritns3 maritns3 64 B Sat Apr 30 13:11:16 2022  0 ⇒ /dev/pts/2
lrwx------ maritns3 maritns3 64 B Sat Apr 30 13:11:16 2022  1 ⇒ /dev/pts/2
lrwx------ maritns3 maritns3 64 B Sat Apr 30 13:11:16 2022  2 ⇒ /dev/pts/2
lr-x------ maritns3 maritns3 64 B Sat Apr 30 13:11:16 2022  3 ⇒ /proc/50332/fd
➜  vn git:(master) ✗ echo 11111111111 > /dev/pts/2
11111111111
```
- [x] 对于上面的实验，找到内核中，这个 Linux 的实现，将 /proc/self/fd/1 的软链接的建立
- [x] 回顾一下 musl 是如何实现标准输入，输出之类的

### stty 的实现
```plain
ioctl(0, TCGETS, {B38400 opost isig icanon echo ...}) = 0
ioctl(1, TIOCGWINSZ, {ws_row=73, ws_col=284, ws_xpixel=0, ws_ypixel=0}) = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
ioctl(0, TIOCGWINSZ, {ws_row=73, ws_col=284, ws_xpixel=0, ws_ypixel=0}) = 0
write(1, "speed 38400 baud; rows 73; colum"..., 50speed 38400 baud; rows 73; columns 284; line = 0;
```

### [x] 在 minicom 上，是如果通过 CTRL-C 杀掉被调试的程序流程

### [x] 向 ttyS0 中 echo 的基本流程是什么样子的

### [x] /dev/pts/dev 是如何创建出来的
- `ptmx_open`
- 同时检查 tty 的程序

## 分析一下内核
- tiocswinsz : 取决于 `tty_struct` 是否实现，如果没有，似乎就是普通的 emulator 就是发送一个信号过去

## 分析一下 这个三个 `file_operations` 的使用位置和差异

```c
static const struct file_operations tty_fops = {
	.llseek		= no_llseek,
	.read_iter	= tty_read,
	.write_iter	= tty_write,
	.splice_read	= generic_file_splice_read,
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
	.llseek		= no_llseek,
	.read_iter	= tty_read,
	.write_iter	= redirected_tty_write,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
};

static const struct file_operations hung_up_tty_fops = {
	.llseek		= no_llseek,
	.read_iter	= hung_up_tty_read,
	.write_iter	= hung_up_tty_write,
	.poll		= hung_up_tty_poll,
	.unlocked_ioctl	= hung_up_tty_ioctl,
	.compat_ioctl	= hung_up_tty_compat_ioctl,
	.release	= tty_release,
	.fasync		= hung_up_tty_fasync,
};
```
- 其中，/dev/console 就是给 `console_fops` 的
- [ ] /dev/console 和 /dev/tty0 之类区别是什么？
  - https://unix.stackexchange.com/questions/60641/linux-difference-between-dev-console-dev-tty-and-dev-tty0
  - https://www.baeldung.com/linux/monitor-keyboard-drivers

## [ ] job control 之类还和 tty 有关的，似乎 setsid 之类的也是从来没有搞清楚过

## 似乎没有太搞清楚过 slave 和 master device 的含义啊

## 使用 flame graph 一个最简单的 printf hello world
其中的 tty 选择的是这个:
- drivers/tty/pty.c

- 发现当前终端的数量和 /dev/pts/ 下的文件的数量是完全对应的

## TODO
- [ ] TTY 到底是什么？https://www.kawabangga.com/posts/4515

简单的构建一个 busybox 之后，其 shell 有这个警告:
```c
/bin/sh: can't access tty; job control turned off
```

## pty pts tty
- https://unix.stackexchange.com/questions/21280/difference-between-pts-and-tty/21294
- https://stackoverflow.com/questions/4426280/what-do-pty-and-tty-mean

- tty : teletype. Usually refers to the serial ports of a computer, to which terminals were attached.
- tpy : pseudo tty
- pts : psuedo terminal slave (an xterm or an ssh connection).
> 是的，还是让人非常的疑惑啊!

## tty
- [ ] line ldisc

```c
static struct tty_ldisc_ops n_tty_ops = {
  .magic           = TTY_LDISC_MAGIC,
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
  .receive_buf2  = n_tty_receive_buf2,
};
```

## 下面是阅读 ldd3 的 tiny_serial.c 和 tiny_tty.c 的结果

### tiny_serial.c
- [x] TINY_SERIAL_MAJOR 的数值设置不要和 /proc/devices 的数值冲突了

uart_register_driver + uart_add_one_port

insmod tiny_serial.ko 之后 ： 多出来了 devices/virtual/tty/ttytiny0

```c
➜  ttytiny0 tree
.
├── close_delay
├── closing_wait
├── custom_divisor
├── dev
├── flags
├── iomem_base
├── iomem_reg_shift
├── io_type
├── irq
├── line
├── port
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   ├── runtime_usage
│   ├── wakeup
│   ├── wakeup_abort_count
│   ├── wakeup_active
│   ├── wakeup_active_count
│   ├── wakeup_count
│   ├── wakeup_expire_count
│   ├── wakeup_last_time_ms
│   ├── wakeup_max_time_ms
│   └── wakeup_total_time_ms
├── subsystem -> ../../../../class/tty
├── type
├── uartclk
├── uevent
└── xmit_fifo_size
```

- [ ] 所以如何使用呀 ?
- [ ] termios 的作用 ?

insmod tiny_tty.ko
```c
devices/virtual/tty/ttty0
devices/virtual/tty/ttty1
devices/virtual/tty/ttty2
devices/virtual/tty/ttty3
```

为什么都需要 set_termios 之类的东西 ?

从 /dev/tty 看， tiny_serial 和 tiny_tty 无区别 ?

## serial
https://en.wikibooks.org/wiki/Serial_Programming
- https://www.kernel.org/doc/html/latest/admin-guide/serial-console.html : 似乎可以找到为什么 qemu 运行 linux 在 host 终端里面有消息了
- https://unix.stackexchange.com/questions/60641/linux-difference-between-dev-console-dev-tty-and-dev-tty0 : 对比 /dev/console /dev/tty /dev/ttyS0

- [ ] Documentation/driver-api/serial/driver.rst

- [ ] /home/maritns3/core/linux/drivers/tty/serial/8250/8250_core.c
  - [ ] port 指的是 ?
  - [x] 对比 16550, 找到证据 都是 uart 的一种。(虽然没有对比 16550，但是分析 uart_ops 的注册函数的赋值就可以知道，实际上的 uart 设备比想想的多很多)

[^1]: 为了让 Hello World 程序更加明显，最好是循环输出多次，例如 100000 。
