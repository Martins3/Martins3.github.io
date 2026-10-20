# doc
- https://docs.kernel.org/admin-guide/pm/index.html
- https://docs.kernel.org/admin-guide/pm/working-state.html
- Documentation/admin-guide/pm/intel_idle.rst

## admin-guide/kernel-parameters.txt 中存在那些和 cstate /pstate 相关的参数

```txt
cpuidle.off=1   [CPU_IDLE]
                    disable the cpuidle sub-system

cpuidle.governor=
                [CPU_IDLE] Name of the cpuidle governor to use.


idle=           [X86]
                    Format: idle=poll, idle=halt, idle=nomwait
                    Poll forces a polling idle loop that can slightly
                    improve the performance of waking up a idle CPU, but
                    will use a lot of power and make the system run hot.
                    Not recommended.
                    idle=halt: Halt is forced to be used for CPU idle.
                    In such case C2/C3 won't be used again.
                    idle=nomwait: Disable mwait for CPU C-states

intel_idle.max_cstate=	[KNL,HW,ACPI,X86]
		0	disables intel_idle and fall back on acpi_idle.
		1 to 9	specify maximum depth of C-state.

processor.max_cstate=	[HW,ACPI]
		Limit processor to maximum C-state
		max_cstate=9 overrides any DMI blacklist limit.

amd_pstate=	[X86,EARLY]
		disable
		  Do not enable amd_pstate as the default
		  scaling driver for the supported processors
		passive
		  Use amd_pstate with passive mode as a scaling driver.
		  In this mode autonomous selection is disabled.
		  Driver requests a desired performance level and platform
		  tries to match the same performance level if it is
		  satisfied by guaranteed performance level.
		active
		  Use amd_pstate_epp driver instance as the scaling driver,
		  driver provides a hint to the hardware if software wants
		  to bias toward performance (0x0) or energy efficiency (0xff)
		  to the CPPC firmware. then CPPC power algorithm will
		  calculate the runtime workload and adjust the realtime cores
		  frequency.
		guided
		  Activate guided autonomous mode. Driver requests minimum and
		  maximum performance level and the platform autonomously
		  selects a performance level in this range and appropriate
		  to the current workload.

amd_prefcore=
		[X86]
		disable
		  Disable amd-pstate preferred core.

intel_pstate=	[X86,EARLY]
		disable
		  Do not enable intel_pstate as the default
		  scaling driver for the supported processors
                active
                  Use intel_pstate driver to bypass the scaling
                  governors layer of cpufreq and provides it own
                  algorithms for p-state selection. There are two
                  P-state selection algorithms provided by
                  intel_pstate in the active mode: powersave and
                  performance.  The way they both operate depends
                  on whether or not the hardware managed P-states
                  (HWP) feature has been enabled in the processor
                  and possibly on the processor model.
		passive
		  Use intel_pstate as a scaling driver, but configure it
		  to work with generic cpufreq governors (instead of
		  enabling its internal governor).  This mode cannot be
		  used along with the hardware-managed P-states (HWP)
		  feature.
		force
		  Enable intel_pstate on systems that prohibit it by default
		  in favor of acpi-cpufreq. Forcing the intel_pstate driver
		  instead of acpi-cpufreq may disable platform features, such
		  as thermal controls and power capping, that rely on ACPI
		  P-States information being indicated to OSPM and therefore
		  should be used with caution. This option does not work with
		  processors that aren't supported by the intel_pstate driver
		  or on platforms that use pcc-cpufreq instead of acpi-cpufreq.
		no_hwp
		  Do not enable hardware P state control (HWP)
		  if available.
		hwp_only
		  Only load intel_pstate on systems which support
		  hardware P state control (HWP) if available.
		support_acpi_ppc
		  Enforce ACPI _PPC performance limits. If the Fixed ACPI
		  Description Table, specifies preferred power management
		  profile as "Enterprise Server" or "Performance Server",
		  then this feature is turned on by default.
		per_cpu_perf_limits
		  Allow per-logical-CPU P-State performance control limits using
		  cpufreq sysfs interface
		no_cas
		  Do not enable capacity-aware scheduling (CAS) on
		  hybrid systems
```
常用的几个:

idle=poll
cpuidle.off=1
intel_pstate=
intel_idle.max_cstate=
processor.max_cstate=

### idle=halt 的作用
<!-- 779f4c98-f009-46e1-89a9-3874eb79bd14 -->

```txt
    idle=           [X86]
                        Format: idle=poll, idle=halt, idle=nomwait
                        Poll forces a polling idle loop that can slightly
                        improve the performance of waking up a idle CPU, but
                        will use a lot of power and make the system run hot.
                        Not recommended.
                        idle=halt: Halt is forced to be used for CPU idle.
                        In such case C2/C3 won't be used again.
                        idle=nomwait: Disable mwait for CPU C-states
```

直接禁用 acpi_idle 了

AMD 的机器上测试，默认:

```txt
🧀  cat /sys/devices/system/cpu/cpuidle/available_governors
menu
🧀  cat /sys/devices/system/cpu/cpuidle/current_driver
acpi_idle
```

修改后
```txt
🧀  cat /sys/devices/system/cpu/cpuidle/available_governors
menu
🧀  cat /sys/devices/system/cpu/cpuidle/current_driver
none
```

既然 idle=halt ，从 sysfs 上已经观察不到了 cstate 的状态了，不过这不重要:

```txt
# sudo cpupower idle-info

CPUidle driver: none
CPUidle governor: menu
analyzing CPU 0:

CPU 0: No idle states
```

CPU 状态:

方法 1 : 使用 sudo cpupower monitor
```txt
 sudo cpupower monitor
              | Nehalem                   || Mperf
 PKG|CORE| CPU| C3   | C6   | PC3  | PC6   || C0   | Cx   | Freq
   0|   0|   0|  0.00|  0.00|  0.00|  0.00||  3.62| 96.38|  2663
   0|   0|  64|  0.00|  0.00|  0.00|  0.00||  0.18| 99.82|  2625
   0|   1|   1|  0.00|  0.00|  0.00|  0.00||  0.11| 99.89|  2557
   0|   1|  65|  0.00|  0.00|  0.00|  0.00||  0.89| 99.11|  2659
   0|   2|   2|  0.00|  0.00|  0.00|  0.00||  0.08| 99.92|  2632
   // ...
```
方法 2 : i7z

方法 3 : pcm
```txt
 Core C-state residencies: C0 (active,non-halted): 0.84 %; C1: 99.16 %; C3: 0.00 %; C6: 0.00 %; C7: 0.00 %;
 Package C-state residencies:  C0: 100.00 %; C2: 0.00 %; C6: 0.00 %;
                             ┌────────────────────────────────────────────────────────────────────────────────┐
 Core    C-state distribution│01111111111111111111111111111111111111111111111111111111111111111111111111111111│
                             └────────────────────────────────────────────────────────────────────────────────┘
                             ┌────────────────────────────────────────────────────────────────────────────────┐
 Package C-state distribution│00000000000000000000000000000000000000000000000000000000000000000000000000000000│
                             └────────────────────────────────────────────────────────────────────────────────┘
```

### 注意 intel_idle.max_cstate=0 是禁用

测试方法:
sudo grubby --update-kernel=ALL --args="processor.max_cstate=1 intel_idle.max_cstate=0"

### intel 上测试

添加参数前:
```txt
🧀  grep . /sys/devices/system/cpu/cpu0/cpuidle/state*/name
/sys/devices/system/cpu/cpu0/cpuidle/state0/name:POLL
/sys/devices/system/cpu/cpu0/cpuidle/state1/name:C1
/sys/devices/system/cpu/cpu0/cpuidle/state2/name:C1E
/sys/devices/system/cpu/cpu0/cpuidle/state3/name:C6
```

添加参数之后:
```txt
🤒  grep . /sys/devices/system/cpu/cpu0/cpuidle/state*/name
/sys/devices/system/cpu/cpu0/cpuidle/state0/name:POLL
/sys/devices/system/cpu/cpu0/cpuidle/state1/name:C1

🧀  grep . -r /sys/devices/system/cpu/cpuidle

/sys/devices/system/cpu/cpuidle/current_governor_ro:menu
/sys/devices/system/cpu/cpuidle/current_driver:acpi_idle
/sys/devices/system/cpu/cpuidle/available_governors:ladder menu
/sys/devices/system/cpu/cpuidle/current_governor:menu
```
这个结果是符合参数的语义的，最好，就仅仅存在 C1 了，不过
可以注意到一个非常类似的语义，这里的 cstate driver 被切换 acpi_idle 了。

## 模块参数
```txt
🧀  pwd
/sys/module/cpuidle/parameters
🧀  ls
governor  off
```

## 其他参考
- 相当好:
    - https://stackoverflow.com/questions/57471862/uses-of-the-monitor-mwait-instructions :
		- [ ] 超线程中，如果使用 mwait 指令将一个 SMT core 设置为 C7，另一个 SMT 不是 C7，可以接受吗？
		- hlt which just goes into the lightest sleep
		- but user-space versions umonitor / umwait were added recently to Tremont (next-gen Goldmont atom)
		- The idle kernel parameter is used, which takes one of the following values: poll, halt, nomwait.
	- https://arthurchiao.art/blog/linux-cpu-1-zh/ : 5 篇 blog ， 查漏补缺，相当不错

- 一般:
    - https://vstinner.github.io/intel-cpus.html

- 差:
    - [深入理解 Linux 电源管理](https://mp.weixin.qq.com/s/3G-H_pxnvtUjqeAXR7JUXQ)

- https://metebalci.com/blog/a-minimum-complete-tutorial-of-cpu-power-management-c-states-and-p-states/
	- 难度比较大，分析的非常深入

> HW-controlled P-states
> In this method, OS is aware of the hardware support for the control P-states and it makes a request specifying only workload requirements, no specific P-state or frequency is requested. Based on the hints given by OS, and many other factors and constraints, a P-state is selected by the hardware.
>
> I am going to say more about this in another post, but just to give you an idea, my current Linux desktop operates in this mode, which I understand by checking IA32_PM_ENABLE, and the maximum (non-guaranteed) performance level is 39, and the lowest performance level is 1. This roughly means there are 39 different P-states. At the moment, 39 is requested by the OS both as minimum and maximum performance and the preference is on performance, these are all because I have disabled the dynamic CPU frequency change in the kernel.

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
