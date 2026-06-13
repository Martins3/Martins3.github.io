# console
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

## 原来配置两次，可以输出两次啊
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
cat /sys/class/tty/console/active
tty0 ttyS0

## /proc/consoles
这个 proc 的作用是什么?

```txt
🧀  cat /proc/consoles
tty0                 -WU (EC p  )    4:2k
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


## 如果直接在虚拟机中操作

如果 ssh 到虚拟机中:
```txt
🧀  ls -la /proc/self/fd
lrwx------ - martins3 18 Dec 08:24 0 -> /dev/pts/25
lrwx------ - martins3 18 Dec 08:24 1 -> /dev/pts/25
lrwx------ - martins3 18 Dec 08:24 2 -> /dev/pts/25
lr-x------ - martins3 18 Dec 08:24 3 -> /proc/1079992/fd
```

如果直接在 serial 或者 vnc 中:
```txt
lrwx------ 1 root root 64 Dec 18 21:27 0 -> /dev/tty1
l-wx------ 1 root root 64 Dec 18 21:27 1 -> /root/tty1
lrwx------ 1 root root 64 Dec 18 21:27 2 -> /dev/tty1
lr-x------ 1 root root 64 Dec 18 21:27 3 -> /proc/1860/fd
```
(为什么不是 tty0 ?)

```txt
lrwx------ 1 root root 64 Dec 18 21:25 0 -> /dev/hvc0
lrwx------ 1 root root 64 Dec 18 21:25 1 -> /dev/hvc0
lrwx------ 1 root root 64 Dec 18 21:25 2 -> /dev/hvc0
lr-x------ 1 root root 64 Dec 18 21:25 3 -> /proc/1228/fd
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
