# sysfs cpu
## /sys/devices/system/cpu
<!-- a650d90b-3c26-4292-adec-4cd6a9b76979 -->

drivers/base/cpu.c 中的:
```c
static struct attribute *cpu_root_attrs[] = {
#ifdef CONFIG_ARCH_CPU_PROBE_RELEASE
	&dev_attr_probe.attr,
	&dev_attr_release.attr,
#endif
	&cpu_attrs[0].attr.attr,
	&cpu_attrs[1].attr.attr,
	&cpu_attrs[2].attr.attr,
	&dev_attr_kernel_max.attr,
	&dev_attr_offline.attr,
	&dev_attr_enabled.attr,
	&dev_attr_isolated.attr,
#ifdef CONFIG_NO_HZ_FULL
	&dev_attr_nohz_full.attr,
#endif
#ifdef CONFIG_CRASH_HOTPLUG
	&dev_attr_crash_hotplug.attr,
#endif
#ifdef CONFIG_GENERIC_CPU_AUTOPROBE
	&dev_attr_modalias.attr,
#endif
	NULL
};
```

```c
/* Keep in sync with cpu_subsys_attrs */
static struct cpu_attr cpu_attrs[] = {
	_CPU_ATTR(online, &__cpu_online_mask),
	_CPU_ATTR(possible, &__cpu_possible_mask),
	_CPU_ATTR(present, &__cpu_present_mask),
};
```
和这些都是可以完整的对应上的:

- crash_hotplug
- enabled
- modalias
- nohz_full : 经典，原来在这个地方啊
- isolated
- kernel_max
- online
- possible
- present
- offline
- uevent

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(不过根据文档生成的，精度应该还可以的)

##  cup0

(常规内容)
-  uevent
-  power
-  driver -> ../../../../bus/cpu/drivers/processor
-  firmware_node -> ../../../LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/ACPI0007:00
-  node0 -> ../../node/node0
-  subsystem -> ../../../../bus/cpu

(有意思，但是现在不看)
-  microcode
-  hotplug
-  crash_notes
-  crash_notes_size
-  cache
-  topology
-  cpu_capacity

-  cpufreq -> ../cpufreq/policy0
-  cpuidle
-  thermal_throttle

1. 接口路径: /sys/devices/system/cpu/cpu<N>/cpuidle/state<M>/ (其中 N 是 CPU 编号，M 是空闲状态编号)
* `above`: 此空闲状态被请求但观察到的空闲时间肯定短于其目标驻留时间的次数。
* `below`: 此空闲状态被请求但观察到的空闲时间肯定更适合更深状态的次数。
* `desc`: 空闲状态的描述。
* `disable`: 是否禁用此空闲状态。
* `default_status`: 此状态的默认状态 ("enabled" 或 "disabled")。
* `latency`: 空闲状态的退出延迟（微秒）。
* `name`: 空闲状态的名称。
* `power`: （文档截断，可能包含功率相关数据）
* `time`: CPU 在此空闲状态花费的总时间（纳秒）。
* `usage`: 此空闲状态被请求的总次数。

2. Intel 性能与能耗偏置提示 (Intel EPB - Energy Performance Bias)
接口路径: /sys/devices/system/cpu/cpu<N>/power/ (其中 N 是 CPU 编号)
* `energy_perf_bias`: 显示或设置此 CPU 的 EPB 值（0-15 数字或字符串），用于指示性能/能耗偏好。


3. thermal_throttle 下的
-  core_throttle_count
-  core_throttle_max_time_ms
-  core_throttle_total_time_ms
-  package_throttle_count
-  package_throttle_max_time_ms
-  package_throttle_total_time_ms

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(似乎是这么回事)
* 主要目的： 监控 CPU 因温度过高而自动降低性能（即“节流”）的事件次数。
* 核心文件： 最重要的文件通常是 core_throttle_count。
    * `core_throttle_count`: 记录了该特定 CPU 核心因热节流而被触发的次数。当 CPU 温度接近或超过安全阈值时，硬件或固件会强制降低 CPU
      频率以降温，每次这样的事件都会被计数器记录下来。
* 位置： 每个逻辑 CPU 核心（cpu0, cpu1, ...）下都有一个对应的 thermal_throttle 目录。
* 用途： 用户空间工具（如 turbostat）会读取这个文件来监控和报告 CPU
  的热节流状况。这对于评估系统散热效果、识别潜在的过热问题或监控长时间运行的高负载任务非常有用。


##  cpufreq

* `affected_cpus`: 显示属于此策略的在线 CPU 列表。
* `bios_limit`: 如果平台固件 (BIOS) 设置了 CPU 频率上限，会在此显示（如果支持）。
* `cpuinfo_cur_freq`: 从硬件获取的当前 CPU 频率（kHz）。
* `cpuinfo_avg_freq`: 基于硬件反馈计算的平均 CPU 频率（kHz，如果支持）。
* `cpuinfo_max_freq`: CPU 可运行的最高频率（kHz）。
* `cpuinfo_min_freq`: CPU 可运行的最低频率（kHz）。
* `cpuinfo_transition_latency`: CPU 在 P-state 之间切换所需的时间（纳秒）。
* `related_cpus`: 显示属于此策略的所有 CPU（在线和离线）。
* `scaling_available_frequencies`: 此策略可用的频率列表（kHz）。
* `scaling_available_governors`: 可附加到此策略的 CPUFreq 调节器列表。
* `scaling_cur_freq`: 当前策略下 CPU 的频率（kHz）。
* `scaling_driver`: 当前使用的 CPUFreq 驱动名称。
* `scaling_governor`: 当前附加到此策略的调节器名称。写入此文件可更改调节器。
* `scaling_max_freq`: 允许 CPU 运行的最高频率（kHz）。写入此文件可设置上限。
* `scaling_min_freq`: 允许 CPU 运行的最低频率（kHz）。写入此文件可设置下限。
* `scaling_setspeed`: 仅当 userspace 调节器激活时有效，用于手动设置频率。

##  cpuidle

全局 CPUIdle 接口路径
* `current_driver`: 当前使用的 CPUIdle 驱动名称。
* `current_governor_ro` 或 `current_governor`: 当前使用的 CPUIdle 调节器名称。
* `available_governors`: 可用的 CPUIdle 调节器列表。

##  intel_pstate
 * `max_perf_pct`, `min_perf_pct`, `turbo_pct`: (常见于 intel_pstate 或 SST 接口) 控制最大/最小性能百分比和睿频启用百分比。
 * `num_pstates_on_package`: (可能存在于 SST 接口) 显示包上的 P-state 数量。
 * `no_turbo`: (可能存在于 SST 接口) 禁用/启用睿频。
 * `hwp_dynamic_boost`: (可能存在于 SST 接口) 启用/禁用硬件 P-state 动态加速。

##  intel_uncore_frequency

Intel Uncore 频率调节 (Intel Uncore Frequency Scaling)
 * `package_<XX>_die_<YY>/`: 按 Package 和 Die 分组的目录。
     * initial_max_freq_khz: 复位时的最大频率（只读）。
     * initial_min_freq_khz: 复位时的最小频率（只读）。
     * max_freq_khz: 设置最大 uncore 频率（可写）。
     * min_freq_khz: 设置最小 uncore 频率（可写）。
     * current_freq_khz: 获取当前 uncore 频率（只读）。
 * `uncore<ZZ>/`: (TPMI 支持下) 按 Fabric Cluster 分组的目录。
     * domain_id, die_id, fabric_cluster_id, package_id: 标识此实例的域、晶粒、集群、插槽 ID。
     * agent_types: 显示此域内的硬件代理类型。
     * max_freq_khz, min_freq_khz, current_freq_khz: 同上，但作用于更细粒度的集群。
     * elc_floor_freq_khz: ELC 功能的频率下限（可读写）。
     * elc_low_threshold_percent: ELC 功能的 CPU 利用率低阈值（可读写）。
     * elc_high_threshold_percent: ELC 功能的 CPU 利用率高阈值（可读写）。
     * elc_high_threshold_enable: 启用/禁用 ELC 高阈值功能（可读写）。

##  umwait_control
enable_c02  max_time

关联文件
arch/x86/kernel/cpu/umwait.c

## 和电源无关的内容，可以后面再说吧:

-  vulnerabilities
-  microcode
-  hotplug
-  smt
-  power : 老熟人了，从来没搞清楚过，很多目录都有这个和 uevent 的

## 常用的总结
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_driver
cat /sys/devices/system/cpu/cpuidle/current_driver

## 问题?
thread_siblings_list

/sys/devices/system/node/node7/cpu255/topology/thread_siblings_list

怎么还有 node 的这个路径啊

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
