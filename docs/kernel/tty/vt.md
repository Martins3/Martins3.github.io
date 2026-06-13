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
