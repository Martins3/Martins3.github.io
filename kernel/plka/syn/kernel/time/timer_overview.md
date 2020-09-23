# kernel/time
| file                     | blank | comment | code | desc |
|--------------------------|-------|---------|------|------|
| timekeeping.c            | 359   | 720     | 1325 |
| hrtimer.c                | 314   | 723     | 1174 |
| timer.c                  | 249   | 779     | 1063 |
| posix-timers.c           | 215   | 242     | 913  |
| tick-sched.c             | 231   | 378     | 820  |
| posix-cpu-timers.c       | 194   | 405     | 817  |
| clocksource.c            | 164   | 381     | 685  |
| ntp.c                    | 170   | 236     | 641  |
| time.c                   | 129   | 251     | 620  |
| alarmtimer.c             | 136   | 209     | 569  |
| tick-broadcast.c         | 124   | 329     | 562  |
| clockevents.c            | 114   | 194     | 470  |
| timer_list.c             | 54    | 14      | 312  |
| tick-common.c            | 72    | 204     | 301  |
| itimer.c                 | 63    | 52      | 292  |
| posix-clock.c            | 72    | 9       | 236  |
| posix-stubs.c            | 33    | 13      | 194  |
| sched_clock.c            | 53    | 82      | 173  |
| tick-internal.h          | 19    | 10      | 138  |
| test_udelay.c            | 29    | 12      | 119  |
| vsyscall.c               | 25    | 26      | 79   |
| tick-oneshot.c           | 16    | 38      | 74   |
| jiffies.c                | 20    | 32      | 68   |
| timeconv.c               | 18    | 48      | 63   |
| tick-sched.h             | 11    | 31      | 59   |
| timecounter.c            | 16    | 29      | 54   |
| tick-broadcast-hrtimer.c | 8     | 50      | 53   |
| timekeeping_debug.c      | 9     | 7       | 39   |
| posix-timers.h           | 4     | 1       | 37   |
| timekeeping.h            | 5     | 4       | 23   |
| timekeeping_internal.h   | 4     | 8       | 22   |
| Makefile                 | 2     | 1       | 18   |
| ntp_internal.h           | 1     | 2       | 12   |

## quetion
1. 时钟中断的 handler 是什么
2. 提供那些服务 ?
    1. 时间
    2. 计时器
    3. tick

3. posix timer 是怎么回事 ?
4. 高精度 和 低精度 时钟的关系是什么 ?
    1. 各自对应什么位置的代码 ?
    2. 
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

## Documentation


#### [2](http://www.wowotech.net/timer_subsystem/time-subsyste-architecture.html)

1. 我们首先看周期性tick的实现。起始点一定是底层的clock source chip driver，该driver会调用注册clock event的接口函数（clockevents_config_and_register或者clockevents_register_device），
一旦增加了一个clock event device，需要通知上层的tick device layer，毕竟有可能新注册的这个device更好、更适合某个tick device呢（通过调用tick_check_new_device函数实现）。
要是这个clock event device被某个tick device收留了（要么该tick device之前没有匹配的clock event device，要么新的clock event device更适合该tick device），那么就启动对该tick device的配置（参考tick_setup_device）。

2. 根据当前系统的配置情况（周期性tick），会调用tick_setup_periodic函数，这时候，该tick device对应的clock_event_device的clock_event_handler被设置为tick_handle_periodic。
底层硬件会周期性的产生中断，从而会周期性的调用tick_handle_periodic从而驱动整个系统的运转。
需要注意的是：即便是配置了CONFIG_NO_HZ和CONFIG_TICK_ONESHOT，系统中没有提供one shot的clock event device，这种情况下，整个系统仍然是运行在周期tick的模式下。

> 总结下来就是 : clockevents 注册函数，然后 tick choose，setup

```c
/* kernel/time/clockevents.c */

/**
 * clockevents_config_and_register - Configure and register a clock event device
 * @dev:	device to register
 * @freq:	The clock frequency
 * @min_delta:	The minimum clock ticks to program in oneshot mode
 * @max_delta:	The maximum clock ticks to program in oneshot mode
 *
 * min/max_delta can be 0 for devices which do not support oneshot mode.
 */
void clockevents_config_and_register(struct clock_event_device *dev,
				     u32 freq, unsigned long min_delta,
				     unsigned long max_delta)
{
	dev->min_delta_ticks = min_delta;
	dev->max_delta_ticks = max_delta;
	clockevents_config(dev, freq);
	clockevents_register_device(dev);
}

/**
 * clockevents_register_device - register a clock event device
 * @dev:	device to register
 */
void clockevents_register_device(struct clock_event_device *dev)
{
	unsigned long flags;

	/* Initialize state to DETACHED */
	clockevent_set_state(dev, CLOCK_EVT_STATE_DETACHED);

	if (!dev->cpumask) {
		WARN_ON(num_possible_cpus() > 1);
		dev->cpumask = cpumask_of(smp_processor_id());
	}

	if (dev->cpumask == cpu_all_mask) {
		WARN(1, "%s cpumask == cpu_all_mask, using cpu_possible_mask instead\n",
		     dev->name);
		dev->cpumask = cpu_possible_mask;
	}

	raw_spin_lock_irqsave(&clockevents_lock, flags);

	list_add(&dev->list, &clockevent_devices);
	tick_check_new_device(dev);
	clockevents_notify_released();

	raw_spin_unlock_irqrestore(&clockevents_lock, flags);
}

/*
 * Check, if the new registered device should be used. Called with
 * clockevents_lock held and interrupts disabled.
 */
void tick_check_new_device(struct clock_event_device *newdev)
{
	struct clock_event_device *curdev;
	struct tick_device *td;
	int cpu;

	cpu = smp_processor_id();
	td = &per_cpu(tick_cpu_device, cpu);
	curdev = td->evtdev;

	/* cpu local device ? */
	if (!tick_check_percpu(curdev, newdev, cpu))
		goto out_bc;

	/* Preference decision */
	if (!tick_check_preferred(curdev, newdev))
		goto out_bc;

	if (!try_module_get(newdev->owner))
		return;

	/*
	 * Replace the eventually existing device by the new
	 * device. If the current device is the broadcast device, do
	 * not give it back to the clockevents layer !
	 */
	if (tick_is_broadcast_device(curdev)) {
		clockevents_shutdown(curdev);
		curdev = NULL;
	}
	clockevents_exchange_device(curdev, newdev);
	tick_setup_device(td, newdev, cpu, cpumask_of(cpu));
	if (newdev->features & CLOCK_EVT_FEAT_ONESHOT)
		tick_oneshot_notify();
	return;

out_bc:
	/*
	 * Can the new device be used as a broadcast device ?
	 */
	tick_install_broadcast_device(newdev);
}


/*
 * Setup the tick device
 */
static void tick_setup_device(struct tick_device *td,
			      struct clock_event_device *newdev, int cpu,
			      const struct cpumask *cpumask)
{
	void (*handler)(struct clock_event_device *) = NULL;
	ktime_t next_event = 0;

	/*
	 * First device setup ?
	 */
	if (!td->evtdev) {
		/*
		 * If no cpu took the do_timer update, assign it to
		 * this cpu:
		 */
		if (tick_do_timer_cpu == TICK_DO_TIMER_BOOT) {
			tick_do_timer_cpu = cpu;

			tick_next_period = ktime_get();
			tick_period = NSEC_PER_SEC / HZ;
#ifdef CONFIG_NO_HZ_FULL
			/*
			 * The boot CPU may be nohz_full, in which case set
			 * tick_do_timer_boot_cpu so the first housekeeping
			 * secondary that comes up will take do_timer from
			 * us.
			 */
			if (tick_nohz_full_cpu(cpu))
				tick_do_timer_boot_cpu = cpu;

		} else if (tick_do_timer_boot_cpu != -1 &&
						!tick_nohz_full_cpu(cpu)) {
			tick_take_do_timer_from_boot();
			tick_do_timer_boot_cpu = -1;
			WARN_ON(tick_do_timer_cpu != cpu);
#endif
		}

		/*
		 * Startup in periodic mode first.
		 */
		td->mode = TICKDEV_MODE_PERIODIC;
	} else {
		handler = td->evtdev->event_handler;
		next_event = td->evtdev->next_event;
		td->evtdev->event_handler = clockevents_handle_noop;
	}

	td->evtdev = newdev;

	/*
	 * When the device is not per cpu, pin the interrupt to the
	 * current cpu:
	 */
	if (!cpumask_equal(newdev->cpumask, cpumask))
		irq_set_affinity(newdev->irq, cpumask);

	/*
	 * When global broadcasting is active, check if the current
	 * device is registered as a placeholder for broadcast mode.
	 * This allows us to handle this x86 misfeature in a generic
	 * way. This function also returns !=0 when we keep the
	 * current active broadcast state for this CPU.
	 */
	if (tick_device_uses_broadcast(newdev, cpu))
		return;

	if (td->mode == TICKDEV_MODE_PERIODIC)
		tick_setup_periodic(newdev, 0);
	else
		tick_setup_oneshot(newdev, handler, next_event);
}


/*
 * Setup the device for a periodic tick
 */
void tick_setup_periodic(struct clock_event_device *dev, int broadcast)
{
	tick_set_periodic_handler(dev, broadcast); // 设置 handler 的

	/* Broadcast setup ? */
	if (!tick_device_is_functional(dev))
		return;

	if ((dev->features & CLOCK_EVT_FEAT_PERIODIC) &&
	    !tick_broadcast_oneshot_active()) {
		clockevents_switch_state(dev, CLOCK_EVT_STATE_PERIODIC);
	} else {
		unsigned int seq;
		ktime_t next;

		do {
			seq = read_seqbegin(&jiffies_lock);
			next = tick_next_period;
		} while (read_seqretry(&jiffies_lock, seq));

		clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT);

		for (;;) {
			if (!clockevents_program_event(dev, next, false))
				return;
			next = ktime_add(next, tick_period);
		}
	}
}


/*
 * Set the periodic handler depending on broadcast on/off
 */
void tick_set_periodic_handler(struct clock_event_device *dev, int broadcast)
{
	if (!broadcast)
		dev->event_handler = tick_handle_periodic;
	else
		dev->event_handler = tick_handle_periodic_broadcast;
}

/**
 * tick_resume_onshot - resume oneshot mode
 */
void tick_resume_oneshot(void)
{
	struct clock_event_device *dev = __this_cpu_read(tick_cpu_device.evtdev);

	clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT);
	clockevents_program_event(dev, ktime_get(), true);
}
```

> 对应的这么多函数中间，到底需要采用 ? 哪一个是通知硬件的 ?
> tick 的 handler 被谁调用，然后又是被谁使用的 ?




```c
/**
 * clockevents_switch_state - set the operating state of a clock event device
 * @dev:	device to modify
 * @state:	new state
 *
 * Must be called with interrupts disabled !
 */
void clockevents_switch_state(struct clock_event_device *dev,
			      enum clock_event_state state)
{
	if (clockevent_get_state(dev) != state) {
		if (__clockevents_switch_state(dev, state))
			return;

		clockevent_set_state(dev, state);

		/*
		 * A nsec2cyc multiplicator of 0 is invalid and we'd crash
		 * on it, so fix it up and emit a warning:
		 */
		if (clockevent_state_oneshot(dev)) {
			if (WARN_ON(!dev->mult))
				dev->mult = 1;
		}
	}
}

static int __clockevents_switch_state(struct clock_event_device *dev,
				      enum clock_event_state state)
{
	if (dev->features & CLOCK_EVT_FEAT_DUMMY)
		return 0;

	/* Transition with new state-specific callbacks */
	switch (state) {
	case CLOCK_EVT_STATE_DETACHED:
		/* The clockevent device is getting replaced. Shut it down. */

	case CLOCK_EVT_STATE_SHUTDOWN:
		if (dev->set_state_shutdown)
			return dev->set_state_shutdown(dev);
		return 0;

	case CLOCK_EVT_STATE_PERIODIC:
		/* Core internal bug */
		if (!(dev->features & CLOCK_EVT_FEAT_PERIODIC))
			return -ENOSYS;
		if (dev->set_state_periodic)
			return dev->set_state_periodic(dev);
		return 0;

	case CLOCK_EVT_STATE_ONESHOT:
		/* Core internal bug */
		if (!(dev->features & CLOCK_EVT_FEAT_ONESHOT))
			return -ENOSYS;
		if (dev->set_state_oneshot)
			return dev->set_state_oneshot(dev);
		return 0;

	case CLOCK_EVT_STATE_ONESHOT_STOPPED:
		/* Core internal bug */
		if (WARN_ONCE(!clockevent_state_oneshot(dev),
			      "Current state: %d\n",
			      clockevent_get_state(dev)))
			return -EINVAL;

		if (dev->set_state_oneshot_stopped)
			return dev->set_state_oneshot_stopped(dev);
		else
			return -ENOSYS;

	default:
		return -ENOSYS;
	}
}


/* 进入到 arch/x86/kernel/apic/apic.c 了 */
// 好吧，就是向 apic 控制器写入寄存器之类的操作 !

/*
 * The local apic timer can be used for any function which is CPU local.
 */
static struct clock_event_device lapic_clockevent = {
	.name				= "lapic",
	.features			= CLOCK_EVT_FEAT_PERIODIC |
					  CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_C3STOP
					  | CLOCK_EVT_FEAT_DUMMY,
	.shift				= 32,
	.set_state_shutdown		= lapic_timer_shutdown,
	.set_state_periodic		= lapic_timer_set_periodic,
	.set_state_oneshot		= lapic_timer_set_oneshot,
	.set_state_oneshot_stopped	= lapic_timer_shutdown,
	.set_next_event			= lapic_next_event,
	.broadcast			= lapic_timer_broadcast,
	.rating				= 100,
	.irq				= -1,
};


static int lapic_timer_set_oneshot(struct clock_event_device *evt)
{
	return lapic_timer_set_periodic_oneshot(evt, true);
}

static inline int
lapic_timer_set_periodic_oneshot(struct clock_event_device *evt, bool oneshot)
{
	/* Lapic used as dummy for broadcast ? */
	if (evt->features & CLOCK_EVT_FEAT_DUMMY)
		return 0;

	__setup_APIC_LVTT(lapic_timer_period, oneshot, 1);
	return 0;
}
```

> @todo clockevent 和 clocksource 使用的是同一个 dev ?


https://en.wikipedia.org/wiki/High_Precision_Event_Timer

> 看上去，clocksource 和 clockevent 都是依赖于底层的函数。

高精度timer总是会被编入最后的kernel中。在这种构架下，各个内核模块也可以调用linux kernel中的高精度timer模块的接口函数来实现高精度timer，但是，这时候高精度timer模块是运行在低精度的模式，也就是说这些hrtimer虽然是按照高精度timer的红黑树进行组织，
**但是系统只是在每一周期性tick到来的时候调用`hrtimer_run_queues`函数，来检查是否有expire的hrtimer。毫无疑问，这里的高精度timer也就是没有意义了。**

2、低精度timer + Dynamic Tick

系统开始的时候并不是直接进入Dynamic tick mode的，而是经历一个切换过程。

开始的时候，系统运行在**周期tick**的模式下，各个cpu对应的tick device的（clock_event_device的）event_handler是tick_handle_periodic。
在timer的软中断上下文中，会调用tick_check_oneshot_change进行是否切换到one shot模式的检查，如果系统中有支持one-shot的clock_event_device，并且没有配置高精度timer的话，那么就会发生tick mode的切换（调用tick_nohz_switch_to_nohz），
这时候，tick device会切换到one shot模式，而event handler被设置为tick_nohz_handler。由于这时候的clock_event_device工作在one shot模式，因此当系统正常运行的时候，在event_handler中每次都要reprogram clock event，以便正常产生tick。
**当cpu运行idle进程的时候，clock event device不再reprogram产生下次的tick信号，这样，整个系统的周期性的tick就停下来。**
> 似乎是需要硬件的 one shot 模式支持在没有周期 tick 的情况下，实现 timer ?

3、高精度timer + Dynamic Tick
同样的，系统开始的时候并不是直接进入Dynamic tick mode的，而是经历一个切换过程。
系统开始的时候是运行在周期tick的模式下，event handler是`tick_handle_periodic`。
在周期tick的软中断上下文中（参考`run_timer_softirq`），如果满足条件，会调用`hrtimer_switch_to_hres`将hrtimer从低精度模式切换到高精度模式上。这时候，系统会有下面的动作：

1. Tick device的clock event设备切换到oneshot mode（参考tick_init_highres函数）
2. Tick device的clock event设备的event handler会更新为hrtimer_interrupt（参考tick_init_highres函数）
3. 设定sched timer（也就是模拟周期tick那个高精度timer，参考tick_setup_sched_timer函数）

这样，当下一次tick到来的时候，系统会调用`hrtimer_interrupt`来处理这个tick（该tick是通过sched timer产生的）。

在Dynamic tick的模式下，各个cpu的tick device工作在one shot模式，该tick device对应的clock event设备也工作在one shot的模式，这时候，硬件Timer的中断不会周期性的产生，但是linux kernel中很多的模块是依赖于周期性的tick的，因此，在这种情况下，系统使用hrtime模拟了一个周期性的tick。在切换到dynamic tick模式的时候会初始化这个高精度timer，该高精度timer的回调函数是tick_sched_timer。这个函数执行的函数类似周期性tick中event handler执行的内容。不过在最后会reprogram该高精度timer，以便可以周期性的产生clock event。当系统进入idle的时候，就会stop这个高精度timer，这样，当没有用户事件的时候，CPU可以持续在idle状态，从而减少功耗。

> 感觉，当使用低精度 timer 的时候，利用的是周期 tick ,  当使用高精度的时候，用于实现 dynamic tick

#### [3](http://www.wowotech.net/timer_subsystem/timer_subsystem_userspace.html)

> @todo 各种用户接口，用于浏览整体代码的时候进行阅读


#### [4](http://www.wowotech.net/timer_subsystem/timekeeping.html)

> @todo 有点骚东西!


#### [5](http://www.wowotech.net/timer_subsystem/posix-clock.html)

> posix timer 的内容

#### [6](http://www.wowotech.net/timer_subsystem/posix-timer.html)

http://www.wowotech.net/timer_subsystem/time_subsystem_index.html


http://btorpey.github.io/blog/2014/02/18/clock-sources-in-linux/
