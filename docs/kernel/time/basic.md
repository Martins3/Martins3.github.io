## 基本问题
```txt
config POSIX_TIMERS
	bool "Posix Clocks & timers" if EXPERT
	default y
	help
	  This includes native support for POSIX timers to the kernel.
	  Some embedded systems have no use for them and therefore they
	  can be configured out to reduce the size of the kernel image.

	  When this option is disabled, the following syscalls won't be
	  available: timer_create, timer_gettime: timer_getoverrun,
	  timer_settime, timer_delete, clock_adjtime, getitimer,
	  setitimer, alarm. Furthermore, the clock_settime, clock_gettime,
	  clock_getres and clock_nanosleep syscalls will be limited to
	  CLOCK_REALTIME, CLOCK_MONOTONIC and CLOCK_BOOTTIME only.

	  If unsure say y.
```

kernel/time/Makefile
```c
ifeq ($(CONFIG_POSIX_TIMERS),y)
 obj-y += posix-timers.o posix-cpu-timers.o posix-clock.o itimer.o
else
 obj-y += posix-stubs.o
endif
```

## 每一个文件都是做什么的

### alarmtimer.c
```c
static const struct k_clock * const posix_clocks[] = {
	[CLOCK_REALTIME]		= &clock_realtime,
	[CLOCK_MONOTONIC]		= &clock_monotonic,
	[CLOCK_PROCESS_CPUTIME_ID]	= &clock_process,
	[CLOCK_THREAD_CPUTIME_ID]	= &clock_thread,
	[CLOCK_MONOTONIC_RAW]		= &clock_monotonic_raw,
	[CLOCK_REALTIME_COARSE]		= &clock_realtime_coarse,
	[CLOCK_MONOTONIC_COARSE]	= &clock_monotonic_coarse,
	[CLOCK_BOOTTIME]		= &clock_boottime,
	[CLOCK_REALTIME_ALARM]		= &alarm_clock,
	[CLOCK_BOOTTIME_ALARM]		= &alarm_clock,
	[CLOCK_TAI]			= &clock_tai,
};
```

由于 CPU 已经 suspend 了，所以需要借助 rtc 来唤醒。

```c
static const struct k_clock clock_realtime = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get_timespec	= posix_get_realtime_timespec,
	.clock_get_ktime	= posix_get_realtime_ktime,
	.clock_set		= posix_clock_realtime_set,
	.clock_adj		= posix_clock_realtime_adj,
	.nsleep			= common_nsleep,
	.timer_create		= common_timer_create,
	.timer_set		= common_timer_set,
	.timer_get		= common_timer_get,
	.timer_del		= common_timer_del,
	.timer_rearm		= common_hrtimer_rearm,
	.timer_forward		= common_hrtimer_forward,
	.timer_remaining	= common_hrtimer_remaining,
	.timer_try_to_cancel	= common_hrtimer_try_to_cancel,
	.timer_wait_running	= common_timer_wait_running,
	.timer_arm		= common_hrtimer_arm,
};
```

posix_get_hrtimer_res 也是可以走 vdso 的，

```c
SYSCALL_DEFINE2(clock_gettime, const clockid_t, which_clock,
		struct __kernel_timespec __user *, tp)
{
	const struct k_clock *kc = clockid_to_kclock(which_clock);
	struct timespec64 kernel_tp;
	int error;

	if (!kc)
		return -EINVAL;

	error = kc->clock_get_timespec(which_clock, &kernel_tp);

	if (!error && put_timespec64(&kernel_tp, tp))
		error = -EFAULT;

	return error;
}
```
获取时间居然是从 k_clock::clock_get_timespec 这里获取到的。

### posix-timers.c && posix-cpu-timer.c
定义各种系统调用
```txt
__do_sys_timer_create
__do_sys_timer_gettime
__do_sys_timer_getoverrun
__do_sys_timer_settime
__do_sys_timer_delete
__do_sys_clock_settime
__do_sys_clock_gettime
__do_sys_clock_adjtime
__do_sys_clock_getres
__do_sys_clock_nanosleep
```
### itimer.c
提供:
```txt
__do_sys_getitimer
__do_sys_alarm
__do_sys_setitimer
```

需要 task_struct::hrtimer 的支持，然后调用通用的 hrtimer API:

```c
struct signal_struct {

    /* ITIMER_REAL timer for the process */
    struct hrtimer real_timer;
    ktime_t it_real_incr;

```

do_setitimer ==> hrtimer_start

### misc
2. timer.c 传统的低精度 timer 模块，基本 tick 的, 实现方法是 ldd3 : chapter 7 中间介绍，很简单的。
1. time.c 文件是一个向用户空间提供时间接口的模块。具体包括：time, stime, gettimeofday, settimeofday, adjtime。
除此之外，该文件还提供一些时间格式转换的接口函数（其他内核模块使用），例如 jiffes 和微秒之间的转换，日历时间（Gregorian date）和 xtime 时间的转换。
xtime 的时间格式就是到 linux epoch 的秒以及纳秒值。
timeconv.c 中包含了从 calendar time 到 broken-down time 之间的转换函数接口。
2. timer_list.c 提供 /proc/timer_list。

- 从 hrtimer 最后是如何处理周期的 tick 的: 参考 tick_setup_sched_timer

### timer.c

低精度时钟，主要提供接口:

- timer_setup : do_init_timer
- add_timer_on
- del_timer
- mod_timer
- mod_timer_pending
- timer_reduce

timer 的 callback 是在软中断里面的，这是非常合理的，
毕竟这就是低精度 timer 的实现机制。
而 hrtimer 的 callback 在硬中断里面。

### tick-common.c && tick-oneshot.c && tick-sched.c

tick-common.c && tick-oneshot.c, 利用 clockevent 来控制周期 tick 的状态，尤其是 tick-oneshot.c，只是包含 switch state, resume 等

> tick-common.c 文件是 periodic tick 模块，用于管理周期性 tick 事件。
> tick-oneshot.c 文件是 for 高精度 timer 的，用于管理高精度 tick 时间。
> tick-sched.c 是用于 dynamic tick 的。
>
> 参考: http://www.wowotech.net/timer_subsystem/time-subsyste-architecture.html

- [ ] 周期性的 tick 和高精度 tick 的本质区别是 ?
- [ ] 高精度 tick 还是周期性的 tick, 但是但是按照 one shot 的方法通知 CPU tick 来了 ?

- tick_freeze
  - tick_suspend_local
    - clockevents_shutdown
      - clockevents_switch_state
        - `__clockevents_switch_state`
          - dev->set_state_shutdown
          - dev->set_state_periodic
            - lapic_clockevent::set_state_periodic
              - lapic_timer_set_periodic
                - lapic_timer_set_periodic_oneshot
                  - `__setup_APIC_LVTT`
          - dev->set_state_oneshot

tick-sched.c 相对复杂一些，毕竟多出来了一个可以不 tick 的状态，该状态的进入退出之类的管理。

### posix-clock.c
- posix_clock_register

```c
static const struct file_operations posix_clock_file_operations = {
	.owner		= THIS_MODULE,
	.read		= posix_clock_read,
	.poll		= posix_clock_poll,
	.unlocked_ioctl	= posix_clock_ioctl,
	.open		= posix_clock_open,
	.release	= posix_clock_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= posix_clock_compat_ioctl,
#endif
};
```

## gettimeofday 喜欢使用 timeval ，但是 kernel 内部包括

clock_gettime 使用的
```c
struct __kernel_timespec {
	__kernel_time64_t       tv_sec;                 /* seconds */
	long long               tv_nsec;                /* nanoseconds */
};
```

kernel 内部用的:
```c
struct timespec64 {
	time64_t	tv_sec;			/* seconds */
	long		tv_nsec;		/* nanoseconds */
};
```

```c
/* Nanosecond scalar representation for kernel time values */
typedef s64	ktime_t;

/*
 * legacy timeval structure, only embedded in structures that
 * traditionally used 'timeval' to pass time intervals (not absolute
 * times). Do not add new users. If user space fails to compile
 * here, this is probably because it is not y2038 safe and needs to
 * be changed to use another interface.
 */
#ifndef __kernel_old_timeval
struct __kernel_old_timeval {
	__kernel_long_t tv_sec;
	__kernel_long_t tv_usec;
};
#endif
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
