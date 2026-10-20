# schedutil
<!-- fa6dc7ea-ea53-478b-83d5-47b905ce3b22 -->

很多默认的 schedutil_governor

关联的文件为: kernel/sched/cpufreq.c

细节分析
- https://deepinout.com/android-system-analysis/android-cpu-related/principle-analysis-of-cpu-governor-schedutil.html
- http://www.wowotech.net/process_management/schedutil_governor.html
- Documentation/scheduler/schedutil.rst
- Documentation/scheduler/sched-util-clamp.rst

在 scheduelr 的核心中调用，当执行两个到两个的时候:
- cpufreq_update_util
  - 执行 update_util_data::func

- gov_set_update_util
- intel_pstate_set_update_util_hook
- sugov_start
  - cpufreq_add_update_util_hook : 注册 update_util_data::func

- sugov_limits
  - `__cpufreq_driver_target`
    - 调用 `struct cpufreq_driver` 来处理，但是 `cpufreq_driver` 是如何赋值的，不清楚哇


基本调用路线:
- asm_sysvec_apic_timer_interrupt
  - sysvec_apic_timer_interrupt
    - instr_sysvec_apic_timer_interrupt
      - irq_exit_rcu
        - __irq_exit_rcu
          - invoke_softirq
            - __do_softirq
              - handle_softirqs
                - sched_balance_softirq
                  - sched_balance_update_blocked_averages
                    - __update_blocked_fair
                      - update_load_avg
                        - cfs_rq_util_change
                          - cpufreq_update_util

当前 13900K 是 active HWP。intel_pstate_set_update_util_hook() 明确判断：

  if (hwp_active && !hwp_boost)
          return;

所以没有开启 hwp_dynamic_boost 时，func == NULL 完全符合设计。
这不表示 scheduler 或驱动异常，只表示硬件正在自主选频，scheduler 不需要持续调用 intel_pstate。

trace 这些函数，也都是没有调用，由此可以认为，并不会轻易的调用到 driver 中:
```txt
sudo bpftrace -e 'kprobe:*intel_pstate* { @[probe] = count(); }'t
```

想要通过 kprobe 和 tracepoint 找，并没有
```txt
sudo perf stat -e power:*
 Performance counter stats for 'system wide':

                 0      power:guest_halt_poll_ns
                 0      power:dev_pm_qos_remove_request
                 0      power:dev_pm_qos_update_request
                 0      power:dev_pm_qos_add_request
                 0      power:pm_qos_update_flags
                 0      power:pm_qos_update_target
                 0      power:pm_qos_remove_request
                 0      power:pm_qos_update_request
                 0      power:pm_qos_add_request
                 0      power:wakeup_source_deactivate
                 0      power:wakeup_source_activate
                 0      power:suspend_resume
                 0      power:device_pm_callback_end
                 0      power:device_pm_callback_start
                 0      power:cpu_frequency_limits
                 0      power:cpu_frequency
                 0      power:pstate_sample
             1,045      power:cpu_idle_miss
            10,690      power:cpu_idle

       3.662953603 seconds time elapsed
```

## TODO
和 docs/kernel/sched/sched-uclamp.md 关系?

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
