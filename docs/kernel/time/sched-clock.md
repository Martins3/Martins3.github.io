## sched_clock

参考 https://www.kernel.org/doc/html/latest/timers/timekeeping.html

> As the name suggests, sched_clock() is used for scheduling the system,
> determining the absolute timeslice for a certain process in the CFS scheduler for example.
> It is also used for printk timestamps when you have selected to include time information in
> printk for things like bootcharts.

```c
notrace u64 sched_clock(void)
{
	u64 now;
	preempt_disable_notrace();
	now = sched_clock_noinstr();
	preempt_enable_notrace();
	return now;
}
```
sched_clock_noinstr 直接调用到
kvm_sched_clock_read 或者 native_sched_clock ，其实就是这 rdstsc 而已。

## 关于 cpu_clock 的问题

```c
/*
 * Similar to cpu_clock(), but requires local IRQs to be disabled.
 *
 * See cpu_clock().
 */
notrace u64 sched_clock_cpu(int cpu)
```
```c
static inline u64 cpu_clock(int cpu)
{
	return sched_clock_cpu(cpu);
}
```
### 注释看， cpu_clock 和 sched_clock_cpu 没有任何区别啊?
为什么一个需要 local IRQs disable ，一个不可以

### sched_clock_cpu 其实没有考虑 CPU 参数啊

```txt
[    0.622130] sched_clock: Marking stable (576000952, 45923638)->(655351796, -33427206)
```

sched_clock_cpu 中:
```c
	if (sched_clock_stable())
		return sched_clock() + __sched_clock_offset;
```
或者说，当一个程序切换到另外一个 CPU 之后，那么时间就开始切换一下。

- sysvec_call_function_single
  - instr_sysvec_call_function_single
    - __sysvec_call_function_single
      - generic_smp_call_function_single_interrupt
        - __flush_smp_call_function_queue
          - csd_do_func
            - sched_ttwu_pending
              - ttwu_do_activate
                - activate_task
                  - enqueue_task
                    - psi_enqueue
                      - psi_task_change
                        - psi_group_change
                          - cpu_clock

感觉就是介绍了半天，实际上啥也没有。
## TODO
windows 果然是有自己的想法的
- hv_setup_sched_clock

## 附录: sched/clock.c 的注释
```c
/*
 * sched_clock() for unstable CPU clocks

 * What this file implements:
 *
 * cpu_clock(i) provides a fast (execution time) high resolution
 * clock with bounded drift between CPUs. The value of cpu_clock(i)
 * is monotonic for constant i. The timestamp returned is in nanoseconds.
 *
 * ######################### BIG FAT WARNING ##########################
 * # when comparing cpu_clock(i) to cpu_clock(j) for i != j, time can #
 * # go backwards !!                                                  #
 * ####################################################################
 *
 * There is no strict promise about the base, although it tends to start
 * at 0 on boot (but people really shouldn't rely on that).
 *
 * cpu_clock(i)       -- can be used from any context, including NMI.
 * local_clock()      -- is cpu_clock() on the current CPU.
 *
 * sched_clock_cpu(i)
 *
 * How it is implemented:
 *
 * The implementation either uses sched_clock() when
 * !CONFIG_HAVE_UNSTABLE_SCHED_CLOCK, which means in that case the
 * sched_clock() is assumed to provide these properties (mostly it means
 * the architecture provides a globally synchronized highres time source).
 *
 * Otherwise it tries to create a semi stable clock from a mixture of other
 * clocks, including:
 *
 *  - GTOD (clock monotonic)
 *  - sched_clock()
 *  - explicit idle events
 *
 * We use GTOD as base and use sched_clock() deltas to improve resolution. The
 * deltas are filtered to provide monotonicity and keeping it within an
 * expected window.
 *
 * Furthermore, explicit sleep and wakeup hooks allow us to account for time
 * that is otherwise invisible (TSC gets stopped).
 *
 */
```
1. cpu_clock 用于封装 unstable cpu clocks 的

cpu_clock(i) provides a fast (execution time) high resolution
clock with bounded drift between CPUs. The value of cpu_clock(i)

## 等待整理
> 通用 sched clock 模块。_这个模块主要是提供一个 sched_clock 的接口函数_，调用该函数可以获取当前时间点到系统启动之间的纳秒值。
> 底层的 HW counter 其实是千差万别的，有些平台可以提供 64-bit 的 HW counter，因此，在那样的平台中，我们可以不使用这个通用 sched clock 模块（不配置 CONFIG_GENERIC_SCHED_CLOCK 这个内核选项），而在自己的 clock source chip driver 中直接提供 sched_clock 接口。
> 使用通用 sched clock 模块的好处是：该模块扩展了 64-bit 的 counter，即使底层的 HW counter 比特数目不足（有些平台 HW counter 只有 32 个 bit）。

CONFIG_GENERIC_SCHED_CLOCK is not set in x86 defconfig


## 这是为什么?
算加上   clocksource=tsc tsc=reliable， 重启后， guest sched clock 还是 kvmclock (edited)

sched_clock 作用到底是什么?

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
