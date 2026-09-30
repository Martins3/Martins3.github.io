## Overview

The kernel distinguishes between two types of clocks:
1. A **global** clock is responsible to provide the periodic tick that is mainly used to update
the jiffies values. In former versions of the kernel, this type of clock was realized by
the programmable interrupt timer (PIT) on IA-32 systems, and on similar chips on other
architectures.
2. One **local** clock per CPU allows for performing process accounting, profiling, and last but
not least, high-resolution timers.

The role of the global clock is assumed by one specifically selected local clock. Note that high-resolution
timers only work on systems that provide per-CPU clock sources. The extensive communication required
between processors would otherwise degrade system performance too much as compared to the benefit
of having high-resolution timers
> 高精度必须 local, 不然片上互联网络就会耗尽时间

A system-global clock that still works at this power management state is then used to periodically activate signals that look as if they would originate from the
original clock sources. The workaround is known as the broadcasting mechanism; more about this follow
in Section 15.6.
> 高精度时钟在 power saving mode 时候是不可用的，此时信号来自于低精度的广播

## Clock Sources

First of all, consider how time values are acquired from the **various sources present** in a machine. The
kernel defines the abstraction of a clock source for this purpose:

```c
/**
 * struct clocksource - hardware abstraction for a free running counter
 *	Provides mostly state-free accessors to the underlying hardware.
 *	This is the structure used for system time.
 *
 * @name:		ptr to clocksource name
 * @list:		list head for registration
 * @rating:		rating value for selection (higher is better)
 *			To avoid rating inflation the following
 *			list should give you a guide as to how
 *			to assign your clocksource a rating
 *			1-99: Unfit for real use
 *				Only available for bootup and testing purposes.
 *			100-199: Base level usability.
 *				Functional for real use, but not desired.
 *			200-299: Good.
 *				A correct and usable clocksource.
 *			300-399: Desired.
 *				A reasonably fast and accurate clocksource.
 *			400-499: Perfect
 *				The ideal clocksource. A must-use where
 *				available.
 * @read:		returns a cycle value, passes clocksource as argument
 * @enable:		optional function to enable the clocksource
 * @disable:		optional function to disable the clocksource
 * @mask:		bitmask for two's complement
 *			subtraction of non 64 bit counters
 * @mult:		cycle to nanosecond multiplier
 * @shift:		cycle to nanosecond divisor (power of two)
 * @max_idle_ns:	max idle time permitted by the clocksource (nsecs)
 * @maxadj:		maximum adjustment value to mult (~11%)
 * @max_cycles:		maximum safe cycle value which won't overflow on multiplication
 * @flags:		flags describing special properties
 * @archdata:		arch-specific data
 * @suspend:		suspend function for the clocksource, if necessary
 * @resume:		resume function for the clocksource, if necessary
 * @mark_unstable:	Optional function to inform the clocksource driver that
 *			the watchdog marked the clocksource unstable
 * @owner:		module reference, must be set by clocksource in modules
 *
 * Note: This struct is not used in hotpathes of the timekeeping code
 * because the timekeeper caches the hot path fields in its own data
 * structure, so no line cache alignment is required,
 *
 * The pointer to the clocksource itself is handed to the read
 * callback. If you need extra information there you can wrap struct
 * clocksource into your own struct. Depending on the amount of
 * information you need you should consider to cache line align that
 * structure.
 */
struct clocksource {
	u64 (*read)(struct clocksource *cs);
	u64 mask;
	u32 mult;
	u32 shift;
	u64 max_idle_ns;
	u32 maxadj;
#ifdef CONFIG_ARCH_CLOCKSOURCE_DATA
	struct arch_clocksource_data archdata;
#endif
	u64 max_cycles;
	const char *name;
	struct list_head list;
	int rating;
	int (*enable)(struct clocksource *cs);
	void (*disable)(struct clocksource *cs);
	unsigned long flags;
	void (*suspend)(struct clocksource *cs);
	void (*resume)(struct clocksource *cs);
	void (*mark_unstable)(struct clocksource *cs);
	void (*tick_stable)(struct clocksource *cs);

	/* private: */
#ifdef CONFIG_CLOCKSOURCE_WATCHDOG
	/* Watchdog related data, used by the framework */
	struct list_head wd_list;
	u64 cs_last;
	u64 wd_last;
#endif
	struct module *owner;
};
```


Finally, the field `flags` of struct clocksource specifies — you will have guessed it — a number of flags.
Only one flag is relevant for our purposes. CLOCK_SOURCE_CONTINUOUS represents a continuous clock,
although the meaning is not quite the mathematical sense of of ‘‘continuous.’’ Instead, it describes that
the clock is free-running if set to 1 and thus cannot skip. If it is set to 0, then some cycles might be lost;
that is, if the last cycle value was n, then the next value does not necessarily need to be n + 1 even if it was
read at the next possible moment. A clock must exhibit this flag to be usable for high-resolution timers.

> 两个例子 : jiffies.c 中间创建的 和 x86 tsc :  time-stamp counter

```c
/*
 * The Jiffies based clocksource is the lowest common
 * denominator clock source which should function on
 * all systems. It has the same coarse resolution as
 * the timer interrupt frequency HZ and it suffers
 * inaccuracies caused by missed or lost timer
 * interrupts and the inability for the timer
 * interrupt hardware to accuratly tick at the
 * requested HZ value. It is also not recommended
 * for "tick-less" systems.
 */
static struct clocksource clocksource_jiffies = {
	.name		= "jiffies",
	.rating		= 1, /* lowest valid rating*/
	.read		= jiffies_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.mult		= TICK_NSEC << JIFFIES_SHIFT, /* details above */
	.shift		= JIFFIES_SHIFT,
	.max_cycles	= 10,
};

static u64 jiffies_read(struct clocksource *cs)
{
	return (u64) jiffies;
}


/*
 * Must mark VALID_FOR_HRES early such that when we unregister tsc_early
 * this one will immediately take over. We will only register if TSC has
 * been found good.
 */
static struct clocksource clocksource_tsc = {  // 当其中的时间
	.name                   = "tsc",
	.rating                 = 300,
	.read                   = read_tsc,
	.mask                   = CLOCKSOURCE_MASK(64),
	.flags                  = CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_VALID_FOR_HRES |
				  CLOCK_SOURCE_MUST_VERIFY,
	.archdata               = { .vclock_mode = VCLOCK_TSC },
	.resume			= tsc_resume,
	.mark_unstable		= tsc_cs_mark_unstable,
	.tick_stable		= tsc_cs_tick_stable,
	.list			= LIST_HEAD_INIT(clocksource_tsc.list),
};

static struct clocksource clocksource_hpet = {
	.name		= "hpet",
	.rating		= 250,
	.read		= read_hpet,
	.mask		= HPET_MASK,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.resume		= hpet_resume_counter,
};

/*
 * We used to compare the TSC to the cycle_last value in the clocksource
 * structure to avoid a nasty time-warp. This can be observed in a
 * very small window right after one CPU updated cycle_last under
 * xtime/vsyscall_gtod lock and the other CPU reads a TSC value which
 * is smaller than the cycle_last reference value due to a TSC which
 * is slighty behind. This delta is nowhere else observable, but in
 * that case it results in a forward time jump in the range of hours
 * due to the unsigned delta calculation of the time keeping core
 * code, which is necessary to support wrapping clocksources like pm
 * timer.
 *
 * This sanity check is now done in the core timekeeping code.
 * checking the result of read_tsc() - cycle_last for being negative.
 * That works because CLOCKSOURCE_MASK(64) does not mask out any bit.
 */
static u64 read_tsc(struct clocksource *cs)
{
	return (u64)rdtsc_ordered();
}

/**
 * rdtsc_ordered() - read the current TSC in program order
 *
 * rdtsc_ordered() returns the result of RDTSC as a 64-bit integer.
 * It is ordered like a load to a global in-memory counter.  It should
 * be impossible to observe non-monotonic rdtsc_unordered() behavior
 * across multiple CPUs as long as the TSC is synced.
 */
static __always_inline unsigned long long rdtsc_ordered(void)
{
	DECLARE_ARGS(val, low, high);

	/*
	 * The RDTSC instruction is not ordered relative to memory
	 * access.  The Intel SDM and the AMD APM are both vague on this
	 * point, but empirically an RDTSC instruction can be
	 * speculatively executed before prior loads.  An RDTSC
	 * immediately after an appropriate barrier appears to be
	 * ordered as a normal load, that is, it provides the same
	 * ordering guarantees as reading from a global memory location
	 * that some other imaginary CPU is updating continuously with a
	 * time stamp.
	 *
	 * Thus, use the preferred barrier on the respective CPU, aiming for
	 * RDTSCP as the default.
	 */
	asm volatile(ALTERNATIVE_2("rdtsc",
				   "lfence; rdtsc", X86_FEATURE_LFENCE_RDTSC,
				   "rdtscp", X86_FEATURE_RDTSCP)
			: EAX_EDX_RET(val, low, high)
			/* RDTSCP clobbers ECX with MSR_TSC_AUX. */
			:: "ecx");

	return EAX_EDX_VAL(val, low, high);
}
```
> clocksource::rate 各种数值说了半天，这个数值到底是什么含义 ?

> @todo 我想知道，到底 clocksource 对应的设备是什么 j8 玩意儿啊 ？

* ***Working with Clock Sources***

How can a clock be used? First of all, it must be registered with the kernel. The function
`clocksource_register` is responsible for this. The source is only added to the global `clocksource_list`
(defined in `kernel/time/clocksource.c`), which sorts all available clock sources by their rating.

`select_clocksource` is called to select the best clock source. Normally this will pick the
clock with the best rating, but it is also possible to specify a preference from userland via
`/sys/devices/system/clocksource/clocksource0/current_clocksource`, which is used by the kernel
instead. Two global variables are provided for this purpose:
1. `current_clocksource` points to the clock source that is currently the best one.
2. `next_clocksource` points to an instance of struct clocksource that is better than the one
used at the moment. The kernel automatically switches to the best clock source when a new
best clock source is registered.

To read the clock, the kernel provides the following functions:
1. `__get_realtime_clock_ts` takes a pointer to an instance of struct timespec as argument, reads
the current clock, converts the result, and stores in the timespec instance.
2. `getnstimeofday` is a front-end for `__get_realtime_clock_ts`, but also works if no highresolution clocks are available in the system.
In this case, getnstimeofday as defined in `kernel/time.c` (instead of `kernel/time/timekeeping.c`) is
used to provide a timespec that fulfills only low-resolution requirements.
> @todo holy shit ! 所以这两个函数的作用到底是什么 ?

## 简单的分析
- posix_get_monotonic_raw : 调用这个 hook
  - ktime_get_raw_ts64
    - timekeeping_get_ns : 然后再去选择正确的 resource

> [4 timekeeping](http://www.wowotech.net/timer_subsystem/timekeeping.html)
>
> timekeeping 模块维护 timeline 的基础是基于 clocksource 模块和 tick 模块。通过 tick 模块的 tick 事件，可以周期性的更新 time line，通过 clocksource 模块、可以获取 tick 之间更精准的时间信息

是的，就是会通过 kernel 来实现这个方向:
- sysvec_apic_timer_interrupt
  - instr_sysvec_apic_timer_interrupt
    - __sysvec_apic_timer_interrupt
      - local_apic_timer_interrupt
        - hrtimer_interrupt
          - __hrtimer_run_queues
            - __run_hrtimer
              - tick_nohz_handler
                - tick_sched_do_timer
                  - update_wall_time

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
