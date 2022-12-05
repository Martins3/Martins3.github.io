# QEMU 中的时钟

<!-- vim-markdown-toc GitLab -->

* [time-meter](#time-meter)
* [timer](#timer)
* [timerlist](#timerlist)
* [timerlistgroup](#timerlistgroup)
* [misc](#misc)

<!-- vim-markdown-toc -->

QEMU 中的 timer 需要完成两个事情，计时和定时。

[time-meter](#time-meter) 和
[timer](#timer) 简要分析 QEMU 如何实现计时器和定时器的功能。
因为 QEMU 有四个不同的时钟，相同的 timer 放到一个 [timerlist](#timerlist) 中。
一个事件监听 thread  需要持有一个 [timerlistgroup](#timerlistgroup)。

## time-meter
因为 guest 可能会停止，而 host 不会停止，所以会出现 guest 和 host 的 timer 会出现差异的，
然后因为的 replay 机制，实际上 timer 变得更加的复杂和奇怪了。

```c
/**
 * QEMUClockType:
 *
 * The following clock types are available:
 *
 * @QEMU_CLOCK_REALTIME: Real time clock
 *
 * The real time clock should be used only for stuff which does not
 * change the virtual machine state, as it runs even if the virtual
 * machine is stopped.
 *
 * @QEMU_CLOCK_VIRTUAL: virtual clock
 *
 * The virtual clock only runs during the emulation. It stops
 * when the virtual machine is stopped.
 *
 * @QEMU_CLOCK_HOST: host clock
 *
 * The host clock should be used for device models that emulate accurate
 * real time sources. It will continue to run when the virtual machine
 * is suspended, and it will reflect system time changes the host may
 * undergo (e.g. due to NTP).
 *
 * @QEMU_CLOCK_VIRTUAL_RT: realtime clock used for icount warp
 *
 * Outside icount mode, this clock is the same as @QEMU_CLOCK_VIRTUAL.
 * In icount mode, this clock counts nanoseconds while the virtual
 * machine is running.  It is used to increase @QEMU_CLOCK_VIRTUAL
 * while the CPUs are sleeping and thus not executing instructions.
 */

typedef enum {
    QEMU_CLOCK_REALTIME = 0,
    QEMU_CLOCK_VIRTUAL = 1,
    QEMU_CLOCK_HOST = 2,
    QEMU_CLOCK_VIRTUAL_RT = 3,
    QEMU_CLOCK_MAX
} QEMUClockType;
```


```c
int64_t qemu_clock_get_ns(QEMUClockType type)
{
    switch (type) {
    case QEMU_CLOCK_REALTIME:
        return get_clock(); // 调用到 clock_gettime 或者 get_clock_realtime, 取决于 CLOCK_MONOTONIC 选项是否打开
    default:
    case QEMU_CLOCK_VIRTUAL:
        if (use_icount) {
            return cpu_get_icount();
        } else {
            return cpu_get_clock();
        }
    case QEMU_CLOCK_HOST:
        return REPLAY_CLOCK(REPLAY_CLOCK_HOST, get_clock_realtime());
    case QEMU_CLOCK_VIRTUAL_RT:
        return REPLAY_CLOCK(REPLAY_CLOCK_VIRTUAL_RT, cpu_get_clock());
    }
}
```
- 如果没有 replay 的情况，后面两个 macro 都是可以直接退化为 get_clock_realtime 和 cpu_get_clock 的。
- cpu_get_clock 和 get_clock 区别在于 vm 停止之后是否计时
- get_clock_realtime 和 get_clock 的区别在于: 当 CLOCK_MONOTONIC 定义了的时候，get_clock 会去调用 clock_gettime 而不是 get_clock_realtime
  - clock_gettime 和 gettimeofday 的区别参看 [stackoverflow](https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof)

- [x] timers_state.cpu_clock_offset 总是不变的
  - rnm, 太真实了，就是在 cpu_enable_ticks 中减去当时的 get_clock 的呀
- [x] 似乎退出的时候，会导致 timers_state.cpu_clock_offset 发生修改
  - 那是因为 cpu_disable_ticks 的原因

cpu_get_clock 的及时就是靠 cpu_disable_ticks 和 cpu_enable_ticks 实现的了

qemu_clock_enable 的调用位置:
- resume_all_vcpus
- qemu_tcg_rr_cpu_thread_fn : 因为单步调试的时候屏蔽中断和 timer 这是一个很正确的操作实际上。

qemu_clock_enable 的原理:
- 如果将 clock 装换为可以使用，立刻检查一下是否有 timer 结束了
- 如果 disable，那么自然需要等待这些 timer list 结束才可以的呀

## timer
因为 guest 需要周期性的注入时钟中断(apic / ioapic)，各种时钟设备(pit / hpet)设备的模拟，以及一些模拟设备的需求。
QEMU 需要实现定时器的功能。

下面是 timer_new 的调用位置:
- apic_realize
- ioapic_realize
- pci_std_vga_realize
- hpet_realize
- rtc_realizefn
- pit_realizefn
- serial_realize_core
- fdctrl_realize_common
- pci_e1000_realize
- ide_init1
- acpi_pm_tmr_init
- text_console_do_init
- gui_setup_refresh
- nvme_init_cq / nvme_init_sq

实现 timer 本来是使用 timer_create 系统调用的，但是在 [aio / timers: Remove alarm timers](https://github.com/qemu/qemu/commit/6d327171551a12b937c5718073b9848d0274c74d)
中，将 timer 机制和 QEMU 本身的事件监听机制合并到一起了。

在 main loop 中，大致的执行流程为:
- main_loop_wait
  - timerlistgroup_deadline_ns : 获取最近将会 timerout 时间
    - qemu_soonest_timeout
  - os_host_main_loop_wait
    - qemu_poll_ns
      - ppoll : 等待
  - qemu_clock_run_all_timers

在 main loop 中使用 ppoll(2) 来监听 fd，一旦 fd 事件到达(比如 socket 上有数据发送)，那么 ppoll 就可以返回了。
实际上，QEMU 监听 timer timeout 的机制和监听 fd 的机制合并在一起的。
对于 timer，让 main loop 从 ppoll 上返回的方法是设置其参数 timeout，这样，只要无论是 fd ready 还是 timer timeout 都会导致 ppoll 返回。
```c
    timeout_ns = qemu_soonest_timeout(timeout_ns,
                                      timerlistgroup_deadline_ns(
                                          &main_loop_tlg));
```

这是一个经典的处理 timer timeout hook 的路径:
```txt
#0  rtc_update_timer (opaque=0x55555669b400) at /home/maritns3/core/xqm/hw/rtc/mc146818rtc.c:427
#1  0x0000555555caea96 in timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:595
#2  timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:506
#3  0x0000555555caec90 in qemu_clock_run_timers (type=<optimized out>) at /home/maritns3/core/xqm/util/qemu-timer.c:695
#4  qemu_clock_run_all_timers () at /home/maritns3/core/xqm/util/qemu-timer.c:695
#5  0x0000555555caf0c1 in main_loop_wait (nonblocking=<optimized out>) at /home/maritns3/core/xqm/util/main-loop.c:525
#6  0x00005555559bbf59 in main_loop () at /home/maritns3/core/xqm/vl.c:1812
#7  0x000055555582b2f9 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4473
```
## timerlist
不同种类的 timer 上的时间的进度不同，为了方便管理，一个类型的 timer 都会插入到相同的
timerlist 上。

比如 timerlist_deadline_ns 就是扫描一个 timerlist 上的所有 timerout 的时间，从而计算出最近的 timeout 时间。

```c
/* A QEMUTimerList is a list of timers attached to a clock. More
 * than one QEMUTimerList can be attached to each clock, for instance
 * used by different AioContexts / threads. Each clock also has
 * a list of the QEMUTimerLists associated with it, in order that
 * reenabling the clock can call all the notifiers.
 */

struct QEMUTimerList {
    QEMUClock *clock;
    QemuMutex active_timers_lock;
    QEMUTimer *active_timers;
    QLIST_ENTRY(QEMUTimerList) list;
    QEMUTimerListNotifyCB *notify_cb;
    void *notify_opaque;

    /* lightweight method to mark the end of timerlist's running */
    QemuEvent timers_done_ev;
};
```
timers_done_ev 是为了防止一个 thread 正在执行 timerlist 的 timer 的 callback 的时候，
结果另一个 thread 却在 enable 或者 disable 其。


因为一旦监听到事件，那么就可以 poll / ppoll / iouring 上返回，然后就会去执行 qemu_clock_run_all_timers
通过 timerlist_notify 可以让立刻从 poll / ppoll / iouring 返回，因为在不同的 thread 中使用监听方法稍有不同。

```c
void timerlist_notify(QEMUTimerList *timer_list)
{
    if (timer_list->notify_cb) {
        timer_list->notify_cb(timer_list->notify_opaque, timer_list->clock->type);
    } else {
        // 似乎并没有人调用到此处
        qemu_notify_event();
    }
}
```

用于注册的 hook 为:
```c
void qemu_timer_notify_cb(void *opaque, QEMUClockType type);

void aio_notify(AioContext *ctx);
```

为什么存在立刻执行 qemu_clock_run_all_timers 的需求?
- 如果添加的 timer 是 soonest 的时候(分析 timer_mod_ns_locked)，这要求 ppoll 提前返回，否则等到 ppoll 返回的时候，这个 timer 要求的时间已经过去了

## timerlistgroup
```c
struct QEMUTimerListGroup {
    QEMUTimerList *tl[QEMU_CLOCK_MAX];
};
```

因为 iothread 的引入，QEMU 不仅仅在 main loop 使用 ppoll 等待，还有可能在 iothread 中来等待时钟。
在 [aio / timers: Split QEMUClock into QEMUClock and QEMUTimerList](https://github.com/qemu/qemu/commit/ff83c66eccf5b5f6b6530d504e3be41559250dcb)
创建出来了 QEMUTimerListGroup，一个事件监听 thread 持有一个 group。

创建一个 QEMUTimerListGroup 的时候， 对于每一个 QEMUClockType 创建一个 timerlist。

- 使用 QEMUTimerListGroup 的地方
  - timerlistgroup_run_timers
  - timerlistgroup_init
  - timerlistgroup_deadline_ns
  - timer_init_full

main_loop_tlg 是 main loop 的 QEMUTimerListGroup, 也是默认使用的。

[timer](#timer) 中列举了各种 timer_new 的调用位置，那些 timer 都是添加到 main_loop_tlg 上的
而添加到其他 QEMUTimerListGroup 的位置默认情况下只有:

```txt
>>> bt
#0  aio_timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1000000, cb=0x555555bdf550 <cache_clean_timer_cb>, opaque=0x555556850630, ctx=0x5555565d75b0) at /home/maritns3/core/
xqm/include/block/aio.h:433
#1  cache_clean_timer_init (bs=0x555556850630, context=0x5555565d75b0) at /home/maritns3/core/xqm/block/qcow2.c:828
#2  0x0000555555be2b72 in qcow2_update_options_commit (bs=0x555556850630, r=0x7fffe841fe20) at /home/maritns3/core/xqm/block/qcow2.c:1208
#3  0x0000555555be423c in qcow2_update_options (bs=0x555556850630, options=<optimized out>, flags=<optimized out>, errp=<optimized out>) at /home/maritns3/core/xqm/block/qcow2.c:1235
#4  0x0000555555be59e2 in qcow2_do_open (bs=0x555556850630, options=0x555556bda000, flags=139266, errp=0x7fffffffcbe0) at /home/maritns3/core/xqm/block/qcow2.c:1513
#5  0x0000555555be6626 in qcow2_open_entry (opaque=0x7fffffffcb80) at /home/maritns3/core/xqm/block/qcow2.c:1792
#6  0x0000555555cc7283 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at /home/maritns3/core/xqm/util/coroutine-ucontext.c:115
#7  0x00007ffff5a6f660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

## misc
- 如果一个 Timer 被添加到了 timerlist 中，那么 QEMUTimer::expire_time 不应该等于 -1
- timer_mod_anticipate_ns 和 timer_mod_ns 的区别: 前者要求，只有提前这个 timer 的时候，才可以修改 timerlist，否则此次操作为空
- 因为 QEMU_CLOCK_VIRTUAL 类型的 clock 只有在 CPU 运行的时候才可以运行的

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://airbus-seclab.github.io/qemu_blog/timers.html
