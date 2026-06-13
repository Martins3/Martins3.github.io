w 这个命令很有意思：

```txt
 22:12:15 up 13:33,  2 users,  load average: 0.18, 0.18, 0.11
USER     TTY        LOGIN@   IDLE   JCPU   PCPU WHAT
martins3 seat0     08:39    0.00s  0.00s  0.00s /nix/store/4kaf1d2iv5j3nx88cw34j07m56v0s32s-gdm-44.1/libexec/gdm-x-session --run-script /nix/store/5vphq1fywflrx1kwybz1k347zm7
martins3 :0        08:39   ?xdm?  31:20   0.00s /nix/store/4kaf1d2iv5j3nx88cw34j07m56v0s32s-gdm-44.1/libexec/gdm-x-session --run-script /nix/store/5vphq1fywflrx1kwybz1k347zm7
```

```txt
 15:44:31 up 2 days,  1:55,  6 users,  load average: 0.00, 0.01, 0.02
USER     TTY        LOGIN@   IDLE   JCPU   PCPU WHAT
martins3 tty2      Sun13    2days  2:21   0.05s /nix/store/vjn4m6rmbnwc20dc6fg7d9ngwz8nck59-gnome-session-47.0.1/libexec/gnome-session-binary
martins3           10:11    3:31m  0.00s   ?    sshd-session: martins3 [priv]
martins3           09:25    3:31m  0.00s   ?    sshd-session: martins3 [priv]
martins3 tty1      Mon19    4:55m  2.11s  2.11s -zsh
martins3           Sun13    3:31m  0.00s  5.39s /nix/store/xv7q10lk4lxfax7naj3b63aj2pyjv9gb-systemd-256.10/lib/systemd/systemd --user
```

这里显示的 user 不知道是从那里搞出来了?

seat0 是啥? 什么是 virtul terminal ?

- https://unix.stackexchange.com/questions/634419/multiseat-setup-disable-display-manager-and-use-tty-on-one-seat

## 太神奇了

如果 cat /proc/ttyS0 ，那么
cat /proc/interrupts | grep ttyS1 才会显示出来

找到这个动态创建的过程!

https://access.redhat.com/articles/3166931

## 分析下在容器中这里面的 pts/0 是咋做的

```txt
🧀  podman top genshin

USER        PID         PPID        %CPU        ELAPSED        TTY         TIME        COMMAND
root        1           0           0.000       44.729600291s  pts/0       0s          /bin/bash
```

## ARM 中的 ttyAMA0 是什么东西?

## 这个东西
https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script

## 收集材料

https://news.ycombinator.com/item?id=39506929

https://registerspill.thorstenball.com/p/how-to-lose-control-of-your-shell

## tty 整理下 ?

- https://cumtchw.blog.csdn.net/article/details/133343779?spm=1001.2014.3001.5502
- https://www.bilibili.com/video/BV1394y1L7Yn/?spm_id_from=333.999.0.0&vd_source=9ca354b6f3f02f29128fcb4b09ae5f0c

- https://bxt.rs/blog/just-how-much-faster-are-the-gnome-46-terminals/

  - 好的，以后都需要使用电竞屏才可以用终端了

- https://news.ycombinator.com/item?id=40419325

## 分析下，到底那个 tty 是给 vga 使用的

现象 1 :
在台式机上 intel 13900K，同时存在集成显卡和 nvidia 的显卡，默认启用 nvidia 的显卡，可以观察到：

1. 启动的日志是通过屏幕输出的，但是之后会暂停，并没有没有出现登录窗口。

- [ ] 是不是使用参数 efifb:off 之后，intel GPU 也可以输出 ?

实验 2:

在 mac 上安装 asahi ，启动之后可以进入到界面中:

ssh 到 mac 上:

```txt
root@bogon:/dev# for i in tty*; do echo $i > $i ; done
tty
```

实验 3:

在 asahi linux 中，内核日志会默认的导向到屏幕上。

如果使用 rk 启动虚拟机，其效果是完全相同的。

实验 4 :

将 intel GPU 直通之后，屏幕的变化是什么?

实验 5 ：

我发现在 asahi linux 中安装 fedora nixos ubuntu 都有些奇怪，例如 ubuntu 的安装
![](./img/ubuntu-install.png)

实验 6 :

理解下这个文档吧:

```txt
console=        [KNL] Output console device and options.
```

实验 7:

似乎 arm 的 不使用 x86 用的 console=ttyS0

例如现在 ubuntu 启动，完全没日志啊!

实验 8:

使用上如下的任何一个，替换 kernel 的 x86 rk 都是可以输出的:

```txt
	kernel_args+=" console=ttyS0,115200n8 console=tty0 "
```

```txt
	kernel_args+=" console=ttyS0,9600 earlyprintk=serial "
```

替换 kernel 的 aarch64 rk 的无需配置参数都是可以正常运行的。

ubuntu 不替换内核，启动默认没有日志，但是配置了 `kernel_args+=" console=ttyS0,115200n8 console=tty0"` 之后，启动之后存在日志。

参考实现: https://access.redhat.com/articles/3166931

实验 9:

oe 虚拟机中为什么有如下四个

ttyS0% ttyS1% ttyS2% ttyS3%

```txt
-serial mon:stdio \
-serial pipe:/home/martins3/hack/vm/oe2/pipe1  \
-serial pipe:/home/martins3/hack/vm/oe2/pipe2  \
-device virtio-serial \
-chardev pty,id=virtiocon0 \
-device virtconsole,chardev=virtiocon0 \
```

测试:

ttyS0% stdio
ttyS1% pipe
ttyS2% pipe
ttyS3% pty

## 如何理解串口的 pts

使用 minicom 来连接: minicom -D /dev/pts/5 qemu 提供的 pts ，最后是
可以登录的。

## 为什么会开机就卡住?

### efifb:off : 5.10 kernel 中问题

### console=ttyAMA0

## 通过分析 qemu 的 -device virtio-gpu-pci ，再一次理解了如果将 VGA 作为 serial 的输出

## 有趣的观察，标准输入输出在 systemd 中切换了

systemd 中程序

```txt
total 0
dr-x------ 2 root root  0 Jun  3 15:41 .
dr-xr-xr-x 9 root root  0 Jun  3 15:41 ..
lr-x------ 1 root root 64 Jun  3 15:41 0 -> /dev/null
lrwx------ 1 root root 64 Jun  3 15:41 1 -> 'socket:[37045]'
lrwx------ 1 root root 64 Jun  3 15:41 2 -> 'socket:[37045]'
lrwx------ 1 root root 64 Jun  3 15:41 7 -> 'socket:[25320]'
```

普通模式运行:

```txt
lrwx------ - martins3  4 Jun 12:18  0 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  1 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  2 -> /dev/pts/17
lrwx------ - martins3  4 Jun 12:18  7 -> socket:[11477421]
```

## 关机的时候，消息会 broadcast 到所有的 console 上

Broadcast message from

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

## qemu 启动的时候，这个输出的含义是什么?

```txt
char device redirected to /dev/pts/8 (label virtiocon0)
```

### 为什么 console 可以多次定义 ?

## 看看这个文档

- Documentation/admin-guide/serial-console.rst

## 对比一下 kunpeng 的机器如何对比

## arm 中的 tty 的

## 可以做的事情

- [ ] 将 kunpeng 和 feiteng 机器都配置上 kernel 的参数，相同的
- [x] 看 ubuntu 的参数
  - 这个 ubuntu 参数有问题，但是还不够

## 这个还是有参考价值的

https://access.redhat.com/articles/3166931

> The primary console for system output will be the last console listed in the kernel parameters. In the above example, the VGA console tty0 is the primary and the serial console is the secondary display. This means messages from init scripts will not go to the serial console, since it is the secondary console, but boot messages and critical warnings will go to the serial console. If init script messages need to be seen on the serial console as well, it should be made the primary by swapping the order of the console parameters:

tty0 为什么会是 VGA
ttyS0 是 serial 的

## 所以，最后一个问题，如果是 ssh ，使用的是什么东西?

## 非常惊讶，minicom 比想象的要复杂

https://salsa.debian.org/minicom-team/minicom

难道接受的字符流需要特殊处理?

还是需要买一个 serial 吧，测试下 ttyUSB0 吧

## 似乎 usb serila 完全可以的

https://github.com/qemu/qemu/blob/master/hw/usb/dev-serial.c

## ttyUSB0 应该可以在 QEMU 中测试

## 那么什么是 ttyACM0

https://unix.stackexchange.com/questions/719351/qemu-pass-tty-device-from-host-to-guest-machine

https://rfc1149.net/blog/2013/03/05/what-is-the-difference-between-devttyusbx-and-devttyacmx/

## 确认一个事情

```txt
	kernel_args+="  console=ttyS1 console=tty0 console=ttyAMA0"
```

现在观察来看，如果 console 后的类型不同，那么是可以复用的。

## ls -la /sys/class 吧

## 通过 sysfs tty 找到每一个 tty / vt /pts 到底属于什么 !


## console 的含义到底是什么， 似乎只是日志没有办法输出，而不是完全不可以用

1. 现象 1 :

将 kernel 的启动参数从

```txt
	kernel_args+=" console=ttyAMA0 console=ttyS0 console=tty0 "
```

修改为

```txt
	kernel_args+=" console=ttyAMA0 console=ttyS0  "
```

可以看到 vnc 中还是可以登录

2. 现象 2 :
  1. efifb:off
  2. intel CPU 集成显卡的连接启动，kernel 实际上无法直接

## bcc-tools 的 ttysnoop 关注一下


## bash -l 的意义是什么

1. podman 执行这个:
```txt
podman exec -it magic bash
```
但是会发现，tmux 的命令提示符很奇怪

但是，如果:
```txt
➜  podman exec -it magic "bash -l"
Error: can only create exec sessions on running containers: container state improper
```
2. wez 中，如果 ssh tmux 过去，也是命令提示符很奇怪

## docker -it 和 docker -dt 中

-t 的 tty 的含义，这个如何理解！

## docker 中 bash 中，为什么 $USER 没有定义
而到来 tmux 中，又是定义了

## 如何理解 ps aux 的输出中的 pts/0 和 pts/1 的含义

```txt
🦭 🧀  ps aux
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root           1  0.0  0.0   4492  3600 pts/0    Ss+  03:20   0:00 bash
root           9  0.3  0.0   7436  6264 pts/1    Ss   03:21   0:00 zsh
root         778  0.0  0.0   7760  4456 pts/1    S+   03:21   0:00 tmux
root         780  0.2  0.0  10304  5784 ?        Ss   03:21   0:00 tmux
root        1094  0.6  0.0   7432  6488 pts/2    Ss   03:21   0:00 -zsh
root        1321  0.0  0.0   6896  3832 pts/2    R+   03:21   0:00 ps aux
```

## n100 有这个日志
```txt
[   11.528480] fbcon: Taking over console
[   11.537260] Console: switching to colour frame buffer device 480x135
```

## 看看硬件

- ch340 驱动是什么?
  - 似乎只是找到了 : drivers/usb/serial/ch341.c
- 即使是 serial 也是需要有驱动的吧?
  - 那种 serial 转 usb 的驱动在那里?

## 可以理解这个插件的原理吗?
```txt
set -g @plugin 'tmux-plugins/tmux-resurrect' # 关机保存 session
```
## 好吧
既然都 8250 了
```txt
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
```

## 这个是做啥的
```txt
config TTY_PRINTK
	tristate "TTY driver to output user messages via printk"
	depends on EXPERT && TTY
	default n
	help
	  If you say Y here, the support for writing user messages (i.e.
	  console messages) via printk is available.

	  The feature is useful to inline user messages with kernel
	  messages.
	  In order to use this feature, you should output user messages
	  to /dev/ttyprintk or redirect console to this TTY, or boot
	  the kernel with console=ttyprintk.

	  If unsure, say N.
```

## 如果使用 pxe 启动的时候，这个时候的键盘鼠标是什么提供的?

需要 seabios 有键盘驱动吗?

## 这个也需要看看的做什么的
fs/devpts/

https://unix.stackexchange.com/questions/93531/what-is-stored-in-dev-pts-files-and-can-we-open-them

```c
static struct file_system_type devpts_fs_type = {
	.name		= "devpts",
	.mount		= devpts_mount,
	.kill_sb	= devpts_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};
```

## 看看类似 busybox 中的是如何处理 tty 的

当然，这里也是一个
https://github.com/Sweets/hummingbird

## 理解下 tty 的 job control 整理下
https://stackoverflow.com/questions/11821378/what-does-bashno-job-control-in-this-shell-mean

为什么 kernel 的 signal 机制中需要考虑到 job control ?

## 那么标准输入，输出和 /dev/stderr 是什么关系?

或者说 /dev/stderr 是如何实现的?

## 那么这个也是 tty 吗?
https://github.com/tsl0922/ttyd
https://github.com/elisescu/tty-share?utm_source=chatgpt.com
https://github.com/ioi/isolate?utm_source=chatgpt.com
https://github.com/mitsuhiko/teetty?utm_source=chatgpt.com

## 最后，说明一下如下内容

说明一下当键盘按下一个键，到屏幕显示结果的过程
1. 中断
2. 字符设备
3. tty 等等


## serial 和 uart 是什么关系?

## 是的，有趣的
https://twobithistory.org/2019/08/22/readline.html

## 这个也是可以看看的
/home/martins3/core/vn/docs/linux/tlpi/6/tlpi-chapter-62.md


## 好吧，原来 readline 也是可以替代的
- https://github.com/akinomyoga/ble.sh

## neovim terminal 也是一个 terminal emulator 啊

对比一下 voidkiss temrinal 和 toggleterm ，两者必然不一样，
其中，voidkiss 执行 gc -s 会自动进入当前的 neovim 中

## hibernation 也是会有 tty 哦
https://news.ycombinator.com/item?id=41483789

## 又一个
https://github.com/ghostty-org/ghostty

## 居然有这个服务
```txt
🧀  sudo systemctl status systemd-vconsole-setup
[sudo] password for martins3:
Sorry, try again.
[sudo] password for martins3:
● systemd-vconsole-setup.service - Virtual Console Setup
     Loaded: loaded (/etc/systemd/system/systemd-vconsole-setup.service; linked; preset: ignored)
     Active: active (exited) since Thu 2025-02-27 14:53:59 CST; 2 days ago
 Invocation: 1b89ee8a1ea74877bb86b6d7f5cb55c9
       Docs: man:systemd-vconsole-setup.service(8)
             man:vconsole.conf(5)
   Main PID: 1061 (code=exited, status=0/SUCCESS)
         IO: 3.1M read, 0B written
   Mem peak: 6.5M
        CPU: 15ms

Feb 27 14:53:59 nixos systemd[1]: Starting Virtual Console Setup...
Feb 27 14:53:59 nixos systemd[1]: Finished Virtual Console Setup.
```

显然 systemd 配置了 启动之后，如果 scsi 没有安装之类的，会发现系统日志会卡到这里，

```txt
[  OK  ] Finished dracut pre-udev hook.
         Starting Rule-based Manager for Device Events and Files...
[  OK  ] Started Rule-based Manager for Device Events and Files.
         Starting Coldplug All udev Devices...
[  OK  ] Finished Coldplug All udev Devices.
[  OK  ] Reached target System Initialization.
[  OK  ] Reached target Basic System.
         Starting dracut initqueue hook...
[  OK  ] Stopped Virtual Console Setup.
         Stopping Virtual Console Setup...
         Starting Virtual Console Setup...
[  OK  ] Finished Virtual Console Setup.
```

  Docs: man:systemd-vconsole-setup.service(8)
        man:vconsole.conf(5)


内核启动的日志中有这个，这个是那个服务来搞的:

```txt
[  OK  ] Started Getty on tty1.
[  OK  ] Started Serial Getty on hvc0.
[  OK  ] Started Serial Getty on ttyS0.
```
## arm qemu 中的 console 是 ttyAMA0
echo 1 | sudo tee /dev/ttyAMA0

是这个配置导致的吗?
```txt
console=tty0 console=ttyS0   panic=0 nokaslr panic=0 selinux=0 console=0 console=abc  console=ttyAMA0  systemd.unified_cgroup_hierarchy=1  mitigations=off  rcutree.sysrq_rcu=1  crashkernel=512M  nfs.recover_lost_locks=1  loglevel=8 zswap.enabled=0  ' \
```

可能性不大，默认配置也是 ttyAMA0
```txt
🧀  cat /proc/cmdline
BOOT_IMAGE=/vmlinuz-6.6.0-28.0.0.34.oe2403.aarch64 root=/dev/mapper/openeuler-root rd.lvm.lv=openeuler/root rd.lvm.lv=openeuler/swap video=VGA-1:640x480-32@60me cgroup_disable=files crashkernel=1024M,high smmu.bypassdev=0x1000:0x17 smmu.bypassdev=0x1000:0x15 console=tty0
```

## 在 vnc 的屏幕中执行 htop
```txt
🧀  ps -elf | grep htop
0 S martins3    3359    2315  0  80   0 - 58305 do_sys 17:36 tty1     00:00:00 htop
0 S martins3    3769    3649  0  80   0 - 58302 do_sys 17:39 ttyS0    00:00:00 htop
```
为什么这里展示的还是 tty1 而不是 tty0 ?
这里的 ttyS0 展示是对的

```txt
🧀  pstree -h
systemd─┬─NetworkManager─┬─dhclient
        │                └─3*{NetworkManager}]
        ├─agetty
        ├─chronyd
        ├─crond
        ├─dbus-daemon
        ├─gssproxy───5*[{gssproxy}]
        ├─irqbalance───{irqbalance}
        ├─2*[login───zsh───htop]
        ├─ovs-vswitchd───5*[{ovs-vswitchd}]
        ├─ovsdb-server
        ├─polkitd───3*[{polkitd}]
        ├─pueued───6*[{pueued}]
        ├─rngd───4*[{rngd}]
        ├─rpcbind
        ├─rsyslogd───2*[{rsyslogd}]
        ├─sshd─┬─2*[sshd───sshd───zsh]
        │      └─sshd───sshd───zsh───pstree
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-udevd
        ├─tailscaled───9*[{tailscaled}]
        └─tuned───3*[{tuned}]
[
```
注意看，这里有两个 login ，分别对应 vnc 和 serial 的

## asahi linux 没有串口，他们是如何调试的？

## 观察 n100 的 bridge 才发现，有的日志可以不是内核，就直接刷到 console 中了

似乎 syslogd 的日志会刷新到 console 中

## 可以最后思考一下这个东西是如何实现的

说了这么多复杂的东西，那么
https://github.com/tsl0922/ttyd

## 有趣的
https://news.ycombinator.com/item?id=43346816

## mesg 这个工具做什么的
https://www.man7.org/linux/man-pages/man1/mesg.1.html

## 这个是有趣的

```txt
作者：Yuhao
链接：https://zhuanlan.zhihu.com/p/510289859
来源：知乎
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。

config SERIAL_AMBA_PL011_CONSOLE
    bool "Support for console on AMBA serial port"
    depends on SERIAL_AMBA_PL011=y
    select SERIAL_CORE_CONSOLE
    select SERIAL_EARLYCON
    help
      Say Y here if you wish to use an AMBA PrimeCell UART as the system
      console (the system console is the device which receives all kernel
      messages and warnings and which allows logins in single user mode).

      Even if you say Y here, the currently visible framebuffer console
      (/dev/tty0) will still be used as the system console by default, but
      you can alter that using a kernel command line option such as
      "console=ttyAMA0". (Try "man bootparam" or see the documentation of
      your boot loader (lilo or loadlin) about how to pass options to the
      kernel at boot time.)
```

如何操作这个:
```c
static void fn_show_ptregs(struct vc_data *vc)
{
	struct pt_regs *regs = get_irq_regs();

	if (regs)
		show_regs(regs);
}
```

看上去就是 kbd_event -> kbd_keycode
-> 	(*k_handler[type])(vc, keysym & 0xff, !down);
-> k_spec
-> fn_show_ptregs

但是，不知道该如何触发

kbd_event 在 qemu 中可以触发吗?
在 vnc 界面可以有 virtual console 吗?

为什么这个东西是
https://web.git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=50145474f6ef4a9c19205b173da6264a644c7489

## 我们对于 sysrq 有重大的理解错误

一直以为 sysrq 只有通过 /proc/sysrq-trigger 可以，实际上，还可以在硬中断中:
```c
static void sysrq_handle_showregs(u8 key)
{
	struct pt_regs *regs = NULL;

	if (in_hardirq())
		regs = get_irq_regs();
	if (regs)
		show_regs(regs);
	perf_event_print_debug();
}
```
那么就说明，其实，是通过 serial 来连接的话，那么这个时候，sysrq_handle_showregs 就不是
通过  /proc ，但是，问题是，我该在串口中输入什么东西才可以?
https://docs.kernel.org/admin-guide/sysrq.html

- https://docs.kernel.org/admin-guide/sysrq.html
- https://superuser.com/questions/786545/how-to-send-a-break-on-a-serial-port-from-command-line-in-raspbian-linux

## 从 kernel config 中切入也是极好的
```txt
CONFIG_TTY=y
CONFIG_VT=y
CONFIG_CONSOLE_TRANSLATIONS=y
CONFIG_VT_CONSOLE=y
CONFIG_VT_CONSOLE_SLEEP=y
# CONFIG_VT_HW_CONSOLE_BINDING is not set
CONFIG_UNIX98_PTYS=y
CONFIG_LEGACY_PTYS=y
CONFIG_LEGACY_PTY_COUNT=256
CONFIG_LEGACY_TIOCSTI=y
CONFIG_LDISC_AUTOLOAD=y
```

## 似乎 qemu 还可以和 minicom 协同起来?

## 是在没有想到 screen 还可以这么使用
https://stackoverflow.com/questions/20190116/serial-pty-in-qemu-how-to-open

## qemu 的 info chardev 也是一个好工具
```txt
(qemu) info chardev
serial2: filename=pipe
serial0-base: filename=stdio
serial1: filename=pipe
seabios: filename=file
mon1: filename=disconnected:unix:/home/martins3/hack/vm/2403-nix/s/qmp,server=on
serial3: filename=file
mon2: filename=unix:/home/martins3/hack/vm/2403-nix/s/hmp,server=on
compat_monitor0: filename=disconnected:unix:/home/martins3/hack/vm/2403-nix/qmp.sock,server=on
mon3: filename=disconnected:unix:/home/martins3/hack/vm/2403-nix/s/qmp-shell,server=on
cmdout: filename=disconnected:unix:/home/martins3/hack/vm/2403-nix/s/cmdout.sock,server=on
qga0: filename=disconnected:unix:/home/martins3/hack/vm/2403-nix/s/qga.sock,server=on
virtiocon0: filename=pty:/dev/pts/5
serial0: filename=mux
```

## 原来 Getty 是做这个的
https://superuser.com/a/1797404

## 为什么 qemu 的 pc-bios/ 需要定义 keymap

## 顺便问一个问题，在 hmp 中输入 sendkey ，是通过那个设备发送给虚拟机的

https://www.reddit.com/r/programming/comments/41u5hw/comment/cz5ejh6/
https://news.ycombinator.com/item?id=38984096

> A tty can refer to a device name, or a physical terminal.
> A terminal can refer to a physical or virtual terminal.
> A console could also refer to a physical device, although I'm not aware of it being used in this context.

## 为什么 n100 的机器上四个 ttyS1~S4

而且 xiaomi 的机器上
```txt
🧀  ls -la /dev/ttyS*
crw-rw----@ 4,64 root 11 Mar 06:24  /dev/ttyS0
crw-rw----@ 4,65 root 11 Mar 06:24  /dev/ttyS1
crw-rw----@ 4,66 root 11 Mar 06:24  /dev/ttyS2
crw-rw----@ 4,67 root 11 Mar 06:24  /dev/ttyS3
crw-rw----@ 4,68 root 11 Mar 06:24  /dev/ttyS4
crw-rw----@ 4,69 root 11 Mar 06:24  /dev/ttyS5
crw-rw----@ 4,70 root 11 Mar 06:24  /dev/ttyS6
crw-rw----@ 4,71 root 11 Mar 06:24  /dev/ttyS7
crw-rw----@ 4,72 root 11 Mar 06:24  /dev/ttyS8
crw-rw----@ 4,73 root 11 Mar 06:24  /dev/ttyS9
crw-rw----@ 4,74 root 11 Mar 06:24  /dev/ttyS10
crw-rw----@ 4,75 root 11 Mar 06:24  /dev/ttyS11
crw-rw----@ 4,76 root 11 Mar 06:24  /dev/ttyS12
crw-rw----@ 4,77 root 11 Mar 06:24  /dev/ttyS13
crw-rw----@ 4,78 root 11 Mar 06:24  /dev/ttyS14
crw-rw----@ 4,79 root 11 Mar 06:24  /dev/ttyS15
crw-rw----@ 4,80 root 11 Mar 06:24  /dev/ttyS16
crw-rw----@ 4,81 root 11 Mar 06:24  /dev/ttyS17
crw-rw----@ 4,82 root 11 Mar 06:24  /dev/ttyS18
crw-rw----@ 4,83 root 11 Mar 06:24  /dev/ttyS19
crw-rw----@ 4,84 root 11 Mar 06:24  /dev/ttyS20
crw-rw----@ 4,85 root 11 Mar 06:24  /dev/ttyS21
crw-rw----@ 4,86 root 11 Mar 06:24  /dev/ttyS22
crw-rw----@ 4,87 root 11 Mar 06:24  /dev/ttyS23
crw-rw----@ 4,88 root 11 Mar 06:24  /dev/ttyS24
crw-rw----@ 4,89 root 11 Mar 06:24  /dev/ttyS25
crw-rw----@ 4,90 root 11 Mar 06:24  /dev/ttyS26
crw-rw----@ 4,91 root 11 Mar 06:24  /dev/ttyS27
crw-rw----@ 4,92 root 11 Mar 06:24  /dev/ttyS28
crw-rw----@ 4,93 root 11 Mar 06:24  /dev/ttyS29
crw-rw----@ 4,94 root 11 Mar 06:24  /dev/ttyS30
crw-rw----@ 4,95 root 11 Mar 06:24  /dev/ttyS31
```

## 这几个驱动，如何理解
```txt
kernel/drivers/tty/n_null.ko
kernel/drivers/tty/serial/serial_base.ko
kernel/drivers/tty/serial/8250/8250.ko
kernel/drivers/tty/serial/8250/8250_base.ko
kernel/drivers/tty/serial/8250/8250_exar.ko
kernel/drivers/tty/serial/8250/8250_lpss.ko
kernel/drivers/tty/serial/8250/8250_mid.ko
kernel/drivers/tty/serial/8250/8250_pci.ko
kernel/drivers/tty/serial/8250/8250_pericom.ko
kernel/drivers/iommu/iova.ko
kernel/drivers/base/firmware_loader/firmware_class.ko
kernel/drivers/block/loop.ko
kernel/drivers/misc/eeprom/eeprom_93cx6.ko
kernel/drivers/misc/mei/mei.ko
kernel/drivers/misc/mei/mei-me.ko
kernel/drivers/scsi/scsi_mod.ko
kernel/drivers/scsi/scsi_common.ko
kernel/drivers/scsi/sd_mod.ko
kernel/drivers/scsi/sr_mod.ko
kernel/drivers/cdrom/cdrom.ko
kernel/drivers/input/serio/serio.ko
kernel/drivers/input/serio/i8042.ko
kernel/drivers/input/serio/serport.ko
kernel/drivers/input/serio/libps2.ko
kernel/drivers/input/input-core.ko
kernel/drivers/input/vivaldi-fmap.ko
kernel/drivers/input/keyboard/atkbd.ko
kernel/drivers/input/mouse/psmouse.ko
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
