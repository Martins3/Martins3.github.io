# 分析一些基本问题



## 也许应该在 aarch
1. 中断
2. simd

## simd
这个做什么的:
- [ ] linux/arch/arm64/kernel/fpsimd.c

```txt
[    0.212919] raid6: neonx8   gen() 14775 MB/s
[    0.281570] raid6: neonx4   gen() 35977 MB/s
[    0.350283] raid6: neonx2   gen() 40227 MB/s
[    0.418895] raid6: neonx1   gen() 35834 MB/s
[    0.487634] raid6: int64x8  gen() 15134 MB/s
[    0.556448] raid6: int64x4  gen() 14247 MB/s
[    0.625161] raid6: int64x2  gen() 12924 MB/s
[    0.693917] raid6: int64x1  gen() 11379 MB/s
[    0.694151] raid6: using algorithm neonx2 gen() 40227 MB/s
[    0.762664] raid6: .... xor() 31377 MB/s, rmw enabled
[    0.762924] raid6: using neon recovery algorithm
```

## 读读
- https://www.kernel.org/doc/html/latest/arch/arm64/index.html

### https://www.kernel.org/doc/html/latest/arm64/memory.html
  - 这个 memory 分布中，其 PCI mmio 的大小相比 x86 小几个数量级了

## entry

### syscall 的上下文保存

mac 物理机上
```txt
@[
    vfs_read+0
    __arm64_sys_read+36
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 125
```

虚拟机中观测到的:
```txt
- ??
  - el0t_64_sync
    - el0t_64_sync_handler
      - el0_svc
        - do_el0_svc
          - el0_svc_common
            - invoke_syscall
              - __invoke_syscall
                - __arm64_sys_read
                  - __se_sys_read
                    - __do_sys_read
                      - ksys_read
                        - vfs_read
```
### kvm 的切入和切出
有点难以观测

### 中断的上下文

在 ARM 的虚拟机中观测到的是
```txt
- el1h_64_irq
  - el1h_64_irq_handler
    - el1_interrupt
      - __el1_irq
        - irq_enter_rcu
          - tick_irq_enter
            - tick_nohz_irq_enter
              - ktime_get
                - timekeeping_get_ns
                  - timekeeping_get_delta
                    - tk_clock_read
                      - arch_counter_read
```

在 mac 中直接观测到的是:
```txt
@[
    arch_counter_read+0
    tick_irq_enter+96
    irq_enter_rcu+152
    el1_interrupt+40
    el1h_64_fiq_handler+24
    el1h_64_fiq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 459
```

## TODO
1. 感觉 arch/aarch64 下多了好多的代码，可以按照 diff 的行来排序一下

## arm 也是可以看频率的

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

代码分析:

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

### 虚拟机中 /sys/devices/system/cpu/cpufreq/ 这个目录是空的

### 问题
可以看温度吗? 似乎至少可以看电量的，这个非常不错的。

## 如何给 arm 的虚拟机注入一个 cache 的错误

```txt
[469.836076] {1}[Hardware Error]: Hardware error from APEI Generic Hardware Error Source: 9
[469.852780] {1}[Hardware Error]: event severity: recoverable
[469.862966] {1}[Hardware Error]:  Error 0, type: recoverable
[469.873206] {1}[Hardware Error]:   section_type: ARM processor error
[469.884267] {1}[Hardware Error]:   MIDR: 0x00000000481fd010
[469.894641] {1}[Hardware Error]:   Multiprocessor Affinity Register (MPIDR): 0x00000000810b0100
[469.913323] {1}[Hardware Error]:   error affinity level: 0
[469.924313] {1}[Hardware Error]:   running state: 0x1
[469.934815] {1}[Hardware Error]:   Power State Coordination Interface state: 0
[469.953129] {1}[Hardware Error]:   Error info structure 0:
[469.964350] {1}[Hardware Error]:   num errors: 1
[469.974606] {1}[Hardware Error]:    error_type: 0, cache error
[469.986055] {1}[Hardware Error]:    error_info: 0x0000000024400014
[469.997816] {1}[Hardware Error]:     cache level: 1
[470.008195] {1}[Hardware Error]:     the error has been corrected
[470.019785] {1}[Hardware Error]:    virtual fault address: 0x0000000000000000
[470.037840] {1}[Hardware Error]:    physical fault address: 0x000000917cabe738
[470.056404] {1}[Hardware Error]:   Vendor specific error info has 16 bytes:
[470.069368] {1}[Hardware Error]:    00000000: 00000000 00000000 00000000 00000000  ................
[470.090263] UCE: kernel recovery 0, qemu-kvm-2634199 is user-thread.
[470.103166] Internal error: Uncorrected hardware memory error in kernel-access : 96000610 [#1] SMP
```

## 那么 ARM 中各种 EL 是不是就是 x86 中的 SMM

可以简单这么理解，但是不是一个问题。

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
