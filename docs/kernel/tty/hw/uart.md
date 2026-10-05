# uart
必须将 CONFIG_SERIAL_8250 配置为 yes ，
否则启动时候 console=ttyS0 没有效果，但是 console=tty0 不受影响。

也是由于 SERIAL_8250_CONSOLE 对于 `CONFIG_SERIAL_8250=y` 有依赖
```txt
config SERIAL_8250_CONSOLE
	bool "Console on 8250/16550 and compatible serial port"
	depends on SERIAL_8250=y
	select SERIAL_CORE_CONSOLE
	select SERIAL_EARLYCON
```

如果把 SERIAL_8250=m ，那么将会有如下的 module 消失了。
```txt
kernel/drivers/tty/serial/serial_base.ko
kernel/drivers/tty/serial/8250/8250.ko
kernel/drivers/tty/serial/8250/8250_base.ko
kernel/drivers/tty/serial/8250/8250_exar.ko
kernel/drivers/tty/serial/8250/8250_lpss.ko
kernel/drivers/tty/serial/8250/8250_mid.ko
kernel/drivers/tty/serial/8250/8250_pci.ko
kernel/drivers/tty/serial/8250/8250_pericom.ko

kernel/drivers/misc/eeprom/eeprom_93cx6.ko
kernel/lib/fonts/font.ko
kernel/lib/math/rational.ko
```

总体符合预期，就是两个路径。

## 这个是做什么的?
drivers/usb/serial/bus.c

## 那么 serial over lan 是什么?

## 在 qemu 的 stdio 中 dmesg ，最后可以触发这个错误

irq4 ，如何分配的
```txt
[  419.999920] link port-storage as upper device of eth2
[  448.267378] serial8250: too much work for irq4
[  448.403917] serial8250: too much work for irq4
[  448.541998] serial8250: too much work for irq4
[  448.696771] serial8250: too much work for irq4
[  448.860427] serial8250: too much work for irq4
[  449.020484] serial8250: too much work for irq4
[  449.155814] serial8250: too much work for irq4
[  449.295326] serial8250: too much work for irq4
[  449.428446] serial8250: too much work for irq4
[  449.582178] serial8250: too much work for irq4
[  455.103230] serial8250_interrupt: 1 callbacks suppressed
[  455.103235] serial8250: too much work for irq4
[  455.259182] serial8250: too much work for irq4
[  455.398221] serial8250: too much work for irq4
[  455.538640] serial8250: too much work for irq4
[  455.693459] serial8250: too much work for irq4
[  455.846877] serial8250: too much work for irq4
[  456.000410] serial8250: too much work for irq4
[  456.153424] serial8250: too much work for irq4
[  456.306691] serial8250: too much work for irq4
[  456.457983] serial8250: too much work for irq4
```

想不到注册中断是后面进行的:
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_openat
        - __se_sys_openat
          - __do_sys_openat
            - do_sys_open
              - do_sys_openat2
                - do_filp_open
                  - path_openat
                    - do_open
                      - vfs_open
                        - do_dentry_open
                          - chrdev_open
                            - tty_open
                              - uart_open
                                - tty_port_open
                                  - uart_port_activate
                                    - uart_startup
                                      - uart_startup
                                        - uart_port_startup
                                          - serial8250_do_startup
                                            - univ8250_setup_irq
                                              - serial_link_irq_chain
                                                - request_irq
                                                  - request_threaded_irq
                                                    - __setup_irq
                                                      - irq_setup_forced_threading

## 原来还有这么多的设备

• 有。对于你目前使用的 x86 PC VM，主要还有 PCI 串口和 USB 串口。 我检查了本地 QEMU 的 -device help，这些都已编译支持：

   设备名           模拟的硬件                                          Linux guest 中的典型设备
  ━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━
   pci-serial       PCI 单口 16550 UART                                 /dev/ttyS*
  ───────────────  ──────────────────────────────────────────────────  ──────────────────────────
   pci-serial-2x    PCI 双口 16550 UART                                 两个 /dev/ttyS*
  ───────────────  ──────────────────────────────────────────────────  ──────────────────────────
   pci-serial-4x    PCI 四口 16550 UART                                 四个 /dev/ttyS*
  ───────────────  ──────────────────────────────────────────────────  ──────────────────────────
   usb-serial       FTDI FT232BM USB 转串口                             /dev/ttyUSB*
  ───────────────  ──────────────────────────────────────────────────  ──────────────────────────
   ipoctal232       IndustryPack 八口 RS-232 模块，需要 tpci200 载板    依赖对应驱动

  pci-serial 和 isa-serial 使用同一套 UART 核心实现，区别主要是设备发现方式、寄存器地址分配和中断连接：前者通过 PCI 枚举、BAR 分配地址，后者挂在 ISA
  总线上，通常使用 0x3f8 等固定地址。QEMU PCI 串口文档 (https://qemu.readthedocs.io/en/master/specs/pci-serial.html)

  例如添加一个 PCI 串口：

  -chardev pty,id=pci_uart
  -device pci-serial,chardev=pci_uart

  添加一个双口 PCI 串口卡：

  -chardev pty,id=uart_a
  -chardev pty,id=uart_b
  -device pci-serial-2x,chardev1=uart_a,chardev2=uart_b

  添加 USB 串口，假设已有名为 usb 的 USB 控制器：

  -chardev pty,id=usb_uart
  -device usb-serial,bus=usb.0,chardev=usb_uart

  这里的 /dev/ttyS*、/dev/ttyUSB* 编号由 guest 驱动分配，不能保证从 0 开始。

  如果范围扩大到其他架构，QEMU 还模拟了很多 machine 自带的 UART，例如：

   UART 类型                               典型平台
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   PL011                                   ARM virt，Linux 中通常是 /dev/ttyAMA*
  ──────────────────────────────────────  ───────────────────────────────────────
   MMIO 16550                              RISC-V virt 等
  ──────────────────────────────────────  ───────────────────────────────────────
   i.MX UART、Cadence UART、STM32 USART    对应 ARM SoC/开发板
  ──────────────────────────────────────  ───────────────────────────────────────
   SiFive UART                             对应 SiFive RISC-V 开发板
  ──────────────────────────────────────  ───────────────────────────────────────
   Zilog ESCC                              部分 SPARC、PowerPC 平台

  这些通常由 machine 创建并配置地址、中断，使用 -serial 指定后端；不能直接把它们当作 PCI 设备添加到 x86 VM。例如 ARM virt 的平台串口就是 PL011。ARM virt
  文档 (https://www.qemu.org/docs/master/system/arm/virt)

  另外，你的 VM 参数里还有 isa-debugcon：它是供固件等使用的简单调试输出端口，不模拟完整 UART，也不会对应 guest 的 /dev/ttyS*。


## 8250 是什么东西?
8250 是一个经典 UART 芯片的型号，源自早期 IBM PC 使用的 8250 UART。所以 CONFIG_SERIAL_8250 的含义是：启用 8250 及
兼容 UART 控制器的驱动。

这里的数字既不是端口号，也不是波特率。

后续芯片延续了它的寄存器接口，并增加功能：

 UART 型号       主要特点
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 8250            早期 UART，没有收发 FIFO
──────────────  ──────────────────────────────────
 16450           改进型号，没有收发 FIFO
──────────────  ──────────────────────────────────
 16550           引入 FIFO，但早期实现有缺陷
──────────────  ──────────────────────────────────
 16550A          修复 FIFO，提供 16 字节收发 FIFO
──────────────  ──────────────────────────────────
 后续兼容型号    更深的 FIFO，以及其他扩展功能

因为这些型号的寄存器布局和基本操作兼容，Linux 把它们归到 8250 驱动家族中，代码位于 drivers/tty/serial/8250/。现代
芯片即使把 UART 集成在 Super I/O 或 SoC 内，只要接口兼容，也可以使用这套驱动。

因此，你当前机器上的串口被识别为 16550A，仍然由 8250 系列驱动管理。这里的 16550A 表示内核识别出的 UART 兼容类型，
不一定意味着主板上有一颗独立的、型号就叫 16550A 的芯片。


## UART 才是更高的 level 级别的

8250 只是 UART 控制器的一种家族，并不是所有串口控制器的统一硬件标准。 与它平级的那些驱动，大多支持其他厂商设计的
UART 控制器；它们在 Linux 中共同接入 serial_core。

可以这样理解这个目录：

用户程序：open / read / write / termios
                    |
                  TTY 层
                    |
               serial_core
                    |
         各驱动实现的 uart_ops
          /         |          \
       8250      amba-pl011     imx ...
         |           |          |
   8250兼容UART   ARM PL011   i.MX UART

为什么都是串口，却需要不同驱动？

因为“线上如何传输数据”和“CPU 如何操作控制器”是两回事。

这些 UART 都可以发送配置为 115200、8N1 的串口数据，但 CPU 操作它们时，可能有不同的：

• 寄存器地址和位定义；
• 波特率分频算法；
• FIFO 深度及状态判断方式；
• 中断确认、清除方式；
• DMA、时钟和电源管理机制。

所以，串口通信格式相同，不代表硬件编程接口兼容。例如，8250 驱动不能直接拿自己的寄存器操作方式去控制 PL011。

目录中的典型例子：

 驱动             支持的控制器
━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 8250/            8250、16550 及众多兼容、扩展型号
───────────────  ─────────────────────────────────────
 amba-pl011.c     ARM PrimeCell PL011 UART
───────────────  ─────────────────────────────────────
 imx.c            NXP i.MX UART
───────────────  ─────────────────────────────────────
 stm32-usart.c    STM32 USART
───────────────  ─────────────────────────────────────
 samsung_tty.c    Samsung SoC UART
───────────────  ─────────────────────────────────────
 sc16is7xx*.c     通过 I²C/SPI 接入主机的串口扩展芯片

它们共享的是软件接口 uart_ops。

struct uart_ops (include/linux/serial_core.h:375) 定义了 start_tx、stop_rx、startup、set_termios 等操作。各驱动把
它们实现为对应硬件的寄存器操作，例如 PL011 的实现 (drivers/tty/serial/amba-pl011.c:2415)：

.start_tx    = pl011_start_tx,
.startup     = pl011_startup,
.set_termios = pl011_set_termios,

serial_core 利用这些接口对接 TTY，用户程序通常不需要知道底下是哪种 UART。也就是说，8250 和 PL011 是并列的硬件驱
动，公共层是 serial_core。

至于为什么 8250/ 自己又有很多文件：它覆盖的硬件太多。8250_port.c 提供通用操作，8250_dw.c、8250_omap.c 等再处理不同
实现的扩展和差异。

不过，目录位置不是严格的硬件兼容性分类。例如这里同时存在 omap-serial.c 和 8250/8250_omap.c，代表独立实现与基于
8250 框架的实现两条路径，Kconfig 会限制常规配置下同时启用它们。因此，不能简单认为“在 8250/ 外面的控制器都完全不兼
容 8250”。

## 所以，现在我们看这个就很容易了
```txt
CONFIG_SERIAL_AMBA_PL011=y
CONFIG_SERIAL_AMBA_PL011_CONSOLE=y
```
https://github.com/akinomyoga/ble.sh
https://github.com/akinomyoga/ble.sh


## serial 和 uart 是什么关系?

• Serial（串行通信）是大类，UART 是实现串行通信的一种硬件。

  串行通信指数据按位依次传输；UART 负责把字节转换成带起始位、停止位的异步串行数据，接收时再还原成字节。所以日常说
  的“串口”，通常指通过 UART 通信的接口。

  要区分两件事：UART 定义收发方式，不定义电压。同样是 UART 数据，设备引脚可能是 TTL 电平，也可能经过转换芯片接到 RS-
  232 或 RS-485 接口。SPI、I²C 也属于串行通信，但不是 UART。

2026-09-27 : 那么 rs232 是什么的驱动是什么?


## 为什么 ls /dev/ttyS* 可以展示这么多东西 ?

🧀  ls -la /dev/ttyS* | wc -l
32

因为 ttyS* 是预约槽位的结果:

具体有两步：先建立 8250 槽位，再注册对应的 ttyS 设备节点。在本地内核源码中，调用链是：

serial8250_init()
  → serial8250_isa_init_ports()
      → 对每个 i < nr_uarts 调用 serial8250_setup_port(i)
  → serial8250_register_ports()
      → 对每个槽位调用 uart_add_one_port()
  → serial_core_add_one_port()
      → tty_port_register_device_attr_serdev()
  → tty_register_device_attr()
      → device_register() → devtmpfs 创建 /dev/ttyS<i>

入口和循环分别在 /home/martins3/data/kernel/linux/drivers/tty/serial/8250/8250_platform.c 的 serial8250_init()、
`__serial8250_isa_init_ports()`，以及 /home/martins3/data/kernel/linux/drivers/tty/serial/8250/8250_core.c 的
serial8250_register_ports()。nr_uarts 的默认值来自 CONFIG_SERIAL_8250_RUNTIME_UARTS。

关键是 /home/martins3/data/kernel/linux/drivers/tty/serial/serial_core.c 的 serial_core_add_one_port()：源码明确规
定，即使没有探测到串口，也注册这个 port，以便以后用 setserial 调整参数。因此创建了 /dev/ttyS1，并不表示 ttyS1 背后
有 UART。具体设备节点的注册在 /home/martins3/data/kernel/linux/drivers/tty/tty_io.c 的
tty_register_device_attr()。

## Links

【23分钟讲解Linux内核UART驱动框架】 https://www.bilibili.com/video/BV1394y1L7Yn/?share_source=copy_web&vd_source=42e22c6b2f211ee75c7e2c895faf3c2a
https://cumtchw.blog.csdn.net/article/details/133343779?spm=1001.2014.3001.5502

## earlyprintk=serial 的作用

是内核启动参数，意思是：在正常控制台驱动初始化之前，就通过串口输出内核日志。主要用于排查启动
  早期卡死、崩溃等问题。

  按当前源码的 x86 实现，它默认使用：

  • 串口：ttyS0（COM1，I/O 地址 0x3f8）
  • 波特率：9600
  • 串口格式：8N1

  可以显式指定串口和波特率，例如：

  earlyprintk=serial,ttyS0,115200 console=ttyS0,115200n8

  这两个参数分别负责不同阶段：

   参数                               作用
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   earlyprintk=serial,ttyS0,115200    输出启动早期日志，通过轮询直接访问串口硬件
  ─────────────────────────────────  ────────────────────────────────────────────
   console=ttyS0,115200n8             正常串口驱动初始化后，继续输出控制台日志

  正常控制台接管后，early console 默认会被注销；追加 ,keep 可以保留。接收端的波特率需要匹配，内核也需要启用
  CONFIG_EARLY_PRINTK。

  具体默认值见 arch/x86/kernel/early_printk.c:155。其他架构的支持和行为可能不同。

可以发现 x86 和 arm 都定义了这个结果:
```txt
🧀  find . -name early_printk.c
./arch/arm/kernel/early_printk.c
./arch/x86/kernel/early_printk.c
./arch/um/kernel/early_printk.c
./arch/m68k/kernel/early_printk.c
./arch/mips/ath25/early_printk.c
./arch/mips/ath79/early_printk.c
./arch/mips/bcm63xx/early_printk.c
./arch/mips/kernel/early_printk.c
./arch/mips/lantiq/early_printk.c
./arch/mips/ralink/early_printk.c
./arch/s390/kernel/early_printk.c
```
## 那么 8250 和 16550 是什么关系？

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
