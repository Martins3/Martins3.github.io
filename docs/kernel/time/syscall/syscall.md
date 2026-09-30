# time syscall

和时间相关的系统调用有哪些?

## 问题
在 man getitimer(2) 中，其中的 which 参数为: ITIMER_PROF ITIMER_REAL ITIMER_VIRTUAL

那么 clock_gettime 中的 monotonic 的参数和这里的如何匹配?
```txt
       int setitimer(int which, const struct itimerval *restrict value,
           struct itimerval *restrict ovalue);
```

## posix timer

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

## gettimeofday(2) time(2) clock_gettime(2)


https://stackoverflow.com/questions/2086126/need-programs-that-illustrate-use-of-settimer-and-alarm-functions-in-gnu-c

When timers are used, there are **three** options to distinguish how elapsed time is counted or in which time base the timer resides.

1. `ITIMER_REAL` measures the actual elapsed time between activation of the timer and time-out
   in order to trigger the signal. In this case, the timer continues to tick regardless of whether the
   system is in kernel mode or user mode or whether the application using the timer is currently
   running or not. A signal of the `SIGALRM` type is sent when the timer times out.
2. `ITIMER_VIRTUAL` runs only during the time spent by the owner process of the timer in user mode.
   In this case, time spent in kernel mode (or when the processor is busy with another application)
   is ignored. Time-out is indicated by the `SIGVTALRM` signal.
3. `ITIMER_PROF` calculates the time spent by the process both in user and kernel mode — time
   continues to elapse when a system call is executed on behalf of the task. Other processes of the
   system are ignored. The signal sent at time-out is `SIGPROF`.

## 代码分析
sudo perf trace -e syscalls:sys_enter_gettimeofday
sudo perf trace -e syscalls:sys_enter_clock_gettime

vn/code/module/c/clock_time.c 测试，发现 clock_time 也是走的 vdso ，
才意识到原来 vdso 是可以用来执行代码的.

实现参考 lib/vdso/gettimeofday.c arch/x86/entry/vdso/vclock_gettime.c

## TODO
- https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof
- https://news.ycombinator.com/item?id=8909146
- https://github.com/nmap/nmap/issues/180

## 总结
- https://www.kernel.org/doc/html/latest/timers/

https://people.cs.rutgers.edu/~pxk/416/notes/c-tutorials/gettime.html

## 有趣
https://stackoverflow.com/questions/32652688/what-is-the-epoch-of-clock-tai

## 从 fio 的角度来看看
https://fio.readthedocs.io/en/latest/fio_doc.html

> clocksource=str
> Use the given clocksource as the base of timing. The supported options are:
>
> gettimeofday
> gettimeofday(2)
>
> clock_gettime
> clock_gettime(2)
>
> cpu
> Internal CPU clock source
>
> cpu is the preferred clocksource if it is reliable, as it is very fast (and fio is heavy on time calls). Fio will automatically use this clocksource if it’s supported and considered reliable on the system it is running on, unless another clocksource is specifically set. For x86/x86-64 CPUs, this means supporting TSC Invariant.
> gtod_reduce=bool
Enable all of the gettimeofday(2) reducing options (disable_clat, disable_slat, disable_bw_measurement) plus reduce precision of the timeout somewhat to really shrink the gettimeofday(2) call count. With this option enabled, we only do about 0.4% of the gettimeofday(2) calls we would have done if all time keeping was enabled.
> gtod_cpu=int
>
> Sometimes it’s cheaper to dedicate a single thread of execution to just getting the current time. Fio (and databases, for instance) are very intensive on gettimeofday(2) calls. With this option, you can set one CPU aside for doing nothing but logging current time to a shared memory location. Then the other threads/processes that run I/O workloads need only copy that segment, instead of entering the kernel with a gettimeofday(2) call. The CPU set aside for doing these time calls will be excluded from other uses. Fio will manually clear it from the CPU mask of other jobs.

1. 对于 fio ， 按道理 gettimeofday 和 clock_gettime 没什么区别，不知道为什么要做区分?

## 分析一下时间调整的 syscall

基本的时间调整:

- __do_sys_adjtimex
  - do_adjtimex

- clock_adjtime
  - posix_clock_realtime_adj : 只有 REALTIME 支持
    - do_adjtimex

- do_adjtimex
  - timekeeping_advance
    - timekeeping_adjust
      - timekeeping_apply_adjustment

通过 update_wall_time 也调用 timekeeping_advance ，那个是基本的关键路径

## timerfd
based on linux/kernel/time/alarmtimer.c


## USER_HZ
好吧，我才注意到，原来有的接口在使用 USER_HZ 作为接口，真的是没事找事:

- sysconf(_SC_CLK_TCK)（glibc 返回的就是 USER_HZ）
- 或 getconf CLK_TCK / shell 里的 $ getconf CLK_TCK

哪些接口的数值是 USER_HZ 单位
┌───────────────────────┬─────────────────────────────────────────┐
│ 接口                  │ 字段                                    │
├───────────────────────┼─────────────────────────────────────────┤
│ times(2)              │ struct tms 四个成员、返回值             │
├───────────────────────┼─────────────────────────────────────────┤
│ /proc/stat            │ cpu/cpuN 的 user/nice/system/idle/…     │
├───────────────────────┼─────────────────────────────────────────┤
│ /proc/<pid>/stat      │ utime、stime、cutime、cstime、starttime │
├───────────────────────┼─────────────────────────────────────────┤


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
