# vt

## 看看
https://unix.stackexchange.com/questions/32884/which-virtual-terminal-is-a-given-x-process-running-on
https://stackoverflow.com/questions/3034567/how-do-i-find-the-current-virtual-terminal
https://askubuntu.com/questions/33078/what-is-a-virtual-terminal-for

## vt
没有图形界面，默认进入的就是 vt

## bmc 的时候也是如此的

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

```txt
[  248.671749] sysrq: HELP : loglevel(0-9) reboot(b) crash(c) terminate-all-tasks(e) memory-full-oom-kill(f) kill-all
-tasks(i) thaw-filesystems(j) sak(k) show-backtrace-all-active-cpus(l) show-memory-usage(m) nice-all-RT-tasks(n) powe
roff(o) show-registers(p) show-all-timers(q) unraw(r) sync(s) show-task-states(t) unmount(u) force-fb(V) show-blocked
-tasks(w) dump-ftrace-buffer(z)
[  252.998756] sysrq: Show Regs
[  253.002434] CPU: 10 PID: 0 Comm: swapper/10 Kdump: loaded Tainted: G           OE     martins3-4.19.x86
_64 #1
[  253.014003] Hardware name: Suma R6240H0/62DB32, BIOS CXYH051029 09/06/2023
[  253.021573] RIP: 0010:native_safe_halt+0xe/0x10
[  253.026796] Code: eb bd 90 90 90 90 90 90 90 90 90 90 e9 07 00 00 00 0f 00 2d f6 0c 53 00 f4 c3 66 90 e9 07 00 00
00 0f 00 2d e6 0c 53 00 fb f4 <c3> 90 0f 1f 44 00 00 41 55 41 54 55 53 e8 b0 6f 85 ff 65 8b 2d c9
[  253.046926] RSP: 0018:ffffb2dd0025fea8 EFLAGS: 00000246 ORIG_RAX: ffffffffffffffde
[  253.055184] RAX: ffffffffa90d7160 RBX: 000000000000000a RCX: 7fffffc5f636d1aa
[  253.063021] RDX: 7fffffffffffffff RSI: 000000000000000a RDI: ffff956cff8a3d40
[  253.070862] RBP: 000000000000000a R08: ffff956cff89dc80 R09: 0000000000000000
[  253.078705] R10: 0000000000000000 R11: 0000000000000000 R12: 0000000000000000
[  253.086549] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[  253.094393] FS:  00007f6d9fb79740(0000) GS:ffff956cff880000(0000) knlGS:0000000000000000
[  253.103199] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  253.109672] CR2: 00007f6d92e31020 CR3: 0000007830c0a000 CR4: 00000000003406e0
[  253.117542] Call Trace:
[  253.120735]  default_idle+0x1c/0x140
[  253.125063]  do_idle+0x1aa/0x250
[  253.129020]  cpu_startup_entry+0x6f/0x80
[  253.133667]  start_secondary+0x18d/0x1e0
[  253.138310]  secondary_startup_64+0xb6/0xc0
[  253.143207] CPU#10: active:     0000000000000001
[  253.148535] CPU#10:   gen-PMC0 ctrl:  0000000000530076
[  253.154384] CPU#10:   gen-PMC0 count: 0000fffb7a1b6a0d
[  253.160215] CPU#10:   gen-PMC0 left:  000000051f5c2910
[  253.166032] CPU#10:   gen-PMC1 ctrl:  0000000000000000
[  253.171839] CPU#10:   gen-PMC1 count: 0000000000000000
[  253.177627] CPU#10:   gen-PMC1 left:  0000000000000000
[  253.183399] CPU#10:   gen-PMC2 ctrl:  0000000000000000
[  253.189154] CPU#10:   gen-PMC2 count: 0000000000000000
[  253.194899] CPU#10:   gen-PMC2 left:  0000000000000000
[  253.200638] CPU#10:   gen-PMC3 ctrl:  0000000000000000
[  253.206366] CPU#10:   gen-PMC3 count: 0000000000000000
[  253.212076] CPU#10:   gen-PMC3 left:  0000000000000000
[  253.217782] CPU#10:   gen-PMC4 ctrl:  0000000000000000
[  253.223474] CPU#10:   gen-PMC4 count: 0000000000000000
[  253.229152] CPU#10:   gen-PMC4 left:  0000000000000000
[  253.234811] CPU#10:   gen-PMC5 ctrl:  0000000000000000
[  253.240451] CPU#10:   gen-PMC5 count: 0000000000000000
[  253.246077] CPU#10:   gen-PMC5 left:  0000000000000000
```

## drivers/tty/vt/keyboard.c 中

这里也会调用 show_state ，和 sysrq 一样的

## 如果使用 fn_show_ptregs 需要判断 in_hardirq() 吗?

sysrq_handle_showallcpus 中判断过，但是这里是没有的，而且奇怪的是，这样的话，岂不是
sysrq 和 vt 的功能是重叠的

```c
static void fn_show_ptregs(struct vc_data *vc)
{
	struct pt_regs *regs = get_irq_regs();

	if (regs)
		show_regs(regs);
}
```

## 测试下，n100 中不要包含 drm 驱动，会有什么影响?
我猜测 vt console 直接无法显示吧

## 原来 drivers/tty/vt/keyboard.c 中的代码一直都是可以调用的
```txt
@[
    fn_enter+5
    kbd_event+959
    input_handle_events_default+66
    input_pass_values+289
    input_event_dispose+322
    input_event+83
    hidinput_report_event+55
    hid_report_raw_event+320
    hid_input_report+251
    hid_irq_in+464
    __usb_hcd_giveback_urb+145
    usb_giveback_urb_bh+169
    process_one_work+325
    bh_worker+555
    tasklet_hi_action+19
    handle_softirqs+220
    irq_exit_rcu+161
    common_interrupt+133
    asm_common_interrupt+38
    cpuidle_enter_state+198
    cpuidle_enter+45
    do_idle+412
    cpu_startup_entry+41
    start_secondary+247
    common_startup_64+300
]: 6
```
## 似乎 Ctrl + Alt + F2 和 Ctrl + Alt + F1 都不是一个东西
功能定位有不同的

## 有趣的 manual
https://manpages.ubuntu.com/manpages/plucky/man4/vt.4freebsd.html

## 想不到 windows 也是考虑这个的
https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

## 理解这个
https://wiki.archlinux.org/title/Linux_console

## 这个 freebsd 也是有的
https://docs-archive.freebsd.org/doc/11.4-RELEASE/usr/local/share/doc/freebsd/handbook/consoles.html

## 看看，写的相当好，可以看看 vt 去掉之后的效果
https://www.reddit.com/r/linux/comments/10eccv9/config_vtn_in_2023/

## 依赖 VT 的一些 config

```txt
CONFIG_VT=y
CONFIG_CONSOLE_TRANSLATIONS=y
CONFIG_VT_CONSOLE=y
CONFIG_VT_CONSOLE_SLEEP=y
# CONFIG_VT_HW_CONSOLE_BINDING is not set

CONFIG_VGA_CONSOLE=y
CONFIG_DUMMY_CONSOLE=y
CONFIG_DUMMY_CONSOLE_COLUMNS=80
CONFIG_DUMMY_CONSOLE_ROWS=25
```
## 注意， 把 VT 去掉之后，console=ttyS0 不受影响
只有 vnc 功能收到影响

忽然意识到，vt 是非常复杂的，他是需要键盘鼠标的

## 在 vnc 中 enter 的效果
```txt
@[
    fn_enter+5
    kbd_event+959
    input_handle_events_default+66
    input_pass_values+289
    input_event_dispose+322
    input_event+83
    atkbd_receive_byte+1689
    ps2_interrupt+158
    serio_interrupt+71
    i8042_handle_data+240
    i8042_interrupt+17
    __handle_irq_event_percpu+71
    handle_irq_event+56
    handle_edge_irq+139
    __common_interrupt+59
    common_interrupt+128
    asm_common_interrupt+38
    default_idle+15
    default_idle_call+48
    do_idle+437
    cpu_startup_entry+41
    start_secondary+247
    common_startup_64+300
]: 5
```

原来如此:
```txt
@[
    fbcon_putcs+0
    con_write+32
    n_tty_write+380
    file_tty_write.isra.0+300
    redirected_tty_write+268
    vfs_write+544
    ksys_write+120
    __arm64_sys_write+36
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+64
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 1
```

```txt
🧀  cat /sys/class/vtconsole/vtcon*/name
(S) dummy device
(M) frame buffer device
```

```txt
🧀  ls /sys/class/graphics/fb*
/sys/class/graphics/fb0:
bits_per_pixel  console  dev     mode   name  power   state   subsystem  virtual_size
blank           cursor   device  modes  pan   rotate  stride  uevent

/sys/class/graphics/fbcon:
cursor_blink  power  rotate  rotate_all  subsystem  uevent
```

cat /sys/class/graphics/fb*/name
hibmcdrmfb

i915drmfb : intel 机器

```txt
🧀  ls -la /sys/class/drm/
lrwxrwxrwx    - root  6 Apr 14:46 card0 -> ../../devices/pci0000:00/0000:00:11.0/0000:03:00.0/drm/card0
lrwxrwxrwx    - root  6 Apr 14:46 card0-VGA-1 -> ../../devices/pci0000:00/0000:00:11.0/0000:03:00.0/drm/card0/card0-VGA-1
.r--r--r-- 4.1k root  6 Apr 14:46 version
```

大致的关联关系是这个样子的:
/dev/tty0 -> /sys/devices/virtual/tty/tty0 -> vtcon1 -> /sys/class/graphics/fb0 -> /sys/class/drm/card0

## VT 与 QEMU 串口前端

VT 是本机图形/键盘控制台路径：`tty0`、`vtcon`、`fbcon` 和 DRM/VNC 都属于
这一层。QEMU 的 serial layout 是另一层：`-serial chardev:X` 创建传统 UART，
guest 通常看到 `/dev/ttyS0`；`-device virtio-serial` 只创建 virtio-serial bus，
`-device virtconsole,chardev=X` 才创建 `/dev/hvc0`，而 `virtserialport` 创建的是
`/dev/virtio-ports/<name>`，不是 tty。

因此去掉 VT/DRM 会影响 VNC 和 `tty0`，但不会让 `console=ttyS0` 失效；同理，
hvc0 依赖 virtio-serial frontend，不依赖 VT。host 侧的 `stdio`、socket、PTY 是
backend，决定 QEMU 字节从哪里进出，不决定 guest 使用 ttyS0、hvc0 还是
virtio-ports。

## vt 是的确是可以控制键盘的

对，你的理解基本正确：VT 把键盘输入和屏幕输出组合成一个 TTY，所以它确实包含键盘处理逻辑。 但键盘硬件的驱动由 input
子系统负责，VT 接收的是 input 层提供的按键事件。

对应用程序来说，两种情况都表现为一个可以读写的终端：

 终端                         输入来源                    输出去向
━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━
 串口 TTY，例如 /dev/ttyS0    UART 接收的数据             UART 发送的数据
───────────────────────────  ──────────────────────────  ────────────────────
 VT，例如 /dev/tty1           键盘事件经转换得到的字符    显示后端绘制的文字

TTY 抽象并不要求输入和输出来自同一个物理设备。

VT 的键盘路径

USB 键盘 / PS/2 键盘
        ↓
usbhid / atkbd 等硬件协议驱动
        ↓
input 子系统：按键码、按下/松开事件
        ↓
drivers/tty/vt/keyboard.c
  处理键盘映射、Shift/Ctrl、组合键、VT 切换等
        ↓
当前前台 VT 的 tty_port 接收缓冲区
        ↓
TTY line discipline（通常是 N_TTY）
  行编辑、回显、Ctrl-C 信号等
        ↓
应用程序 read(/dev/ttyN)

具体代码中：

• kbd_handler (drivers/tty/vt/keyboard.c:1690) 是一个 input_handler，通过 input 框架连接键盘并接收事件。
• kbd_keycode() (drivers/tty/vt/keyboard.c:1429) 选择 vc_cons[fg_console]，处理当前前台 VT 的按键。
• put_queue() (drivers/tty/vt/keyboard.c:326) 通过 tty_insert_flip_char() 和 tty_flip_buffer_push() 把转换后的字节
  送进 TTY。

例如，在通常的文本模式下，键盘按下 A 键，VT 根据 Shift、Caps Lock 和键盘映射决定输入 a 还是 A。若开启规范模式，之
后由 N_TTY 缓存这一行，直到回车才交给应用读取。

VT 的显示路径

应用程序 write(/dev/ttyN)
        ↓
TTY 层
        ↓
vt.c：con_write() → do_con_write()
  解析字符和终端转义序列，更新光标、颜色、屏幕内容
        ↓
vc->vc_sw：struct consw 显示后端接口
        ↓
fbcon / vgacon 等
        ↓
屏幕

写操作注册在 con_ops (drivers/tty/vt/vt.c:3912)。所以你说的 keyboard + framebuffer 是典型组合，不过显示后端也可以
是 VGA 文本控制台等，并不限定为 framebuffer。

键盘和输出也不是直接相连的：通常是键盘输入进入 N_TTY 后，由 N_TTY 的回显机制再走 VT 输出路径，把字符显示出来。

多个 VT 各自保存终端状态，共享键盘和显示设备；切换 VT 时，键盘输入随前台 VT 切换，屏幕展示也随之切换。没有连接实体
键盘时，VT 仍然可以存在并显示输出。


## 为什么 vt 存在 tty0 这种这种特殊的 tty ，但是 ttyS0 不存在

因为多个 VT 是在共享一套本地显示和键盘，需要“切换当前使用哪一个”；多个串口则是独立的通信端口，没有统一的“当前串口”。 tty0 就是把 VT 的
这个“当前”状态暴露给程序。

在典型的本地控制台上：

tty1 ─┐
tty2 ─┼─ 前台选择 ─ 显示器、键盘
tty3 ─┘
          ↑
     tty0 打开时选中它

你按 Alt+F2，是把前台切到 tty2。其他 VT 的程序仍然能运行，只是没有占据当前显示和键盘输入。

而串口是：

ttyS0   ─ 串口 A ─ 外部设备 A
ttyS1   ─ 串口 B ─ 外部设备 B
ttyUSB0 ─ USB 串口 ─ 外部设备 C

### dev/tty 也是一个特殊文件了

ssh 到机器上，然后:

```txt
# cd /dev
# for i in tty*; do echo $i > $i ; done
tty
```



## chvt
https://github.com/legionus/kbd/blob/master/src/chvt.c

不用键盘的按，就来切换

## 这个目录中内容看看 drivers/video/console/
drivers/video/console/vgacon.c

是一定会走到这个地方吗?


## ctrl-alt-del

内核路径是：

EV_KEY(KEY_LEFTCTRL)
EV_KEY(KEY_LEFTALT)
EV_KEY(KEY_DELETE)
        |
        v
drivers/tty/vt/keyboard.c:kbd_event()
        |
        v
kbd_keycode() 根据 modifier 查 VT keymap
        |
        v
Ctrl+Alt+Delete -> K_BOOT
        |
        v
fn_boot_it()
        |
        v
ctrl_alt_del()

默认 keymap 明确把 Delete 键映射为 Boot：

drivers/tty/vt/defkeymap.map:246

control alt keycode 111 = Boot

对应的处理函数在：

drivers/tty/vt/keyboard.c:614

static void fn_boot_it(struct vc_data *vc)
{
    ctrl_alt_del();
}

ctrl_alt_del() 有两条分支：

kernel/reboot.c:828

if (C_A_D)
    schedule_work(&cad_work);
else
    kill_cad_pid(SIGINT, 1);

• /proc/sys/kernel/ctrl-alt-del = 1：排队执行 kernel_restart(NULL)，也就是内核直接重启。
• /proc/sys/kernel/ctrl-alt-del = 0：向 /proc/sys/kernel/cad_pid 指定的进程发送 SIGINT。默认 cad_pid 在启动时设置为 PID 1：init/
  main.c:1639。

我在 sysrq-lab-20260925 中做了两次测试：

1. ctrl-alt-del=1：通过 guest 内的 uinput 创建虚拟键盘，发送真实的 Ctrl、Alt、Delete 事件。QEMU PID 没变，但 guest boot ID 从 630c...
   变成 d235...，uptime 重新从约 766 秒开始，确认发生了 guest 重启。

2. ctrl-alt-del=0：把 cad_pid 临时指向测试进程，发送同样的事件，测试进程记录到：

pid=591 signal=2

说明第二条 SIGINT 路径也生效了。最后已恢复：

kernel.ctrl-alt-del = 1
kernel.cad_pid = 1

有一个调试细节：这个 VM 使用 -display none，所以 QMP 的 send-key 虽然返回成功，但不会产生 guest 输入事件；evtest 看不到任何键。实际测
试必须使用 guest 内的 uinput，或者启动带真实显示/输入路由的 QEMU。当前内核源码和 VM 配置都没有留下修改，工作树也是干净的。

## 再次确认下，vt 就是在 内核实现这些工作

为什么这个东西是
https://web.git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=50145474f6ef4a9c19205b173da6264a644c7489

## 两行经常遇到的日志

```txt
[   11.528480] fbcon: Taking over console
[   11.537260] Console: switching to colour frame buffer device 480x135
```


两条日志都属于 fbcon（framebuffer 控制台）接管 VT 控制台 的过程，一条是“准备接管”的提示，一条是“实际切换完成”的提示。

1. fbcon: Taking over console

- 源码：drivers/video/fbdev/core/fbcon.c:3340，位于 fbcon_output_notifier() 函数中。

```c
  static int fbcon_output_notifier(struct notifier_block *nb,
               unsigned long action, void *data)
  {
     WARN_CONSOLE_UNLOCKED();

     pr_info("fbcon: Taking over console\n");

     dummycon_unregister_output_notifier(&fbcon_output_nb);
     schedule_work(&fbcon_deferred_takeover_work);   // 真正接管被延迟到工作队列里执行
     return NOTIFY_OK;
  }
```

- 含义：这条只在启用了 CONFIG_FRAMEBUFFER_CONSOLE_DEFERRED_TAKEOVER（延迟接管）时出现。系统启动早期，控制台由 dummy_con（dummy console，一个空壳，不输
  出任何东西）占着。fbcon 一开始不接管，只是注册一个 output notifier 监听“第一次有人往控制台写输出”的事件。一旦发生第一次输出，就打印这行，注销
  dummycon 的通知器，并调度工作队列 fbcon_deferred_takeover_work，稍后真正去绑定 fbcon。

2. Console: switching to colour frame buffer device 480x135

- 源码：drivers/tty/vt/vt.c:4066，位于 do_bind_con_driver() 函数中。

```c
     pr_info("Console: switching ");
     if (!deflt)
        pr_cont("consoles %d-%d ", first + 1, last + 1);
     if (j >= 0) {
        struct vc_data *vc = vc_cons[j].d;

        pr_cont("to %s %s %dx%d\n",
           vc->vc_can_do_color ? "colour" : "mono",
           desc, vc->vc_cols, vc->vc_rows);
        ...
```

- 含义：VT 子系统把虚拟控制台从旧的 console driver 切换（bind）到 fbcon。其中：
    - desc（即 "frame buffer device"）来自 fbcon_startup() 里的 display_desc，见 fbcon.c:1016：
      ```c
        static const char *fbcon_startup(void)
        {
            static const char display_desc[] = "frame buffer device";
            ...
      ```
    - colour 表示 vc->vc_can_do_color 为真（该 fb 支持彩色）。
    - 480x135 是文本模式的列数 × 行数（vc_cols × vc_rows），不是像素。它由分辨率除以字体大小算出——以常见的 8×16 字体为例，480×8=3840、135×16=2160，正好
      对应 3840×2160（4K）的屏幕。

调用链

```
  fbcon_output_notifier()                     fbcon.c:3340  ← 打印 "Taking over console"（延迟接管路径）
    └─ schedule_work(fbcon_deferred_takeover_work)
         └─ fbcon_register_existing_fbs()      fbcon.c:3311
              └─ do_fb_registered() → do_fbcon_takeover()  fbcon.c:606
                   └─ do_take_over_console(&fb_con, ...)   vt.c:4581
                        ├─ do_register_con_driver()
                        └─ do_bind_con_driver()            vt.c:3989
                             ├─ csw->con_startup()  (= fbcon_startup, 返回 "frame buffer device")
                             └─ pr_info("Console: switching to colour frame buffer device 480x135\n")
```

简单说：第一条说明 fbcon 收到了第一次控制台输出、开始（延迟）接管；第二条说明 fbcon 已经真正绑定到 VT，并且报告了它把屏幕初始化为 480 列 × 135 行的彩色
文本控制台。 两者之间隔着一个调度到工作队列的异步步骤，所以时间戳上会略有先后。

## TODO
https://news.ycombinator.com/item?id=41483789

其中分析到了 tty 相关的东西:

## 有趣的东西，又是 framebuffer ，又是 tty 的
https://news.ycombinator.com/item?id=43931845


## vt100
首先，我们发现 pstree -h 提到了 vt100

pstree --help
```txt
  -G, --vt100         use VT100 line drawing characters
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

**是的，VT100 最初是一台真实的硬件终端。今天仍有实物保存，但日常软件中出现的“VT100”，更多是在说它留下来的兼容规则。**它由 DEC 在 1978
年推出，外形是一个厚重的 CRT 屏幕配一把键盘，计算机历史博物馆有实物照片 (https://www.computerhistory.org/revolution/input-output/14/349/1697)。

理解它，要先看当时人与计算机如何交互。一台主机运行操作系统和应用，用户通过接在主机上的终端操作它：

用户 ⇄ VT100 的键盘和屏幕 ⇄ 串口 ⇄ 主机上的 shell、程序

你在 VT100 上敲下命令，终端把按键传给主机；主机执行命令，再把结果传回来，由终端显示。它自身也有处理器，主要负责通信、键盘和屏幕控制。这个使用体验和今天
通过 SSH 操作另一台机器很接近。

这里有个关键问题：**主机传回来的内容，既包含文字，也包含“怎么显示”的指令。**例如显示
hello、移动光标、清屏、让文字加下划线、切换到画线字符集。这些指令以特殊的字节序列发送，也就是前面提到的“转义序列”。VT100 用户手册
(https://vt100.net/docs/vt100-ug/chapter3.html)

后来，个人电脑能够用一个软件窗口承担原来那台硬件终端的工作。软件接收同样的字符和控制指令，在窗口里画出结果，所以称为 terminal
emulator，终端模拟器。例如 xterm 就明确模拟 DEC 的 VT 系列终端，并支持后续型号的功能。xterm 官方说明 (https://invisible-island.net/xterm/)

因此，“终端”和“shell”也各有职责：终端负责接收按键、显示内容；bash、zsh 这样的 shell 负责理解命令、启动程序。以前它们分处终端硬件和主机，今天可以一起运
行在你的电脑里。

回到 pstree -G：它选择的是按 VT100 的画线规则输出数据。原来的硬件终端可以解释这些数据，支持该规则的软件终端也可以。一个硬件产品的名称，就这样延续成了软
件兼容性的名称。

### VT100 某种意义上是 ANSI 协议的一种
一个现代终端，例如 kitty，大致可能支持：

```txt
ECMA-48         很多
VT100           基本全部常用部分
VT220           很多
DEC private     大量常用模式
xterm extension 大量
kitty extension 自己额外一套
```

## vt 由于在内核中实现，导致 console font 都需要实现在内核中

这里的 console font 指 Linux 本地文字控制台的字体，比如按 Ctrl+Alt+F3 后看到的登录界面，以及屏幕上的内核启动日志。

关键在于：内核不仅管理终端的输入输出，也能直接在屏幕上绘制文字。

可以分两种情况理解：

 场景                                谁把字符画到屏幕上      谁负责字体
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━
 GNOME Terminal、Kitty 等图形终端    用户态终端程序          用户态程序和字体库
──────────────────────────────────  ──────────────────────  ────────────────────
 Linux 文字控制台，如 /dev/tty1      内核的控制台显示驱动    内核

对于图形终端，shell 通过 PTY 传输字符，终端程序收到字符后绘制。内核的 PTY 层处理字节流，不需要知道“A”长什么样。

对于 Linux 文字控制台，典型的显示路径是：

程序输出字符
    ↓
内核 VT 层：处理字符、光标、换行、终端控制序列
    ↓
fbcon：查询字体位图，把字符转换成像素
    ↓
framebuffer → 屏幕

因此，fbcon 要显示一个 A，必须有描述它形状的字模。你当前源码里的 drivers/video/fbdev/core/bitblit.c:129 就从 vc_font.data 取出字符
位图，再调用显示驱动绘制。内核自带字库，也让文字输出可以在用户态尚未启动、不可用或系统发生 panic 时使用。

“Terminus 10x18”就是新增了一套字模：每个字符占宽 10、高 18 像素的固定格子。 它是位图字体；
源码中的 字体描述 (lib/fonts/font_ter10x18.c:5133) 明确记录了这些尺寸。

以 1440×900 的显示模式计算：

 字体尺寸    大约能显示的列数 × 行数
━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━
 8×16                       180 × 56
──────────  ─────────────────────────
 10×18                      144 × 50
──────────  ─────────────────────────
 16×32                       90 × 28

这次改动的动机是：8×16 在相关笔记本屏幕上显得太小，16×32 又占空间太多；Terminus 10×18
提供一个更容易辨认、同时保留足够行数的选择。补丁还指出，已有的另一套 10×18 字体可读性较差，所以改善也来自字形设计。原始补丁说明
(https://lkml.rescloud.iu.edu/2511.2/01022.html)

所以，这项改动影响的是由内核绘制的文字界面；你在 Kitty 等图形终端里选择的字体，仍由那个终端程序负责。

可以看看内核代码 /lib/fonts/
## 基本流程

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

## 显然，vt 就是需要支持

### 到达 vt 阶段
对，Linux 的 VT（例如 /dev/tty1）确实由内核解析 ANSI/VT 风格的转义序列，清屏、移动光标、设置文字颜色等都在内核中
实现；支持的是一组终端控制序列，并非所有终端扩展。

当前内核树中的核心代码在 drivers/tty/vt/vt.c，写入后的主要路径是：

用户程序向 /dev/tty1 写入字节
  → con_write()
  → do_con_write()
  → do_con_trol()：解析转义序列的状态机
  → 对应操作：清屏、移动光标、修改属性等
  → 显示后端更新屏幕

以 ESC [ 2 J（通常写作 \x1b[2J）为例：

1. do_con_trol() (drivers/tty/vt/vt.c:2689) 配合 handle_esc() 识别 ESC [，收集参数 2。
2. 遇到 J，调用 csi_J() (drivers/tty/vt/vt.c:1535)。
3. csi_J() 将屏幕字符缓冲区填成擦除字符，再通过显示后端更新可见内容。

几个典型操作：

 序列          效果                              实现
━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━
 ESC [ 2 J     清除整个可见屏幕，光标位置不变    csi_J()
────────────  ────────────────────────────────  ───────────
 ESC [ H       光标移到左上角                    gotoxay()
────────────  ────────────────────────────────  ───────────
 ESC [ K       清除光标到行末                    csi_K()
────────────  ────────────────────────────────  ───────────
 ESC [ 31 m    设置红色前景                      csi_m()

所以“清屏并回到左上角”通常是连续发送 \x1b[2J\x1b[H。

这里要区分 VT 和一般的 TTY：桌面终端中的 /dev/pts/N 也经过内核 TTY 层，
但这些转义序列通常由用户态终端模拟器解析；
Linux VT 的解析器则就在上述内核代码里。

### 到达 vt drm 驱动

做了 4 组实验，已恢复到桌面 tty2。本机控制台 framebuffer 是 i915drmfb，3840×2160、32 bpp。

 实验                                       实测结果
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 后台 tty8，发送 ESC[2J                     到 csi_J()，未进入绘制路径
─────────────────────────────────────────  ────────────────────────────
 图形模式 tty2，发送 ESC[2J                 到 csi_J()，未进入绘制路径
─────────────────────────────────────────  ────────────────────────────
 切到前台文本 tty8，发送 ESC[2J             完整进入 fbcon → i915
─────────────────────────────────────────  ────────────────────────────
 前台 tty8，发送 ESC[10X，擦除 10 个字符    进入矩形填充路径

清屏实际分成同步绘制和异步刷新两段。 以下由多个探针及源码拼接，编译器尾调用会让单份 backtrace 缺少部分层级：

写入 ESC[2J]
  → tty_write / n_tty_write
  → con_write → do_con_write
  → do_con_trol → csi_J
  → do_update_region
  → fbcon_putcs
  → bit_putcs
  → intel_fbdev_defio_imageblit       ← 已进入 i915
      ├─ cfb_imageblit               ← CPU 向 framebuffer 写像素
      └─ drm_fb_helper_damage_area   ← 记录更新区域，排队 work

这里清屏是把空格字形重新画满屏幕。本次控制台有 135 行，fbcon_putcs、intel_fbdev_defio_imageblit 和 cfb_imageblit 各命中 135 次。

接下来在 kworker/10:2 中抓到：

drm_fb_helper_damage_work
  → drm_fb_helper_fb_dirty
  → intelfb_dirty
  → intel_user_framebuffer_dirty
  → __intel_frontbuffer_flush

所以只按写入进程的 PID 过滤会漏掉后半段。我用同一个 drm_fb_helper 对象关联了异步调用。这里的像素绘制由 CPU 完成，显示硬件再扫描 framebuffer 输出
画面。

另一个有意思的区别：擦除字符 ESC[10X 才走矩形填充：

do_con_trol → csi_ECMA / csi_X
  → fbcon_clear → bit_clear
  → intel_fbdev_defio_fillrect
  → cfb_fillrect

```txt
SYNC comm=vt-erase-chars kprobe:i915:intel_fbdev_defio_fillrect

        intel_fbdev_defio_fillrect+5
        bit_clear+110
        __fbcon_clear+568
        csi_ECMA.constprop.0+818
        do_con_write+913
        con_write+22
        process_output_block+149
        n_tty_write+431
        iterate_tty_write+290
        file_tty_write.isra.0+141
        vfs_write+641
        ksys_write+123
        do_syscall_64+226
        entry_SYSCALL_64_after_hwframe+118
```

## vt 中如何支持颜色?

显然也是支持的:
```txt
     csi_m() (drivers/tty/vt/vt.c:1750) 对 30～37 的处理是：

     vc->vc_par[i] -= CSI_m_FG_COLOR_BEG;  /* 33 - 30 = 3 */
     vc->state.color = color_table[vc->vc_par[i]] |
                       (vc->state.color & 0xf0);
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
