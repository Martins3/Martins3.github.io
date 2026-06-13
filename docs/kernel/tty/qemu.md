# qemu

## 可以反向的 mux 吗?
可以 mux 啊，还有 logfile 啊
https://superuser.com/questions/1373226/how-to-redirect-qemu-serial-output-to-both-a-file-and-the-terminal-or-a-port

也就是一个设备写入到两个设备中:

## 到底谁在用
/dev/hvc0 和 /dev/vport6p0 会使用
```txt
  lspci -s 00:08.0 -v
00:08.0 Communication controller: Red Hat, Inc. Virtio console
        Subsystem: Red Hat, Inc. Device 0003
        Flags: bus master, fast devsel, latency 0, IRQ 20
        I/O ports at c080 [size=64]
        Memory at fe0c6000 (32-bit, non-prefetchable) [size=4K]
        Memory at 38000001c000 (64-bit, prefetchable) [size=16K]
        Capabilities: <access denied>
        Kernel driver in use: virtio-pci
        Kernel modules: virtio_pci

```

## qemu 关于 monitor 的命令行参数
<!-- 8f2f4e27-34d8-40c9-bea5-565f229d5817 -->
其实很容易

1. 首先有一个经典的 -monitor stdio ，其效果为 stdio 可以同时做 serial 和 hmp
2. -qmp / -qmp-pretty 就是说，这个 uds 就是直接中 qmp 的
3. -mon 才是正规做法
	- -mon 可以反复多次配置，从而让虚拟机有多个 monitor
4. -serial 用于指定如何提供一个串口给虚拟机，这个有一个特殊模式
也就是 -serial mon:stdio ，这里的 stdio 可以替换掉
	- -serial 可以反复配置，从而让虚拟机中看到多个 serial
	- mon 提供 hmp 和 hmp ，通过是否配置 control 参数控制

回到之前配置方法:
1. 使用 arg_serial="-serial mon:stdio" 会让 serial 和 monitor 共享 stdio ，通过 ctrl-a c 来切换
	这个是使用的经典模式，但是这个 cmdline 太逆天了
2. 如果单独的设置 -serial stdio 和 -monitor stdio ，qemu 会报错，但是可以用这个方法来聚合他们:
```sh
	arg_serial+=" -chardev $main_serial "
	arg_serial+=" -mon chardev=main_char,mode=readline "
	arg_serial+=" -serial chardev:main_char"
```

如何理解
arg_serial+=" -device virtio-serial"
	- 内核中 virtio_console 或 virtserial
arg_serial+=" -device virtconsole,chardev=main_char"
	- 提供具体的端口

也就说，这个比传统的 serial 更好?

你确定?
```txt
-device virtio-serial
（通常完整形式是 -device virtio-serial-pci 或类似）
这会向虚拟机添加一个 virtio-serial 控制器（virtio-serial PCI 设备）。
这个控制器是一个 virtio PCI 设备，支持多个虚拟串行端口（最多几十个），性能比传统串口更好。它是 virtio-serial 通道的基础，虚拟机内的 Linux 内核会检测到它，并加载 virtio_console 或 virtserial 驱动。
-device virtconsole,chardev=main_char
这会在 virtio-serial 控制器上添加一个具体的 虚拟控制台端口（virtio console port）。
virtconsole 是设备类型，表示这是一个专用于控制台（console）的 virtio 串行端口（guest 中常表现为 /dev/hvc0 或类似，支持早起打印和登录）。
chardev=main_char 将这个端口连接到之前定义的字符设备后端（chardev），这里假设 main_char 是通过 -chardev ... 定义的（如 stdio、socket、file 等）。这样，主机就可以通过该 chardev 与虚拟机的这个控制台进行交互（例如用作串行控制台）。
```


这些设备的关系是什么?
```txt
name "isa-serial", bus ISA
name "pci-serial", bus PCI
name "pci-serial-2x", bus PCI
name "pci-serial-4x", bus PCI
name "usb-serial", bus usb-bus

name "virtio-serial-device", bus virtio-bus
name "virtio-serial-pci", bus PCI, alias "virtio-serial"
name "virtio-serial-pci-non-transitional", bus PCI
name "virtio-serial-pci-transitional", bus PCI

name "virtconsole", bus virtio-serial-bus
name "virtserialport", bus virtio-serial-bus
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(virtio-serial-device 真的就是 MMIO 的设备吗?)
- virtio-serial-device（bus: virtio-bus）
	- 使用 MMIO（内存映射 I/O）传输方式的 virtio-serial 控制器。主要用于非 PCI 架构（如 ARM 的 virt 机器、RISC-V 等），或某些嵌入式/简单场景。guest 通过 virtio-mmio 驱动检测到它。
- virtio-serial-pci（bus: PCI, alias: "virtio-serial"）
	- 最常见的版本，使用 PCI 总线传输的 virtio-serial 控制器。适用于 x86 等有 PCI 的架构，提供更好兼容性和性能。guest 通过 virtio-pci 驱动检测。
- virtio-serial-pci-non-transitional（bus: PCI）
	- 现代模式（virtio 1.0 非过渡模式）：严格遵循 virtio 1.0 规范，使用更高的 PCI 设备 ID（0x1040+ 范围）。适合现代 guest 驱动（支持 virtio 1.0），性能更好，但不兼容旧版 guest。
- virtio-serial-pci-transitional（bus: PCI）
	- 过渡模式（virtio 1.0 过渡/legacy 兼容模式）：默认兼容旧版 virtio 0.9（使用低 PCI ID 0x1000+ 范围），但可与 guest 协商升级到 1.0。QEMU 在 PCI 总线上默认使用此模式，以确保向后兼容。

```txt
控制器 (virtio-serial-pci 或 virtio-serial-device)
   └── 创建 virtio-serial-bus (内部总线)
         ├── virtconsole (控制台端口 → guest /dev/hvc0)
         └── virtserialport (通用端口 → guest /dev/virtio-ports/...)
```

virtio-serial 是什么设备
virtiocon 是什么

> virtconsole 是 virtserialport 的特化版本（端口 0 常专用于 console）

	# virtio-serial 是 virtconsole 和 virtserialport 的基础设备:
	# qemu-system-x86_64: -device virtconsole,chardev=virtiocon0: No 'virtio-serial-bus' bus found for device 'virtconsole'
	# qemu-system-x86_64: -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0: No 'virtio-serial-bus' bus found for device ' virtserialport'


这样的输出结果是预期的吗?
```txt
 ls -la /sys/class/tty/tty*/device/driver

lrwxrwxrwx    - root 15 Dec 10:51 00:03:0.0 -> ../../../../devices/pnp0/00:03/00:03:0/00:03:0.0
lrwxrwxrwx    - root 15 Dec 10:51 00:04:0.0 -> ../../../../devices/pnp0/00:04/00:04:0/00:04:0.0
lrwxrwxrwx    - root 15 Dec 10:51 serial8250:0.2 -> ../../../../devices/platform/serial8250/serial8250:0/serial8250:0.2
lrwxrwxrwx    - root 15 Dec 10:51 serial8250:0.3 -> ../../../../devices/platform/serial8250/serial8250:0/serial8250:0.3

ls -la /sys/class/tty/ttyS1/device
🧀  l
Permissions Size User Date Modified Name
lrwxrwxrwx     - root 15 Dec 21:43   driver -> ../../../../../bus/serial-base/drivers/port
drwxr-xr-x     - root 15 Dec 21:43   power
lrwxrwxrwx     - root 15 Dec 21:43   subsystem -> ../../../../../bus/serial-base
drwxr-xr-x     - root 15 Dec 21:43   tty
.rw-r--r--  4.1k root 15 Dec 21:43   uevent
```
这里说明，一共四个 ttyS*

如果 virtio serial ，也就是 /dev/virtio-ports/ 的设备

```txt
🧀  l
Permissions Size User Date Modified Name
.r--r--r--  4.1k root 15 Dec 21:52   device
lrwxrwxrwx     - root 15 Dec 21:52   driver -> ../../../../bus/virtio/drivers/virtio_console
.r--r--r--  4.1k root 15 Dec 21:52   features
.r--r--r--  4.1k root 15 Dec 21:52   modalias
drwxr-xr-x     - root 15 Dec 21:52   power
.r--r--r--  4.1k root 15 Dec 21:52   status
lrwxrwxrwx     - root 15 Dec 21:52   subsystem -> ../../../../bus/virtio
.rw-r--r--  4.1k root 15 Dec 21:52   uevent
.r--r--r--  4.1k root 15 Dec 21:52   vendor
drwxr-xr-x     - root 15 Dec 21:52   virtio-ports
virtio-ports/vport6p0/device🔒 🦇
🧀  pwd
/sys/class/virtio-ports/vport6p0/device
```

 ls /sys/class/virtio-ports/*/device/driver ，其驱动其实是 virtio 的
其实容易找到 drivers/char/virtio_console.c

让 stdio 走 hvc0 ，然后配置 tty=hvc0 就可以了
```txt
	arg_serial+=" -chardev stdio,id=stdio"
	arg_serial+=" -device virtconsole,chardev=stdio"
```


### 等待整理

	# 这个命令是 -serial 的展开模式，有趣!
	# https://fedoraproject.org/wiki/Features/VirtioSerial
	#
	# 很有趣，这里的几个设备关系，应该是，-device virtiocon 或者 -device virtserialport 是前端设备，
	# 必须 attach 到 virtio-serial 这个载体，-chardev 指定后端
	# TODO 类似的 virtserialport 还有多少
	#
	# 这里参考 man qemu(1) -chardev 中介绍如何将 chardev 复用



### 引用的结果


### manual
```txt
       -monitor dev
              Redirect the monitor to host device dev (same devices
              as the serial port). The  default  device  is  vc  in
              graphical  mode  and stdio in non graphical mode. Use
              -monitor none to disable the default monitor.

       -qmp dev
              Like -monitor but opens in 'control' mode. For  exam‐
              ple, to make QMP available on localhost port 4444:

                 -qmp tcp:localhost:4444,server=on,wait=off

              Not all options are configurable via this syntax; for
              maximum flexibility use the -mon option and an accom‐
              panying -chardev.

       -qmp-pretty dev
              Like -qmp but uses pretty JSON formatting.

       -mon [chardev=]name[,mode=readline|con‐
       trol][,pretty[=on|off]]
              Set up a monitor connected to the chardev name.  QEMU
              supports  two  monitors:  the  Human Monitor Protocol
              (HMP; for human interaction), and  the  QEMU  Monitor
              Protocol  (QMP;  a JSON RPC-style protocol).  The de‐
              fault  is  HMP;  mode=control  selects  QMP  instead.
              pretty  is  only  valid when mode=control, turning on
              JSON pretty printing to ease human reading and debug‐
              ging.

              For example:

                 -chardev socket,id=mon1,host=localhost,port=4444,server=on,wait=off \
                 -mon chardev=mon1,mode=control,pretty=on

              enables the QMP monitor on localhost port  4444  with
              pretty-printing.
```

```txt
       -serial dev
              Redirect  the virtual serial port to host character device dev.
              The default device is vc in graphical mode  and  stdio  in  non
              graphical mode.

              This option can be used several times to simulate multiple ser‐
              ial ports.

              You  can  use  -serial none to suppress the creation of default
              serial devices.

              Available character devices are:

              vc[:WxH]
                     Virtual console. Optionally, a width and height  can  be
                     given in pixel with

                        vc:800x600

                     It  is also possible to specify width or height in char‐
                     acters:

                        vc:80Cx24C

              pty[:path]
                     [Linux only] Pseudo TTY (a new PTY is automatically  al‐
                     located).

                     If  path  is specified, QEMU will create a symbolic link
                     at that location which points to the new PTY device.

                     This avoids having to make QMP or HMP monitor queries to
                     find out what the new PTY device path is.

                     Note that while QEMU will remove the symlink when it ex‐
                     its gracefully, it will not do so in case of crashes  or
                     on  certain  startup  errors. It is recommended that the
                     user checks and removes the symlink  after  QEMU  termi‐
                     nates to account for this.

              none   No  device  is  allocated.  Note  that for machine types
                     which emulate systems where a serial  device  is  always
                     present  in real hardware, this may be equivalent to the
                     null option, in that the serial device is still  present
                     but all output is discarded. For boards where the number
                     of  serial  ports is truly variable, this suppresses the
                     creation of the device.

              null   A guest will see the UART or serial device as present in
                     the machine, but all output is discarded, and  there  is
                     no  input.   Conceptually  equivalent to redirecting the
                     output to /dev/null.

              chardev:id
                     Use a named character device defined with  the  -chardev
                     option.

              /dev/XXX
                     [Linux only] Use host tty, e.g. /dev/ttyS0. The host se‐
                     rial  port  parameters are set according to the emulated
                     ones.

              /dev/parportN
                     [Linux only, parallel port only] Use host parallel  port
                     N.   Currently SPP and EPP parallel port features can be
                     used.

              file:filename
                     Write output to filename. No character can be read.

              stdio  [Unix only] standard input/output

              pipe:filename
                     name pipe filename

              COMn   [Windows only] Use host serial port n

              udp:[remote_host]:remote_port[@[src_ip]:src_port]
                     This implements UDP Net  Console.  When  remote_host  or
                     src_ip  are  not specified they default to 0.0.0.0. When
                     not using a specified src_port a random port is automat‐
                     ically chosen.

                     If you just want a simple readonly console you  can  use
                     netcat  or  nc, by starting QEMU with: -serial udp::4555
                     and nc as: nc -u -l -p 4555. Any time QEMU writes  some‐
                     thing to that port it will appear in the netconsole ses‐
                     sion.

                     If  you  plan  to send characters back via netconsole or
                     you want to stop and start QEMU  a  lot  of  times,  you
                     should  have  QEMU use the same source port each time by
                     using something like -serial  udp::4555@:4556  to  QEMU.
                     Another  approach  is to use a patched version of netcat
                     which can listen to a TCP  port  and  send  and  receive
                     characters  via  udp.  If  you have a patched version of
                     netcat which activates telnet  remote  echo  and  single
                     char transfer, then you can use the following options to
                     set  up a netcat redirector to allow telnet on port 5555
                     to access the QEMU port.

                     QEMU Options:
                            -serial udp::4555@:4556

                     netcat options:
                            -u -P 4555 -L 0.0.0.0:4556 -t -p 5555 -I -T

                     telnet options:
                            localhost 5555

              tcp:[host]:port[,server=on|off][,wait=on|off][,node‐
              lay=on|off][,reconnect-ms=milliseconds]
                     The TCP Net Console has two modes of operation.  It  can
                     send  the serial I/O to a location or wait for a connec‐
                     tion from a location. By default the TCP Net Console  is
                     sent  to  host at the port. If you use the server=on op‐
                     tion QEMU will wait for a client socket  application  to
                     connect  to  the  port  before  continuing,  unless  the
                     wait=on|off option was specified. The nodelay=on|off op‐
                     tion disables the Nagle buffering algorithm. The  recon‐
                     nect-ms  option only applies if server=no is set, if the
                     connection goes down it will attempt to reconnect at the
                     given interval. If host is omitted, 0.0.0.0 is  assumed.
                     Only  one  TCP connection at a time is accepted. You can
                     use telnet=on to connect to the corresponding  character
                     device.

                     Example to send tcp console to 192.168.0.2 port 4444
                            -serial tcp:192.168.0.2:4444

                     Example to listen and wait on port 4444 for connection
                            -serial tcp::4444,server=on

                     Example to not wait and listen on ip 192.168.0.100 port
                     4444
                            -serial tcp:192.168.0.100:4444,server=on,wait=off

              telnet:host:port[,server=on|off][,wait=on|off][,nodelay=on|off]
                     The  telnet protocol is used instead of raw tcp sockets.
                     The options work the same as if you had specified  -ser‐
                     ial  tcp.   The  difference is that the port acts like a
                     telnet server or client using telnet option negotiation.
                     This will also allow you to  send  the  MAGIC_SYSRQ  se‐
                     quence  if  you  use  a telnet that supports sending the
                     break sequence. Typically in unix telnet you do it  with
                     Control-]  and then type "send break" followed by press‐
                     ing the enter key.

              websocket:host:port,server=on[,wait=on|off][,nodelay=on|off]
                     The WebSocket  protocol  is  used  instead  of  raw  tcp
                     socket. The port acts as a WebSocket server. Client mode
                     is not supported.

              unix:path[,server=on|off][,wait=on|off][,reconnect-ms=millisec‐
              onds]
                     A  unix  domain  socket is used instead of a tcp socket.
                     The option works the same as if you had specified  -ser‐
                     ial  tcp  except the unix domain socket path is used for
                     connections.

              mon:dev_string
                     This is a special option to allow the monitor to be mul‐
                     tiplexed onto another serial port. The  monitor  is  ac‐
                     cessed  with key sequence of Control-a and then pressing
                     c. dev_string should be any one of  the  serial  devices
                     specified  above.  An  example  to multiplex the monitor
                     onto a telnet server listening on port 4444 would be:

                     -serial mon:telnet::4444,server=on,wait=off

                     When the monitor is multiplexed to stdio  in  this  way,
                     Ctrl+C  will  not  terminate  QEMU  any more but will be
                     passed to the guest instead.

              braille
                     Braille device. This will  use  BrlAPI  to  display  the
                     braille output on a real or fake device.

              msmouse
                     Three  button  serial  mouse. Configure the guest to use
                     Microsoft protocol.
```

问题:
如何理解在 graphic mode 下
默认的 monitor 是 vc ，然后 -serial 默认也是 vc 吗?

> [!NOTE]
> 参考神奇海螺的意见，有待验证

这个输出结果我是非常怀疑的:
```txt
**这里的“vc”指的是 **Virtual Console**（虚拟控制台）。

在 QEMU 中，当使用图形模式（例如默认的 SDL 或 GTK 窗口显示虚拟机画面）时，QEMU 的 **monitor**（监控控制台，用于输入 QEMU 命令如 info、quit 等）默认连接到一个 **虚拟文本控制台** 上。这个虚拟控制台就是“vc”。

- 你可以通过按 **Ctrl+Alt+2**（或类似快捷键）切换到 monitor 界面（Ctrl+Alt+1 切换回虚拟机图形输出）。
- “vc” 是一种字符设备后端（chardev backend），它会在 QEMU 的图形窗口中分配一个独立的文本区域来显示 monitor 输出，支持指定宽度和高度（如 -chardev vc,width=...）。

而在非图形模式（例如加了 -nographic 参数）下，默认 monitor 设备则是 stdio（直接使用标准输入输出）。

这与串口（serial）设备的默认行为类似，也是在图形模式下默认 vc，非图形下 stdio。

如果你想禁用默认 monitor，可以用 -monitor none。
```



### -serial 的展开
```txt
-chardev <backend> ,id=serial0
-device isa-serial,chardev=serial0
```
其实是存在自动展开的:
```txt
或（架构相关）：

x86：isa-serial

ARM：pl011

virt：8250
```

libvirt 也是用 serial 配置的，所以，其实不算是糟糕的
```txt
	-device virtio-serial-pci,id=virtio-serial0,max_ports=31,bus=pci.1,addr=0x0
	-device virtserialport,bus=virtio-serial0.0,nr=1,chardev=charchannel0,id=channel0,name=org.qemu.guest_agent.0
	-chardev pty,id=charserial0
	-chardev file,id=charserial1,path=/dev/fdset/1,append=on
	-serial chardev:charserial0
	-add-fd set=1,fd=71
	-serial chardev:charserial1
```

### 通过理解 pty ，可以解释下面问题了
```txt
	# TODO 这个命令展开是什么样的
	arg_serial+=" -serial file:$vm_dir/$which_qemu/serial_file "

	# 提供给 minicom 测试
	# TODO 又有一种新的定义 chardev 方式，似乎和 vmtest 定义的东西是一样的
	# 但是 TODO 无法实现 minicom -D unix#minicom.sock ���操作，这是预期的吗?
	# 显然，只是我们当时没有理解
	arg_serial+=" -chardev socket,id=minicom,path=$vm_dir/$which_qemu/minicom.sock,server=on,wait=off"
	arg_serial+=" -serial chardev:minicom"
```

需要解释的问题:
1. pty 和 ptmx 对应内核的那个模块


### vc
TODO 如果只是单独的 -serial stdio ，但是没有 -display none 的时候
在界面上和 stdio 上是存在两个登录入口的，不知道这两个登录入口有什么区别
如果用 w 检查 user ，发现只有一个 user ，是不是一个是 vga 的串口，一个是 serial 的串口

这个是 vc 的原因吗?

### ctrl c
2. 使用 arg_display="-serial stdio -display none" ，stdio 中 ctrl-c 直接将 qemu 杀死了

### hvc0 和 virtio serial 的紧密关系
https://unix.stackexchange.com/questions/751827/debian-vm-doesn-t-boot-on-qemu-with-console-hvc0-kernel-parameter

echo 1 | sudo tee /dev/hvc0

virtio_console

hvc0 的代码在 : drivers/tty/hvc/
作为对比，drivers/char/virtio_console.c 的位置

从基本原理上，hvc 是不通过 virtio 的，但是发现
在 qemu 中 hw/char/virtio-console.c 两个是在一起的:

```c
static void virtconsole_register_types(void)
{
    type_register_static(&virtserialport_info);
    type_register_static(&virtconsole_info);
}
```

```txt
 17)               |  hvc_push() {
 17)               |    arch_irq_work_raise() {
 17)   1.054 us    |      x2apic_send_IPI_self();
 17)   2.835 us    |    }
 17)               |    put_chars [virtio_console]() {
 17)               |      _raw_spin_lock_irqsave() {
 17)   0.447 us    |        do_raw_spin_lock();
 17)   1.840 us    |      }
 17)               |      _raw_spin_unlock_irqrestore() {
 17)   0.403 us    |        do_raw_spin_unlock();
 17)   1.317 us    |      }
 17)               |      kmemdup_noprof() {
 17)               |        __kmalloc_node_track_caller_noprof() {
 17)   0.415 us    |          fs_reclaim_acquire();
 17)   0.420 us    |          fs_reclaim_release();
 17)   2.711 us    |        }
 17)   3.621 us    |      }
 17)               |      __send_to_port [virtio_console]() {
 17)               |        _raw_spin_lock_irqsave() {
 17)   0.551 us    |          do_raw_spin_lock();
 17)   2.091 us    |        }
```

```c
/* The operations for console ports. */
static const struct hv_ops hv_ops = {
	.get_chars = get_chars,
	.put_chars = put_chars,
	.notifier_add = notifier_add_vio,
	.notifier_del = notifier_del_vio,
	.notifier_hangup = notifier_del_vio,
};
```
#### 关于 hvc0 的最后一个问题，为什么 console=/dev/hvc0 但是 console=/dev/vport6p0 就不行

🧀  echo 1 | sudo tee /dev/vport6p3

是可以观察到输出的

## 通过 pty driver 来理解 qemu

qemu 也需要打开一个 master ，然后一端从虚拟机哪里接受，另外一端从

## socat 原来就是一个 pty client 啊

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
