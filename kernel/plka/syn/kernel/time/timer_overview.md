# kernel/time

<!-- vim-markdown-toc GitLab -->

- [quetion](#quetion)
- [todo](#todo)
- [wowotech](#wowotech)
    - [1](#1)
    - [2 软件架构](#2-软件架构)
    - [3](#3)
    - [4](#4)
    - [5](#5)
    - [6](#6)

<!-- vim-markdown-toc -->

| file                     | blank | comment | code | desc                                                                                                                                                                                                                                                                                                                                                                             |
|--------------------------|-------|---------|------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| timekeeping.c            | 359   | 720     | 1325 |                                                                                                                                                                                                                                                                                                                                                                                  |
| hrtimer.c                | 314   | 723     | 1174 |                                                                                                                                                                                                                                                                                                                                                                                  |
| timer.c                  | 249   | 779     | 1063 | 传统的低精度timer模块，基本tick的。                                                                                                                                                                                                                                                                                                                                              |
| posix-timers.c           | 215   | 242     | 913  |                                                                                                                                                                                                                                                                                                                                                                                  |
| posix-cpu-timers.c       | 194   | 405     | 817  |                                                                                                                                                                                                                                                                                                                                                                                  |
| posix-clock.c            | 72    | 9       | 236  |                                                                                                                                                                                                                                                                                                                                                                                  |
| posix-stubs.c            | 33    | 13      | 194  |                                                                                                                                                                                                                                                                                                                                                                                  |
| clocksource.c            | 164   | 381     | 685  | find the best clocksource                                                                                                                                                                                                                                                                                                                                                        |
| ntp.c                    | 170   | 236     | 641  |                                                                                                                                                                                                                                                                                                                                                                                  |
| time.c                   | 129   | 251     | 620  | time.c文件是一个向用户空间提供时间接口的模块。具体包括：time, stime, gettimeofday, settimeofday,adjtime。除此之外，该文件还提供一些时间格式转换的接口函数（其他内核模块使用），例如jiffes和微秒之间的转换，日历时间（Gregorian date）和xtime时间的转换。xtime的时间格式就是到linux epoch的秒以及纳秒值。 timeconv.c中包含了从calendar time到broken-down time之间的转换函数接口。 |
| alarmtimer.c             | 136   | 209     | 569  |                                                                                                                                                                                                                                                                                                                                                                                  |
| clockevents.c            | 114   | 194     | 470  |                                                                                                                                                                                                                                                                                                                                                                                  |
| timer_list.c             | 54    | 14      | 312  |                                                                                                                                                                                                                                                                                                                                                                                  |
| tick-common.c            | 72    | 204     | 301  | 管理周期性tick事件                                                                                                                                                                                                                                                                                                                                                               |
| tick-oneshot.c           | 16    | 38      | 74   | 理高精度tick时间                                                                                                                                                                                                                                                                                                                                                                 |
| tick-sched.c             | 231   | 378     | 820  | 用于dynamic tick的                                                                                                                                                                                                                                                                                                                                                               |
| tick-broadcast.c         | 124   | 329     | 562  |                                                                                                                                                                                                                                                                                                                                                                                  |
| tick-broadcast-hrtimer.c | 8     | 50      | 53   |                                                                                                                                                                                                                                                                                                                                                                                  |
| itimer.c                 | 63    | 52      | 292  |	interval timer模块                                                                                                                                                                                                                                                                                                                                                                                  |
| sched_clock.c            | 53    | 82      | 173  |                                                                                                                                                                                                                                                                                                                                                                                  |
| test_udelay.c            | 29    | 12      | 119  |                                                                                                                                                                                                                                                                                                                                                                                  |
| vsyscall.c               | 25    | 26      | 79   |                                                                                                                                                                                                                                                                                                                                                                                  |
| jiffies.c                | 20    | 32      | 68   |                                                                                                                                                                                                                                                                                                                                                                                  |
| timecounter.c            | 16    | 29      | 54   |                                                                                                                                                                                                                                                                                                                                                                                  |
| timekeeping_debug.c      | 9     | 7       | 39   |                                                                                                                                                                                                                                                                                                                                                                                  |
| posix-timers.h           | 4     | 1       | 37   |                                                                                                                                                                                                                                                                                                                                                                                  |
| timekeeping.h            | 5     | 4       | 23   |                                                                                                                                                                                                                                                                                                                                                                                  |
| timekeeping_internal.h   | 4     | 8       | 22   |                                                                                                                                                                                                                                                                                                                                                                                  |
| Makefile                 | 2     | 1       | 18   |                                                                                                                                                                                                                                                                                                                                                                                  |
| ntp_internal.h           | 1     | 2       | 12   |                                                                                                                                                                                                                                                                                                                                                                                  |

- [ ] itimer, should be really simple.


## quetion
1. 时钟中断的 handler 是什么
2. 提供那些服务 ?
    1. 时间
    2. 计时器
    3. tick

3. posix timer 是怎么回事 ?
4. 高精度 和 低精度 时钟的关系是什么 ?
    1. 各自对应什么位置的代码 ?
    2. hrtimer use redblack tree too ?
5. 架构的层次是什么 ?
6. 多核导致的挑战是什么 ?
    1. 全局时钟
        1. 是所有人都可以接受，还是 ?
    2. 局部时钟控制器 : 不同的位置需要同步吗 ?
2. 广播模式

4. clock sources, clock event devices, and tick devices 都是啥呀 ?

## todo
1. posix-clock.c 中间的内容似乎就是 : clock 设备管理的 ?
3. 找到 clock source chip driver 具体代表什么 ?
3. clock event 是不是需要 clock 硬件具有timer 的功能 ? 也就是向硬件告诉什么时候 interrupts 我，而不是每 nanoseconds 过来提醒一次。(这是显然的)
    1. 那么，我想知道，提交给硬件这个消息的方法是什么 ?
    2. 
4. dynimic 和 high resolution timer 之间的切换是什么 ?
5. hrtimer 是如何利用那 clock source 和 clockevent 构建的 ?
6. clock source 和 clock event 到底有什么，难道不是仅仅一千行吗 ?
7. 据说，低精度 timer 是基于高精度 timer 的，找到对应的证据是什么 ?

- [ ] what do clockevents.c and clocksource.c contains ?
- [ ] tick one shot, tick broadcast, tick sched

- [ ] article 2 need more examine

> @todo clockevent 和 clocksource 使用的是同一个 dev ?

https://en.wikipedia.org/wiki/High_Precision_Event_Timer

> 看上去，clocksource 和 clockevent 都是依赖于底层的函数。

## wowotech

#### [1](http://www.wowotech.net/timer_subsystem/clock-id-in-linux.html)
```c
#define CLOCK_REALTIME            0
#define CLOCK_MONOTONIC            1
#define CLOCK_PROCESS_CPUTIME_ID    2
#define CLOCK_THREAD_CPUTIME_ID        3
#define CLOCK_MONOTONIC_RAW        4
#define CLOCK_REALTIME_COARSE        5
#define CLOCK_MONOTONIC_COARSE        6
#define CLOCK_BOOTTIME            7
#define CLOCK_REALTIME_ALARM        8
#define CLOCK_BOOTTIME_ALARM        9
#define CLOCK_SGI_CYCLE            10    /* Hardware specific */
#define CLOCK_TAI            11
```
- [ ] What is `CLOCK_*` ?


#### [2 软件架构](http://www.wowotech.net/timer_subsystem/time-subsyste-architecture.html)

> 1. 我们首先看周期性tick的实现。起始点一定是底层的clock source chip driver，该driver会调用注册clock event的接口函数（clockevents_config_and_register或者clockevents_register_device），
一旦增加了一个clock event device，需要通知上层的tick device layer，毕竟有可能新注册的这个device更好、更适合某个tick device呢（通过调用tick_check_new_device函数实现）。
要是这个clock event device被某个tick device收留了（要么该tick device之前没有匹配的clock event device，要么新的clock event device更适合该tick device），那么就启动对该tick device的配置（参考tick_setup_device）。
根据当前系统的配置情况（周期性tick），会调用tick_setup_periodic函数，这时候，该tick device对应的clock_event_device的clock_event_handler被设置为tick_handle_periodic。
底层硬件会周期性的产生中断，从而会周期性的调用tick_handle_periodic从而驱动整个系统的运转。
需要注意的是：即便是配置了CONFIG_NO_HZ和CONFIG_TICK_ONESHOT，系统中没有提供one shot的clock event device，这种情况下，整个系统仍然是运行在周期tick的模式下。
高精度timer总是会被编入最后的kernel中。在这种构架下，各个内核模块也可以调用linux kernel中的高精度timer模块的接口函数来实现高精度timer，但是，这时候高精度timer模块是运行在低精度的模式，也就是说这些hrtimer虽然是按照高精度timer的红黑树进行组织，
**但是系统只是在每一周期性tick到来的时候调用`hrtimer_run_queues`函数，来检查是否有expire的hrtimer。毫无疑问，这里的高精度timer也就是没有意义了。**


- [ ] clockevents_config_and_register
  - [ ] clockevents_config
  - [ ] clockevents_register_device
    -  tick_check_new_device : Check, if the new registered device should be used. Called with clockevents_lock held and interrupts disabled.
      - tick_check_preferred :  Prefer oneshot capable device, Use the higher rated one, but prefer a CPU local device with a lower rating than a non-CPU local device
      - tick_setup_device
        - [ ] tick_device_uses_broadcast
        - tick_setup_periodic
          - tick_set_periodic_handler
        - tick_setup_oneshot
          - `newdev->event_handler = handler;`

```c
/*
 * Set the periodic handler depending on broadcast on/off
 */
void tick_set_periodic_handler(struct clock_event_device *dev, int broadcast)
{
	if (!broadcast)
		dev->event_handler = tick_handle_periodic; // XXX this will be called when interrupt comes
	else
		dev->event_handler = tick_handle_periodic_broadcast;
}
```

- [ ] tick_set_periodic_handler
  - tick_periodic
    - do_timer
      - jiffies_64 += ticks;
      - calc_global_load : in the loadavg.c
  - update_process_times
    - **scheduler_tick**, **account_process_tick** , **irq_work_tick**, etc
    - run_local_timers
      - hrtimer_run_queues
        - hrtimer_is_hres_enabled : controlled by kernel cmdline parameter
        - tick_check_oneshot_change
          - tick_nohz_switch_to_nohz
        - hrtimer_switch_to_hres : nohz or hres
          - tick_setup_sched_timer
            - tick_sched_timer : called when hrtimer expires
      - raise_softirq

- tick_handle_periodic_broadcast
  - tick_do_periodic_broadcast : determine which CPUs to broadcast by mask
    - tick_do_broadcast : `struct tick_device`::`struct tick_device`::broadcast(mask);
      - lapic_timer_broadcast : as a example
        - `apic->send_IPI_mask(mask, LOCAL_TIMER_VECTOR);`
    


The orgin of interrupt:
```c
/*
 * Default timer interrupt handler for PIT/HPET
 */
static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	global_clock_event->event_handler(global_clock_event);
	return IRQ_HANDLED;
}

static void __init setup_default_timer_irq(void)
{
	unsigned long flags = IRQF_NOBALANCING | IRQF_IRQPOLL | IRQF_TIMER;

	/*
	 * Unconditionally register the legacy timer interrupt; even
	 * without legacy PIC/PIT we need this for the HPET0 in legacy
	 * replacement mode.
	 */
	if (request_irq(0, timer_interrupt, flags, "timer", NULL))
		pr_info("Failed to register legacy timer interrupt\n");
}
```

```c
enum tick_nohz_mode {
	NOHZ_MODE_INACTIVE,
	NOHZ_MODE_LOWRES,
	NOHZ_MODE_HIGHRES,
};
```
- [ ] nohz mode : lowres, highres

```c
DEFINE_PER_CPU(struct tick_device, tick_cpu_device);
```

> 2. 低精度timer + Dynamic Tick
系统开始的时候并不是直接进入Dynamic tick mode的，而是经历一个切换过程。
开始的时候，系统运行在**周期tick**的模式下，各个cpu对应的tick device的（clock_event_device的）event_handler是tick_handle_periodic。
在timer的软中断上下文中，会调用tick_check_oneshot_change进行是否切换到one shot模式的检查，如果系统中有支持one-shot的clock_event_device，并且没有配置高精度timer的话，那么就会发生tick mode的切换（调用tick_nohz_switch_to_nohz），
这时候，tick device会切换到one shot模式，而event handler被设置为tick_nohz_handler。由于这时候的clock_event_device工作在one shot模式，因此当系统正常运行的时候，在event_handler中每次都要reprogram clock event，以便正常产生tick。
**当cpu运行idle进程的时候，clock event device不再reprogram产生下次的tick信号，这样，整个系统的周期性的tick就停下来。**

> 3. 高精度timer + Dynamic Tick
同样的，系统开始的时候并不是直接进入Dynamic tick mode的，而是经历一个切换过程。
系统开始的时候是运行在周期tick的模式下，event handler是`tick_handle_periodic`。
在周期tick的软中断上下文中（参考`run_timer_softirq`），如果满足条件，会调用`hrtimer_switch_to_hres`将hrtimer从低精度模式切换到高精度模式上。这时候，系统会有下面的动作：

> 1. Tick device的clock event设备切换到oneshot mode（参考tick_init_highres函数）
> 2. Tick device的clock event设备的event handler会更新为hrtimer_interrupt（参考tick_init_highres函数）
> 3. 设定sched timer（也就是模拟周期tick那个高精度timer，参考tick_setup_sched_timer函数）

> 这样，当下一次tick到来的时候，系统会调用`hrtimer_interrupt`来处理这个tick（该tick是通过sched timer产生的）。

> 在Dynamic tick的模式下，各个cpu的tick device工作在one shot模式，该tick device对应的clock event设备也工作在one shot的模式，这时候，硬件Timer的中断不会周期性的产生，但是linux kernel中很多的模块是依赖于周期性的tick的，因此，在这种情况下，系统使用hrtime模拟了一个周期性的tick。在切换到dynamic tick模式的时候会初始化这个高精度timer，该高精度timer的回调函数是tick_sched_timer。这个函数执行的函数类似周期性tick中event handler执行的内容。不过在最后会reprogram该高精度timer，以便可以周期性的产生clock event。当系统进入idle的时候，就会stop这个高精度timer，这样，当没有用户事件的时候，CPU可以持续在idle状态，从而减少功耗。

- [ ]  感觉，当使用低精度 timer 的时候，利用的是周期 tick ,  当使用高精度的时候，用于实现 dynamic tick

#### [3](http://www.wowotech.net/timer_subsystem/timer_subsystem_userspace.html)

> @todo 各种用户接口，用于浏览整体代码的时候进行阅读


#### [4](http://www.wowotech.net/timer_subsystem/timekeeping.html)

> @todo 有点骚东西!


#### [5](http://www.wowotech.net/timer_subsystem/posix-clock.html)

> posix timer 的内容

#### [6](http://www.wowotech.net/timer_subsystem/posix-timer.html)

http://www.wowotech.net/timer_subsystem/time_subsystem_index.html
