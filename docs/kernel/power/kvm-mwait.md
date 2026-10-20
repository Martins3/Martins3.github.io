# kvm mwait
## QEMU 启动时 `-overcommit cpu-pm=on` 如何影响 guest idle

在 x86 KVM 场景中，这个选项通过两方面影响 CPU 空闲行为：

- 向 guest 暴露可用的 MONITOR/MWAIT 能力，让 guest Linux 在启动时根据 CPUID 选择 `mwait_idle()`。
- 请求 KVM 关闭相应指令的 VM exit，让 guest 可以直接执行 MONITOR/MWAIT；支持时也关闭 HLT、PAUSE 等退出。

### QEMU 启动：设置 CPUID 和 KVM 退出控制

1. 添加 cpuid feature :

在虚拟机中通过 `/proc/cpuinfo` 中显示为 `monitor`。

2. 关闭 mwait 导致 cpu 退出 : KVM_X86_DISABLE_EXITS_MWAIT

在 Intel VMX 路径，Linux `arch/x86/kvm/vmx/vmx.c` 中的 `vmx_exec_control()` 据此清除 VMCS 控制位：
```c
if (kvm_mwait_in_guest(vmx->vcpu.kvm))
    exec_control &= ~(CPU_BASED_MWAIT_EXITING |
                      CPU_BASED_MONITOR_EXITING);
if (kvm_hlt_in_guest(vmx->vcpu.kvm))
    exec_control &= ~CPU_BASED_HLT_EXITING;
```

### guest 启动：根据 CPUID 选择 idle 函数

Linux `arch/x86/kernel/process.c` 中的 `select_idle_routine()` 调用
`prefer_mwait_c1_over_halt()`，主要检查：

- 没有 `idle=nomwait` 等启动参数覆盖默认选择。
- 存在 `X86_FEATURE_MWAIT`，且没有相关 MONITOR/APIC bug。
- 查询 CPUID leaf `5`；如果宣告支持 MWAIT 扩展，则至少存在一个 C1 substate。

检查通过后，`select_idle_routine()` 将 `x86_idle` 更新为 `mwait_idle()`。
在本文的普通 x86 KVM 场景中，未通过时使用 `default_idle()`。这解释了调用路线的变化：

```text
QEMU: -overcommit cpu-pm=on
  ├─ 允许暴露 MONITOR/MWAIT，并提供对应 CPUID 信息
  │    └─ guest 启动：select_idle_routine()
  │         └─ prefer_mwait_c1_over_halt() 检查通过
  │              └─ x86_idle = mwait_idle
  └─ KVM_ENABLE_CAP(KVM_CAP_X86_DISABLE_EXITS)
       └─ KVM 关闭 MONITOR/MWAIT 等指令拦截

```

idle 流程中，参考 docs/kernel/power/cstate/cstate.md
最后会进入到 mwait_idle 中去


### 实验记录

```txt
       -overcommit mem-lock=on|off

       -overcommit cpu-pm=on|off
              Run qemu with hints about host resource overcommit. The default is to assume that host over‐
              commits all resources.

              Locking  qemu  and  guest  memory can be enabled via mem-lock=on (disabled by default). This
              works when host memory is not overcommitted and reduces the worst-case latency for guest.

              Guest ability to manage power state of host cpus (increasing latency for other processes  on
              the  same host cpu, but decreasing latency for guest) can be enabled via cpu-pm=on (disabled
              by default). This works best when host CPU is not overcommitted. When used,  host  estimates
              of  CPU  cycle  and  power utilization will be incorrect, not taking into account guest idle
              time.
```
qemu 启动参数添加上 `-overcommit cpu-pm=on` 之后：

htop 观察，发现所有的 cpu 都是满利用率的:
```txt
cat /proc/loadavg
29.14 14.54 7.56 33/847 38231
```

但是，观察物理机频率，又是这样的:
```txt
cat /proc/cpuinfo | grep -i mhz
cpu MHz         : 1100.410
cpu MHz         : 1099.032
cpu MHz         : 1100.000
cpu MHz         : 1099.059
cpu MHz         : 1100.440
cpu MHz         : 1100.000
cpu MHz         : 1100.000
cpu MHz         : 1099.034
cpu MHz         : 1099.988
cpu MHz         : 1098.929
cpu MHz         : 1100.000
cpu MHz         : 1098.944
cpu MHz         : 1100.960
cpu MHz         : 1102.217
cpu MHz         : 1097.738
cpu MHz         : 1100.000
cpu MHz         : 802.140
cpu MHz         : 800.311
cpu MHz         : 800.471
cpu MHz         : 800.974
cpu MHz         : 797.983
cpu MHz         : 798.458
cpu MHz         : 801.020
cpu MHz         : 800.781
cpu MHz         : 799.107
cpu MHz         : 797.096
cpu MHz         : 801.507
cpu MHz         : 800.570
cpu MHz         : 801.729
cpu MHz         : 799.676
cpu MHz         : 801.797
cpu MHz         : 798.260
```
CPU 温度为 24 ~ 29 度。

这与 host 显示很高的 CPU 利用率并不矛盾：硬件可以在 guest 的 MWAIT 中等待，
但 vCPU 没有通过通常的阻塞路径将 CPU 交还 host 调度器，host 的统计不会将这段 guest idle
计为空闲。温度、频率和 CPU 利用率也不能单独证明具体进入了哪个硬件 C-state。
参见 [QEMU `-overcommit` 参数说明](https://www.qemu.org/docs/master/system/qemu-manpage.html)。

虚拟机中观察到结果:
```txt
-   91.18%     0.00%  swapper          [kernel.kallsyms]                               [k] common_startup_64
   - common_startup_64
      - 89.49% start_secondary
         - cpu_startup_entry
            - 89.49% do_idle
               - 89.38% cpuidle_idle_call
                  - default_idle_call
                     - mwait_idle
                        - 0.56% asm_sysvec_apic_timer_interrupt
                             sysvec_apic_timer_interrupt
                             __irq_exit_rcu
                             handle_softirqs
      - 1.69% x86_64_start_kernel
           x86_64_start_reservations
           start_kernel
           rest_init
           cpu_startup_entry
         - do_idle
            - 1.68% cpuidle_idle_call
               - default_idle_call
                    mwait_idle
```
物理机的 CPU 被抢占的相当厉害，ccls 索引基本无法推进:

guest 中没有可用的 cpuidle/cpufreq driver：
```txt
sudo cpupower idle-info
CPUidle driver: none
CPUidle governor: menu
analyzing CPU 2:

CPU 2: No idle states

sudo cpupower frequency-info
analyzing CPU 14:
  no or unknown cpufreq driver is active on this CPU
  CPUs which run at the same hardware frequency: Not Available
  CPUs which need to have their frequency coordinated by software: Not Available
  maximum transition latency:  Cannot determine or is not supported.
Not Available
  available cpufreq governors: Not Available
  Unable to determine current policy
  current CPU frequency: Unable to call hardware
  current CPU frequency:  Unable to call to kernel
  boost state support:
    Supported: no
    Active: no
```

## 为什么虚拟机中没有结果:

物理机中可以观察到:
- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
              - acpi_processor_driver_init
                - driver_register
                  - bus_add_driver
                    - driver_attach
                      - bus_for_each_dev
                        - __driver_attach
                          - __driver_attach
                            - driver_probe_device
                              - __driver_probe_device
                                - really_probe
                                  - call_driver_probe
                                    - acpi_processor_start
                                      - __acpi_processor_start
                                        - acpi_processor_power_init

- `drivers/cpuidle/cpuidle-haltpoll.c` 中的 `haltpoll_init()`
- `drivers/idle/intel_idle.c` 中的 `intel_idle_init()`
三个都找到；

- cpuidle_register_device 的注册是类似的方法，但是虚拟机中没有调用位置。

## CLK 2021 关于 AMD vCPU 的优化

https://github.com/ChinaLinuxKernel/CLK/blob/master/CLK2021/3-1%20AMD%E6%9E%B6%E6%9E%84%E8%99%9A%E6%8B%9F%E6%9C%BA%E6%80%A7%E8%83%BD%E6%8E%A2%E7%B4%A2%E4%B8%8E%E5%AE%9E%E8%B7%B5.pdf

- intel idle : 执行 mwait 指令
- acpi idle : 执行 mwait hlt 以及 ioport
- halt poll : halt 之前 poll 一段时间

对于本文的 `mwait_idle()` 路径，MONITOR 监控的是 guest 内当前 idle task 的
`current_thread_info()->flags`，不是 host 中 QEMU vCPU 线程的 flags。
guest 调度器设置 `TIF_NEED_RESCHED` 时，对受监控内存的写入可以唤醒 MWAIT；
guest 从 idle 路径返回后，由 guest 调度器调度待运行任务。中断也可以唤醒等待。

在 KVM 放行 MONITOR/MWAIT 后，这条路径可以减少空闲和唤醒过程中的虚拟化开销，
但不表示 VM 永不退出，也不表示 host 的调度完全停止。
`haltpoll` 则是在 halt 前轮询一段时间，不能将它与这里的 MONITOR/MWAIT 路径混为一谈。

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
