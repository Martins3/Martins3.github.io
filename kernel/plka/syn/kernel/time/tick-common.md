# tick-common.md

1. 目前可以看懂的部分除了这个蛇皮部分，感觉其余的部分都是关于如何停止 tick 的 以及tick_check_new_device 之类用于切换新的 clock_event_device 的 check 函数

## tick_handle_periodic : 驱动整个系统的函数

```c
/*
 * Event handler for periodic ticks
 */
void tick_handle_periodic(struct clock_event_device *dev) // 这个东西曾经注册到 clock event device 中间了，所以整个 tick 机制就是基于 clock event device 的，那么 clock source 提供了什么东西
{
	int cpu = smp_processor_id();
	ktime_t next = dev->next_event;

	tick_periodic(cpu);

#if defined(CONFIG_HIGH_RES_TIMERS) || defined(CONFIG_NO_HZ_COMMON)
	/*
	 * The cpu might have transitioned to HIGHRES or NOHZ mode via
	 * update_process_times() -> run_local_timers() ->
	 * hrtimer_run_queues().
	 */
	if (dev->event_handler != tick_handle_periodic)
		return;
#endif

	if (!clockevent_state_oneshot(dev))
		return;
	for (;;) {
		/*
		 * Setup the next period for devices, which do not have
		 * periodic mode:
		 */
		next = ktime_add(next, tick_period);

		if (!clockevents_program_event(dev, next, false))
			return;
		/*
		 * Have to be careful here. If we're in oneshot mode,
		 * before we call tick_periodic() in a loop, we need
		 * to be sure we're using a real hardware clocksource.
		 * Otherwise we could get trapped in an infinite
		 * loop, as the tick_periodic() increments jiffies,
		 * which then will increment time, possibly causing
		 * the loop to trigger again and again.
		 */
		if (timekeeping_valid_for_hres())
			tick_periodic(cpu);
	}
}
```
1. tick_period 的唯一调用者，进而这就是完成整个系统的 tick


```c
/*
 * Periodic tick
 */
static void tick_periodic(int cpu)
{
	if (tick_do_timer_cpu == cpu) { // todo 有意思，多核，TMD 连时钟都不放过
		write_seqlock(&jiffies_lock);

		/* Keep track of the next tick event */
		tick_next_period = ktime_add(tick_next_period, tick_period);

		do_timer(1);
		write_sequnlock(&jiffies_lock);
		update_wall_time();
	}

	update_process_times(user_mode(get_irq_regs())); // todo 有意思
	profile_tick(CPU_PROFILING); // todo
}
```
> 显然，这是非常不科学的，那就是可能 thread 仅仅运行了其中部分时间。
