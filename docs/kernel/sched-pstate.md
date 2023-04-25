# Linux scheduelr

- 研究下休眠的原理，这个时候，应该来说，内核中
- 研究下 pstate 和 cstate 的原理，编译的时候，CPU 频率提升，是操作系统来控制频率的吗
- 测试 suspend 的原理

## /proc/cpuinfo | grep 'cpu MHz'
- show_cpuinfo

```c
	if (cpu_has(c, X86_FEATURE_TSC)) {
		unsigned int freq = arch_freq_get_on_cpu(cpu);

		seq_printf(m, "cpu MHz\t\t: %u.%03u\n", freq / 1000, (freq % 1000));
	}
```
- [ ] 为什么需要 tsc 啊
```c
#define X86_FEATURE_TSC			( 0*32+ 4) /* Time Stamp Counter */
#define X86_FEATURE_APERFMPERF		( 3*32+28) /* P-State hardware coordination feedback capability (APERF/MPERF MSRs) */
```
- scheduler_tick
  - arch_scale_freq_tick : 通过 msr 寄存器获取

- https://vstinner.github.io/intel-cpus.html : cstate pstate 的介绍

- 关键的内核文档: https://docs.kernel.org/admin-guide/pm/working-state.html

- cpuidle.off = 1

## 一些系统的接口
/sys/devices/system/cpu/intel_pstate
/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq



## 工具
### cpupower
cpupower idle-info

```txt
sudo cpupower frequency-info -g governor

sudo cpupower frequency-set -g performance
```
实际上就是去设置:
- /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

- /sys/devices/system/cpu/cpu0/acpi_cppc/


这个的源码在什么位置。
```txt
🧀  cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 4
Available idle states: POLL C1_ACPI C2_ACPI C3_ACPI
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 102555
Duration: 1768149
C1_ACPI:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 593921
Duration: 89988252
C2_ACPI:
Flags/Description: ACPI FFH MWAIT 0x21
Latency: 127
Usage: 925818
Duration: 509306411
C3_ACPI:
Flags/Description: ACPI FFH MWAIT 0x60
Latency: 1048
Usage: 1196072
Duration: 5598272250
```

```txt
🧀  sudo cpupower monitor
[sudo] password for martins3:
    | Nehalem                   || Mperf              || Idle_Stats
 CPU| C3   | C6   | PC3  | PC6  || C0   | Cx   | Freq || POLL | C1_A | C2_A | C3_A
   0|  0.00|  2.65|  0.00|  0.00||  1.06| 98.94|  5483||  0.00|  0.86|  2.98| 95.15
   1|  0.00|  2.65|  0.00|  0.00||  0.07| 99.93|  5475||  0.00|  0.02|  0.66| 99.25
   2|  0.00|  1.88|  0.00|  0.00||  0.72| 99.28|  5488||  0.01|  1.06|  2.84| 95.39
   3|  0.00|  1.88|  0.00|  0.00||  0.00|100.00|  5229||  0.00|  0.00|  0.00| 99.99
   4|  0.00|  1.01|  0.00|  0.00||  0.57| 99.43|  5484||  0.00|  0.93|  1.30| 97.15
   5|  0.00|  1.01|  0.00|  0.00||  0.00|100.00|  5275||  0.00|  0.00|  0.00| 99.99
   6|  0.00|  2.49|  0.00|  0.00||  0.32| 99.68|  5490||  0.00|  0.46|  3.47| 95.77
   7|  0.00|  2.49|  0.00|  0.00||  0.02| 99.98|  5475||  0.00|  0.03|  0.00| 99.95
   8|  0.00|  0.00|  0.00|  0.00||  1.40| 98.60|  5504||  0.01|  2.59|  0.22| 95.83
   9|  0.00|  0.00|  0.00|  0.00||  0.00|100.00|  5659||  0.00|  0.00|  0.00| 99.99
  10|  0.00|  5.22|  0.00|  0.00||  0.79| 99.21|  5532||  0.00|  1.04|  5.79| 92.41
  11|  0.00|  5.22|  0.00|  0.00||  0.02| 99.98|  5493||  0.00|  0.00|  0.00| 99.99
  12|  0.00|  2.56|  0.00|  0.00||  2.02| 97.98|  5483||  0.00|  0.65|  2.97| 94.38
  13|  0.00|  2.56|  0.00|  0.00||  0.00|100.00|  5270||  0.00|  0.00|  0.00| 100.0
  14|  0.00|  0.38|  0.00|  0.00||  0.86| 99.14|  5465||  0.00|  0.89|  1.36| 96.83
  15|  0.00|  0.38|  0.00|  0.00||  0.00|100.00|  5271||  0.00|  0.00|  0.00| 100.0
  16|  0.00| 96.64|  0.00|  0.00||  0.46| 99.54|  4291||  0.00|  0.21|  1.25| 98.10
  17|  0.00| 96.26|  0.00|  0.00||  0.35| 99.65|  4293||  0.00|  0.63|  1.48| 97.57
  18|  0.00| 97.14|  0.00|  0.00||  0.58| 99.42|  4291||  0.00|  0.28|  0.59| 98.57
  19|  0.00| 95.97|  0.00|  0.00||  0.68| 99.32|  4292||  0.00|  1.02|  2.44| 95.89
  20|  0.00| 96.45|  0.00|  0.00||  1.10| 98.90|  4292||  0.00|  0.35|  0.10| 98.47
  21|  0.00| 96.63|  0.00|  0.00||  0.72| 99.28|  4292||  0.00|  0.28|  0.00| 99.01
  22|  0.00| 96.61|  0.00|  0.00||  0.34| 99.66|  4292||  0.00|  0.56|  0.18| 98.94
  23|  0.00| 96.66|  0.00|  0.00||  0.40| 99.60|  4294||  0.01|  0.74|  0.24| 98.64
  24|  0.00| 97.44|  0.00|  0.00||  0.08| 99.92|  4293||  0.00|  0.08|  0.00| 99.86
  25|  0.00| 99.30|  0.00|  0.00||  0.67| 99.33|  4292||  0.00|  0.00|  0.00| 99.34
  26|  0.00| 96.53|  0.00|  0.00||  0.10| 99.90|  4293||  0.00|  0.86|  0.06| 98.99
  27|  0.00| 99.98|  0.00|  0.00||  0.01| 99.99|  4214||  0.00|  0.00|  0.07| 99.93
  28|  0.00| 99.98|  0.00|  0.00||  0.01| 99.99|  4433||  0.00|  0.00|  0.00|100.00
  29|  0.00| 99.57|  0.00|  0.00||  0.37| 99.63|  4293||  0.00|  0.00|  0.00| 99.63
  30|  0.00| 99.98|  0.00|  0.00||  0.00|100.00|  4370||  0.00|  0.00|  0.00| 100.0
  31|  0.00| 99.74|  0.00|  0.00||  0.22| 99.78|  4295||  0.00|  0.00|  0.00| 99.78
```


### turbostat

## 更多的参数
idle=poll
cpuidle.off=1
processor.max_cstate=1
intel_idle.max_cstate=
intel_pstate=
processor.max_cstate=

## https://stackoverflow.com/questions/57471862/uses-of-the-monitor-mwait-instructions
- 不知道为什么，mwait 会导致 guest 退出

- cpuidle.off=1 : 用于关闭 idle 的 subsystem 的
- hlt which just goes into the lightest sleep
- but user-space versions umonitor / umwait were added recently to Tremont (next-gen Goldmont atom)
- The idle kernel parameter is used, which takes one of the following values: poll, halt, nomwait.

## 资源

## 需要分析的代码

```c
unsigned int cpufreq_quick_get(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	unsigned int ret_freq = 0;
	unsigned long flags;

	read_lock_irqsave(&cpufreq_driver_lock, flags);

	if (cpufreq_driver && cpufreq_driver->setpolicy && cpufreq_driver->get) {
		ret_freq = cpufreq_driver->get(cpu);
		read_unlock_irqrestore(&cpufreq_driver_lock, flags);
		return ret_freq;
	}

	read_unlock_irqrestore(&cpufreq_driver_lock, flags);

	policy = cpufreq_cpu_get(cpu);
	if (policy) {
		ret_freq = policy->cur;
		cpufreq_cpu_put(policy);
	}

	return ret_freq;
}
```
- [ ] 如何理解这里的 cpurfreq_policy 啊？

## UEIF 中存在调整 cstate 的代码
- 当然啊

## sudo acpi -t
这个命令居然可以看温度

## 分析 cpupower frequency-info

```txt
🧀  sudo cpupower frequency-info
analyzing CPU 12:
  driver: intel_pstate
  CPUs which run at the same hardware frequency: 12
  CPUs which need to have their frequency coordinated by software: 12
  maximum transition latency:  Cannot determine or is not supported.
  hardware limits: 800 MHz - 5.50 GHz
  available cpufreq governors: performance powersave
  current policy: frequency should be within 800 MHz and 5.50 GHz.
                  The governor "powersave" may decide which speed to use
                  within this range.
  current CPU frequency: Unable to call hardware
  current CPU frequency: 5.50 GHz (asserted by call to kernel)
  boost state support:
    Supported: yes
    Active: yes
```
governor : performance powersave

## 列举一下 governor
但是

```c
struct cpufreq_governor schedutil_gov = {
	.name			= "schedutil",
	.owner			= THIS_MODULE,
	.flags			= CPUFREQ_GOV_DYNAMIC_SWITCHING,
	.init			= sugov_init,
	.exit			= sugov_exit,
	.start			= sugov_start,
	.stop			= sugov_stop,
	.limits			= sugov_limits,
};

static struct cpufreq_governor cpufreq_gov_userspace = {
	.name		= "userspace",
	.init		= cpufreq_userspace_policy_init,
	.exit		= cpufreq_userspace_policy_exit,
	.start		= cpufreq_userspace_policy_start,
	.stop		= cpufreq_userspace_policy_stop,
	.limits		= cpufreq_userspace_policy_limits,
	.store_setspeed	= cpufreq_set,
	.show_setspeed	= show_speed,
	.owner		= THIS_MODULE,
};

static struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.owner		= THIS_MODULE,
	.flags		= CPUFREQ_GOV_STRICT_TARGET,
	.limits		= cpufreq_gov_performance_limits,
};

static struct dbs_governor od_dbs_gov = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("ondemand"),
	.kobj_type = { .default_groups = od_groups },
	.gov_dbs_update = od_dbs_update,
	.alloc = od_alloc,
	.free = od_free,
	.init = od_init,
	.exit = od_exit,
	.start = od_start,
};

static struct cpufreq_governor cpufreq_gov_powersave = {
	.name		= "powersave",
	.limits		= cpufreq_gov_powersave_limits,
	.owner		= THIS_MODULE,
	.flags		= CPUFREQ_GOV_STRICT_TARGET,
};
```

## driver

```c
static struct cpufreq_driver intel_pstate = {
	.flags		= CPUFREQ_CONST_LOOPS,
	.verify		= intel_pstate_verify_policy,
	.setpolicy	= intel_pstate_set_policy,
	.suspend	= intel_pstate_suspend,
	.resume		= intel_pstate_resume,
	.init		= intel_pstate_cpu_init,
	.exit		= intel_pstate_cpu_exit,
	.offline	= intel_pstate_cpu_offline,
	.online		= intel_pstate_cpu_online,
	.update_limits	= intel_pstate_update_limits,
	.name		= "intel_pstate",
};

static struct cpufreq_driver acpi_cpufreq_driver = {
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= acpi_cpufreq_target,
	.fast_switch	= acpi_cpufreq_fast_switch,
	.bios_limit	= acpi_processor_get_bios_limit,
	.init		= acpi_cpufreq_cpu_init,
	.exit		= acpi_cpufreq_cpu_exit,
	.resume		= acpi_cpufreq_resume,
	.name		= "acpi-cpufreq",
	.attr		= acpi_cpufreq_attr,
};
```

## [ ]  https://deepinout.com/android-system-analysis/android-cpu-related/principle-analysis-of-cpu-governor-schedutil.html

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

- [ ] 可以顺这这个文档继续看看吧!
