# Linux Kernel Development : 

The passing of time is important to the kernel. A large number of kernel functions are time-driven, as opposed to event-driven


## 1 Kernel Notion of Time


## 2 The Tick Rate : HZ
> 这一个都是小学生内容

## 2.1 The Ideal Hz Value

## 2.2 Advantages with a Large Hz

## 2.3 Disadvantage with a Large Hz
> A Tickless OS
> 
> You might wonder whether an operating system even needs a fixed timer interrupt. Although
> that has been the norm for 40 years, with nearly all general-purpose operating systems employing a timer interrupt similar to the system described in this chapter, the Linux kernel supports an option known as a tickless operation. When a kernel is built with the CONFIG_HZ
> configuration option set, the system dynamically schedules the timer interrupt in accordance
> with pending timers. Instead of firing the timer interrupt every, say, 1ms, the interrupt is dynamically scheduled and rescheduled as needed. If the next timer is set to go off in 3ms,
> the timer interrupt fires in 3ms. After that, if there is no work for 50ms, the kernel reschedules the interrupt to go off in 50ms.
> 
> The reduction in overhead is welcome, but the real gain is in power savings, particular on an
> idle system. On a standard tick-based system, the kernel needs to service timer interrupts,
> even during idle periods. With a tickless system, moments of idleness are not interrupted by
> unnecessary time interrupts, reducing system power consumption. Whether the idle period
> is 200 milliseconds or 200 seconds, over time the gains add up to tangible power savings.

## 3 Jiffies

The global variable `jiffies` holds the number of ticks that have occurred since the system booted.

What actually happens is slightly more complicated:The kernel initializes `jiffies` to a special
initial value, causing the variable to overflow more often, catching bugs.When the actual
value of jiffies is sought, this “offset” is first subtracted.

## 3.1 Internal Representation of Jiffies

On 64-bit architectures, `jiffies_64` and `jiffies` refer to the same thing. Code can
either read jiffies or call `get_jiffies_64()` as both actions have the same effect.

## 3.2 Jiffies Wraparound

```c
/*
 *	These inlines deal with timer wrapping correctly. You are 
 *	strongly encouraged to use them
 *	1. Because people otherwise forget
 *	2. Because if the timer wrap changes in future you won't have to
 *	   alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)((b) - (a)) < 0))
#define time_before(a,b)	time_after(b,a)
```
> 需要利用这种 macro 实现时间的比较


## 3.3 User-Space and Hz
In kernels earlier than 2.6, changing the value of HZ resulted in user-space anomalies.This
happened because values were exported to user-space in units of ticks-per-second.As
these interfaces became permanent, applications grew to rely on a specific value of HZ.
Consequently, changing HZ would scale various exported values by some constant—without user-space knowing! Uptime would read 20 hours when it was in fact two!

To prevent such problems, the kernel needs to scale all exported jiffies values. It
does this by defining `USER_HZ`, which is the HZ value that user-space expects. 
> 有些 application 依赖于特定的 HZ 数值，所以，需要使用一些数值来处理

## 4 Hardware Clocks and Timers
Architectures provide two hardware devices to help with time keeping: the system timer,
which we have been discussing, and the real-time clock.The actual behavior and implementation of these devices varies between different machines, but the general purpose
and design is about the same for each.



## 4.1 Real-Time Clock
The real-time clock (RTC) provides a nonvolatile device for storing the system time.
The RTC continues to keep track of time even when the system is off by way of a small
battery typically included on the system board. On the PC architecture, the RTC and the
CMOS are integrated, and a single battery keeps the RTC running and the BIOS settings
preserved.
On boot, the kernel reads the RTC and uses it to initialize the wall time, which is
stored in the xtime variable.The kernel does not typically read the value again; however,
some supported architectures, such as x86, periodically save the current wall time back to
the RTC. Nonetheless, the real time clock’s primary importance is only during boot,
when the `xtime` variable is initialized.
> 初始化的时候实现确定 wall time



## 4.2 System Timer
The system timer serves a much more important (and frequent) role in the kernel’s timekeeping.The idea behind the system timer, regardless of architecture, is the same—to
provide a mechanism for driving an interrupt at a periodic rate. Some architectures
implement this via an electronic clock that oscillates at a programmable frequency. Other
systems provide a decrementer:A counter is set to some initial value and decrements at a
fixed rate until the counter reaches zero.When the counter reaches zero, an interrupt is
triggered. In any case, the effect is the same
> 实现 trigger interrupt 中断


## 5 The Time Interrupt Handler

The architecture-dependent routine is registered as the interrupt handler for the system timer and, thus, runs when the timer interrupt hits. Its exact job depends on the
given architecture, of course, but most handlers perform at least the following work:
- Obtain the `xtime_lock` lock, which protects access to `jiffies_64` and the wall time value, `xtime`.
- Acknowledge or reset the system timer as required.
- Periodically save the updated wall time to the real time clock.
- Call the architecture-independent timer routine, `tick_periodic()`

The architecture-independent routine, tick_periodic(), performs much more work:
- Increment the `jiffies_64` count by one. (This is safe, even on 32-bit architectures, because the xtime_lock lock was previously obtained.)
- Update resource usages, such as consumed system and user time, for the currently running process.
- Run any dynamic timers that have expired (discussed in the following section).
- Execute `scheduler_tick()`, as discussed in Chapter 4.
- Update the wall time, which is stored in xtime.
- Calculate the infamous load average.

> 调用的代码主要在


## 6 The Time of Day

```c
/**
 * ktime_get_real_ts64 - Returns the time of day in a timespec64.
 * @ts:		pointer to the timespec to be set
 *
 * Returns the time of day in a timespec64 (WARN if suspended).
 */
void ktime_get_real_ts64(struct timespec64 *ts)
{
	struct timekeeper *tk = &tk_core.timekeeper;
	unsigned int seq;
	u64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqcount_begin(&tk_core.seq);

		ts->tv_sec = tk->xtime_sec;
		nsecs = timekeeping_get_ns(&tk->tkr_mono);

	} while (read_seqcount_retry(&tk_core.seq, seq));

	ts->tv_nsec = 0;
	timespec64_add_ns(ts, nsecs);
}
EXPORT_SYMBOL(ktime_get_real_ts64);
```

## 7 Timers

#### 7.1 Using Timers
> 都是非常普通的内容，简简单单的

A potential race condition that must be guarded against exists when deleting timers.
When `del_timer()` returns, it guarantees only that the timer is no longer active (that is,
that it will not be executed in the future). On a multiprocessing machine, however, the
timer handler might already be executing on another processor.To deactivate the timer
and wait until a potentially executing handler for the timer exits, use del_timer_sync():
`del_timer_sync(&my_timer)`;


#### 7.2 Timer Race Condition

#### 7.2 Timer Implementation

update_process_times 调用:
```c
/*
 * Called by the local, per-CPU timer interrupt on SMP.
 */
void run_local_timers(void)
{
	struct timer_base *base = this_cpu_ptr(&timer_bases[BASE_STD]);

	hrtimer_run_queues();
	/* Raise the softirq only if required. */
	if (time_before(jiffies, base->clk)) {
		if (!IS_ENABLED(CONFIG_NO_HZ_COMMON))
			return;
		/* CPU is awake, so check the deferrable base. */
		base++;
		if (time_before(jiffies, base->clk))
			return;
	}
	raise_softirq(TIMER_SOFTIRQ);
}
```

Timers are stored in a linked list. However, it would be unwieldy for the kernel to either constantly traverse the entire list looking for expired timers, or keep the list sorted by
expiration value; the insertion and deletion of timers would then become expensive. Instead, the kernel partitions timers into five groups based on their expiration value.Timers
move down through the groups as their expiration time draws closer.The partitioning
ensures that, in most executions of the timer softirq, the kernel has to do little work to
find the expired timers. Consequently, the timer management code is efficient.
> 优秀啊，划分为几个组

## 8 Delaying Execution

> 暂时，还是不理解为什么有人需要 delay execution

#### 8.1 
```c
while (time_before(jiffies, timeout)) ;

while (time_before(jiffies, delay)) cond_resched();
```
> 其中还有一点 volatile 的问题


#### 8.2 Small Delays

```c
void udelay(unsigned long usecs)
void ndelay(unsigned long nsecs)
void mdelay(unsigned long msecs)
```

#### 8.3 scheduel_timeout()
A more optimal method of delaying execution is to use `schedule_timeout()`.This call
puts your task to sleep until at least the specified time has elapsed.There is no guarantee
that the sleep duration will be exactly the specified time—only that the duration is at least
as long as specified.When the specified time has elapsed, the kernel wakes the task up and
```c
/**
 * schedule_timeout - sleep until timeout
 * @timeout: timeout value in jiffies
 *
 * Make the current task sleep until @timeout jiffies have
 * elapsed. The routine will return immediately unless
 * the current task state has been set (see set_current_state()).
 *
 * You can set the task state as follows -
 *
 * %TASK_UNINTERRUPTIBLE - at least @timeout jiffies are guaranteed to
 * pass before the routine returns unless the current task is explicitly
 * woken up, (e.g. by wake_up_process())".
 *
 * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
 * delivered to the current task or the current task is explicitly woken
 * up.
 *
 * The current task state is guaranteed to be TASK_RUNNING when this
 * routine returns.
 *
 * Specifying a @timeout value of %MAX_SCHEDULE_TIMEOUT will schedule
 * the CPU away without a bound on the timeout. In this case the return
 * value will be %MAX_SCHEDULE_TIMEOUT.
 *
 * Returns 0 when the timer has expired otherwise the remaining time in
 * jiffies will be returned.  In all cases the return value is guaranteed
 * to be non-negative.
 */
signed long __sched schedule_timeout(signed long timeout)
{
	struct process_timer timer;
	unsigned long expire;

	switch (timeout)
	{
	case MAX_SCHEDULE_TIMEOUT:
		/*
		 * These two special cases are useful to be comfortable
		 * in the caller. Nothing more. We could take
		 * MAX_SCHEDULE_TIMEOUT from one of the negative value
		 * but I' d like to return a valid offset (>=0) to allow
		 * the caller to do everything it want with the retval.
		 */
		schedule();
		goto out;
	default:
		/*
		 * Another bit of PARANOID. Note that the retval will be
		 * 0 since no piece of kernel is supposed to do a check
		 * for a negative retval of schedule_timeout() (since it
		 * should never happens anyway). You just have the printk()
		 * that will tell you if something is gone wrong and where.
		 */
		if (timeout < 0) {
			printk(KERN_ERR "schedule_timeout: wrong timeout "
				"value %lx\n", timeout);
			dump_stack();
			current->state = TASK_RUNNING;
			goto out;
		}
	}

	expire = timeout + jiffies;

	timer.task = current;
	timer_setup_on_stack(&timer.timer, process_timeout, 0);
	__mod_timer(&timer.timer, expire, 0);
	schedule();
	del_singleshot_timer_sync(&timer.timer);

	/* Remove the timer from the object tracker */
	destroy_timer_on_stack(&timer.timer);

	timeout = expire - jiffies;

 out:
	return timeout < 0 ? 0 : timeout;
}
EXPORT_SYMBOL(schedule_timeout);
```
> @todo TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的说明可以分析一下原理

## Conclusion
