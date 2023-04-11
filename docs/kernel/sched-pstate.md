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
