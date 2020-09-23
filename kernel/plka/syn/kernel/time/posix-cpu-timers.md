# posix-cpu-timers.c

> 暂时没有什么兴趣。

为了处理 两种情况，其中的内容:

       CLOCK_PROCESS_CPUTIME_ID (since Linux 2.6.12)
              Per-process CPU-time clock (measures CPU time consumed by all threads in the process).

       CLOCK_THREAD_CPUTIME_ID (since Linux 2.6.12)
              Thread-specific CPU-time clock.

```c
const struct k_clock clock_posix_cpu = {
	.clock_getres	= posix_cpu_clock_getres,
	.clock_set	= posix_cpu_clock_set,
	.clock_get	= posix_cpu_clock_get,
	.timer_create	= posix_cpu_timer_create,
	.nsleep		= posix_cpu_nsleep,
	.timer_set	= posix_cpu_timer_set,
	.timer_del	= posix_cpu_timer_del,
	.timer_get	= posix_cpu_timer_get,
	.timer_rearm	= posix_cpu_timer_rearm,
};

const struct k_clock clock_process = {
	.clock_getres	= process_cpu_clock_getres,
	.clock_get	= process_cpu_clock_get,
	.timer_create	= process_cpu_timer_create,
	.nsleep		= process_cpu_nsleep,
};

const struct k_clock clock_thread = {
	.clock_getres	= thread_cpu_clock_getres,
	.clock_get	= thread_cpu_clock_get,
	.timer_create	= thread_cpu_timer_create,
};
```

