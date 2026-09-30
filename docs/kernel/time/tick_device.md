## struct tick_device


## 切换模式

- sysvec_apic_timer_interrupt
  - instr_sysvec_apic_timer_interrupt
    - __sysvec_apic_timer_interrupt
      - local_apic_timer_interrupt
        - tick_handle_periodic
          - tick_periodic
            - update_process_times
              - run_local_timers
                - hrtimer_run_queues
                  - hrtimer_switch_to_hres
                    - tick_init_highres : 在这里把 clock_event_device::event_handler 注册为 hrtimer_interrupt
                    - tick_setup_sched_timer


切换之前 : 系统启动之后，立刻 br update_process_times
- common_interrupt
  - __common_interrupt
    - call_irq_handler
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


之后的调用路线 : hrtimer + dynamic tick 的模式
- asm_sysvec_apic_timer_interrupt
  - sysvec_apic_timer_interrupt
    - instr_sysvec_apic_timer_interrupt
      - __sysvec_apic_timer_interrupt
        - local_apic_timer_interrupt
          - hrtimer_interrupt
            - __hrtimer_run_queues
              - __run_hrtimer
                - tick_nohz_handler
                  - tick_sched_handle
                    - update_process_times

## [ ] broadcast 模式
kernel/time/tick-broadcast.c
可以先看看 https://lwn.net/Articles/574962/

一时间都不知道怎么测试这个问题，似乎需要先搞一个 deep C-state 模拟出来才可以

## Tick Devices

struct tick_device 只是 struct clock_event_device 的封装:
```c
enum tick_device_mode {
	TICKDEV_MODE_PERIODIC,
	TICKDEV_MODE_ONESHOT,
};

struct tick_device {
	struct clock_event_device *evtdev;
	enum tick_device_mode mode;
};
```

Again, **the kernel distinguishes global and local (per-CPU) tick devices.** The local devices are collected
in `tick_cpu_device` (defined in kernel/time/tick-internal.h). Note that the kernel automatically
creates a tick device when a new clock event device is registered.

Several global variables are additionally defined in `include/time/tick-internal.h`:
- `tick_cpu_device` is a per-CPU list containing one instance of struct tick_device for each CPU in the system.
- `tick_next_period` specifies the time (in nanoseconds) when the next global tick event will happen.
- `tick_do_timer_cpu` contains the CPU number whose tick device assumes the role of the global tick device.
- `tick_period` stores the interval between ticks in nanoseconds. It is the counterpart to HZ that denotes the frequency at which ticks occur.

To set up a tick device, the kernel provides the function `tick_setup_device`. The prototype is as follows,
and the code flow diagram is depicted in Figure 15-8

![](../../kernel/plka/img/15-8.png)

## 先看看代码

- do_idle 中为什么调用 tick_check_broadcast_expired 来决定是否进入 cpu_idle_poll 的状态

- tick_broadcast_oneshot_control

- tick_broadcast_enter

broadcast 还是和 idle 机制联系到一起的 : cpuidle_enter_state

感觉还是需要使用 n100 的机器来调试测试一下。

### 分析一下 tick_nohz_idle_exit

- [ ] do_idle 总是会调用这两个，但是如果不支持 no hz，其中的内容的意义是什么？

  - tick_nohz_idle_enter
  - tick_nohz_idle_exit

- 为什么这几个函数总是没有人调用的:

```txt
tick_freeze -- 这个是系统 suspend 的时候处理的
tick_suspend -- 最终被 tick_freeze 调用
tick_handle_periodic / tick_nohz_handler -- 模式不对
```

### amd_e400_idle 和 broadcast tick 的关系

之前在某一个虚拟机中观察到了:
```txt
#14 [ffffffff819e7e90] default_idle at ffffffff816ab33e
#15 [ffffffff819e7eb0] amd_e400_idle at ffffffff810347be
#16 [ffffffff819e7ed8] arch_cpu_idle at ffffffff81035006
#17 [ffffffff819e7ee8] cpu_startup_entry at ffffffff810e7bca
#18 [ffffffff819e7f30] rest_init at ffffffff81692c57
#19 [ffffffff819e7f40] start_kernel at ffffffff81b45060
#20 [ffffffff819e7f88] x86_64_start_reservations at ffffffff81b445ef
#21 [ffffffff819e7f98] x86_64_start_kernel at ffffffff81b44740
```

```txt
commit cb81deefb59de01325ab822f900c13941bfaf67f
Author: Thomas Gleixner <tglx@linutronix.de>
Date:   Wed Feb 28 23:13:00 2024 +0100

    x86/idle: Sanitize X86_BUG_AMD_E400 handling

    amd_e400_idle(), the idle routine for AMD CPUs which are affected by
    erratum 400 violates the RCU constraints by invoking tick_broadcast_enter()
    and tick_broadcast_exit() after the core code has marked RCU non-idle.  The
    functions can end up in lockdep or tracing, which rightfully triggers a
    RCU warning.

    The core code provides now a static branch conditional invocation of the
    broadcast functions.

    Remove amd_e400_idle(), enforce default_idle() and enable the static branch
    on affected CPUs to cure this.

      [ bp: Fold in a fix for a IS_ENABLED() check fail missing a "CONFIG_"
        prefix which tglx spotted. ]

    Reported-by: Borislav Petkov <bp@alien8.de>
    Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
    Signed-off-by: Borislav Petkov (AMD) <bp@alien8.de>
    Link: https://lore.kernel.org/r/877cim6sis.ffs@tglx
```


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
