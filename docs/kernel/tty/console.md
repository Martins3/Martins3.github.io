# console

同一个 ttyS0 可以同时具有两个身份：

普通 TTY：程序可以 read/write /dev/ttyS0
内核 console：printk 也会把日志写到 ttyS0

是否配置 console=ttyS0，只改变第二个身份，不决定这个 TTY 是否存在。

## 为什么没有 console=tty0，VNC 仍然可以登录

因为这是两条独立路径：

console=tty0
    -> 让 printk 把内核日志写到当前 VT

getty@tty1.service
    -> agetty 打开 /dev/tty1
    -> 显示 login:
    -> 登录后 Shell 的 controlling terminal 是 tty1

因此删掉 console=tty0 后：

- VNC 中可能看不到内核启动日志；
- getty@tty1.service 仍然启动；
- 登录提示和 Shell 仍然存在；
- ps、w 显示 tty1，这是正确结果。

getty 使用固定的 /dev/tty1，不能使用会随 VT 切换而变化的 /dev/tty0。

如果没有 consolt=tty0 ，其实最后屏幕上也是会有让人登录的入口的，
但是只是从 grub 开始后，没有 dmesg 的日志了
## 多个 console= 的准确规则

对于不同类型的设备：

console=ttyS0 console=tty0

内核日志会发往 ttyS0 和 tty0。正常情况下最后一个有效目标 tty0 是 /dev/console 的后端。

交换顺序：

console=tty0 console=ttyS0

仍然向两者打印，但 /dev/console 通常由 ttyS0 承担。

对于同一类型：

console=ttyS0 console=ttyS1

不能简单套用“最后一个生效”。内核文档明确规定：
同一设备类型重复出现时，只有第一个设备获得输出，/dev/console 还会受到实际驱动注册顺序影响。

另外：

console=0

在 kernel/printk/printk.c 的 console_setup() 中会被解析成 ttyS0。
它确实是 console=ttyS0 的数字简写，不是 tty0。

## /dev/console 做啥的 ?

```txt
lrwxrwxrwx     - root 15 Dec 22:15   console -> ../../devices/virtual/tty/console
```

/dev/console 的驱动在 : drivers/tty/tty_io.c

感觉  sudo tee /dev/console 有点像是一个软链接

执行 lsinitrd 的时候，可以看到，这个如何理解:
```txt
crw-r--r--   1 root     root       5,   1 May 23  2024 dev/console
crw-r--r--   1 root     root       1,  11 May 23  2024 dev/kmsg
crw-r--r--   1 root     root       1,   3 May 23  2024 dev/null
crw-r--r--   1 root     root       1,   8 May 23  2024 dev/random
crw-r--r--   1 root     root       1,   9 May 23  2024 dev/urandom
```

## 多个 console= 参数
```txt
console=tty0  console=ttyS0
```

最后一个 console= 参数，这里，也就是 ttyS0 ，将会是
/dev/console 的指向的

只有不去配置 console=tty0 ，启动的时候发屏幕才会没有输出。
不会因为顺序问题，所有的屏幕都是有的。

## 小问题
只是似乎 ARM 环境中，日志显示会推迟一会
- [ ] Mac 虚拟机中测试下，应该是物理机的问题。

## 原来配置两次，可以输出两次啊 ? 真的吗?

这是 bug 吧
```txt
 __hrtimer_run_queues+0x20b/0x420
 __hrtimer_run_queues+0x20b/0x420
 hrtimer_interrupt+0x118/0x260
 hrtimer_interrupt+0x118/0x260
 __sysvec_apic_timer_interrupt+0x6a/0x190
 __sysvec_apic_timer_interrupt+0x6a/0x190
 sysvec_apic_timer_interrupt+0x6c/0x90
 sysvec_apic_timer_interrupt+0x6c/0x90
 </IRQ>
 </IRQ>
 <TASK>
 <TASK>
 asm_sysvec_apic_timer_interrupt+0x1a/0x20
 asm_sysvec_apic_timer_interrupt+0x1a/0x20
RIP: 0010:_raw_spin_unlock_irqrestore+0x36/0x70
RIP: 0010:_raw_spin_unlock_irqrestore+0x36/0x70
```

## 感觉 alpine.sh 中的这个其实没完全清楚的
配合原理去理解一下分析一下吧，感觉很多东西都是有错觉

```sh
# x86 配置这两个都可以:
# kernel_args+=" console=ttyS0,9600 earlyprintk=serial "
# 和想象不一样，console 最多只是支持一个串口，ttyS1 - ttyS3 对应的不会输出的
#
# 打开 earlyprintk=serial,0,115200 会让速度变慢
#
# 原来配置两次，可以输出两次啊
kernel_args+=" console=0 console=abc console=hvc0 "
# 集成 vmtest 的时候发现的
# 1. console=ttyS0 也可以替换为 console=0 ，两者效果等价，但是原理是否等价，没有检查代码
# 2. 如果 console=tty0 和 console=ttyS0 同时出现，只有一个有交互窗口，这个其实合理，所以看来
# systemd 给我们做了一些事情，才让 console=tty0 和 console=ttyS0 可以同时工作
# kernel_args+="console=ttyAMA0 "
#
if check_option console; then
	kernel_args+=" console=tty0"
fi
if [[ $ARCH == aarch64 ]]; then
	kernel_args+=" console=ttyAMA0 "
fi
# x86 中如果只配置这个，那么在终端中没有输出
# kernel_args+=" console=tty0 "
# arm 可以不配置任何参数，一样可以输出:
```

## 如何理解这个东西
tty0 ttyS0

## /proc/consoles
这个 proc 的作用是什么?

```txt
🧀  cat /proc/consoles
tty0                 -WU (EC p  )    4:2k
```

```txt
cat /proc/consoles
hvc0                 -W- (EC  p  )  229:0
ttyS0                -W- (E   p a)    4:64
```

## 我的天啊

oe2 的虚拟机中:

fs/proc/consoles.c

```txt
➜  cat /proc/consoles
tty0                 -WU (EC p  )    4:1
netcon0              -W- (E     )
ttyS0                -W- (E  p a)    4:64
```

问题 :

1. 这里显示了 netcon0 是那里配置的，我不信


```txt
🧀  cat /proc/consoles
tty0                 -WU (EC p  )    4:2
```

### 看看 console 的 driver

```c
static struct console vt_console_driver = {
	.name		= "tty",
	.setup		= vt_console_setup,
	.write		= vt_console_print,
	.device		= vt_console_device,
	.unblank	= unblank_screen,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
};
```

好家伙，这么多的 console driver 啊



## 那么这个目录如何理解?
/home/martins3/data/kernel/linux-drm/drivers/video/console/vgacon.c

看看 kernel 中都包含那些源码

## 内核 console、TTY 与 shell 是不同层

| 接口 | Linux 支持 | console 注册代码 |
|---|---|---|
| VT：tty0、tty1 等 | `CONFIG_VT`、`CONFIG_VT_CONSOLE`；显示还需要相应后端 | `drivers/tty/vt/vt.c` 的 `vt_console_driver`、`con_init()` |
| 8250 串口：ttyS0 | `CONFIG_SERIAL_8250`、`CONFIG_SERIAL_8250_CONSOLE` | `drivers/tty/serial/8250/8250_core.c` 的 `univ8250_console`、`univ8250_console_init()` |
| Virtio console：hvcN | `CONFIG_VIRTIO_CONSOLE` 选择 `CONFIG_HVC_DRIVER`，加 virtio transport | `drivers/char/virtio_console.c` 的 `init_port_console()`；`drivers/tty/hvc/hvc_console.c` 的 `hvc_console` |
| 普通 virtserialport：vportXpY | 同一个 `CONFIG_VIRTIO_CONSOLE` 驱动提供字符设备 read/write | 没有独立的 vport console 注册 |

前三者都需要驱动支持，它们分别注册 `.name="tty"`、`"ttyS"`、`"hvc"` 的
`struct console`，通过 `register_console()` 接入 printk。`console=` 负责选择已支持的
控制台，不会为任意字符设备自动创建 console 驱动。早期输出还要求相关驱动已内建或已就绪。

VT 输出走 `vt_console_print()` 和底层显示后端；8250 输出从
`univ8250_console_write()` 进入 `serial8250_console_write()`；HVC 输出走
`hvc_console_print()` 和 virtio-console 的 `hv_ops.put_chars`。

TTY 为用户态提供终端 ioctl、行规程、回显和作业控制。内核 console 为 printk 等提供
输出入口。Bash 是用户态程序：普通管道也能传它的输入输出，完整的终端交互则通常由 TTY/PTY
提供。此 demo 的 vport 始终只是传输层；PTY 模式由 guest 的 PTY 提供终端语义，
pipe 模式则保留普通管道语义，没有新增内核 console。

## 从驱动的角度发继续理解一下吧


static struct console vt_console_driver = {
	.name		= "tty",
	.setup		= vt_console_setup,
	.write		= vt_console_print,
	.device		= vt_console_device,
	.unblank	= unblank_screen,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
};


从这里看，console 真的没有什么东西，就是注册到 printk 上而已

## 有趣的现象，这个我们是理解了
tty 中的文件是如何使用的?
在 ssh 等

不知道为什么 tty0 现在不会输出日志，直接进入 console 了，
继续看看是为什么?

## console

https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html

```txt
        console=        [KNL] Output console device and options.

                tty<n>  Use the virtual console device <n>.

                ttyS<n>[,options]
                ttyUSB0[,options]
                        Use the specified serial port.  The options are of
                        the form "bbbbpnf", where "bbbb" is the baud rate,
                        "p" is parity ("n", "o", or "e"), "n" is number of
                        bits, and "f" is flow control ("r" for RTS or
                        omit it).  Default is "9600n8".

                        See Documentation/admin-guide/serial-console.rst for more
                        information.  See
                        Documentation/networking/netconsole.rst for an
                        alternative.

                uart[8250],io,<addr>[,options]
                uart[8250],mmio,<addr>[,options]
                uart[8250],mmio16,<addr>[,options]
                uart[8250],mmio32,<addr>[,options]
                uart[8250],0x<addr>[,options]
                        Start an early, polled-mode console on the 8250/16550
                        UART at the specified I/O port or MMIO address,
                        switching to the matching ttyS device later.
                        MMIO inter-register address stride is either 8-bit
                        (mmio), 16-bit (mmio16), or 32-bit (mmio32).
                        If none of [io|mmio|mmio16|mmio32], <addr> is assumed
                        to be equivalent to 'mmio'. 'options' are specified in
                        the same format described for ttyS above; if unspecified,
                        the h/w is not re-initialized.

                hvc<n>  Use the hypervisor console device <n>. This is for
                        both Xen and PowerPC hypervisors.

                { null | "" }
                        Use to disable console output, i.e., to have kernel
                        console messages discarded.
                        This must be the only console= parameter used on the
                        kernel command line.

                If the device connected to the port is not a TTY but a braille
                device, prepend "brl," before the device type, for instance
                        console=brl,ttyS0
                For now, only VisioBraille is supported.
```

## 参数问题基本上

```txt
	kernel_args+="  console=ttyS1 console=tty0 console=ttyAMA0"
```

现在观察来看，如果 console 后的类型不同，那么是可以复用的。

### console 的含义到底是什么， 似乎只是日志没有办法输出，而不是完全不可以用

1. 现象 1 :

将 kernel 的启动参数从

```txt
	kernel_args+=" console=ttyAMA0 console=ttyS0 console=tty0 "
```

修改为

```txt
	kernel_args+=" console=ttyAMA0 console=ttyS0  "
```

可以看到 vnc 中还是可以登录，应该是没有日志输出吧？


## 如何理解这个场景

```txt
› $ echo 1 > /dev/kmsg
  $ cat /proc/consoles
  tty0                 -WU (EC p  )    4:1
  ttyS0                -W- (E  p a)    4:64

  $ 参数是这个样子的
  nf_conntrack.hashsize=262144 tsx=on console=ttyS0,115200n8
  console=ttyS1,115200n8 console=tty0 precise_iostat=0
  nvme_core.multipath=0 slab_nomerge nvme.poll_queues=4
  transparent_hugepage=never
```

• 不是“只有最后一个 console= 生效”。这里有两套规则：

  - 不同类型的 console 可以同时生效，内核 printk 会输出到所有已启用
    console。

  - 同一种类型出现多次时，只使用第一个。ttyS0 和 ttyS1 都属于 ttyS 串口类
    型，因此 ttyS0 生效，ttyS1 被忽略。

  - “最后一个生效”通常是指：当每种类型只出现一次时，最后指定的设备成为 /
    dev/console 的首选设备，并不意味着前面的设备停止接收 printk。

所以你当前实际生效的是：

ttyS0    串口 console
tty0     VGA/虚拟终端 console，同时是 preferred console
ttyS1    未生效

/proc/consoles 已经直接证明了这一点：

tty0   -WU (EC p  )
              ^
              C = preferred console，即 /dev/console 指向的 console

ttyS0  -W- (E  p a)
            ^
            E = enabled
               p = 接收 printk

  其他标志：

  - W：支持写
  - U：支持 unblank
  - a：可在 CPU offline 场景安全调用
  - 4:1、4:64：字符设备的 major/minor

因此：

echo 1 > /dev/kmsg

这条消息在 console loglevel 允许时，会输出到 tty0 和 ttyS0，不会输出到
ttyS1。SSH 所在的 pts 不是 kernel console，所以当前 SSH 窗口看不到也是
正常的。

如果你想让 ttyS1 替代 ttyS0，启动参数应改为：

console=ttyS1,115200n8 console=tty0

如果想让串口成为 /dev/console 的首选设备，则顺序写成：

console=tty0 console=ttyS1,115200n8

内核文档在 Linux Serial Console 的 “Using multiple console devices” 章节:
(https://www.kernel.org/doc/html/latest/admin-guide/serial-console.html)
明确描述了同类型只取第一个的规则；/proc/consoles 文档
(https://docs.kernel.org/filesystems/proc.html#proc-consoles) 给出了这些标志的含义。


## console 的解读

/dev/console 不是一个普通文件，它是内核提供的“系统控制台”入口。检查它当前指向哪里，主要看以下几项：

cat /proc/consoles
cat /sys/class/tty/console/active
tr '\0' '\n' </proc/cmdline | grep '^console='

本机当前结果是：

/proc/consoles:
tty0

/proc/cmdline:
console=tty0

/sys/class/tty/console/active:
tty0

这表示：

- /dev/console 当前绑定到内核 console tty0
- tty0 是虚拟终端的复用入口
- 具体显示到哪个虚拟终端，可以查看：

cat /sys/class/tty/tty0/active

本机输出为：

tty2

因此，在当前环境中：

printf 'test from dev console\n' >/dev/console

这段内容应该写到当前活动的虚拟终端 tty2，而不是所有 /dev/ttyN，也不是当前 SSH 会话自动对应
的终端。

还可以确认设备号：

stat /dev/console
cat /sys/class/tty/console/dev

本机是字符设备 major:minor = 5:1。/dev/console 本身通常不会直接显示成某个具体的 /dev/tty2，
它由内核根据当前 console 驱动和活动终端进行转发。

需要区分两类输出：

1. 写入 /dev/console

echo hello >/dev/console

这会写到内核选定的 console，也就是本机的 tty0，实际显示在活动 VT tty2。

2. 内核 printk

dmesg
journalctl -k

内核日志可能同时被 console、/dev/kmsg、systemd-journald 等读取。即使某条日志出现在 journal
中，也不代表它是通过 /dev/console 写入的。

如果需要观察哪些进程当前打开了 /dev/console，可以用：

sudo lsof /dev/console
sudo fuser -v /dev/console

这些只能看到当前仍保持打开状态的进程；想确认某个程序是否实际写入，可以跟踪它：

strace -ff -e trace=openat,write your-command

对于本机，最关键的判断链就是：

/dev/console
  -> 内核 console tty0
  -> 当前活动虚拟终端 tty2




## Links
- https://access.redhat.com/articles/3166931 : 基本原理和 console= 的配置方法
- https://www.kernel.org/doc/html/latest/admin-guide/serial-console.html

## dmesg -D 和 dmesg -E 来禁用

在此了解内核中的 console 和 dmesg 差别了

dmesg -D 之后，内核中的内存缓冲区还是会有记录的。


基本流程为:

```txt
  dmesg -D                         dmesg -E
      │                                │
      │ type = 6                       │ type = 7
      │ SYSLOG_ACTION_CONSOLE_OFF       │ SYSLOG_ACTION_CONSOLE_ON
      └────────────────┬───────────────┘
                       ▼
                libc 的 klogctl()
                       ▼
                syslog 系统调用
                       ▼
                  do_syslog()
```

这里的 syslog 系统调用负责内核日志，与用户态的 syslog() 日志库函数不同。操作编号定义在 /home/martins3/data/kernel/linux/include/linux/syslog.h。

do_syslog() 中的核心代码如下，省略了其他操作：

```txt
static int saved_console_loglevel = LOGLEVEL_DEFAULT;

/* dmesg -D */
case SYSLOG_ACTION_CONSOLE_OFF:
    if (saved_console_loglevel == LOGLEVEL_DEFAULT)
        saved_console_loglevel = console_loglevel;

    console_loglevel = minimum_console_loglevel;
    break;

/* dmesg -E */
case SYSLOG_ACTION_CONSOLE_ON:
    if (saved_console_loglevel != LOGLEVEL_DEFAULT) {
        console_loglevel = saved_console_loglevel;
        saved_console_loglevel = LOGLEVEL_DEFAULT;
    }
    break;
```

其实就是设置一下 printk 的 level 而已。

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
