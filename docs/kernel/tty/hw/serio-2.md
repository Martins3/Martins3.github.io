The KeyBoard driver is really intesting :
1. Keyboard initialization function i8042_setup_kbd()
2. The AT or PS/2 keyboard interrupt function atkbd_interrupt()

## serio
serio_driver 是 Linux serio 总线上的设备协议驱动：接收底层送来的字节，解释成按键、鼠标移动、触摸坐标等。最典型的实

  例就是 atkbd 键盘驱动和 psmouse 鼠标驱动。

  serio 来自 Serial I/O，但它和通常说的 UART/COM 串口驱动处在不同层次。

  它具体负责什么

  可以先区分两个结构，定义在 include/linux/serio.h:21：

   对象                   代表什么                      主要职责
  ━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   struct serio           一个可收发字节的端口          提供 write/open/close 等底层操作
  ─────────────────────  ────────────────────────────  ────────────────────────────────────────────────
   struct serio_driver    使用这个端口的设备协议驱动    匹配设备、初始化设备、解析收到的字节、处理断开

  例如，PS/2 键盘的输入路径是：

  PS/2 键盘
      ↓
  i8042 控制器驱动：读取硬件，取出字节
      ↓
  serio_interrupt()：把字节交给绑定的 serio_driver
      ↓
  atkbd：解析扫描码、处理按下/松开
      ↓
  input 子系统 → /dev/input/eventX → 用户程序

  serio_interrupt() (drivers/input/serio/serio.c:961) 的核心就是调用 serio->drv->interrupt(serio, data, dfl)。这里
  的 .interrupt 是接收字节的回调，不意味着这个驱动直接注册和处理硬件 IRQ。

  哪些设备使用它

  这棵内核里主要有这些类别：

   设备类别                                        驱动例子
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   AT、PS/2 键盘                                   atkbd
  ──────────────────────────────────────────────  ──────────────────────────────────────────
   PS/2 鼠标、通过 PS/2 接口工作的触摸板/指点杆    psmouse，包含 Synaptics、ALPS 等协议支持
  ──────────────────────────────────────────────  ──────────────────────────────────────────
   RS-232 串口鼠标                                 sermouse
  ──────────────────────────────────────────────  ──────────────────────────────────────────
   老式工作站键盘                                  sunkbd、lkkbd
  ──────────────────────────────────────────────  ──────────────────────────────────────────
   串口触摸屏、数位板                              elo、gunze、wacom_w8001、wacom_serial4
  ──────────────────────────────────────────────  ──────────────────────────────────────────
   串口游戏控制器、六自由度控制器                  iforce-serio、spaceball、spaceorb

  它以输入设备为主，但并不限于输入设备：代码里还有使用 serio_driver 的 串口转 I²C 设备 i2c-taos-evm，以及 Pulse-
  Eight 等 CEC 适配器。

  因此，“设备使用串行通信”并不自动意味着使用 serio，要看驱动采用哪个内核框架。例如普通 USB 键鼠通常走 USB HID。

###  和 serial 是一个完全不同的东西

如果你说的 serial 是 drivers/tty/serial/ 下的 UART 驱动：

• serial/UART 驱动管理串口控制器：波特率、数据位、FIFO、中断、DMA 等，常通过 TTY 提供 /dev/ttyS*。
• serio_driver理解端口另一端设备的协议：这些字节表示哪个按键、哪个坐标、哪条设备命令。
• 两者可以通过 serport 连接。它是一个 TTY line discipline，把 TTY 收到的字节转交给 serio，具体见 drivers/input/
  serio/serport.c:113。

以串口鼠标为例：

RS-232 鼠标 → UART 驱动 → TTY → serport → serio → sermouse → input

而 PS/2 键盘的路径是 i8042 → serio → atkbd，不经过 UART/TTY。这也是为什么不能把 serio_driver 直接理解成“串口控制器
驱动”。


### 这种设备很少见了
• 当前机器没有已注册的 serio 设备。 实际检查：

  $ ls -l /sys/bus/serio/devices
  total 0

  /proc/bus/input/devices 中，目前的物理键鼠都走 USB：

   设备                             接口
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━
   ROYUAN Gaming keyboard           USB
  ───────────────────────────────  ────────────────
   Compx VGN Mouse 2.4G Receiver    USB 无线接收器

  它们的 Bus=0003，设备路径也在 USB 树下，没有经过 serio → atkbd/psmouse。

  这说明当前内核没有枚举出 serio 端口；仅凭这个结果，不能判断主板是否具备 PS/2
  硬件接口。


### 虚拟机中
ls -la  /sys/bus/serio/devices

virtme
lrwxrwxrwx - root 27 Sep 12:02 serio0 -> ../../../devices/platform/i8042/serio0
lrwxrwxrwx - root 27 Sep 12:02 serio1 -> ../../../devices/platform/i8042/serio1

这是 qemu pc 中携带的:

 -machine pc,hpet=off,smm=off,usb=off

  虽然命令里有 -nodefaults，但 -machine pc 自己会创建 PC 平台的基础设备，其中默认包含 i8042。当前 QEMU 的 HMP info qtree 也明确显示：

  dev: i8042
    kbd-irq = 1
    mouse-irq = 12
  dev: ps2-kbd
  dev: ps2-mouse

  所以 guest 中的对应关系是：

  /sys/devices/platform/i8042/serio0   PS/2 keyboard port
  /sys/devices/platform/i8042/serio1   PS/2 auxiliary/mouse port


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
