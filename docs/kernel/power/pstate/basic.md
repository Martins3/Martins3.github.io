# pstate

核心文档:
- Documentation/cpu-freq/
- Documentation/admin-guide/pm/
- [intel_pstate CPU Performance Scaling Driver](https://docs.kernel.org/admin-guide/pm/intel_pstate.html)

## cpufreq governor 有哪些

这个源码树中的标准 governor 包括：

 governor        主要代码
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 performance     drivers/cpufreq/cpufreq_performance.c:14
──────────────  ────────────────────────────────────────────
 powersave       drivers/cpufreq/cpufreq_powersave.c:14
──────────────  ────────────────────────────────────────────
 userspace       drivers/cpufreq/cpufreq_userspace.c:129
──────────────  ────────────────────────────────────────────
 ondemand        drivers/cpufreq/cpufreq_ondemand.c:375
──────────────  ────────────────────────────────────────────
 conservative    drivers/cpufreq/cpufreq_conservative.c:317
──────────────  ────────────────────────────────────────────
 schedutil       kernel/sched/cpufreq_schedutil.c:927

实际可用哪些取决于驱动模式和内核配置，权威结果是：

cat /sys/devices/system/cpu/cpufreq/policy0/scaling_driver
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors

主动 intel_pstate 只会列出内置的 performance powersave；
被动 intel_cpufreq 才会列出内核已注册的通用 governors。

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

static struct dbs_governor cs_governor = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("conservative"),
	.kobj_type = { .default_groups = cs_groups },
	.gov_dbs_update = cs_dbs_update,
	.alloc = cs_alloc,
	.free = cs_free,
	.init = cs_init,
	.exit = cs_exit,
	.start = cs_start,
};

static struct cpufreq_governor cpufreq_gov_powersave = {
	.name		= "powersave",
	.limits		= cpufreq_gov_powersave_limits,
	.owner		= THIS_MODULE,
	.flags		= CPUFREQ_GOV_STRICT_TARGET,
};
```


## pstate driver 有哪些?

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

### cppc_cpufreq

acpi_cpufreq 和 cppc_cpufreq 的关系:

drivers/cpufreq/Makefile 可以看到 acpi_cpufreq 是 x86 特有的程序，而
cppc_cpufreq 是 arm 的体系下的，他们在层次是相同的，都需要 acpi table 来维持生活

它们是 CPUFreq 框架下两个并列的调频驱动，都使用 ACPI 定义的接口，但使用的是不同的性能控制机制。 cppc_cpufreq 不依赖或调用
acpi_cpufreq。

 对比                  acpi_cpufreq                                   cppc_cpufreq
━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 控制机制              传统 ACPI P-state                              CPPC（协作式处理器性能控制）
────────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
 固件接口              _PSS 提供性能档位，_PCT 描述控制/状态寄存器    _CPC 提供性能能力和控制/反馈接口
────────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
 请求方式              从固件列出的离散 P-state 中选一个              请求一个抽象的性能值，如 desired_perf
────────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
 当前源码支持的架构    x86                                            ARM、ARM64、RISC-V

## sysfs 结果分析

/sys/devices/system/cpu/cpu12/cpufreq 分析:

intel_pstate

```txt

🧀  grep . *
affected_cpus:12
base_frequency:3000000
cpuinfo_avg_freq:5500000
cpuinfo_max_freq:5500000
cpuinfo_min_freq:800000
cpuinfo_transition_latency:0
energy_performance_available_preferences:default performance balance_performance balance_power power
energy_performance_preference:performance
related_cpus:12
scaling_available_governors:performance powersave
scaling_cur_freq:5500000
scaling_driver:intel_pstate
scaling_governor:performance
scaling_max_freq:5500000
scaling_min_freq:800000
scaling_setspeed:<unsupported>
```

也是 intel_pstate ，但是 scaling_driver 变成 acpi_cpufreq
可以注意到，这个时候 scaling_available_governors 就是完整的
```txt
grep . *

affected_cpus:12
cpuinfo_avg_freq:3900000
cpuinfo_max_freq:3900000
cpuinfo_min_freq:1000000
cpuinfo_transition_latency:20000
related_cpus:12
scaling_available_governors:conservative ondemand userspace powersave performance schedutil
scaling_cur_freq:3900000
scaling_driver:intel_cpufreq
scaling_governor:performance
scaling_max_freq:3900000
scaling_min_freq:3900000
scaling_setspeed:<unsupported>
```

如果是 acpi_cpufreq 的结果:
```txt
$ grep . *
affected_cpus:12
bios_limit:2601000
boost:1
grep: cpuinfo_cur_freq: Permission denied
cpuinfo_max_freq:2601000
cpuinfo_min_freq:1200000
cpuinfo_transition_latency:10000
freqdomain_cpus:12
related_cpus:12
scaling_available_frequencies:2601000 2600000 2500000 2400000 2300000 2200000 2100000 2000000 1900000 1800000 1700000 1600000 1500000 1400000 1300000 1200000
scaling_available_governors:conservative ondemand userspace powersave performance schedutil
scaling_cur_freq:2900111
scaling_driver:acpi-cpufreq
scaling_governor:performance
scaling_max_freq:2601000
scaling_min_freq:1200000
scaling_setspeed:<unsupported>
```

## 关联的 cpuid
hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req

也就是 arch/x86/include/asm/cpufeatures.h
```txt
#define X86_FEATURE_HWP			(14*32+ 7) /* "hwp" Intel Hardware P-states */
```
13th Gen Intel(R) Core(TM) i9-13900K 有 X86_FEATURE_HWP 而服务器上没有。


## pstate driver 对性能基本无影响

### INTEL(R) XEON(R) GOLD 6530 上切换 pstate driver 无任何影响

sudo cgexec -g cpu,memory,cpuset:/ sysbench cpu --cpu-max-prime=1000000 run --threads=1

```txt
    events per second:     5.81
```
内核有多种切换，从 5.4 / 6.6 / 6.18 / 7.2 都是有的。


### 13th Gen Intel(R) Core(TM) i9-13900K 上使用 acpi-cpufreq 会导致性能下降

```txt
🧀  sudo cpupower frequency-info
[sudo] password for martins3:
analyzing CPU 25:
  driver: acpi-cpufreq
  CPUs which run at the same hardware frequency: 25
  CPUs which need to have their frequency coordinated by software: 25
  maximum transition latency: 10.0 us
  hardware limits: 800 MHz - 3.00 GHz
  available frequency steps:  3.00 GHz, 3.00 GHz, 2.90 GHz, 2.80 GHz, 2.70 GHz, 2.60 GHz, 2.50 GHz, 2.40 GHz, 2.30 GHz, 2.20 GHz, 2.10 GHz, 2.00 G
Hz, 1.90 GHz, 1.80 GHz, 1.70 GHz, 1.60 GHz, 1.50 GHz, 1.40 GHz, 1.30 GHz, 1.20 GHz, 1.10 GHz, 1000 MHz, 900 MHz, 800 MHz
  available cpufreq governors: conservative ondemand userspace powersave performance schedutil
  current policy: frequency should be within 800 MHz and 3.00 GHz.
                  The governor "schedutil" may decide which speed to use
                  within this range.
  current CPU frequency: 2.20 GHz (asserted by call to kernel)
  boost state support:
    Supported: yes
    Active: yes
```

此时，governor 自动切换为 schedutil
```txt
🤒  cd /sys/devices/system/cpu/cpufreq/policy17/
scaling_driver:acpi-cpufreq
scaling_governor:schedutil
```

结果为:
```txt
/usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=1
events per second:     7.14
```
作为对比，如果使用 intel-pstate
```txt
    events per second:     8.41
```
多次测试可复现的结果。

## intel pstate 原理
https://access.redhat.com/solutions/2895271

这段是什么意思
In case of passive mode the intel_pstate behaves like a traditional
cpufreq driver and doesn't implement or force its internal governors
and uses cpufreq generic governors (like "performance", "ondemand", "powersave").

不过最后的 root cause 就非常诡异了

## nutanix 关于联想服务器的配置
https://portal.nutanix.com/page/documents/kbs/details?targetId=kA00e000000btAuCAI

> Lenovo ThinkAgile HX hardware based on Intel x86 architecture may show poor Controller VM (CVM) and user VM application performance when processor power management (P-State) in BIOS is configured with some of the non-default modes on Nutanix AHV hosts.

P-State Control
P-states provide a way to scale the frequency and voltage at which the processor runs with the goal of reducing the power consumption of the CPU. The number of available P-states a processor can operate can be different for each model of CPU even for different models of the same CPU family.

The default setting for the P-State Control is None (disabled). However, there have been some instances found in the field where it is configured in a different mode.

The P-state control modes available on a Lenovo HX system are the following:

- Autonomous
	- This mode is part of Intel’s Hardware Power Management (HWPM) feature and is the default in ThinkSystem servers. With the Autonomous selection, the P-states are completely controlled by system hardware. From the operating system's perspective, there are no P-states and the OS always thinks the processor is running at its rated frequency. The processor’s efficiency is totally dependent on Intel’s hardware management engine (ME) implementation.
- Legacy
	- When Legacy is selected, the processor P-states will be presented to the OS and the OS power management (OSPM) will directly control which P-state is selected. Understanding P-State Control on Intel Xeon Scalable Processors to Maximize Energy Efficiency provides slightly higher latency than Autonomous since its desired frequency is calculated by the OS. It is also useful on older OS's that do not support Cooperative mode and older hardware without HWPM support.
- Cooperative
	- Cooperative mode is also part of Intel’s HWPM feature. Cooperative mode is a combination of Autonomous and Legacy modes. Like Autonomous mode, the P-states are still controlled by the system hardware. However, in Cooperative mode, the OS is aware that different processor operating frequencies exist and it can provide hints to the system hardware for the minimum, desired, and maximum P-states. However, the system hardware does not always have to honor the OS-requested P-state. The final P-state chosen is influenced by the Power/Performance Bias setting, which is configurable under the Operating Modes menu.
- None (default)
	- When P-states are set to None, P-states are completely disabled and the processors run at either their rated frequency or in turbo mode (if Turbo Boost is enabled). None is the best choice when the absolute lowest latency and highest performance are desired. The trade-off is that power consumption is higher.

AHV 中，当联想的机器配置为 : Expected output with P-States configured with None (default), Cooperative and Legacy 的时候，

pstate driver 可以正常加载。
```txt
cpupower frequency-info
analyzing CPU 0:
  driver: intel_pstate
```

Autonomous:
```txt
[root@host]# cpupower frequency-info
analyzing CPU 0:
```

The command turbostat can be used to verify the frequency at which the CPU is operating (TSC_MHz indicates the clock speed while Bzy_MHz indicates the actual current clock speed).
In the case of the problematic setting of Cooperative or Autonomous leads into low clock speed, reducing the amount of IOPS and increasing latency - in this example ranging between 1000 and 1500 MHZ while the expectation is for the speed to be running at least at TSC_MHz:

## 为什么 Hygon 中没有任何 pstate driver

### acpi-cpufreq
```
ACPI Processor 枚举
    |
    +-- CPU 下存在 _PCT
            |
            +-- cpufreq_add_device("acpi-cpufreq")
                    |
                    +-- 创建 acpi-cpufreq platform device
                            |
                            +-- MODULE_ALIAS("platform:acpi-cpufreq")
                                    |
                                    +-- 自动加载 acpi-cpufreq.ko
```

Hygon ACPI 表中 `_PCT = 0`，所以流程在第一步就断了：

### amd-pstate

源码有明确 vendor 限制（drivers/cpufreq/amd-pstate.c）：

```c
static int __init amd_pstate_init(void)
{
	struct device *dev_root;
	int ret;

	if (boot_cpu_data.x86_vendor != X86_VENDOR_AMD)
		return -ENODEV;
```

Hygon 是 `HygonGenuine / X86_VENDOR_HYGON`，不是 `X86_VENDOR_AMD`；

## pstate 的参与过程
CPU 处于 C0，即正在执行指令时，也可以有不同频率；C1、C6 等表示空闲状态，并不表示几个不同的频率档位。内核 CPUIdle 文档
(https://docs.kernel.org/admin-guide/pm/cpuidle.html)

schedutil 的调频过程可以理解为：

任务入队、出队、调度 tick 等事件
    ↓
调度器更新 CPU 利用率，并调用 cpufreq_update_util()
    ↓
schedutil 的 sugov_update_*() 回调
    ↓
根据利用率计算所需频率或性能水平
    ↓
CPUFreq 驱动向硬件提出请求
    ↓
硬件调整运行频率

对于普通任务，利用率主要来自 PELT 等调度器统计。经典的频率计算可以简化为：

目标频率 ≈ 1.25 × 参考频率 × util / capacity

假设参考频率为 4 GHz，归一化利用率为 40%，算出的目标就是约 2 GHz。实际还会受到最小/最大频率、支持的频率档位、uclamp、I/O wakeup boost
和调频速率限制等影响；硬件实际频率也可能受温度、功耗限制。内核 CPUFreq 文档中的 schedutil 说明
(https://docs.kernel.org/admin-guide/pm/cpufreq.html#schedutil)

在本地源码 kernel/sched/cpufreq_schedutil.c (/home/martins3/data/kernel/t2/kernel/sched/cpufreq_schedutil.c) 中：

• sugov_start()：通过 cpufreq_add_update_util_hook() 注册调度器回调。
• sugov_get_util()、get_next_freq()：获取利用率、计算目标频率。
• 支持快速切换时：调用 cpufreq_driver_fast_switch()。
• 否则：由 sugov_work() 调用 __cpufreq_driver_target()。
• 某些驱动支持直接提交性能需求，走 cpufreq_driver_adjust_perf()。

## 那么这个请求到底是从那里来的?
在常见的 x86 上，最终通常执行 WRMSR 指令，写 CPU 的性能控制 MSR。具体写哪个，取决于驱动以及是否启用 HWP/CPPC。

 驱动 / 硬件路径                           写入的寄存器                         写入内容
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Intel 传统 P-state                        IA32_PERF_CTL，地址 0x199            目标 P-state，常见编码是目标倍频
────────────────────────────────────────  ───────────────────────────────────  ──────────────────────────────────
 Intel HWP                                 IA32_HWP_REQUEST，地址 0x774         最小、最大、期望性能及能效偏好
────────────────────────────────────────  ───────────────────────────────────  ──────────────────────────────────
 AMD/Hygon 的传统 acpi-cpufreq MSR 路径    MSR_AMD_PERF_CTL，地址 0xC0010062    目标 P-state 编号
────────────────────────────────────────  ───────────────────────────────────  ──────────────────────────────────
 AMD amd-pstate 的 CPPC MSR 路径           MSR_AMD_CPPC_REQ，地址 0xC00102B3    最小、最大、期望性能及能效偏好

这些地址和字段定义可以在 arch/x86/include/asm/msr-index.h (/home/martins3/data/kernel/t2/arch/x86/include/asm/msr-index.h) 中找到。

以 schedutil → Intel 驱动 为例，源码路径非常直接。 使用通用 governor 时，Intel 驱动工作在 passive 模式，sysfs 名称为 intel_cpufreq。Intel 驱动文档
(https://docs.kernel.org/admin-guide/pm/intel_pstate.html#passive-mode)

在 drivers/cpufreq/intel_pstate.c (/home/martins3/data/kernel/t2/drivers/cpufreq/intel_pstate.c) 中，快速调频路径为：

sugov_update_single_freq()
  → cpufreq_driver_fast_switch()
    → intel_cpufreq_fast_switch()
      → intel_cpufreq_update_pstate()
          ├─ 未启用 HWP：intel_cpufreq_perf_ctl_update()
          │               → wrmsrl(MSR_IA32_PERF_CTL, value)
          │
          └─ 启用 HWP：intel_cpufreq_hwp_update()
                          → wrmsrl(MSR_HWP_REQUEST, value)

wrmsrl() 最终通过 x86 的 WRMSR 指令写 MSR；wrmsrl_on_cpu() 则安排在指定逻辑 CPU 上执行写入。

对于传统 Intel 路径，常见的目标倍频编码是：

value = (u64)pstate << 8;

例如，在参考时钟为 100 MHz、采用该倍频编码的型号上，请求倍频 30 对应约 3 GHz，倍频字段就是 30 << 8。完整寄存器值还可能包含其他控制位，编码也需对应具体
型号。

HWP 则提交“性能需求”，由处理器内部控制器决定实际频率。 IA32_HWP_REQUEST 的主要字段为：

bits  7:0   Minimum Performance
bits 15:8   Maximum Performance
bits 23:16  Desired Performance
bits 31:24  Energy Performance Preference

这里的 performance 是硬件定义的性能刻度，并非直接填写 MHz。硬件结合请求、功耗和温度条件选择运行点。Intel HWP 文档
(https://docs.kernel.org/admin-guide/pm/intel_pstate.html#active-mode-with-hwp)

还有一种例外：acpi-cpufreq 可以通过 ACPI _PCT 指定的 I/O 端口控制频率。对应 drivers/cpufreq/acpi-cpufreq.c (/home/martins3/data/kernel/t2/drivers/
cpufreq/acpi-cpufreq.c) 中的 cpu_freq_write_io()：

acpi_os_write_port(reg->address, val, reg->bit_width);

所以，准确的理解是：驱动写入硬件提供的性能请求接口，CPU 内部电源管理逻辑再完成时钟、电压调整；这里没有选择 C-state 的步骤。


## 为什么 acpi-cpufreq 和 intel-pstate 两个模块是如何互斥的

首先，intel-pstate 是built-in 的:

 配置                       类型        意义
━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 CONFIG_X86_INTEL_PSTATE    bool        只能内建，不能编译为模块
─────────────────────────  ──────────  ────────────────────────────
 CONFIG_X86_ACPI_CPUFREQ    tristate    可以内建，也可以编译为模块


ACPI 驱动还会主动检查，发现位置已占用就返回

drivers/cpufreq/acpi-cpufreq.c 中，基本的调用流程为:

acpi_cpufreq_init()
  → platform_driver_probe()
    → acpi_cpufreq_probe()

其中 acpi_cpufreq_probe() 很早就检查：

```txt
/* don't keep reloading if cpufreq_driver exists */
if (cpufreq_get_current_driver())
return -ENODEV;
```

## 启用 acpi-cpufreq 的前提
依赖，ACPI table:
- `_PCT`：Processor Control，告诉 OS 用什么寄存器控制和读取 P-state。
- `_PSS`：Performance Supported States，提供频率、功耗、切换延迟、control/status value 等状态列表。
- `_PPC`：可选的平台性能上限，通常需要结合 `_PSS` 使用。

acpi-cpufreq 基本依赖：

```
_PCT + _PSS
```

缺失时通常无法注册 policy。

## intel_pstate 和 acpi 的关系

可以，而且这是正常情况。

intel_pstate 和 acpi-cpufreq 的控制来源不同：

```
acpi-cpufreq:
    依赖 BIOS ACPI _PCT/_PSS
    根据 ACPI 表控制 P-state

intel_pstate/intel_cpufreq:
    直接使用 Intel CPU MSR、HWP、APERF/MPERF
    不要求 BIOS 提供 _PCT/_PSS
```

### intel_pstate 的主要前提

非 HWP 模式主要要求：

- Intel CPU family/model 在支持列表中
- CPU 暴露 APERF/MPERF
- 相关 P-state MSR 有效
- 没有 `intel_pstate=disable`
- 平台没有声明由固件进行 out-of-band power management

HWP 模式主要使用：

- `MSR_PM_ENABLE`
- `MSR_HWP_REQUEST`
- HWP/EPP CPU feature

这些都不依赖 `_PCT`/`_PSS`。

## intel pstate 如何反向影响 cpufreq governor

写 scaling_governor
        |
        +-- intel_pstate（主动模式）
        |      -> 驱动内置 policy
        |
        +-- intel_cpufreq（被动模式）
               -> 通用 cpufreq governor

源码里有两套同名的 performance/powersave ，到底走哪套，由 scaling_driver 决定。

### 主动模式：scaling_driver=intel_pstate

主动模式只提供： performance powersave

它们不是 struct cpufreq_governor，而是 CPUFreq core 内置的两个 policy 编号：

```txt
#define CPUFREQ_POLICY_POWERSAVE   1
#define CPUFREQ_POLICY_PERFORMANCE 2
```

定义在 include/linux/cpufreq.h:587。

调用路径：

echo performance > scaling_governor
  -> store_scaling_governor()
  -> cpufreq_parse_policy()
  -> cpufreq_set_policy()
  -> intel_pstate_set_policy()

对应代码：

- 字符串转换：cpufreq_parse_policy() (drivers/cpufreq/cpufreq.c:653)
- sysfs 写入处理：store_scaling_governor() (drivers/cpufreq/cpufreq.c:796)
- 调用驱动 setpolicy()：cpufreq_set_policy() (drivers/cpufreq/cpufreq.c:2688)
- Intel 实际策略入口：intel_pstate_set_policy() (drivers/cpufreq/intel_pstate.c:2887)
- 驱动注册：struct cpufreq_driver intel_pstate (drivers/cpufreq/intel_pstate.c:3085)

无 HWP 时：

- performance 在 intel_pstate_set_policy() 中直接选择 max_perf_ratio
- powersave 注册 scheduler utilization hook
- 动态目标由 get_target_pstate() (drivers/cpufreq/intel_pstate.c:2534) 计算
- 最后由 intel_pstate_adjust_pstate() (drivers/cpufreq/intel_pstate.c:2586) 写入 P-state

有 HWP 时，两种策略最终进入 intel_pstate_hwp_set() (drivers/cpufreq/intel_pstate.c:1261)：

```txt
if (policy == PERFORMANCE)
        min = max;       /* 性能范围锁到上界 */
...
if (policy == PERFORMANCE)
        epp = 0;         /* 完全倾向性能 */
```

然后写 MSR_HWP_REQUEST。

### 被动模式：scaling_driver=intel_cpufreq

被动模式使用真正的通用 governor。

performance 的全部核心逻辑只有：

__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);

代码在 drivers/cpufreq/cpufreq_performance.c:14。

powersave 则是：

__cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);

代码在 drivers/cpufreq/cpufreq_powersave.c:14。

目标频率交给 intel_cpufreq：

- 普通切换：intel_cpufreq_target() (drivers/cpufreq/intel_pstate.c:3207)
- 快速切换：intel_cpufreq_fast_switch() (drivers/cpufreq/intel_pstate.c:3230)
- 驱动注册：struct cpufreq_driver intel_cpufreq (drivers/cpufreq/intel_pstate.c:3396)

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
