# cstate pstate 相关的工具

## pcm

## turbostat
Package       Core    CPU     Bzy_MHz

man turbostat

Bzy_MHz average clock rate while the CPU was not idle (ie. in "c0" state).

似乎只是通过 mperf 寄存器获取的而已

例如如果想要仅仅展示 Bzy_MHz 和 TSC_MHz ，可以这么观察
```txt
sudo turbostat --show CPU,Bzy_MHz,TSC_MHz
```
在 13900K 中观察，Bzy_MHz 就是实际频率（无负载的时候很低，有负载的时候达到 5500 MHz），TSC_MHz 就是稳定的。

## sudo acpi -t
这个命令可以看温度

## acpitool

## i7z
比想象的好用，类似 pcm 了

```txt
Socket [0] - [physical cores=24, logical cores=32, max online cores ever=24]
  TURBO ENABLED on 24 Cores, Hyper Threading ON
  Max Frequency without considering Turbo 3094.83 MHz (99.83 x [31])
  Max TURBO Multiplier (if Enabled) with 1/2/3/4/5/6 Cores is  58x/58x/55x/55x/55x/55x
  Real Current Frequency 2098.69 MHz [99.83 x 21.02] (Max of below)
        Core [core-id]  :Actual Freq (Mult.)      C0%   Halt(C1)%  C3 %   C6 %  Temp      VCore
        Core 1 [0]:       1097.91 (11.00x)      1.52    98.1       0    1.37    22      0.7667
        Core 2 [2]:       1097.05 (10.99x)      2.16    92.1       0    7.09    22      0.7667
        Core 3 [4]:       1097.66 (10.99x)      1.18    98.6       0       1    24      0.7667
        Core 4 [6]:       1098.50 (11.00x)      1.34    97.6       0    1.88    20      0.7667
        Core 5 [8]:       1097.40 (10.99x)      1.56    97.2       0    2.25    23      0.7672
        Core 6 [10]:      1097.82 (11.00x)       2.5    89.8       0    9.31    24      0.7672
        Core 7 [12]:      1096.51 (10.98x)      1.81    95.4       0    3.93    24      0.7672
        Core 8 [14]:      1098.05 (11.00x)      1.05    96.9       0    2.75    20      0.7722
        Core 9 [16]:      798.54 (8.00x)           1    0.261      0    99.7    27      0.7722
        Core 10 [17]:     801.90 (8.03x)           1    0.0683     0    99.9    27      0.7722
        Core 11 [18]:     799.80 (8.01x)           1    0.0683     0    99.9    27      0.7722
        Core 12 [19]:     782.84 (7.84x)           1    0.0837     0    99.9    27      0.7722
        Core 13 [20]:     802.24 (8.04x)           1    0.0899     0    99.9    27      0.7722
        Core 14 [21]:     790.34 (7.92x)           1    0.112      0    99.9    27      0.7722
        Core 15 [22]:     787.87 (7.89x)           1    0.0848     0    99.9    27      0.7717
        Core 16 [23]:     801.55 (8.03x)           1    0.162      0    99.8    27      0.7667
        Core 17 [24]:     1538.17 (15.41x)         1    5.07       0    94.6    24      0.7717
        Core 18 [25]:     1557.41 (15.60x)         1    4.16       0    95.7    24      0.7717
        Core 19 [26]:     1313.84 (13.16x)         1    3.25       0    96.6    24      0.7717
        Core 20 [27]:     2098.69 (21.02x)         1    3.46       0    96.4    24      0.7717
        Core 21 [28]:     1303.24 (13.05x)         1    4.71       0      95    25      0.7717
        Core 22 [29]:     1840.79 (18.44x)         1       3       0    96.7    25      0.7717
        Core 23 [30]:     1502.66 (15.05x)         1    3.39       0    96.5    25      0.7717
        Core 24 [31]:     1239.85 (12.42x)         1    3.56       0    96.2    25      0.7720
```

## powertop

观察到底那个程序用的功耗高

不知道是如何实现的:
```txt
Summary: 553.4 wakeups/second,  0.0 GPU ops/seconds, 0.0 VFS ops/sec and 8.4% CPU use

                Usage       Events/s    Category       Description
              3.9 ms/s     252.7        Timer          tick_nohz_handler
              3.1 ms/s      92.1        Interrupt      [3] net_rx(softirq)
              0.8 ms/s      50.9        kWork          flush_to_ldisc
              2.8 ms/s      39.2        Interrupt      [7] sched(softirq)
              5.6 ms/s      36.2        Process        [PID 80729] viddy cat /proc/cpuinfo | grep MHz
            198.3   s/s  13.7        kWork          console_callback
            492.2   s/s   9.8        Process        [PID 459] [xfsaild/nvme0n1]
             46.5   s/s   9.8        kWork          toggle_allocation_gate
             23.1 ms/s      0.00        Process        [PID 80726] viddy cat /proc/cpuinfo | grep MHz
             54.7   s/s   6.9        Timer          inactive_task_timer
              4.5 ms/s       4.9        Process        [PID 41771] tmux
            319.4   s/s   5.9        Process        [PID 18] [rcu_preempt]
            317.8   s/s   5.9        kWork          handle_update
             50.9   s/s   4.9        kWork          psi_avgs_work
             90.9   s/s   3.9        Process        [PID 629] [irq/143-iwlwifi]
              6.2 ms/s       1.0        Process        [PID 524508] tokio-runtime-w
              4.4 ms/s       1.0        Process        [PID 524481] powertop
              4.2 ms/s       1.0        Process        [PID 524510] sh
```

## cpupower

cpupower idle-info

```txt
sudo cpupower frequency-info -g governor
sudo cpupower frequency-set -g performance
```
实际上就是去设置:
- /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
- /sys/devices/system/cpu/cpu0/acpi_cppc/

### msr ?
这个输出的含义还是需要理解的，这些数据都是从什么地方获取的?

是通过读取 msr 空间的吗?
```txt
sudo cpupower frequency-info
modprobe: FATAL: Module msr not found in directory /lib/modules/6.13.2
analyzing CPU 2:
  driver: intel_pstate
  CPUs which run at the same hardware frequency: 2
  CPUs which need to have their fr
```

### cpupower monitor
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
