## idle 子系统
<!-- 82f09d0a-4d7c-4160-93a9-6aababacdc7e -->

这里主要分析下，idle 子系统的路径选择

核心在 kernel/sched/idle.c 中，这里进行了关键的路径选择

- do_idle
	- **case 1** 如果 idle=poll : cpu_idle_poll ，循环调用 cpu_relax()
	- cpuidle_idle_call
		- **case 2** 没有 cpuidle : default_idle_call
		- **case 3** 有 cpuidle : call_cpuidle

1. 无需多言，感觉没啥意义，如果想要 poll ，
可以把 cstate 其他状态都关闭，最后就是会进入到
drivers/cpuidle/poll_state.c 中 poll_idle 最后也是 cpu_relax


2. x86 虚拟机的选择，如果 idle=halt 也会进入到这里，也就是绕过那些复杂的硬件机制，
仅仅使用最简单的机制，也就是 hlt 指令
- default_idle_call
  - default_idle
    - arch_safe_halt
      - pv_native_safe_halt
        - native_safe_halt

3. 如果 idle driver 正常的使用
```txt
-   33.79%     0.00%  swapper          [kernel.kallsyms]                   [k] common_startup_64
   - common_startup_64
      - 31.19% start_secondary
         - cpu_startup_entry
            - 29.75% do_idle
               - 25.76% cpuidle_enter
                  - cpuidle_enter_state
                     - 10.07% acpi_idle_enter
                          acpi_processor_ffh_cstate_enter
```

```txt
   - 71.35% cpu_startup_entry
      - do_idle
         - 57.81% cpuidle_enter
            - cpuidle_enter_state
                 11.70% intel_idle <- 去掉时钟中断
               - 0.94% ct_idle_exit
                    ct_kernel_enter.isra.0
         - 3.87% menu_select
            - 1.76% tick_nohz_get_sleep_length
               - 1.26% tick_nohz_next_event
                    0.71% __get_next_timer_interrupt
              0.76% cpuidle_governor_latency_req
```

## 细节
### asahi linux 中使用 ftrace 的结果

```txt
 4)               |  do_idle() {
 4)   0.083 us    |    nohz_run_idle_balance();
 4)               |    tick_nohz_idle_enter() {
 4)               |      ktime_get() {
 4)   0.042 us    |        arch_counter_read();
 4)   0.167 us    |      }
 4)   0.292 us    |    }
 4)   0.041 us    |    arch_cpu_idle_enter();
 4)               |    rcu_nocb_flush_deferred_wakeup() {
 4)   0.209 us    |      do_nocb_deferred_wakeup.isra.0();
 4)   0.416 us    |    }
 4)               |    cpuidle_idle_call() {
 4)   0.042 us    |      cpuidle_get_cpu_driver();
 4)   0.083 us    |      cpuidle_not_available();
 4)               |      cpuidle_select() {
 4)               |        menu_select() {
 4)               |          cpuidle_governor_latency_req() {
 4)   0.084 us    |            get_cpu_device();
 4)   0.084 us    |            pm_qos_read_value();
 4)   0.125 us    |            cpu_latency_qos_limit();
 4)   0.625 us    |          }
 4)   0.042 us    |          nr_iowait_cpu();
 4)               |          tick_nohz_get_sleep_length() {
 4)   0.084 us    |            can_stop_idle_tick();
 4)               |            tick_nohz_next_event() {
 4)   0.084 us    |              rcu_needs_cpu();
 4)               |              get_next_timer_interrupt() {
 4)   0.041 us    |                _raw_spin_lock();
 4)   0.167 us    |                _raw_spin_unlock();
 4)               |                hrtimer_get_next_event() {
 4)   0.083 us    |                  _raw_spin_lock_irqsave();
 4)   0.042 us    |                  _raw_spin_unlock_irqrestore();
 4)   0.416 us    |                }
 4)   0.959 us    |              }
 4)   0.042 us    |              timekeeping_max_deferment();
 4)   1.416 us    |            }
 4)               |            hrtimer_next_event_without() {
 4)   0.083 us    |              _raw_spin_lock_irqsave();
 4)   0.041 us    |              __hrtimer_next_event_base();
 4)   0.083 us    |              __hrtimer_next_event_base();
 4)   0.041 us    |              _raw_spin_unlock_irqrestore();
 4)   0.500 us    |            }
 4)   2.416 us    |          }
 4)   0.083 us    |          tick_nohz_tick_stopped();
 4)   3.500 us    |        }
 4)   3.666 us    |      }
 4)               |      tick_nohz_idle_stop_tick() {
 4)               |        tick_nohz_stop_tick() {
 4)               |          calc_load_nohz_start() {
 4)   0.125 us    |            calc_load_nohz_fold();
 4)   0.334 us    |          }
 4)               |          quiet_vmstat() {
 4)               |            need_update() {
 4)   0.042 us    |              first_online_pgdat();
 4)   0.250 us    |            }
 4)               |            refresh_cpu_vm_stats() {
 4)   0.042 us    |              first_online_pgdat();
 4)   0.083 us    |              next_zone();
 4)   0.042 us    |              next_zone();
 4)   0.084 us    |              next_zone();
 4)   0.125 us    |              next_zone();
 4)   0.083 us    |              next_zone();
 4)   0.041 us    |              first_online_pgdat();
 4)   0.084 us    |              next_online_pgdat();
 4)   0.167 us    |              fold_diff();
 4)   1.750 us    |            }
 4)   2.209 us    |          }
 4)               |          hrtimer_start_range_ns() {
 4)   0.083 us    |            _raw_spin_lock_irqsave();
 4)               |            __hrtimer_start_range_ns() {
 4)   0.084 us    |              __remove_hrtimer();
 4)   0.084 us    |              enqueue_hrtimer();
 4)               |              hrtimer_update_next_event() {
 4)   0.083 us    |                __hrtimer_next_event_base();
 4)   0.083 us    |                __hrtimer_next_event_base();
 4)   0.292 us    |              }
 4)               |              tick_program_event() {
 4)               |                clockevents_program_event() {
 4)               |                  ktime_get() {
 4)   0.041 us    |                    arch_counter_read();
 4)   0.167 us    |                  }
 4)   0.250 us    |                  arch_timer_set_next_event_phys();
 4)   0.666 us    |                }
 4)   0.792 us    |              }
 4)               |        nohz_balance_enter_idle() {
 4)   0.083 us    |          __rcu_read_lock();
 4)   0.083 us    |          __rcu_read_unlock();
 4)   0.625 us    |        }
 4)   5.625 us    |      }
 4)               |      cpuidle_enter() {
 4)   0.084 us    |        tick_nohz_get_next_hrtimer();
 4)   0.042 us    |        sched_idle_set_state();
 4)               |        cpu_pm_enter() {
 4)   0.125 us    |          _raw_spin_lock_irqsave();
 4)               |          raw_notifier_call_chain_robust() {
 4)               |            notifier_call_chain() {
 4)   0.083 us    |              arch_timer_cpu_pm_notify();
 4)               |              fpsimd_cpu_pm_notifier() {
 4)               |                fpsimd_save_and_flush_cpu_state() {
 4)   0.042 us    |                  fpsimd_save_user_state();
 4)   0.083 us    |                  fpsimd_flush_cpu_state();
 4)   0.292 us    |                }
 4)   0.375 us    |              }
 4)   0.167 us    |              hyp_init_cpu_pm_notifier();
 4)   0.083 us    |              cpu_pm_pmu_notify();
 4)   0.083 us    |              cpu_pm_pmu_notify();
 4)   1.292 us    |            }
 4)   1.416 us    |          }
 4)   0.042 us    |          _raw_spin_unlock_irqrestore();
 4)   1.792 us    |        }
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
