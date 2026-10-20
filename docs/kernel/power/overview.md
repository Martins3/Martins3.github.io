# cstate 和 pstate 基础

- cpuidle core
- cpuidle governors
- cpuidle drivers

pstate 机制也是类似

## idle=poll 和 hyperthread 不可混用
<!-- ee82bb83-e17e-4e7e-b3d1-3ef717fc85be -->

arch/x86/kernel/process.c
```txt
	pr_warn_once("WARNING: polling idle and HT enabled, performance may degrade\n");
```
细节参考:
https://access.redhat.com/solutions/792343

## echo b > /proc/sysrq-trigger 和 reboot 命令的区别
/proc/sysrq-trigger 是内核提供的控制接口。写入字符会调用对应的内核处理函数，b 对应重启。当前源码中的调用链是：

写入 /proc/sysrq-trigger
  → write_sysrq_trigger()
  → __handle_sysrq('b', false)
  → sysrq_handle_reboot()
  → emergency_restart()
  → machine_emergency_restart()
  → 平台相关的复位操作

入口见 sysrq.c (drivers/tty/sysrq.c:1206)，b 的处理函数见 sysrq_handle_reboot() (drivers/tty/sysrq.c:163)。

它不等待服务退出，不同步文件系统，不卸载磁盘，也不执行正常的设备关闭流程，直接进入紧急重启路径。当前版本会尝试保存内核
日志、刷新控制台，但这不等于同步文件数据。见 emergency_restart() (kernel/reboot.c:94)。

最后如何复位取决于架构和平台。例如 x86 可以通过 ACPI、EFI、复位端口等机制完成，失败时还可能尝试其他方式。见 x86 重启实
现 (arch/x86/kernel/reboot.c:580)。

普通 reboot 多做了什么？

以常见的 systemd 系统、普通硬重启为例，大致流程是：

reboot 命令
  → 请求 systemd 关机重启
  → 停止服务、结束进程
  → 同步数据、卸载文件系统或重新挂载为只读
  → reboot() 系统调用
  → kernel_restart()
      → 调用 reboot notifier
      → device_shutdown()：执行设备驱动的 shutdown 回调
      → 迁移到执行重启的 CPU
      → syscore_shutdown()
      → machine_restart()
  → 平台相关的复位操作

用户态行为参见 systemd 文档 (https://www.freedesktop.org/software/systemd/man/256/systemctl.html)，内核部分见
kernel_restart() (kernel/reboot.c:297)。

## asahi linux 的 cstate 和 pstate driver

asahi linux 也是用的 arm ，实际上我们看到，就是 linux 和

```txt
# sudo cpupower idle-info

CPUidle driver: apple_idle
CPUidle governor: menu
analyzing CPU 6:

Number of idle states: 2
Available idle states: WFI CPU PD
WFI:
Flags/Description: CPU clock-gated
Latency: 1
Residency: 1
Usage: 30864
Duration: 5411280
CPU PD:
Flags/Description: CPU/cluster powered down
Latency: 10
Residency: 10000
Usage: 2438
Duration: 292304740
```

```txt
# sudo cpupower frequency-info

analyzing CPU 6:
  driver: apple-cpufreq
  CPUs which run at the same hardware frequency: 4 5 6 7
  CPUs which need to have their frequency coordinated by software: 4 5 6 7
  maximum transition latency: 62.0 us
  hardware limits: 660 MHz - 3.20 GHz
  available frequency steps:  660 MHz, 924 MHz, 1.19 GHz, 1.45 GHz, 1.70 GHz, 1.97 GHz, 2.21 GHz, 2.40 GHz, 2.57 GHz, 2.72 GHz, 2.87 GHz, 2.99 GHz, 3.10 GHz, 3.20 GHz
  available cpufreq governors: conservative ondemand userspace powersave performance schedutil
  current policy: frequency should be within 660 MHz and 3.20 GHz.
                  The governor "schedutil" may decide which speed to use
                  within this range.
  current CPU frequency: 3.20 GHz (asserted by call to hardware)
  boost state support:
    Active: no
  boost frequency steps: 3.32 GHz, 3.41 GHz, 3.50 GHz
```

https://github.com/AsahiLinux/linux/blob/asahi/drivers/cpuidle/cpuidle-apple.c

所以，可以确定的是，arm 也是都是使用类似的体系的。

## sudo dmidecode -t processor | grep -i speed 的含义是什么

显然这都是 smbios 的结果，
感觉实际上意义不是很大

   字段             SMBIOS 定义
  ━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   Max Speed        系统对此处理器插槽支持的最大频率，描述的是系统能力，不一定等于当前 CPU 的最高睿频
  ───────────────  ───────────────────────────────────────────────────────────────────────────────────
   Current Speed    系统启动时的处理器频率，不是执行命令时的实时频率



13900K 的
```txt
🧀  sudo dmidecode -t processor | grep -i speed
[sudo] password for martins3:
        Max Speed: 5800 MHz
        Current Speed: 2970 MHz
```

INTEL(R) XEON(R) GOLD 6530
```txt
sudo dmidecode -t processor | grep -i speed
        Max Speed: 4000 MHz
        Current Speed: 2100 MHz
        Max Speed: 4000 MHz
        Current Speed: 2100 MHz
```

Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz (67.88
```txt
sudo dmidecode -t processor | grep -i speed
[sudo] password for martins3:
        Max Speed: 4000 MHz
        Current Speed: 2300 MHz
        Max Speed: 4000 MHz
        Current Speed: 2300 MHz
```

```txt
      1 cpu MHz         : 999
      1 cpu MHz         : 1000
      1 cpu MHz         : 1672
      1 cpu MHz         : 1707
      1 cpu MHz         : 1794
      1 cpu MHz         : 2411
      1 cpu MHz         : 2712
      1 cpu MHz         : 2799
      3 cpu MHz         : 2800
     53 cpu MHz         : 3900
```

小米笔记本:
Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz
```txt
🧀  sudo dmidecode -t processor | grep -i speed
[sudo] password for martins3:
        Max Speed: 8300 MHz
        Current Speed: 1700 MHz
```
看来，就是厂商瞎写也无所谓的

n100 小主机:
```txt
🧀  sudo dmidecode -t processor | grep -i speed
[sudo] password for martins3:
        Max Speed: 3400 MHz
        Current Speed: 2871 MHz
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
