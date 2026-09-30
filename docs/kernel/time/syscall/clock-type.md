# clock 类型

man clock_gettime 后，发现系统中有更多的 timer 类型。

man clock_getres(2) 可以得到:
类型的变化:
```txt
       Sufficiently recent versions of glibc and the Linux kernel support the following clocks:

       CLOCK_REALTIME
              A  settable system-wide clock that measures real (i.e., wall-clock) time.  Setting this clock requires appropriate privileges.  This clock is affected by discontinuous jumps in the system time (e.g., if the system administrator manu‐
              ally changes the clock), and by the incremental adjustments performed by adjtime(3) and NTP.

       CLOCK_REALTIME_ALARM (since Linux 3.0; Linux-specific)
              Like CLOCK_REALTIME, but not settable.  See timer_create(2) for further details.

       CLOCK_REALTIME_COARSE (since Linux 2.6.32; Linux-specific)
              A faster but less precise version of CLOCK_REALTIME.  This clock is not settable.  Use when you need very fast, but not fine-grained timestamps.  Requires per-architecture support, and probably also architecture support for this flag
              in the vdso(7).

       CLOCK_TAI (since Linux 3.10; Linux-specific)
              A nonsettable system-wide clock derived from wall-clock time but ignoring leap seconds.  This clock does not experience discontinuities and backwards jumps caused by NTP inserting leap seconds as CLOCK_REALTIME does.

              The acronym TAI refers to International Atomic Time.

       CLOCK_MONOTONIC
              A nonsettable system-wide clock that represents monotonic time since—as described by POSIX—"some unspecified point in the past".  On Linux, that point corresponds to the number of seconds that the system has been running since it was
              booted.

              The CLOCK_MONOTONIC clock is not affected by discontinuous jumps in the system time (e.g., if the system administrator manually changes the clock), but is affected by the incremental adjustments performed by adjtime(3) and NTP.  This
              clock does not count time that the system is suspended.  All CLOCK_MONOTONIC variants guarantee that the time returned by consecutive calls will not go backwards, but successive calls may—depending on the architecture—return  identi‐
              cal (not-increased) time values.

       CLOCK_MONOTONIC_COARSE (since Linux 2.6.32; Linux-specific)
              A faster but less precise version of CLOCK_MONOTONIC.  Use when you need very fast, but not fine-grained timestamps.  Requires per-architecture support, and probably also architecture support for this flag in the vdso(7).

       CLOCK_MONOTONIC_RAW (since Linux 2.6.28; Linux-specific)
              Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments or the incremental adjustments performed by adjtime(3).  This clock does not count time that the system is suspended.

       CLOCK_BOOTTIME (since Linux 2.6.39; Linux-specific)
              A  nonsettable  system-wide  clock that is identical to CLOCK_MONOTONIC, except that it also includes any time that the system is suspended.  This allows applications to get a suspend-aware monotonic clock without having to deal with
              the complications of CLOCK_REALTIME, which may have discontinuities if the time is changed using settimeofday(2) or similar.

       CLOCK_BOOTTIME_ALARM (since Linux 3.0; Linux-specific)
              Like CLOCK_BOOTTIME.  See timer_create(2) for further details.

       CLOCK_PROCESS_CPUTIME_ID (since Linux 2.6.12)
              This is a clock that measures CPU time consumed by this process (i.e., CPU time consumed by all threads in the process).  On Linux, this clock is not settable.

       CLOCK_THREAD_CPUTIME_ID (since Linux 2.6.12)
              This is a clock that measures CPU time consumed by this thread.  On Linux, this clock is not settable.

       Linux also implements dynamic clock instances as described below.

   Dynamic clocks
       In addition to the hard-coded System-V style clock IDs described above, Linux also supports POSIX clock operations on certain character devices.  Such devices are called "dynamic" clocks, and are supported since Linux 2.6.39.

       Using the appropriate macros, open file descriptors may be converted into clock IDs and passed to clock_gettime(), clock_settime(), and clock_adjtime(2).  The following example shows how to convert a file descriptor into a dynamic clock ID.

           #define CLOCKFD 3
           #define FD_TO_CLOCKID(fd)   ((~(clockid_t) (fd) << 3) | CLOCKFD)
           #define CLOCKID_TO_FD(clk)  ((unsigned int) ~((clk) >> 3))

           struct timespec ts;
           clockid_t clkid;
           int fd;

           fd = open("/dev/ptp0", O_RDWR);
           clkid = FD_TO_CLOCKID(fd);
           clock_gettime(clkid, &ts);
```

### clock 类型总结表格

man clock_getres(2) 中并没有表格，只有上面的散文描述，这里把描述整理成矩阵
(以 man-pages 6.13 为准，并用内核 `posix_clocks[]`，
即 kernel/time/posix-timers.c 中定义的表，对照)：

| clock ID                 | clock_settime 可设置 | 单调(不回退) | 计入 suspend 时间 | 受 NTP/adjtime 频率调整 | 精度             | 备注                                                                                                              |
| ------------------------ | -------------------- | ------------ | ----------------- | ----------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------- |
| CLOCK_REALTIME           | 是，需要特权         | 否           | 否                | 是                      | 时钟源精度(vdso) | 1970 Epoch，忽略闰秒(会被 NTP 慢慢拉)，绝对时间定时器会受影响                                                     |
| CLOCK_REALTIME_COARSE    | 否                   | 否           | 否                | 是(每 tick 更新)        | 1 jiffy          | 直接读 timekeeper 缓存的 tick 值，快但粗                                                                          |
| CLOCK_TAI                | 否                   | 否           | 否                | 是                      | 时钟源精度       | REALTIME + 闰秒偏移，不会因为插入闰秒而跳变；旧版 man 写作 ignoring leap seconds，6.13 改为 counting leap seconds |
| CLOCK_MONOTONIC          | 否                   | 是           | 否                | 是                      | 时钟源精度(vdso) | 起点是开机时刻，不受 settimeofday 影响，suspend 期间不计时                                                        |
| CLOCK_MONOTONIC_RAW      | 否                   | 是           | 否                | 否                      | 时钟源精度       | 直接用硬件 clocksource 的原始计数，不做 NTP 频率修正，suspend 期间不计时                                          |
| CLOCK_MONOTONIC_COARSE   | 否                   | 是           | 否                | 是(每 tick 更新)        | 1 jiffy          | 见下一节，取 tk_xtime，不走 rdtsc，所以 tsc 异常也不会回退                                                        |
| CLOCK_BOOTTIME           | 否                   | 是           | 是                | 是                      | 时钟源精度       | MONOTONIC + suspend 时间，适合需要考虑休眠的单调计时                                                              |
| CLOCK_REALTIME_ALARM     | 否                   | 否           | 否                | 是                      | 时钟源精度       | 与 REALTIME 取值相同，只能给 timer_create 用，可唤醒 suspend，需 CAP_WAKE_ALARM                                   |
| CLOCK_BOOTTIME_ALARM     | 否                   | 是           | 是                | 是                      | 时钟源精度       | 与 BOOTTIME 取值相同，可唤醒 suspend，需 CAP_WAKE_ALARM                                                           |
| 动态 clock(fd)           | 视设备而定           | 视设备而定   | 视设备而定        | 视设备而定              | 视设备而定       | 由字符设备 fd 通过 FD_TO_CLOCKID 转换而来，如 /dev/ptp0，只支持 clock_gettime / clock_settime / clock_adjtime     |

- CLOCK_PROCESS_CPUTIME_ID 进程内所有线程消耗的 CPU 时间(utime + stime)                                                                      |
- CLOCK_THREAD_CPUTIME_ID  仅当前线程消耗的 CPU 时间                                                                                         |

几个容易踩的点：
- 只有 CLOCK_REALTIME 可以 clock_settime，也只有它的 clock_adjtime 走 posix_clock_realtime_adj 。

### CLOCK_MONOTONIC_COARSE
```c
/* Monotonic system-wide clock, updated only on ticks.  */
# define CLOCK_MONOTONIC_COARSE		6
```

和 CLOCK_MONOTONIC_COARSE : 直接获取 tk_xtime ，不会去执行 rdtsc
- ktime_get_raw_ts64
- ktime_get_coarse_ts64

所以，使用 CLOCK_MONOTONIC_COARSE 不可能出现时间回退，即便是在 tsc 是异常的


### ALARM
用于唤醒系统的
```txt
       CLOCK_REALTIME_ALARM (since Linux 3.0)
              This clock is like CLOCK_REALTIME, but will wake the system
              if it is suspended.  The caller must have the
              CAP_WAKE_ALARM capability in order to set a timer against
              this clock.

       CLOCK_BOOTTIME_ALARM (since Linux 3.0)
              This clock is like CLOCK_BOOTTIME, but will wake the system
              if it is suspended.  The caller must have the
              CAP_WAKE_ALARM capability in order to set a timer against
              this clock.
```

#### 为什么需要单独的 clock

POSIX timer 落在 hrtimer 上，suspend 之后硬件 timer 不走，所以 CLOCK_MONOTONIC /
CLOCK_REALTIME 上的定时器在 suspend 期间等于被冻住，只能等别的事件把系统唤醒之后才补发。
CLOCK_REALTIME_ALARM / CLOCK_BOOTTIME_ALARM 走 kernel/time/alarmtimer.c 的另一套机制，
目的就是让定时器自己把系统叫醒:

- timer_create() 传这两个 clock 时，`posix_clocks[]` 里对应的是 alarmtimer.c 的 alarm_clock
- 进 suspend 时 `alarmtimer_suspend()` 遍历两个 alarm base(所有 alarm 挂在 timerqueue 上)，
  取出最近的一个到期时间，用 `rtc_timer_start()` 把它写进 RTC
- RTC 到点产生中断把系统唤醒，resume 之后 `alarmtimer_resume()` 先把 RTC alarm
  撤掉，这个 alarm 再被正常投递出去

所以它不是另一种时间源，取值和 CLOCK_REALTIME / CLOCK_BOOTTIME 完全一样，
只是多了一条“能唤醒 suspend”的属性。两者的区别在 `alarm_bases[]` 的 get_ktime:
ALARM_REALTIME 用 `ktime_get_real()`，ALARM_BOOTTIME 用 `ktime_get_boottime()`。

| 项目                   | CLOCK_REALTIME_ALARM                   | CLOCK_BOOTTIME_ALARM                |
| ---------------------- | -------------------------------------- | ----------------------------------- |
| 取值                   | 同 CLOCK_REALTIME                      | 同 CLOCK_BOOTTIME                   |
| 定时器的到期时间基准   | 墙上时间，会被 NTP / settimeofday 影响 | boot 以来的时间，suspend 期间也照走 |
| clock_settime          | 不支持，EINVAL                         | 不支持，EINVAL                      |
| 建定时器               | 需要 CAP_WAKE_ALARM，否则 EPERM        | 同左                                |
| 没有支持 wakeup 的 RTC | EOPNOTSUPP                             | 同左                                |
| 可用的接口             | timer_create(2)、timerfd_create(2)     | 同左                                |

#### 例子

代码在 docs/kernel/time/code/alarm-clock.c，分两部分: 正常情况下的用法，
以及真的 suspend 下去的唤醒测试。

```sh
make                                                    # 编出 alarm-clock.out，本机跑
/usr/bin/gcc -static -Wall -g -o alarm-clock-static.out alarm-clock.c   # 静态链接，传到 VM 里跑
```

普通用法不需要 suspend，它就是个普通的 POSIX timer，只是需要 CAP_WAKE_ALARM:

```sh
# 普通用户
./alarm-clock.out
# timer_create/timerfd_create: Operation not permitted

# root
sudo ./alarm-clock.out
```

```txt
=== 1. clock_gettime ===
  CLOCK_REALTIME         1789557764.429991676
  CLOCK_REALTIME_ALARM   1789557764.429993550
  CLOCK_BOOTTIME         1415868.665011767
  CLOCK_BOOTTIME_ALARM   1415868.665012125
  REALTIME_ALARM - REALTIME = -93 ns
  BOOTTIME_ALARM - BOOTTIME = -96 ns
  BOOTTIME - MONOTONIC      = -16 ns

=== 2. clock_settime ===
  clock_settime(8) failed: Invalid argument
  clock_settime(9) failed: Invalid argument

=== 3. timer_create(CLOCK_REALTIME_ALARM) + 相对 1 秒定时器 ===
  信号到达，实际等了 1.000 秒

=== 3. timer_create(CLOCK_BOOTTIME_ALARM) + 相对 1 秒定时器 ===
  信号到达，实际等了 1.000 秒

=== 4. timerfd_create(CLOCK_BOOTTIME_ALARM) + 1 秒定时器 ===
  可读，expirations = 1，实际等了 1.000 秒
```

真正的用法是让它把系统叫醒。程序会同时起两个定时器: 一个是做对照的 CLOCK_MONOTONIC，
一个是 alarm clock，然后自己把 mem 写进 /sys/power/state:

```sh
sudo ./alarm-clock.out suspend 10            # CLOCK_BOOTTIME_ALARM
sudo ./alarm-clock.out suspend 8 realtime    # CLOCK_REALTIME_ALARM
```

在 oe2403 (QEMU + openEuler 24.03) 里 suspend 10 秒的输出:

```txt
=== suspend 10 秒，由 CLOCK_BOOTTIME_ALARM 唤醒 ===
  RTC: rtc_cmos 00:05, wakeup =  enabled
  两个 10 秒的定时器都建好了
  写 mem 到 /sys/power/state ...
  已经 resume
    MONOTONIC 增加了 2.496 秒(不含 suspend 时间)
    BOOTTIME  增加了 10.101 秒(包含 suspend 时间)
  收到 alarm timer 的信号，BOOTTIME 才过了 10.101 秒
  而 CLOCK_MONOTONIC 定时器还剩 7.504228009 秒
```

对照的 MONOTONIC 定时器说明了两件事: suspend 期间它不走(BOOTTIME 走了 10.1 秒，
它只走了进 suspend 的 2.5 秒)；而且它只按 awake 时间计时，进 suspend 花掉的 2.5 秒
也算进它的 10 秒里，所以 resume 之后还剩 7.5 秒。alarm timer 则是 BOOTTIME 一走到
10.1 秒就到了，说明系统是被 RTC 在 deadline 上叫醒的。

#### 踩坑

1. CAP_WAKE_ALARM。`alarm_timer_create()` 先查有没有支持 wakeup 的 RTC
   (`alarmtimer_get_rtcdev()` 要求 RTC 有 RTC_FEATURE_ALARM 且 device_may_wakeup)，
   没有就 EOPNOTSUPP；再查 `capable(CAP_WAKE_ALARM)`，没有就 EPERM。
   timerfd_create() 在 fs/timerfd.c 里也是同样的两级检查。
2. 小于 2 秒的 alarm 会直接拒绝 suspend。`alarmtimer_suspend()` 里有
   `if (ktime_to_ns(min) < 2 * NSEC_PER_SEC)` 就返回 -EBUSY 的逻辑，
   于是写 /sys/power/state 会失败:

   ```txt
   === suspend 1 秒，由 CLOCK_BOOTTIME_ALARM 唤醒 ===
     (注意 alarmtimer_suspend() 有个 2 秒的限制，小于 2 秒会拒绝 suspend)
     RTC: rtc_cmos 00:05, wakeup =  enabled
     两个 1 秒的定时器都建好了
     写 mem 到 /sys/power/state ...
     suspend 失败: Device or resource busy
   ```

   dmesg 里对应 PM: Some devices failed to suspend, or early wake event detected。
3. 不能设置。这两个 clock 在 `posix_clocks[]` 里是 alarm_clock，没有 clock_set 回调，
   clock_settime() 直接返回 EINVAL。
4. timerfd_create() 只接受 CLOCK_MONOTONIC / CLOCK_REALTIME / CLOCK_BOOTTIME /
   CLOCK_REALTIME_ALARM / CLOCK_BOOTTIME_ALARM，COARSE / TAI 这些会 EINVAL。
5. 选哪个取决于“多久之后叫醒我”是按什么时间算的: REALTIME_ALARM 的到期时间是墙上
   时间，NTP 或 settimeofday 调时间，唤醒时刻也跟着变；BOOTTIME_ALARM 按
   `ktime_get_boottime()` 算，suspend 期间继续计时，适合 X 秒之后叫醒我这种语义。
6. 在 QEMU 里做 suspend 测试要小心：oe2403 上 virtio-balloon 的 free page reporting
   在 resume 之后会 BUG，栈是 `virtballoon_free_page_report()` -> `virtqueue_get_buf_ctx()`
   -> `detach_buf_split()` 里的 BUG_ON (drivers/virtio/virtio_ring.c)，任何 S3 都会触发，
   `rtcwake -m mem -s 5` 也一样。测这些 clock 之前先 `modprobe -r virtio_balloon`。
7. host 的 ~/.nix-profile gcc 编出来的二进制 interpreter 指向 /nix/store，拷到 VM 里跑
   会报 No such file or directory，要在 VM 里跑就用 `/usr/bin/gcc -static` 编静态的。
8. `rtcwake -m mem -s 5` 是另一条路: 它直接对 /dev/rtc0 做 ioctl 设 RTC alarm，
   不经过 alarmtimer。


## TODO
1. clock_id 各种类型 CLOCK_REALTIME 的区分
  - 如何理解 TAI
2. 那些系统调用使用 clock_id，那些不使用，例如  timer_create 是使用的



## CLOCK_BOOTTIME 的理解
在这个内核树里可以直接对着代码验证。CLOCK_REALTIME 和 CLOCK_BOOTTIME 都不是独立的时钟，它们都由同一个 timekeeper 的单调基准推导出来：

```c
  realtime = monotonic + tk->offs_real;   /* offs_real = -wall_to_monotonic */
  boottime = monotonic + tk->offs_boot;   /* offs_boot 累加 suspend 时间 */
```

所以

```
  realtime - boottime == offs_real - offs_boot == getboottime64()
```

也就是系统启动那一刻的墙上时间（kernel/time/timekeeping.c:2648）。它只在 offs_real 或 offs_boot 被改写时变化。

不会改变差值的操作
- NTP 的频率调整 / slew（普通 adjtimex(ADJ_FREQUENCY)、adjtime(3) 的 ADJ_OFFSET_SINGLESHOT、chronyd/ntpd 的常规慢速纠偏）：这些只改 tk->tkr_mono.mult（
  共享），realtime 和 monotonic 一起被加速/减速，offs_real 不变；adjtime 的一次性 offset 也是折叠进 tick length 变成速率调整
  （kernel/time/ntp.c:470-487）。因此差值在 slew 期间保持不变，但两个时钟都偏离"真实秒"。
- suspend/resume：__timekeeping_inject_sleeptime()（timekeeping.c:2093）对 xtime 加 delta、对 wall_to_monotonic 减 delta（monotonic 不动）、对
  offs_boot 加 delta（tk_update_sleep_time() :269），即 realtime 和 boottime 各前进同样的 delta ⇒ realtime−boottime 不变。变的是
  realtime−monotonic（+delta）和 monotonic−boottime（−delta）。RTC 那条 resume 路径（drivers/rtc/class.c: rtc_resume）也只是注入同一个 sleeptime，不是
  用 RTC 覆盖 realtime。

**这里 suspend/resume** 是否计入到 realtime ，和前面的表格不同。

会改变差值的操作
1. 对 realtime 做 step：settimeofday / clock_settime(CLOCK_REALTIME) / adjtimex 的 ADJ_SETOFFSET / NTP 守护进程的 makestep（chronyd -s、ntpd -g）/ 开机
   hctosys 读 RTC —— 差值跳变，跳变量就是 step 量（do_settimeofday64() timekeeping.c:1655，__timekeeping_inject_offset() 1770）。
2. 闰秒：内核在 accumulate_nsecs_to_secs() 里做 tk->xtime_sec += leap; + tk_set_wall_to_mono(wtm - 1s)（timekeeping.c:2492），即 realtime 变 1s 而
   monotonic/boottime 不动。vdso 快速路径也特意为此在 next_leap_ktime 后把 offs_real 减 1s（timekeeping.c:2821）。

这都容易理解，如果时间不准了，然后对于 realtime 做一个大的修正，但是不用修正 BOOTIIME

## CLOCK_TAI 是做啥的
https://stackoverflow.com/questions/32652688/what-is-the-epoch-of-clock-tai

```txt
       CLOCK_TAI (since Linux 3.10; Linux-specific)
              A nonsettable system-wide clock  derived  from  wall-clock
              time but ignoring leap seconds.  This clock does not expe‐
              rience  discontinuities  and backwards jumps caused by NTP
              inserting leap seconds as CLOCK_REALTIME does.

              The acronym TAI refers to International Atomic Time.
```

### 如何理解这个的 CLOCK_TAI 是什么东西?
```txt
/*
 * Calculates CLOCK_REALTIME and reports the TSC value from which it did
 * so. Returns true if host is using TSC based clocksource.
 *
 * DO NOT USE this for anything related to migration. You want CLOCK_TAI
 * for that.
 */
static bool kvm_get_walltime_and_clockread(struct timespec64 *ts,
					   u64 *tsc_timestamp)
{
	/* checked again under seqlock below */
	if (!gtod_is_based_on_tsc(pvclock_gtod_data.clock.vclock_mode))
		return false;

	return gtod_is_based_on_tsc(do_realtime(ts, tsc_timestamp));
}
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
