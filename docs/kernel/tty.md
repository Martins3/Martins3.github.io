## tty

## TODO
- [ ] TTY 到底是什么？https://www.kawabangga.com/posts/4515

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

uart_add_one_port ==> uart_configure_port

serial8250_request_port ==> serial8250_request_std_resource ==> request_mem_region(`port->membase` 初始化)

- [ ] eldd chapter 6

```c
static const struct uart_ops serial8250_pops = {
  .tx_empty = serial8250_tx_empty,
  .set_mctrl  = serial8250_set_mctrl,
  .get_mctrl  = serial8250_get_mctrl,
  .stop_tx  = serial8250_stop_tx,
  .start_tx = serial8250_start_tx,
  .throttle = serial8250_throttle,
  .unthrottle = serial8250_unthrottle,
  .stop_rx  = serial8250_stop_rx,
  .enable_ms  = serial8250_enable_ms,
  .break_ctl  = serial8250_break_ctl,
  .startup  = serial8250_startup,
  .shutdown = serial8250_shutdown,
  .set_termios  = serial8250_set_termios,
  .set_ldisc  = serial8250_set_ldisc,
  .pm   = serial8250_pm,
  .type   = serial8250_type,
  .release_port = serial8250_release_port,
  .request_port = serial8256_request_port,
  .config_port  = serial8250_config_port,
  .verify_port  = serial8250_verify_port,
#ifdef CONFIG_CONSOLE_POLL
  .poll_get_char = serial8250_get_poll_char,
  .poll_put_char = serial8250_put_poll_char,
#endif
};
```
- [ ] 使用 serial8250_stop_tx 为例子:
  - 参数是 uart_port
  - `__stop_tx`
    - `__do_stop_tx`
      - serial8250_clear_THRI : 向 8250 的 UART_IER 寄存器写入 UART_IER_THRI (**微机原理与接口**原理真不错)
        - [ ] 从这里来说，port 才是真正在直接控制具体的 8250 芯片，而 8250_core.c 处理是 platform_driver 之类的事情吧！
```c
static inline void serial_out(struct uart_8250_port *up, int offset, int value)
{
  up->port.serial_out(&up->port, offset, value);
}
```



```c
static struct uart_driver serial8250_reg = {
  .owner      = THIS_MODULE,
  .driver_name    = "serial",
  .dev_name   = "ttyS",
  .major      = TTY_MAJOR,
  .minor      = 64,
  .cons     = SERIAL8250_CONSOLE,
};

static struct console univ8250_console = {
  .name   = "ttyS",
  .write    = univ8250_console_write,
  .device   = uart_console_device,
  .setup    = univ8250_console_setup,
  .exit   = univ8250_console_exit,
  .match    = univ8250_console_match,
  .flags    = CON_PRINTBUFFER | CON_ANYTIME,
  .index    = -1,
  .data   = &serial8250_reg,
};

static struct platform_driver serial8250_isa_driver = {
  .probe    = serial8250_probe,
  .remove   = serial8250_remove,
  .suspend  = serial8250_suspend,
  .resume   = serial8250_resume,
  .driver   = {
    .name = "serial8250",
  },
};

/*
 * This "device" covers _all_ ISA 8250-compatible serial devices listed
 * in the table in include/asm/serial.h
 */
static struct platform_device *serial8250_isa_devs;
```

```c
static int __init serial8250_init(void)

int uart_register_driver(struct uart_driver *drv)
```

- 在 serial_core.c 中间，几乎所有的函数都是 `uart_*` 的，所以 uart 是 serial 的协议基础:

- [ ] serial8250_isa_devs 和 serial8250_reg 的关系是什么 ?
