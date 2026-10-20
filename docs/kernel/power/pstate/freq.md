# 如何获取到 CPU 频率

我们知道通过 /proc/cpuinfo 来获取到 CPU 的频率，但是 sysfs 也是可以的

## 基本观察
kunpeng 机器上
```txt
sudo cat /sys/devices/system/cpu/cpufreq/policy*/cpuinfo_cur_freq
2400000
2400000
2400000
2400000
```

在 mac 中测试，如果是有高负载:

```txt
cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq

2424000
3204000
```
如果没有负载
```txt
cat /sys/devices/system/cpu/cpufreq/policy*/cpuinfo_cur_freq

600000
2208000
```

x86 也可以用这个方法:

cat /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq

aarch64 中，这两个输出相同:
- scaling_cur_freq
- cpuinfo_cur_freq

```txt
 4)               |    cpufreq_verify_current_freq() {
 4)               |      apple_soc_cpufreq_get_rate [apple_soc_cpufreq]() {
 4)   0.167 us    |        cpufreq_cpu_get_raw();
 4)   1.000 us    |      }
 4)   1.291 us    |    }
```
直接到这个文件中看，: drivers/cpufreq/apple-soc-cpufreq.c

先到此为止了，到时候整理一下吧。

所以，到时候，

在 13900k 中
```txt
cat /proc/cpuinfo| grep MHz
```
的输出的 /sys 中输出相同吗，是一个来源吗？


## 原理
ARM64：主要有三类实现。

 驱动/平台       .get 最终访问什么                         返回值如何产生
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 cpufreq-dt      Common Clock Framework：clk_get_rate()    从时钟框架取得 CPU 时钟频率
──────────────  ────────────────────────────────────────  ───────────────────────────────────────────
 scmi-cpufreq    SCMI 固件接口                             获取 performance level，再转换为频率
──────────────  ────────────────────────────────────────  ───────────────────────────────────────────
 cppc_cpufreq    ACPI CPPC feedback counters               用 delivered/reference 计数器差值推算频率

具体来说：

1. 设备树 + 时钟框架

   cpufreq_generic_get()
     → clk_get_rate(policy->clk) / 1000

   SoC 时钟驱动可以通过读取 PLL、分频器、时钟选择器等寄存器计算频率。不过，clk_get_rate() 默认允许返回缓存值；设置
   CLK_GET_RATE_NOCACHE 才会触发重新计算，所以读这个文件不保证每次都访问硬件。见 clk.c (drivers/clk/clk.c:2032)。

2. SCMI：向管理固件查询

   scmi_cpufreq_get_rate()
     → perf_ops->freq_get()
     → 获取 performance level
     → 换算为 Hz，再转换为 kHz

   底层可以发送 PERF_LEVEL_GET 消息，也可以用 ioread32() 读取固件提供的 Fast Channel 地址。Linux 获取的是固件报告的性能档位。
   见 perf.c (drivers/firmware/arm_scmi/perf.c:765)。

3. CPPC：读取反馈计数器，可能落到 AMU

   ACPI _CPC 描述 delivered/reference counter 的位置。它们可能位于 MMIO、PCC 共享内存，或者 FFH（架构固定硬件接口）。

   当前 cppc_cpufreq_get_rate() 两次采样之间延迟 2 μs，计算：

   delivered_perf = reference_perf × Δdelivered / Δreference
   频率 = 将 delivered_perf 按 CPPC 能力参数转换为 kHz

   见 cppc_cpufreq.c (drivers/cpufreq/cppc_cpufreq.c:765)。

   ARM64 的 FFH 反馈计数器可以对应 AMU：
    • core counter：SYS_AMEVCNTR0_CORE_EL0
    • constant counter：SYS_AMEVCNTR0_CONST_EL0

   内核在目标 CPU 上通过 MRS 系统寄存器读取指令取得计数。原理类似 x86 APERF/MPERF：

   平均运行频率 ≈ constant counter 的参考频率 × Δcore / Δconstant

   见 topology.c (arch/arm64/kernel/topology.c:28)。AMU 需要硬件和固件支持，并非所有 ARM CPU 都有；当前 CPPC 驱动遇到无效或不
   增长的反馈计数，还可能回退到 desired performance。

因此，判断某台机器上的 cpuinfo_cur_freq，首先看 scaling_driver，然后追它的 .get。
仅凭 x86/ARM 架构，无法确定读的是硬件计数器、 性能档位还是缓存的时钟频率。

## 13900k 上反而没有 cpuinfo_cur_freq
因为 intel_pstate 没实现
  cpuinfo_avg_freq → arch_freq_get_on_cpu() → APERF/MPERF 采样结果
  scaling_cur_freq → arch_freq_get_on_cpu() → APERF/MPERF 采样结果
  cpuinfo_cur_freq → 驱动 .get              → 未实现，不创建文件

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
