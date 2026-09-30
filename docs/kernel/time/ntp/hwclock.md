# hwclock
<!-- 677541d4-a7c1-4a6c-837b-94676b615fe7 -->

## 基本介绍
hwclock 是 Linux 下操作硬件时钟（RTC，主板 CMOS 里那块由电池维持的时钟）的管理工具。
系统里有两条时间线：

- System Clock：内核维护的软件时间，开机后由 NTP/chronyd 同步
- Hardware Clock (RTC)：断电后靠主板纽扣电池继续走，开机时用来初始化 System Clock

主要功能

┌────────────────────────────────────────────┬────────────────────────────────────────────────┐
│ 命令                                       │ 作用                                           │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock -r / --show                        │ 显示 RTC 时间（默认动作）                      │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock -w / --systohc                     │ 把系统时间写入 RTC（关机前常用）               │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock -s / --hctosys                     │ 用 RTC 时间设置系统时间（开机早期常用）        │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock --set --date "2025-01-01 12:00:00" │ 直接设定 RTC                                   │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock -a / --adjust                      │ 按累计漂移量校正 RTC                           │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock --systz                            │ 只把时区/时间标度配置传给内核，不读也不写 RTC  │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock --predict                          │ 按漂移率预测未来某时刻 RTC 的值                │
├────────────────────────────────────────────┼────────────────────────────────────────────────┤
│ hwclock --param-get/--param-set            │ 读写 RTC 芯片参数（如 Backup Switchover Mode） │
└────────────────────────────────────────────┴────────────────────────────────────────────────┘

典型使用场景

1. 开机：systemd 的 systemd-hwclock.service 用 --hctosys 把 RTC 同步到系统时间。
2. 关机：--systohc 把已由 NTP 校准过的系统时间写回 RTC，避免 RTC 单独漂移。
3. RTC 走时不准：/etc/adjtime 记录漂移率和上次校准时间，--adjust 据此补偿。
4. UTC vs localtime：用 -u / -l 指定 RTC 存的是 UTC 还是本地时间（Linux 一般用 UTC，双系统 Windows 常是 localtime，这是"时
   间差 8 小时"问题的根源）。

常用例子

```bash
  hwclock -r                     # 看 RTC 现在几点
  sudo hwclock -w                # 系统时间 -> RTC
  sudo hwclock -s                # RTC -> 系统时间
  sudo hwclock --set --date "2025-06-01 10:00:00"
  sudo hwclock -a -v             # 校正漂移并打印细节
  sudo hwclock -r --verbose      # 当前 RTC 是 UTC 还是 localtime、漂移因子等
```

注意：需要 root，因为它要访问 /dev/rtc0（或指定 --rtc、老的 ISA 机器用 --directisa）。另外它和 date、timedatectl 分工不同
——date/timedatectl 管的是系统时间，写到 RTC 那一步才落到 hwclock（或 timedatectl set-local-rtc）。


## hwclock --systohc 的实现原理发

hwclock --systohc 走的是 RTC 字符设备 ioctl 写 RTC 这条路径，
不是 settimeofday()，也不是内核 NTP 的 11 分钟同步路径。

```txt
openat(AT_FDCWD, "/dev/rtc0", O_RDONLY) = 3
openat(AT_FDCWD, "/sys/class/rtc/rtc0", ...)
read(..., "rtc_cmos rtc_cmos\n", ...) = 18
ioctl(3, RTC_SET_TIME, {tm_sec=13, tm_min=35, tm_hour=3, ...}) = 0
```

内核中的调用链为:
hwclock
  -> ioctl(/dev/rtc0, RTC_SET_TIME, &tm)
    -> rtc_dev_ioctl()
      -> rtc_set_time()
        -> rtc->ops->set_time(...)
          -> cmos_set_time()
            -> mc146818_set_time()

实验测试结果:

```txt
@[
        mc146818_set_time+5
        rtc_set_time.part.0+126
        rtc_dev_ioctl+784
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

```txt
$  timedatectl status
      Local time: Fri 2026-04-03 11:39:22 CST
  Universal time: Fri 2026-04-03 03:39:22 UTC
        RTC time: Fri 2026-04-03 03:40:50
       Time zone: Asia/Shanghai (CST, +0800)
     NTP enabled: no
NTP synchronized: yes
 RTC in local TZ: no
      DST active: n/a

$ hwclock --systohc
$  timedatectl status
      Local time: Fri 2026-04-03 11:39:53 CST
  Universal time: Fri 2026-04-03 03:39:53 UTC
        RTC time: Fri 2026-04-03 03:39:53
       Time zone: Asia/Shanghai (CST, +0800)
     NTP enabled: no
NTP synchronized: yes
 RTC in local TZ: no
      DST active: n/a
```

## 为什么 vmcore 目录的时间是 hwclock 的时间

vmcore 目录名不是崩溃那个内核取的，而是捕获内核（capture/crash kernel）启动后自己算的；而捕获内核开机时没有 NTP，它的墙钟
只能从硬件时钟（RTC）初始化——等价于开机做了一次 hwclock -s（rtc_hctosys）。所以目录名/mtime 就是 RTC 的时间。

链路

1. 目录名在捕获内核的 initramfs 里生成：/usr/lib/dracut/modules.d/99kdumpbase/kdump.sh 顶部
   ```sh
     DATEDIR=$(date +%Y-%m-%d-%T)
   ```
   最终落到 /var/crash/<HOST_IP>-$DATEDIR/，vmcore 的 mtime 同样是这个内核写的。

2. 捕获内核是 kexec 起来的一个全新内核，不继承第一个内核的时间（时间不是 kexec 传递的状态）。它的时间初始化来自两处：
    - kernel/time/timekeeping.c 的 read_persistent_wall_and_boot_offset() → read_persistent_clock64()；x86 默认实现在
      arch/x86/kernel/rtc.c 的 read_persistent_clock64()，即 mach_get_cmos_time（arch/x86/kernel/x86_init.c 里
      x86_init.platform.get_wallclock），直接读 CMOS。
    - 之后 drivers/rtc/class.c 的 rtc_hctosys() 再拿 rtc0 的值 do_settimeofday64()，内核日志里就是这行：
      ```
        rtc_cmos 00:00: setting system clock to 2024-05-23 07:05:11 UTC (1716447911)
      ```
   （你库里 ~/data/dmesg-collections/df-dmesg/* 上每台机器启动都有这行。）

3. 所以在捕获内核里 date 和 hwclock -r 基本是同一个数；而崩溃前的第一个内核里 date 是 chronyd 校准过的系统时间，两者本来就
   可能不一样。


## 和 NTP 的关系


ntp 会 11 分钟的时间来刷新一下，但是坑，需要卡 0.5s 的这个时间点去
设置， 如果写早了或写晚了，就会错过这次正确同步机会。

commit c9e6189fb031 ("ntp: Make the RTC synchronization more reliable")
```c
  /* ntp.c:499 */
  #define SYNC_PERIOD_NS (11ULL * 60 * NSEC_PER_SEC)   /* 660 s，硬编码 */
```

```c
  /* ntp.c:620 注释 + 627 sync_hw_clock() */
   * If we have an externally synchronized Linux clock, then update RTC clock
   * accordingly every ~11 minutes.
```

## 有时候
n100 的启动过程中:
```txt
[    0.364718] rtc_cmos rtc_cmos: RTC can wake from S4
[    0.365569] rtc_cmos rtc_cmos: registered as rtc0
[    0.365752] rtc_cmos rtc_cmos: setting system clock to 2025-02-23T03:33:52 UTC (1740281632)
[    0.365782] rtc_cmos rtc_cmos: alarms up to one month, y3k, 114 bytes nvram
```
现在看，其实 rtc 不是 legacy 设备:

13900k : /sys/devices/platform/rtc_cmos/rtc
13900k qemu : /sys/devices/pnp0/00:06/rtc
kunpeng : /sys/devices/platform/rtc-efi.0/rtc
kunpeng qemu : /sys/devices/platform/rtc-efi.0/rtc

> Nonetheless, the real time clock’s primary importance is only during boot,
when the `xtime` variable is initialized.

原来 xtime 存放 walltime


## qemu 的参数
```sh
-rtc driftfix=slew,clock=rt
```

rtc 会穿透给 guest os 吗? 尤其是如果 linux guest os 使用了 kvm-clock .

qemu-options.hx
```txt
SRST
``-rtc [base=utc|localtime|datetime][,clock=host|rt|vm][,driftfix=none|slew]``
    Specify ``base`` as ``utc`` or ``localtime`` to let the RTC start at
    the current UTC or local time, respectively. ``localtime`` is
    required for correct date in MS-DOS or Windows. To start at a
    specific point in time, provide datetime in the format
    ``2006-06-17T16:01:21`` or ``2006-06-17``. The default base is UTC.

    By default the RTC is driven by the host system time. This allows
    using of the RTC as accurate reference clock inside the guest,
    specifically if the host time is smoothly following an accurate
    external reference clock, e.g. via NTP. If you want to isolate the
    guest time from the host, you can set ``clock`` to ``rt`` instead,
    which provides a host monotonic clock if host support it. To even
    prevent the RTC from progressing during suspension, you can set
    ``clock`` to ``vm`` (virtual clock). '\ ``clock=vm``\ ' is
    recommended especially in icount mode in order to preserve
    determinism; however, note that in icount mode the speed of the
    virtual clock is variable and can in general differ from the host
    clock.

    Enable ``driftfix`` (i386 targets only) if you experience time drift
    problems, specifically with Windows' ACPI HAL. This option will try
    to figure out how many timer interrupts were not processed by the
    Windows guest and will re-inject them.
ERST
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
