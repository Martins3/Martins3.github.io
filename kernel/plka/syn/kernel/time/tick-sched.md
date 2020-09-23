# tick-sched.c

## Principal 
1. No idle tick implementation for low and high resolution timers

## quetions
1. 什么时候 start ，end


## Todo
1. 构建call graph

## API : tick_nohz_get_sleep_length
1. 提供给 cpuidle 驱动使用
```c
/**
 * tick_nohz_get_sleep_length - return the expected length of the current sleep
 * @delta_next: duration until the next event if the tick cannot be stopped
 *
 * Called from power state control code with interrupts disabled
 */
ktime_t tick_nohz_get_sleep_length(ktime_t *delta_next)
{
	struct clock_event_device *dev = __this_cpu_read(tick_cpu_device.evtdev);
	struct tick_sched *ts = this_cpu_ptr(&tick_cpu_sched);
	int cpu = smp_processor_id();
	/*
	 * The idle entry time is expected to be a sufficient approximation of
	 * the current time at this point.
	 */
	ktime_t now = ts->idle_entrytime;
	ktime_t next_event;

	WARN_ON_ONCE(!ts->inidle);

	*delta_next = ktime_sub(dev->next_event, now);

	if (!can_stop_idle_tick(cpu, ts)) // todo 
		return *delta_next;

	next_event = tick_nohz_next_event(ts, cpu); // todo 
	if (!next_event)
		return *delta_next;

	/*
	 * If the next highres timer to expire is earlier than next_event, the
	 * idle governor needs to know that.
	 */
	next_event = min_t(u64, next_event,
			   hrtimer_next_event_without(&ts->sched_timer));

	return ktime_sub(next_event, now);
}
```

## API : tick_nohz_idle_stop_tick
sched/idle.c 中间使用

```c
/**
 * tick_nohz_idle_stop_tick - stop the idle tick from the idle task
 *
 * When the next event is more than a tick into the future, stop the idle tick
 */
void tick_nohz_idle_stop_tick(void)
{
	__tick_nohz_idle_stop_tick(this_cpu_ptr(&tick_cpu_sched));
}


static void tick_nohz_stop_tick(struct tick_sched *ts, int cpu)
{
	struct clock_event_device *dev = __this_cpu_read(tick_cpu_device.evtdev);
	u64 basemono = ts->timer_expires_base;
	u64 expires = ts->timer_expires;
	ktime_t tick = expires;

	/* Make sure we won't be trying to stop it twice in a row. */
	ts->timer_expires_base = 0;

	/*
	 * If this CPU is the one which updates jiffies, then give up
	 * the assignment and let it be taken by the CPU which runs
	 * the tick timer next, which might be this CPU as well. If we
	 * don't drop this here the jiffies might be stale and
	 * do_timer() never invoked. Keep track of the fact that it
	 * was the one which had the do_timer() duty last.
	 */
	if (cpu == tick_do_timer_cpu) {
		tick_do_timer_cpu = TICK_DO_TIMER_NONE;
		ts->do_timer_last = 1;
	} else if (tick_do_timer_cpu != TICK_DO_TIMER_NONE) {
		ts->do_timer_last = 0;
	}

	/* Skip reprogram of event if its not changed */
	if (ts->tick_stopped && (expires == ts->next_tick)) {
		/* Sanity check: make sure clockevent is actually programmed */
		if (tick == KTIME_MAX || ts->next_tick == hrtimer_get_expires(&ts->sched_timer))
			return;

		WARN_ON_ONCE(1);
		printk_once("basemono: %llu ts->next_tick: %llu dev->next_event: %llu timer->active: %d timer->expires: %llu\n",
			    basemono, ts->next_tick, dev->next_event,
			    hrtimer_active(&ts->sched_timer), hrtimer_get_expires(&ts->sched_timer));
	}

	/*
	 * nohz_stop_sched_tick can be called several times before
	 * the nohz_restart_sched_tick is called. This happens when
	 * interrupts arrive which do not cause a reschedule. In the
	 * first call we save the current tick time, so we can restart
	 * the scheduler tick in nohz_restart_sched_tick.
	 */
	if (!ts->tick_stopped) {
		calc_load_nohz_start();
		quiet_vmstat();

		ts->last_tick = hrtimer_get_expires(&ts->sched_timer);
		ts->tick_stopped = 1;
		trace_tick_stop(1, TICK_DEP_MASK_NONE);
	}

	ts->next_tick = tick;

	/*
	 * If the expiration time == KTIME_MAX, then we simply stop
	 * the tick timer.
	 */
	if (unlikely(expires == KTIME_MAX)) {
		if (ts->nohz_mode == NOHZ_MODE_HIGHRES)
			hrtimer_cancel(&ts->sched_timer);
		return;
	}

	if (ts->nohz_mode == NOHZ_MODE_HIGHRES) {
		hrtimer_start(&ts->sched_timer, tick,
			      HRTIMER_MODE_ABS_PINNED_HARD);
	} else {
		hrtimer_set_expires(&ts->sched_timer, tick);
		tick_program_event(tick, 1);
	}
}
```


## core struct

```c
/**
 * struct tick_sched - sched tick emulation and no idle tick control/stats
 * @sched_timer:	hrtimer to schedule the periodic tick in high
 *			resolution mode
 * @check_clocks:	Notification mechanism about clocksource changes
 * @nohz_mode:		Mode - one state of tick_nohz_mode
 * @inidle:		Indicator that the CPU is in the tick idle mode
 * @tick_stopped:	Indicator that the idle tick has been stopped
 * @idle_active:	Indicator that the CPU is actively in the tick idle mode;
 *			it is resetted during irq handling phases.
 * @do_timer_lst:	CPU was the last one doing do_timer before going idle
 * @got_idle_tick:	Tick timer function has run with @inidle set
 * @last_tick:		Store the last tick expiry time when the tick
 *			timer is modified for nohz sleeps. This is necessary
 *			to resume the tick timer operation in the timeline
 *			when the CPU returns from nohz sleep.
 * @next_tick:		Next tick to be fired when in dynticks mode.
 * @idle_jiffies:	jiffies at the entry to idle for idle time accounting
 * @idle_calls:		Total number of idle calls
 * @idle_sleeps:	Number of idle calls, where the sched tick was stopped
 * @idle_entrytime:	Time when the idle call was entered
 * @idle_waketime:	Time when the idle was interrupted
 * @idle_exittime:	Time when the idle state was left
 * @idle_sleeptime:	Sum of the time slept in idle with sched tick stopped
 * @iowait_sleeptime:	Sum of the time slept in idle with sched tick stopped, with IO outstanding
 * @timer_expires:	Anticipated timer expiration time (in case sched tick is stopped)
 * @timer_expires_base:	Base time clock monotonic for @timer_expires
 * @next_timer:		Expiry time of next expiring timer for debugging purpose only
 * @tick_dep_mask:	Tick dependency mask - is set, if someone needs the tick
 */
struct tick_sched {
	struct hrtimer			sched_timer; // todo 几乎所有的
	unsigned long			check_clocks;
	enum tick_nohz_mode		nohz_mode;

	unsigned int			inidle		: 1;
	unsigned int			tick_stopped	: 1;
	unsigned int			idle_active	: 1;
	unsigned int			do_timer_last	: 1;
	unsigned int			got_idle_tick	: 1;

	ktime_t				last_tick;
	ktime_t				next_tick;
	unsigned long			idle_jiffies;
	unsigned long			idle_calls;
	unsigned long			idle_sleeps;
	ktime_t				idle_entrytime;
	ktime_t				idle_waketime;
	ktime_t				idle_exittime;
	ktime_t				idle_sleeptime;
	ktime_t				iowait_sleeptime;
	unsigned long			last_jiffies;
	u64				timer_expires;
	u64				timer_expires_base;
	u64				next_timer;
	ktime_t				idle_expires;
	atomic_t			tick_dep_mask;
};
```

1. 各种全局变量以及 hrtimer 中间的内容是什么 ?
2. 
