# posix-timers.c

## TODO
1. Man
2. implementation


## Man


```c
       #include <time.h>

       int clock_getres(clockid_t clk_id, struct timespec *res);

       int clock_gettime(clockid_t clk_id, struct timespec *tp);

       int clock_settime(clockid_t clk_id, const struct timespec *tp);
```

       More clocks may be implemented.  The interpretation of the corresponding time values and the effect on timers is unspecified.

       Sufficiently recent versions of glibc and the Linux kernel support the following clocks:

       CLOCK_REALTIME
              System-wide clock that measures real (i.e., wall-clock) time.  Setting this clock requires appropriate privileges.  This clock is affected by discontinuous jumps in the system time (e.g., if the system administrator manually changes the clock), and by the incremental adjustments performed by adjtime(3) and NTP.

       CLOCK_REALTIME_COARSE (since Linux 2.6.32; Linux-specific)
              A faster but less precise version of CLOCK_REALTIME.  Use when you need very fast, but not fine-grained timestamps.  Requires per-architecture support, and probably also architecture support for this flag in the vdso(7).

       CLOCK_MONOTONIC
              Clock that cannot be set and represents monotonic time since—as described by POSIX—"some unspecified point in the past".  On Linux, that point corresponds to the number of seconds that the system has been running since it was booted.

              The CLOCK_MONOTONIC clock is not affected by discontinuous jumps in the system time (e.g., if the system administrator manually changes the clock), but is affected by the incremental adjustments performed by adjtime(3) and NTP.  This clock does not count time that the system is suspended.

       CLOCK_MONOTONIC_COARSE (since Linux 2.6.32; Linux-specific)
              A faster but less precise version of CLOCK_MONOTONIC.  Use when you need very fast, but not fine-grained timestamps.  Requires per-architecture support, and probably also architecture support for this flag in the vdso(7).

       CLOCK_MONOTONIC_RAW (since Linux 2.6.28; Linux-specific)
              Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments or the incremental adjustments performed by adjtime(3).  This clock does not count time that the system is suspended.

       CLOCK_BOOTTIME (since Linux 2.6.39; Linux-specific)
              Identical to CLOCK_MONOTONIC, except it also includes any time that the system is suspended.  This allows applications to get a suspend-aware monotonic clock without having to deal with the complications of CLOCK_REALTIME, which may have discontinuities if the time is changed using settimeofday(2) or similar.

       CLOCK_PROCESS_CPUTIME_ID (since Linux 2.6.12)
              Per-process CPU-time clock (measures CPU time consumed by all threads in the process).

       CLOCK_THREAD_CPUTIME_ID (since Linux 2.6.12)
              Thread-specific CPU-time clock.


   Historical note for SMP systems
       Before Linux added kernel support for CLOCK_PROCESS_CPUTIME_ID and CLOCK_THREAD_CPUTIME_ID, glibc implemented these clocks on many platforms using timer registers from the CPUs (TSC on i386, AR.ITC on Itanium).  These registers may differ between CPUs and as a consequence these clocks may return bogus results if a process is migrated to another CPU.

       If the CPUs in an SMP system have different clock sources, then there is no way to maintain a correlation between the timer registers since each CPU will run at a slightly different frequency.  If that is the case, then clock_getcpuclockid(0) will return ENOENT to signify this condition.  The two clocks will then be useful only if it can be ensured that a process stays on a certain CPU.

       The processors in an SMP system do not start all at exactly the same time and therefore the timer registers are typically running at an offset.  Some architectures include code that attempts to limit these offsets on bootup.  However, the code cannot guarantee to accurately tune the offsets.  Glibc contains no provisions to deal with these offsets (unlike the Linux Kernel).  Typically these offsets are small and therefore the effects may be negligible in most cases.

       Since glibc 2.4, the wrapper functions for the system calls described in this page avoid the abovementioned problems by employing the kernel implementation of CLOCK_PROCESS_CPUTIME_ID and CLOCK_THREAD_CPUTIME_ID, on systems that provide such an implementation (i.e., Linux 2.6.12 and later).

> @todo 看不懂


所以，其实 clock 和 timer 之间的关系是什么 ?

```c
       #include <signal.h>
       #include <time.h>

       int timer_create(clockid_t clockid, struct sigevent *sevp,
                        timer_t *timerid);


```

The timers created by timer_create() are commonly known as "POSIX (interval) timers".  The POSIX timers API consists of the following interfaces:

*  timer_create(): Create a timer.

*  timer_settime(2): Arm (start) or disarm (stop) a timer.

*  timer_gettime(2): Fetch the time remaining until the next expiration of a timer, along with the interval setting of the timer.

*  timer_getoverrun(2): Return the overrun count for the last timer expiration.

*  timer_delete(2): Disarm and delete a timer.

> Man timer_create() 的实例的效果非常好，所有的程序都分析很有道理。

> Man sigevent(7)


## clock_adjtime
> @todo 不知道为什么 clock_adjtime 是找不到对应的 manual 的

```c

int do_clock_adjtime(const clockid_t which_clock, struct __kernel_timex * ktx)
{
	const struct k_clock *kc = clockid_to_kclock(which_clock);

	if (!kc)
		return -EINVAL;
	if (!kc->clock_adj)
		return -EOPNOTSUPP;

	return kc->clock_adj(which_clock, ktx);
}

SYSCALL_DEFINE2(clock_adjtime, const clockid_t, which_clock,
		struct __kernel_timex __user *, utx)
{
	struct __kernel_timex ktx;
	int err;

	if (copy_from_user(&ktx, utx, sizeof(ktx)))
		return -EFAULT;

	err = do_clock_adjtime(which_clock, &ktx);

	if (err >= 0 && copy_to_user(utx, &ktx, sizeof(ktx)))
		return -EFAULT;

	return err;
}


SYSCALL_DEFINE2(clock_settime, const clockid_t, which_clock,
		const struct __kernel_timespec __user *, tp)
{
	const struct k_clock *kc = clockid_to_kclock(which_clock);
	struct timespec64 new_tp;

	if (!kc || !kc->clock_set)
		return -EINVAL;

	if (get_timespec64(&new_tp, tp))
		return -EFAULT;

	return kc->clock_set(which_clock, &new_tp);
}
```



## k_clock

```c
struct k_clock {
	int	(*clock_getres)(const clockid_t which_clock,
				struct timespec64 *tp);
	int	(*clock_set)(const clockid_t which_clock,
			     const struct timespec64 *tp);
	int	(*clock_get)(const clockid_t which_clock,
			     struct timespec64 *tp);
	int	(*clock_adj)(const clockid_t which_clock, struct __kernel_timex *tx);
	int	(*timer_create)(struct k_itimer *timer);
	int	(*nsleep)(const clockid_t which_clock, int flags,
			  const struct timespec64 *);
	int	(*timer_set)(struct k_itimer *timr, int flags,
			     struct itimerspec64 *new_setting,
			     struct itimerspec64 *old_setting);
	int	(*timer_del)(struct k_itimer *timr);
	void	(*timer_get)(struct k_itimer *timr,
			     struct itimerspec64 *cur_setting);
	void	(*timer_rearm)(struct k_itimer *timr);
	s64	(*timer_forward)(struct k_itimer *timr, ktime_t now);
	ktime_t	(*timer_remaining)(struct k_itimer *timr, ktime_t now);
	int	(*timer_try_to_cancel)(struct k_itimer *timr);
	void	(*timer_arm)(struct k_itimer *timr, ktime_t expires,
			     bool absolute, bool sigev_none);
	void	(*timer_wait_running)(struct k_itimer *timr);
};
```

> @todo 各种 k_clock 的分析是什么

1. 才发现，各种 syscall 实现就是 TM 的调用对应的 k_clock 函数
2. 当然具体实现还是有的


```c
static const struct k_clock clock_realtime = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get		= posix_clock_realtime_get,
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

static const struct k_clock clock_monotonic = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get		= posix_ktime_get_ts,
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

static const struct k_clock clock_monotonic_raw = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get		= posix_get_monotonic_raw,
};

static const struct k_clock clock_realtime_coarse = {
	.clock_getres		= posix_get_coarse_res,
	.clock_get		= posix_get_realtime_coarse,
};

static const struct k_clock clock_monotonic_coarse = {
	.clock_getres		= posix_get_coarse_res,
	.clock_get		= posix_get_monotonic_coarse,
};

static const struct k_clock clock_tai = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get		= posix_get_tai,
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

static const struct k_clock clock_boottime = {
	.clock_getres		= posix_get_hrtimer_res,
	.clock_get		= posix_get_boottime,
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

static const struct k_clock * const posix_clocks[] = { // 很好，这就是全部的秘密了
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

1. 为什么有的而 k_clock 不用注册所有的函数

## posix_timer_fn

```c
/*
 * This function gets called when a POSIX.1b interval timer expires.  It
 * is used as a callback from the kernel internal timer.  The
 * run_timer_list code ALWAYS calls with interrupts on.

 * This code is for CLOCK_REALTIME* and CLOCK_MONOTONIC* timers.
 */
static enum hrtimer_restart posix_timer_fn(struct hrtimer *timer)
```

> 当 interval timer expires 的时候触发

```c
/**
 * struct hrtimer - the basic hrtimer structure
 * @node:	timerqueue node, which also manages node.expires,
 *		the absolute expiry time in the hrtimers internal
 *		representation. The time is related to the clock on
 *		which the timer is based. Is setup by adding
 *		slack to the _softexpires value. For non range timers
 *		identical to _softexpires.
 * @_softexpires: the absolute earliest expiry time of the hrtimer.
 *		The time which was given as expiry time when the timer
 *		was armed.
 * @function:	timer expiry callback function
 * @base:	pointer to the timer base (per cpu and per clock)
 * @state:	state information (See bit values above)
 * @is_rel:	Set if the timer was armed relative
 * @is_soft:	Set if hrtimer will be expired in soft interrupt context.
 * @is_hard:	Set if hrtimer will be expired in hard interrupt context
 *		even on RT.
 *
 * The hrtimer structure must be initialized by hrtimer_init()
 */
struct hrtimer {
	struct timerqueue_node		node;
	ktime_t				_softexpires;
	enum hrtimer_restart		(*function)(struct hrtimer *);
	struct hrtimer_clock_base	*base;
	u8				state;
	u8				is_rel;
	u8				is_soft;
	u8				is_hard;
};
```
> 此函数被注册地方

```c
/**
 * struct k_itimer - POSIX.1b interval timer structure.
 * @list:		List head for binding the timer to signals->posix_timers
 * @t_hash:		Entry in the posix timer hash table
 * @it_lock:		Lock protecting the timer
 * @kclock:		Pointer to the k_clock struct handling this timer
 * @it_clock:		The posix timer clock id
 * @it_id:		The posix timer id for identifying the timer
 * @it_active:		Marker that timer is active
 * @it_overrun:		The overrun counter for pending signals
 * @it_overrun_last:	The overrun at the time of the last delivered signal
 * @it_requeue_pending:	Indicator that timer waits for being requeued on
 *			signal delivery
 * @it_sigev_notify:	The notify word of sigevent struct for signal delivery
 * @it_interval:	The interval for periodic timers
 * @it_signal:		Pointer to the creators signal struct
 * @it_pid:		The pid of the process/task targeted by the signal
 * @it_process:		The task to wakeup on clock_nanosleep (CPU timers)
 * @sigq:		Pointer to preallocated sigqueue
 * @it:			Union representing the various posix timer type
 *			internals.
 * @rcu:		RCU head for freeing the timer.
 */
struct k_itimer {
	struct list_head	list;
	struct hlist_node	t_hash;
	spinlock_t		it_lock;
	const struct k_clock	*kclock;
	clockid_t		it_clock;
	timer_t			it_id;
	int			it_active;
	s64			it_overrun;
	s64			it_overrun_last;
	int			it_requeue_pending;
	int			it_sigev_notify;
	ktime_t			it_interval;
	struct signal_struct	*it_signal;
	union {
		struct pid		*it_pid;
		struct task_struct	*it_process;
	};
	struct sigqueue		*sigq;
	union {
		struct {
			struct hrtimer	timer; // todo 虽然 k_itimer 持有了 k_itimer，但是并不知道是如何使用的这个的
		} real;
		struct cpu_timer	cpu;
		struct {
			struct alarm	alarmtimer;
		} alarm;
	} it; // 根据要求不同，k_itimer 将会使用不同种类的 timer  
	struct rcu_head		rcu;
};
```
