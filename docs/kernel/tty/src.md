还有好几个文章可以看看
http://www.wowotech.net/345.html


https://hn.algolia.com/?q=tty

https://dev.to/napicella/linux-terminals-tty-pty-and-shell-part-2-2cb2

## serial 的文档这么长
https://tldp.org/HOWTO/Serial-HOWTO.html#toc19

## 很好
终端、Shell、tty 和控制台（console）有什么区别？ - 大川的回答 - 知乎
https://www.zhihu.com/question/21711307/answer/2231006377

## 在这里有好几篇关于 terminal 相关的东西，不错
https://kernel.guide/

https://news.ycombinator.com/item?id=43452789

## 这里的第一个 common
https://news.ycombinator.com/item?id=43261600

从 80s 使用 tty ????

> I like tmux a lot, but like its predecessor "screen" I mostly use it for explicitly running long-lived jobs (i.e. for its detach feature), and for very special situations where I have elaborate tmux window configurations with dedicated stuff running in each window/pane.
> Note that I have been using text-only terminals since the 1980s, but I've adapted my tty usage over time.
>
> The problem that tmux (or screen) brings are first and foremost:
>
> * Smooth/fast scrolling goes away. I can no longer give my trackpad a slight push to find myself tens or hundreds of lines in the scrollback history, and visually scan by slightly pushing my fingers back and forth. Instead I have to use the horrendous in-tmux scrollback using "Ctrl-b [".
>
> * My terminal app's tabs and windows are not tmux's tabs and windows. I cannot freely arrange them in space, snap them off with the mouse, easily push them to another desktop, and so on. I have to start a multiple tmux clients and do awkward keyboard interactions with them for any of the same.
>
> * tmux's terminal emulation and my terminal emulator's terminal emulation (heh) are not congruent. As a result, programs cannot make full use of my actual terminal's capabilities. For example selecting, copying, and pasting text sometimes behave weirdly, and there are other annoyances.
>
> What I'd really like to have instead is terminal session management at a higher level, i.e. involving my actual graphical terminal app itself. Attaching to a running session would mean restoring the terminal app's windows and tabs, and the entire scrollback history within (potentially with some lazy loading).
>
> tmux could likely be a major part of that, by providing the option of replacing its tty-facing frontend with a binary protocol that the graphical terminal app talks to, while keeping the backend (i.e. the part that provides the tty to anything running inside it) the same as it is today.
>
> As it is, the downsides of using tmux all the time are too high.

## 不知道为什么，似乎 tty 总是和 keyboard 有关
看看这个有趣的
https://news.ycombinator.com/item?id=43520297

## 这么一想，岂不是 neovide 和 alacritty 的功能很类似吗?
https://github.com/neovide/neovide

## 有趣的
printf是怎么输出到控制台的呢？ - 闪光吧Linux的回答 - 知乎
https://www.zhihu.com/question/456916638/answer/3099313413

## gotty
https://github.com/yudai/gotty

还有高手:
https://github.com/tsl0922/ttyd

## 这里的 Code With Copilot Agent Mode 获取的 VSCODE 中的 terminal 都是如何实现的 ?
https://github.com/Martins3/My-Linux-Config/issues/189


## last 命令，好东西

## https://github.com/trzsz/trzsz-ssh 中提供了一个 option

-t -T RequestTTY

这个如何理解

## 有趣的东西，又是 framebuffer ，又是 tty 的
https://news.ycombinator.com/item?id=43931845

## [ ] virsh 命令中，console 和 ttyconsole 有啥区别

https://news.ycombinator.com/item?id=43971716

## 看看这个代码分析
https://news.ycombinator.com/item?id=41483789

而且有分析了 tty

## 为什么 qemu 中的 printk 会这么慢

每次 kmalloc 之后 pr_info 一次，perf 了一下，基本上所有的时间都是在 print 上。
```txt
  100.00%     0.00%  insmod   [unknown]          [k] 0x20646564616f6c00
     0x20646564616f6c00
   - syscall
      - 99.90% init_module
         - 99.90% printk
              vprintk_emit
              console_unlock
            - vt_console_print
               - 93.99% lf
                    con_scroll
                  - fbcon_scroll
                     - 93.01% fbcon_redraw.isra.19
                        - 90.32% fbcon_putcs
                           - 87.55% bit_putcs
                              - 84.48% drm_fb_helper_cfb_imageblit
                                   cfb_imageblit
                                1.47% drm_fb_helper_dirty.isra.27
                             0.73% get_color
                          1.28% console_conditional_schedule
                     - 0.95% bit_clear
                          drm_fb_helper_cfb_fillrect
                          cfb_fillrect
                          bitfill_aligned
               - 5.86% fbcon_putcs
                    bit_putcs
                    drm_fb_helper_cfb_imageblit
                    cfb_imageblit
```

## src 中的东西结果为

vmware 中的结果
```txt
  kbd_event
  input_to_handler
  input_pass_values.part.0
  input_event_dispose
  input_handle_event
  input_event
  atkbd_receive_byte
  ps2_interrupt
  serio_interrupt
  i8042_interrupt
  __handle_irq_event_percpu
  handle_irq_event
  handle_edge_irq
  __common_interrupt
  common_interrupt
  asm_common_interrupt
  acpi_safe_halt
  acpi_idle_do_entry
  acpi_idle_enter
  cpuidle_enter_state
  cpuidle_enter
  cpuidle_idle_call
  do_idle
  cpu_startup_entry
  start_secondary
  secondary_startup_64_no_verify
```

和 qemu 中结果无差别
```txt
@[
kbd_event+5
input_handle_events_default+66
input_pass_values+307
input_event_dispose+322
input_event+78
atkbd_receive_byte+1350
ps2_interrupt+158
serio_interrupt+71
i8042_handle_data+247
i8042_interrupt+17
__handle_irq_event_percpu+74
handle_irq_event+59
handle_edge_irq+151
__common_interrupt+62
common_interrupt+128
asm_common_interrupt+38
pv_native_safe_halt+15
default_idle+19
default_idle_call+48
do_idle+437
cpu_startup_entry+41
start_secondary+247
common_startup_64+318
]: 69
```
忽然意识到，这个东西是在硬中断里面的。

## 想不到还有这个目录
/proc/sys/kernel/pty


## 为什么有的环境中，必须添加上 xterm-256color
cmd="TERM=xterm-256color ssh $ssh_port $ssh_user@$ssh_ip"

## netty
没有任何关系的东西


## 有趣
在Linux中，按上下左右键为什么变成^[[A^[[B^[[C^[[D？ - 海念着梦与夜的回答 - 知乎
https://www.zhihu.com/question/31429658/answer/3601508760

## 这里的 vt100 是什么意思
```txt

🧀  pstree --help
pstree: unrecognized option '--help'
Usage: pstree [-acglpsStTuZ] [ -h | -H PID ] [ -n | -N type ]
              [ -A | -G | -U ] [ PID | USER ]
   or: pstree -V

Display a tree of processes.

  -a, --arguments     show command line arguments
  -A, --ascii         use ASCII line drawing characters
  -c, --compact-not   don't compact identical subtrees
  -C, --color=TYPE    color process by attribute
                      (age)
  -g, --show-pgids    show process group ids; implies -c
  -G, --vt100         use VT100 line drawing characters
  -h, --highlight-all highlight current process and its ancestors
  -H PID, --highlight-pid=PID
                      highlight this process and its ancestors
  -l, --long          don't truncate long lines
  -n, --numeric-sort  sort output by PID
  -N TYPE, --ns-sort=TYPE
                      sort output by this namespace type
                              (cgroup, ipc, mnt, net, pid, time, user, uts)
  -p, --show-pids     show PIDs; implies -c
  -s, --show-parents  show parents of the selected process

```

## 继续看看
https://www.zhihu.com/question/65280843

## 什么东西
https://github.com/ghostty-org/ghostty

这是非常有趣的东西了，作者提到真的有趣的 libghostty 这个工具:
https://news.ycombinator.com/item?id=47206009

## 理解这几个命令的区别

ssh -t martins3@172.20.251.143 "bash -l -c './guest.sh'"

在远程中，放一个 test.sh ，其中 echo $PATH

ssh martins3@172.20.251.143 ./test.sh

ssh -t martins3@172.20.251.143 "bash ./test.sh"
   ssh martins3@172.20.251.143 "bash -l -c './test.sh'"

## 在 nvim 中 help terminal 可以有

```txt
Terminal emulator				*terminal* *terminal-emulator*

Nvim embeds a VT220/xterm terminal emulator based on libvterm. The terminal is
presented as a special 'buftype', asynchronously updated as data is received
from the connected program.

Terminal buffers behave like normal buffers, except:
- With 'modifiable', lines can be edited but not deleted.
- 'scrollback' controls how many lines are kept.
- Output is followed ("tailed") if cursor is on the last line.
- 'modified' is the default. You can set 'nomodified' to avoid a warning when
  closing the terminal buffer.
- 'bufhidden' defaults to "hide".
```

## ldd3 也分析过 tty 相关的东西

## 通过 dmesg -D 和 dmesg -E 在此了解内核中的 console 和 dmesg 差别了

dmesg -D 之后，内核中的内存缓冲区还是会有记录的。

## Xshell

## 有趣的东西
https://news.ycombinator.com/item?id=44728796

## 才意识到 telnet 是一个 insecure ssh
<!-- 91e6d422-0058-4c0f-b67c-ccf48dcb4cc3 -->

看上去 telnet 是相当容易实现的

qemu 都是支持 telnet 来作为起 monitor 的:
```txt
-monitor telnet:127.0.0.1:1234,server,nowait
```

## 看看这个东西
https://github.com/LukeGus/Termix

## unix 环境高级编程的中有一个章节讲解 pty 的

既然如此，想必 Linux programing interface 中也是有对应的章节了。

## https://github.com/libuv/libuv

有趣的:
```txt
ANSI escape code controlled TTY
```

## setsid 是做什么的，每次在写 initramfs 的 shell 的时候就会遇到这个问题。

docs/linux/tlpi/3/tlpi-chapter-34.md 之前分析过 job-control 的

## 为什么会有这个效果

```c
printf("\033[H\033[J"); // 清屏
```

## 为什么 tmux 在 windows 下没有

但是 wezterm 又是可以在 windows 下有的。

https://www.reddit.com/r/tmux/comments/l580mi/is_there_a_tmuxlike_equivalent_for_windows/

## 为什么类似 docker 改权限

以及 修改 ulimit
https://serverfault.com/questions/637212/increasing-ulimit-on-centos

都是需要 logout 才可以的?

## 为什么有时候 tty0 和 ttyS0 的输出不一样

这个事情其实经常观察到，也就是我们发现，
```txt
[    3.683365] evm: Initialising EVM extended attributes:
[    3.684762] evm: security.selinux
[    3.685676] evm: security.ima
[    3.686558] evm: security.capability
[    3.687479] evm: HMAC attrs: 0x1
[    3.690394] rtc_cmos 00:05: setting system clock to 2025-09-02 06:35:03 UTC (1756794903)
[    3.798937] usb 2-1: new full-speed USB device number 2 using uhci_hcd
[    3.939217] usb 2-1: not running at top speed; connect to a high speed hub
[    3.974800] usb 2-1: New USB device found, idVendor=0627, idProduct=0001, bcdDevice= 0.00
[    3.977150] usb 2-1: New USB device strings: Mfr=1, Product=3, SerialNumber=10
[    3.979045] usb 2-1: Product: QEMU USB Tablet
[    3.980185] usb 2-1: Manufacturer: QEMU
[    3.981237] usb 2-1: SerialNumber: 28754-0000:00:01.2-1
[    3.990441] input: QEMU QEMU USB Tablet as /devices/pci0000:00/0000:00:01.2/usb2/2-1/2-1:1.0/0003:0627:0001.0001/input/input5
[    3.993523] hid-generic 0003:0627:0001.0001: input,hidraw0: USB HID v0.01 Mouse [QEMU QEMU USB Tablet] on usb-0000:00:01.2-1/input0
[    4.304742] Freeing unused decrypted memory: 2040K
[    4.414756] Freeing unused kernel image memory: 2404K
[    4.419934] Write protecting the kernel read-only data: 18432k
[    4.425055] Freeing unused kernel image memory: 2012K
[    4.426572] Freeing unused kernel image memory: 324K
[    4.427759] Run /init as init process
[    4.431649] Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00
[    4.431649]
[    4.437813] Call Trace:
[    4.438499]  dump_stack+0x66/0x8b
[    4.439328]  panic+0xe4/0x292
[    4.440090]  do_exit+0x857/0xc30
[    4.440900]  do_group_exit+0x3a/0xa0
[    4.441823]  __x64_sys_exit_group+0x14/0x20
[    4.442859]  do_syscall_64+0x5f/0x240
[    4.443799]  entry_SYSCALL_64_after_hwframe+0x5c/0xc1
[    4.453611] Kernel Offset: 0x22600000 from 0xffffffff81000000 (relocation range: 0xffffffff80000000-0xffffffffbfffffff)
[    4.456003] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00
[    4.456003]  ]---
[    4.458398] ------------[ cut here ]------------
[    4.459532] sched: Unexpected reschedule of offline CPU#0!
[    4.460826] WARNING: CPU: 3 PID: 1 at arch/x86/kernel/smp.c:128 native_smp_send_reschedule+0x34/0x40
[    4.462858] Modules linked in:
[    4.467445] RIP: 0010:native_smp_send_reschedule+0x34/0x40
[    4.468758] Code: 05 e1 dc 41 01 73 15 48 8b 05 f8 d9 10 01 be fd 00 00 00 48 8b 40 30 e9 6a 8a bb 00 89 fe 48 c7 c7 10 59 68 a4 e8 fc 5c 06 00 <0f> 0b c3 66 0f 1f 84 00 00 00 00 00 0f 1f 44 00 00 53 48 83 ec 18
[    4.472727] RSP: 0018:ffff94cd014c3ed0 EFLAGS: 00010082
[    4.473928] RAX: 0000000000000000 RBX: ffff94bf067d4680 RCX: ffffffffa485afc8
[    4.475561] RDX: 0000000000000001 RSI: 0000000000000086 RDI: 0000000000000046
[    4.477171] RBP: 0000000000000000 R08: 0000000000000366 R09: 000000000000000f
[    4.478766] R10: 0000000000000004 R11: ffffffffa50bd64d R12: ffffa7c94625bd68
[    4.480376] R13: ffff94cd014dd640 R14: ffffffffa374b1b0 R15: ffff94cd014dd778
[    4.481954] FS:  00007efeedfc8740(0000) GS:ffff94cd014c0000(0000) knlGS:0000000000000000
[    4.483910] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    4.485244] CR2: 00007f9c43f72388 CR3: 000000021860a000 CR4: 00000000003406e0
[    4.486848] Call Trace:
[    4.487546]  <IRQ>
[    4.488114]  update_process_times+0x40/0x50
[    4.489125]  tick_sched_handle+0x21/0x70
[    4.490079]  ? tick_sched_do_timer+0x55/0x80
[    4.491070]  tick_sched_timer+0x37/0x70
[    4.491982]  __hrtimer_run_queues+0x108/0x290
[    4.493076]  hrtimer_interrupt+0xe5/0x240
[    4.494042]  smp_apic_timer_interrupt+0x6a/0x130
[    4.495104]  apic_timer_interrupt+0xf/0x20
[    4.496059]  </IRQ>
[    4.496663]  ? panic+0x249/0x292
[    4.497466]  ? do_exit+0x857/0xc30
[    4.498365]  ? do_group_exit+0x3a/0xa0
[    4.499280]  ? __x64_sys_exit_group+0x14/0x20
[    4.500337]  ? do_syscall_64+0x5f/0x240
[    4.501254]  ? entry_SYSCALL_64_after_hwframe+0x5c/0xc1
[    4.502487] ---[ end trace 90c772a274fd47de ]---
```

## 为什么 Broadcast 可以让所有的 console 都可以接受到消息
```txt
dracut -f --add-drivers sha3_generic /boot/vmlinuz-martins3-4.19.x86_64
Failed to install module sha3_generic

Broadcast message from systemd-journald@localhost.localdomain (Fri 2025-09-05 03:28:53 EDT):

dracut[5080]: Failed to install module sha3_generic


Message from syslogd@localhost at Sep  5 03:28:53 ...
 dracut:Failed to install module sha3_generic
```

## nvim 为什么必须借助 tmux 才可以拷贝，

ssh 过去， 打开 nvim 就无法拷贝了?

对于 zellij 没有什么配置，发现默认也是不能拷贝的

## 用户态的数量是如何确定的

例如 13900k 上可以看到 4 个 user
```txt
🧀  uptime
 15:22:19 up 1 day, 18:25,  4 users,  load average: 0.39, 0.19, 0.24
```

但是 2403-nix 中，无论 ssh 进去多少次，都是一样的:
```txt
 15:18:07 up  1:10,  0 user,  load average: 0.21, 0.14, 0.04
```

## fedora 启动有这个，这是做什么的
Finished systemd-vconsole-setup.service - Virtual Console Setup.

## 逆天了，现在的 terminal 也实在是太强了一点
https://github.com/mmulet/term.everything

## windows 中的这个东西
https://cmder.app/

在例如这个例子，也就是 windows 也有 console 的概念:
https://github.com/gammasoft71/Examples_Win32/blob/master/Win32.System/Console/ConsoleColor/ConsoleColor.cpp

## bash 的启动过程
https://blog.flowblok.id.au/2013-02/shell-startup-scripts.html

## 搜索一下 windows terminal ，readme 中也有 pty 之类的东西

这是 windows 为了兼容，最近引入的东西吗?

## 哦，原来还可以有这里逆天操作
```txt
write -h

Usage:
 write [options] <user> [<ttyname>]

Send a message to another user.

Options:
 -h, --help     display this help
 -V, --version  display version

For more details see write(1).
```

## 完全不能理解 wsl 是通过什么方法登录的
```txt
systemd─┬─NetworkManager───3*[{NetworkManager}]
        ├─2*[agetty]
        ├─dbus-broker-lau───dbus-broker
        ├─init-systemd(Fe─┬─SessionLeader───Relay(233)───bash───bash───sudo───sudo───yum───{yum}
        │                 ├─SessionLeader───Relay(340)───bash───pstree
        │                 ├─init───{init}
        │                 ├─login───bash
        │                 └─{init-systemd(Fe}
        ├─2*[systemd───(sd-pam)]
        ├─systemd-homed
        ├─systemd-hostnam
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-oomd
        ├─systemd-resolve
        ├─systemd-udevd
        └─systemd-userdbd───3*[systemd-userwor]
```
Relay 是什么东西?

https://wsl.dev/technical-documentation/relay/

https://github.com/microsoft/WSL

## windows 下的 serial 驱动
https://github.com/microsoft/Windows-driver-samples/tree/main/serial

https://github.com/raphamorim/rio

## 想不到还有这种操作

https://github.com/axboe/liburing/issues/1185
  - 记录在 code/src/c/iouring/multishot-read.c

## login shell 和非 login shell 是什么区别?
https://unix.stackexchange.com/questions/38175/difference-between-login-shell-and-non-login-shell


## 还有这种操作
 lsof /dev/pts/*

 https://stackoverflow.com/questions/27021641/how-to-fix-request-failed-on-channel-0

> There is a limit of 256 pseudo terminals on a system. Maybe you have an application that is leaking pseudo terminals. Use
> lsof /dev/pts/*
> to see what processes have open pseudo-terminals

可以展示谁在使用 tty

## android 中的 termux 也是有趣的东西

## man last(1)

last reboot 是一个好命令，但是为什么 man last(1) 的时候发现，这个东西也是和 tty 有关的

## 如果内核触发日志，基本上没一个 tmux 的 windows 都可以收到这个消息
```txt
Message from syslogd@localhost at Nov  3 01:23:47 ...
 kernel:Disabling IRQ #16
```
## 为什么在 systemd 中，需要将日志设置为这个东西

```txt
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
```

想不到标准输出都替换为了这个:
```txt
root@localhost:/proc/7416/fd# ls -la
total 0
dr-x------ 2 root root  3 Nov  6 14:22 .
dr-xr-xr-x 9 root root  0 Nov  6 14:20 ..
lr-x------ 1 root root 64 Nov  6 14:22 0 -> /dev/null
lrwx------ 1 root root 64 Nov  6 14:22 1 -> 'socket:[38538]'
lrwx------ 1 root root 64 Nov  6 14:22 2 -> 'socket:[38538]'
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

- stdout：如果连接到终端（TTY），默认是 行缓冲（line-buffered）。
- 但在 systemd service 中，stdout/stderr 被重定向到 /dev/null 或 journal，不再是终端。
- 此时，stdout 变为 全缓冲（fully buffered），默认缓冲区大小通常是 4KB 或 8KB。
- stderr 默认是 无缓冲 或 行缓冲（取决于实现），但在某些环境下也可能被缓冲。


## sshd 冒号后面的东西是什么?
```txt
4 S root        2169       1  0  80   0 -  3868 -      Nov05 ?        00:00:00 sshd: /usr/sbin/sshd -D [listener] 0 of 10-100 startups
4 S root       17627    2169  0  80   0 -  4492 -      15:13 ?        00:00:00 sshd: martins3 [priv]
5 S martins3   17639   17627 42  80   0 -  5572 -      15:13 ?        00:01:11 sshd: martins3@notty
4 S root       17711    2169  0  80   0 -  4491 -      15:15 ?        00:00:00 sshd: martins3 [priv]
5 S martins3   17723   17711  0  80   0 -  4560 -      15:15 ?        00:00:00 sshd: martins3@pts/0
```

## tail -f qemu 的 serial 日志，可以实现清屏

## 观察到了一个现象

一个 process ，fork 出来一个 child ，
child 不断的 printf，process 如果直接关闭，
也不去 wait child ，那么到时候，terminal 中的现象
就是:

也可以输入命令，但是屏幕会不断的跳输出出来。

## stdbuf 和 timeout 命令

man stdbuf 看看

timeout

这么想，想要在命令行中监控各种 process 是有一个完成的体系的

## pstree 展示命令，如果屏幕受限，那么会自动的压缩为 +

```txt
        ├─tmux: server─┬─zsh───nvim───nvim─┬─efm-langserver───10*[{efm-langserve+
        │              │                   ├─node───10*[{node}]
        │              │                   ├─zsh───pstree
        │              │                   └─4*[{nvim}]
        │              ├─zsh───nvim───nvim─┬─.ccls-wrapped───37*[{.ccls-wrapped}+
        │              │                   ├─efm-langserver───10*[{efm-langserve+
        │              │                   ├─node───10*[{node}]
        │              │                   ├─zsh
        │              │                   └─4*[{nvim}]
        │              ├─zsh───docker───20*[{docker}]
        │              ├─zsh───bash───nvim───nvim─┬─efm-langserver───4*[{efm-lan+
        │              │                          ├─node───10*[{node}]
        │              │                          └─4*[{nvim}]
        │              ├─zsh───nvim───nvim─┬─.ccls-wrapped───37*[{.ccls-wrapped}+
        │              │                   ├─zsh───node─┬─node───10*[{node}]
        │              │                   │            └─10*[{node}]
        │              │                   └─4*[{nvim}]
        │              ├─zsh───.tig-wrapped
        │              ├─zsh───nvim───nvim─┬─.ccls-wrapped───37*[{.ccls-wrapped}+
        │              │                   └─4*[{nvim}]
        │              └─3*[zsh]
        ├─udisksd───6*[{udisksd}]
        └─wpa_supplicant
```

## 有趣，似乎写一个 termianl multiplexer 也不难
https://github.com/alejandroqh/term39

也就 1 万行而已

## 思考一下在 bmc 界面中，使用 aspeed 网卡，然后使用 vim 的结果是什么

## vn/docs/linux/tlpi/6/tlpi-chapter-64.md

这里的

## 原来就是这一族函数
https://linux.die.net/man/3/cfmakeraw

## 在 asahi linux 中启动虚拟机，虚拟机发最后有这个日志

```txt
[   14.912919] fbcon: Taking over console
[   14.913646] Console: switching to colour frame buffer device 160x50

Fedora Linux 42 (Server Edition)
Kernel 6.17.9-200.fc42.aarch64 on aarch64 (ttyAMA0)

Web console: https://localhost:9090/

localhost login:
```

## pty 太神奇了

来连接 pty 的方法:
```txt
# 这时候你立刻在另一个终端执行：
screen /dev/pts/12 115200
# 或者
minicom -D /dev/pts/12
```

## 为什么 /dev/ 下有那么多 /dev/ttyS*
而且很多都是不可以 io 的

## 在 arm 中，不去配置 console=ttyAMA0 也是可以的
```txt
🧀  cat /proc/cmdline
BOOT_IMAGE=(hd0,gpt2)/vmlinuz-6.17.9-200.fc42.aarch64 root=/dev/mapper/fedora-root ro rd.lvm.lv=fedora/root nokaslr console=ttyS0,9600 earlyprink=serial mitigations=off selinux=0 audit=0
```
console=ttyAMA0 似乎就像是不用配置一样，注意，这里不是说
console=ttyS0,9600 等价于 ttyAMA0 的


## 哦，原来 asciiquarium 这么复杂

```txt
├─ptyxis─┬─ptyxis-agent─┬─zsh───.asciiquarium-w
│        │              ├─{dconf worker}
│        │              ├─{gdbus}
│        │              ├─{gmain}
│        │              └─{pool-spawner}
│        ├─{[pango] fontcon}
│        ├─{dconf worker}
│        ├─{gdbus}
│        ├─{gmain}
│        ├─{pool-spawner}
│        ├─2*[{ptyxis:disk$0}]
│        ├─{ptyxis:disk$1}
│        └─{ptyxis:disk$2}
```

而且在 asahi linux 上，这个会消耗 18% 的 CPU


## termianl graphical
https://hpjansson.org/chafa/
https://news.ycombinator.com/item?id=46278208

## qemu 有这个参数，这是做什么的?
或者说，这个是如何实现的
-daemonize

## 好
https://github.com/rockorager/prise

## 可以对比一下

这些实验都是可以放到 mac 上来对比一下的

## mdev 的 mtty

属于是炫技了

## 为什么 firecracker 的 ttyS0 去支持 serial 而不是 virtconsole

## 忽然意识到，现在 13900k 是可以同时接入两个 vga 线的

而且似乎两个屏幕都是可以同时显示的，似乎可以是一个屏幕显示 ui ，一个屏幕就是 tty0

## consolt=tty0 的意义到底是什么

如果没有，其实最后屏幕上也是会有让人登录的入口的，
但是只是从 grub 开始后，没有 dmesg 的日志了

## 看看这个东西
https://blogsystem5.substack.com/p/ssh-agent-switcher-release?utm_source=substack&publication_id=2042083&post_id=182615455&utm_medium=email&utm_content=share&utm_campaign=email-share&triggerShare=true&isFreemail=true&r=3ot3d&triedRedirect=true

## Terminus 10x18

docs/kernel/release/6.19.md 中提到的:
The "Terminus 10x18" console font, meant to improve readability on mid-resolution (1440x900) laptop screens, has been added.

这是非常奇怪的，相当于内核中有字体的支持了

可以看看这里的:
lib/fonts/

## 有趣的思维游戏

tmux 中一个窗口打开，然后 ssh 自己，然后继续tmux attach ，会发生什么事情?

## VIRTIO_CONSOLE
https://projectacrn.github.io/latest/developer-guides/hld/virtio-console.html

不是，那么这个和 hvc0 的关系是什么?

? 可以让 qemu 的启动使用 virtio-console 吗?

需要到时候 virtio-console builtin 吗?

## 有这个问题吗

ttyS0 ttyS1 多个，最后那个是 active 的
同类的只有可以吗?

我记得 redhat 有一个 KB 来描述这个问题来着?

## 忽然想到，neovim 中又打开 terminal 的实现原理是什么

## systemd 提供了一个 loginctl

## arm 下我们打开了这两个 config
```txt
# TODO qemu 默认使用的这个设备，x86_64 的那个有什么区别?
CONFIG_SERIAL_AMBA_PL011=y
CONFIG_SERIAL_AMBA_PL011_CONSOLE=y
```

## 经典痛苦面具的地方
https://github.com/arighi/virtme-ng/tree/main/virtme_ng_init

## 再问一次，w 是如何实现的
🤒  w
 16:53:27 up 19 days,  1:09,  4 users,  load average: 0.13, 0.75, 1.85
USER     TTY        LOGIN@   IDLE   JCPU   PCPU WHAT
martins3           15:46    6days  0.00s  0.03s sshd-session: martins3 [priv]
martins3 tty1      02Feb26  8days  0.37s  0.37s -zsh
martins3           22Jan26  6days  0.00s  1.61s /usr/lib/systemd/systemd --user

## zellij 居然还支持 web 模式啊
https://zellij.dev/tutorials/web-client/


## 为什么内部的 ssh 有时候需要添加上这个

TERM=xterm-256color  ssh

似乎是我的 tmux 的原因?

## 这个实在是太酷炫了
https://github.com/orhun/ratty/releases/tag/v0.2.0

## 当使用 fedora 的图形界面的时候，进行 logout 之后，原来这个时候，

所有的图形程序都会被 kill 掉的，但是 tmux 和 qemu 之类的东西却不会被 kill 掉，这个时候，重新登录之后，tmux 和 qemu 之类的东西继续在

所以，这么想，logout 之后，相当于当前的 session 中的程序就是那些图形程序?

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
