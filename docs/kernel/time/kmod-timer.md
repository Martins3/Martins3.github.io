![](./img/15-12.png)

```c
/**
 * struct hrtimer_clock_base - the timer base for a specific clock
 * @cpu_base:		per cpu clock base
 * @index:		clock type index for per_cpu support when moving a
 *			timer to a base on another cpu.
 * @clockid:		clock id for per_cpu support
 * @seq:		seqcount around __run_hrtimer
 * @running:		pointer to the currently running hrtimer
 * @active:		red black tree root node for the active timers
 * @get_time:		function to retrieve the current time of the clock
 * @offset:		offset of this clock to the monotonic base
 */
struct hrtimer_clock_base {
	struct hrtimer_cpu_base	*cpu_base;
	unsigned int		index;
	clockid_t		clockid;
	seqcount_t		seq;
	struct hrtimer		*running;
	struct timerqueue_head	active;
	ktime_t			(*get_time)(void);
	ktime_t			offset;
} __hrtimer_clock_base_align;
```
The meaning of the fields is as follows:
- hrtimer_cpu_base points to the per-CPU basis to which the clock base belongs.
- index distinguishes between CLOCK_MONOTONIC and CLOCK_REALTIME.
- rb_root is the root of a red-black tree on which all active timers are sorted.
- first points to the timer that will expire first.
- Processing high-res timers is initiated from the high-resolution timer softIRQ HRTIMER_SOFTIRQ
as described in the next section. softirq_time stores the time at which the softIRQ was issued,
and get_softirq_time is a function to obtain this time. If high-resolution mode is not active,
then the stored time will be coarse-grained.
- get_time reads the fine-grained time. This is simple for the monotonic clock (the value delivered
by the current clock source can be directly used), but some straightforward arithmetic is required
to convert the value into the real system time.
- resolution denotes the resolution of the timer in nanoseconds.
- When the real-time clock is adjusted, a discrepancy between the expiration values of timers
stored on the CLOCK_REALTIME clock base and the current real time will arise. The offset field
helps to fix the situation by denoting an offset by which the timers needs to be corrected. Since
this is only a temporary effect that happens only seldomly, the complications need not be discussed in more detail.
- reprogram is a function that allows for reprogramming a given timer event, that is, changing the
expiration time.

> hrtimer_cpu_base : rb tree 的根部

Two clock bases are established for each CPU using the following data structure:

```c
/**
 * struct hrtimer_cpu_base - the per cpu clock bases
 * @lock:		lock protecting the base and associated clock bases
 *			and timers
 * @cpu:		cpu number
 * @active_bases:	Bitfield to mark bases with active timers
 * @clock_was_set_seq:	Sequence counter of clock was set events
 * @hres_active:	State of high resolution mode
 * @in_hrtirq:		hrtimer_interrupt() is currently executing
 * @hang_detected:	The last hrtimer interrupt detected a hang
 * @softirq_activated:	displays, if the softirq is raised - update of softirq
 *			related settings is not required then.
 * @nr_events:		Total number of hrtimer interrupt events
 * @nr_retries:		Total number of hrtimer interrupt retries
 * @nr_hangs:		Total number of hrtimer interrupt hangs
 * @max_hang_time:	Maximum time spent in hrtimer_interrupt
 * @softirq_expiry_lock: Lock which is taken while softirq based hrtimer are
 *			 expired
 * @timer_waiters:	A hrtimer_cancel() invocation waits for the timer
 *			callback to finish.
 * @expires_next:	absolute time of the next event, is required for remote
 *			hrtimer enqueue; it is the total first expiry time (hard
 *			and soft hrtimer are taken into account)
 * @next_timer:		Pointer to the first expiring timer
 * @softirq_expires_next: Time to check, if soft queues needs also to be expired
 * @softirq_next_timer: Pointer to the first expiring softirq based timer
 * @clock_base:		array of clock bases for this cpu
 *
 * Note: next_timer is just an optimization for __remove_hrtimer().
 *	 Do not dereference the pointer because it is not reliable on
 *	 cross cpu removals.
 */
struct hrtimer_cpu_base {
	raw_spinlock_t			lock;
	unsigned int			cpu;
	unsigned int			active_bases;
	unsigned int			clock_was_set_seq;
	unsigned int			hres_active		: 1,
					in_hrtirq		: 1,
					hang_detected		: 1,
					softirq_activated       : 1;
#ifdef CONFIG_HIGH_RES_TIMERS
	unsigned int			nr_events;
	unsigned short			nr_retries;
	unsigned short			nr_hangs;
	unsigned int			max_hang_time;
#endif
#ifdef CONFIG_PREEMPT_RT
	spinlock_t			softirq_expiry_lock;
	atomic_t			timer_waiters;
#endif
	ktime_t				expires_next;
	struct hrtimer			*next_timer;
	ktime_t				softirq_expires_next;
	struct hrtimer			*softirq_next_timer;
	struct hrtimer_clock_base	clock_base[HRTIMER_MAX_CLOCK_BASES];
} ____cacheline_aligned;
```

- https://lwn.net/Articles/461592/

### soft hard mode
- https://lwn.net/Articles/461592/

```c
/*
 * Mode arguments of xxx_hrtimer functions:
 *
 * HRTIMER_MODE_ABS		- Time value is absolute
 * HRTIMER_MODE_REL		- Time value is relative to now
 * HRTIMER_MODE_PINNED		- Timer is bound to CPU (is only considered
 *				  when starting the timer)
 * HRTIMER_MODE_SOFT		- Timer callback function will be executed in
 *				  soft irq context
 * HRTIMER_MODE_HARD		- Timer callback function will be executed in
 *				  hard irq context even on PREEMPT_RT.
 */
enum hrtimer_mode {
	HRTIMER_MODE_ABS	= 0x00,
	HRTIMER_MODE_REL	= 0x01,
	HRTIMER_MODE_PINNED	= 0x02,
	HRTIMER_MODE_SOFT	= 0x04,
	HRTIMER_MODE_HARD	= 0x08,

	HRTIMER_MODE_ABS_PINNED = HRTIMER_MODE_ABS | HRTIMER_MODE_PINNED,
	HRTIMER_MODE_REL_PINNED = HRTIMER_MODE_REL | HRTIMER_MODE_PINNED,

	HRTIMER_MODE_ABS_SOFT	= HRTIMER_MODE_ABS | HRTIMER_MODE_SOFT,
	HRTIMER_MODE_REL_SOFT	= HRTIMER_MODE_REL | HRTIMER_MODE_SOFT,

	HRTIMER_MODE_ABS_PINNED_SOFT = HRTIMER_MODE_ABS_PINNED | HRTIMER_MODE_SOFT,
	HRTIMER_MODE_REL_PINNED_SOFT = HRTIMER_MODE_REL_PINNED | HRTIMER_MODE_SOFT,

	HRTIMER_MODE_ABS_HARD	= HRTIMER_MODE_ABS | HRTIMER_MODE_HARD,
	HRTIMER_MODE_REL_HARD	= HRTIMER_MODE_REL | HRTIMER_MODE_HARD,

	HRTIMER_MODE_ABS_PINNED_HARD = HRTIMER_MODE_ABS_PINNED | HRTIMER_MODE_HARD,
	HRTIMER_MODE_REL_PINNED_HARD = HRTIMER_MODE_REL_PINNED | HRTIMER_MODE_HARD,
};
```
- 这些模式都是什么意思 ?

```c
enum  hrtimer_base_type {
	HRTIMER_BASE_MONOTONIC,
	HRTIMER_BASE_REALTIME,
	HRTIMER_BASE_BOOTTIME,
	HRTIMER_BASE_TAI,
	HRTIMER_BASE_MONOTONIC_SOFT,
	HRTIMER_BASE_REALTIME_SOFT,
	HRTIMER_BASE_BOOTTIME_SOFT,
	HRTIMER_BASE_TAI_SOFT,
	HRTIMER_MAX_CLOCK_BASES,
};
```
- 这些 base type 又是什么意思

## hrtimer migratin
https://lwn.net/Articles/574379/


## hrtimer

- [ ] https://stackoverflow.com/questions/35800850/why-does-my-hrtimer-callback-return-too-early-after-forwarding-it

- common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_level_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - timer_interrupt
                  - tick_handle_periodic
                    - tick_periodic
                      - update_process_times
                        - run_local_timers
                          - hrtimer_run_queues
                            - __hrtimer_run_queues

- `__hrtimer_run_queues` 一共有三个调用位置:
  - [ ] softirq : 这个是为了替代原来的低精度的 timer 所以设计的吗，为了处理这个东西，搞了不少内容。
  - hrtimer_interrupt
  - hrtimer_run_queues : Called from run_local_timers in hardirq context every jiffy
- `local_apic_timer_interrupt` 会调用 `hrtimer_interrupt` ，而其注册位置为 `tick_init_highres`
- `hrtimer_reprogram` : 重新设置计时器的位置

- 因为内核是可以支持 hrtimer 来模拟的

## 低精度 timer 的工作原理

low-resolution timers are implemented on top of the high-resolution mechanism

```txt
[20484.715263]  dump_stack_lvl+0x80/0xa0
[20484.715369]  timer_fn+0x1a/0x30 [martins3]
[20484.715479]  call_timer_fn+0x27/0x120
[20484.715579]  __run_timer_base.part.0+0x20c/0x290
[20484.715704]  run_timer_softirq+0x73/0xc0
[20484.715800]  handle_softirqs+0x10b/0x3b0
[20484.715904]  __irq_exit_rcu+0xac/0xd0
[20484.715995]  irq_exit_rcu+0xe/0x20
[20484.716082]  sysvec_apic_timer_interrupt+0x3e/0x80
[20484.716205]  asm_sysvec_apic_timer_interrupt+0x1a/0x20
```
触发:

- update_process_times : 这个是 hrtimer 来触发的
  - run_local_timers

然后在 softirq 的流程中:
- run_timer_softirq
  - run_timer_base
    - __run_timer_base
      - `__run_timers`
        - expire_timers
          - call_timer_fn : 这里调用具体的函数

- [ ] 那么，这么说，update_process_times 可以保证其更新频率一定是 CONFIG_HZ 次，可以找找证据

## 问题
1. 主要是理解下 hrtimer_mode


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
