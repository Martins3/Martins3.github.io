## Clock Event Devices

Recall that clock event devices allow for registering an event that is going to happen at a defined point
of time in the future. In comparison to a full-blown timer implementation, however, only a single event
can be stored. The key elements of every `clock_event_device` are `set_next_event` because it allows for
setting the time at which the event is going to take place, and `event_handler`, which is called when the
event actually happens.

Recall that clock event devices allow for registering an event that is going to happen at a defined point
of time in the future. In comparison to a full-blown timer implementation, however, only a single event
can be stored. The key elements of every `clock_event_device` are `set_next_event` because it allows for
setting the time at which the event is going to take place, and `event_handler`, which is called when the
event actually happens.
```c
/**
 * struct clock_event_device - clock event device descriptor
 * @event_handler:	Assigned by the framework to be called by the low
 *			level handler of the event source
 * @set_next_event:	set next event function using a clocksource delta
 * @set_next_ktime:	set next event function using a direct ktime value
 * @next_event:		local storage for the next event in oneshot mode
 * @max_delta_ns:	maximum delta value in ns
 * @min_delta_ns:	minimum delta value in ns
 * @mult:		nanosecond to cycles multiplier
 * @shift:		nanoseconds to cycles divisor (power of two)
 * @state_use_accessors:current state of the device, assigned by the core code
 * @features:		features
 * @retries:		number of forced programming retries
 * @set_state_periodic:	switch state to periodic
 * @set_state_oneshot:	switch state to oneshot
 * @set_state_oneshot_stopped: switch state to oneshot_stopped
 * @set_state_shutdown:	switch state to shutdown
 * @tick_resume:	resume clkevt device
 * @broadcast:		function to broadcast events
 * @min_delta_ticks:	minimum delta value in ticks stored for reconfiguration
 * @max_delta_ticks:	maximum delta value in ticks stored for reconfiguration
 * @name:		ptr to clock event name
 * @rating:		variable to rate clock event devices
 * @irq:		IRQ number (only for non CPU local devices)
 * @bound_on:		Bound on CPU
 * @cpumask:		cpumask to indicate for which CPUs this device works
 * @list:		list head for the management code
 * @owner:		module reference
 */
struct clock_event_device {
	void			(*event_handler)(struct clock_event_device *);
	int			(*set_next_event)(unsigned long evt, struct clock_event_device *);
	int			(*set_next_ktime)(ktime_t expires, struct clock_event_device *);
	ktime_t			next_event;
	u64			max_delta_ns;
	u64			min_delta_ns;
	u32			mult;
	u32			shift;
	enum clock_event_state	state_use_accessors;
	unsigned int		features;
	unsigned long		retries;

	int			(*set_state_periodic)(struct clock_event_device *);
	int			(*set_state_oneshot)(struct clock_event_device *);
	int			(*set_state_oneshot_stopped)(struct clock_event_device *);
	int			(*set_state_shutdown)(struct clock_event_device *);
	int			(*tick_resume)(struct clock_event_device *);

	void			(*broadcast)(const struct cpumask *mask);
	void			(*suspend)(struct clock_event_device *);
	void			(*resume)(struct clock_event_device *);
	unsigned long		min_delta_ticks;
	unsigned long		max_delta_ticks;

	const char		*name;
	int			rating;
	int			irq;
	int			bound_on;
	const struct cpumask	*cpumask;
	struct list_head	list;
	struct module		*owner;
} ____cacheline_aligned;

/*
 * The local apic timer can be used for any function which is CPU local.
 */
static struct clock_event_device lapic_clockevent = { // arch 定义的 instance， 定义在 arch/x86/kernel/apic/apic.c 中间，类似还有好几个
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
```
❑ name is a human-readable representation for the event device. It shows up in /proc/timerlist.
❑ max_delta_ns and min_delta_ns specify the maximum or minimum, respectively, difference
between the current time and the time for the next event. Clocks work with individual frequencies at which device cycles occur, but the generic time subsystem expects a nanosecond value
when the event shall take place. The auxiliary function clockevent_delta2ns helps to convert
one representation into the other.
Consider, for instance, that the current time is 20, min_delta_ns is 2, and max_delta_ns is 40 (of
course, the exemplary values do not represent any situation possible in reality). Then the next
event can take place during the time interval [22, 60] where the boundaries are included.
❑ mult and shift are a multiplier and a divider, respectively, used to convert between clock cycles
and nanosecond values.
❑ The function pointed to by event_handler is called by the hardware interface code (which usually is architecture-specific) to pass clock events on to the generic layers.
❑ irq specifies the number of the IRQ that is used by the event device. Note that this is only
required for global devices. Per-CPU local devices use different hardware mechanisms to emit
signals and set irq to −1.
❑ cpumask specifies for which CPUs the event device works. A simple bitmask is employed for this
purpose. Local devices are usually only responsible for a single CPU.
❑ broadcast is required for the broadcasting implementation that provides a workaround for nonfunctional local APICs on IA-32 and AMD64 in power-saving mode. See Section 15.6 for more
details.
❑ rating allows — in analogy to the mechanism described for clock devices — comparison of
clock event devices by explicitly rating their accuracy.
❑ All instances of struct clock_event_device are kept on the global list clockevent_devices,
and list is the list head required for this purpose.
The auxiliary function clockevents_register_device is used to register a new clock event
device. This places the device on the global list.
❑ ktime_t stores the absolute time of the next event.

Each event device is characterized by several features stored as a bit string in features. A number of
constants in <clockchips.h> define possible features. For our purposes, two are of interest12:
❑ Clock event devices that support periodic events (i.e., events that are repeated over and over
again without the need to explicitly activate them by reprogramming the device) are identified
by CLOCK_EVT_FEAT_PERIODIC.
❑ CLOCK_EVT_FEAT_ONESHOT marks a clock capable of issuing one-shot events that happen exactly
once. Basically, this is the opposite of periodic events.

`set_mode` points to a function that allows for toggling the desired mode of operation between periodic
and one-shot mode. `mode` designates the current mode of operation. A clock can only be in either periodic
or one-shot mode at a time, but it can nevertheless provide the ability to work in both modes — actually,
most clocks allow both possibilities.
> set_mode 被替换为了，但是应该 periodic 和 one-shot mode 不可以重叠是确定的

> skip 本 section 的几行，我疯了

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
