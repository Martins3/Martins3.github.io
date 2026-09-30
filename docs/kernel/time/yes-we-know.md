## CLOCK_MONOTONIC 和 CLOCK_MONOTONIC_RAW 的区别在于

CLOCK_MONOTONIC 的频率可以被调整，但是 CLOCK_MONOTONIC_RAW 完全不会:
https://stackoverflow.com/questions/14270300/what-is-the-difference-between-clock-monotonic-clock-monotonic-raw

## tk_data 是一个唯一全局变量，timekeeper 也是唯一的
```c
/*
 * The most important data for readout fits into a single 64 byte
 * cache line.
 */
struct tk_data {
	seqcount_raw_spinlock_t	seq;
	struct timekeeper	timekeeper;
	struct timekeeper	shadow_timekeeper;
	raw_spinlock_t		lock;
} ____cacheline_aligned;

static struct tk_data tk_core;
```

## masterclock 是每一个虚拟机一个的

在 kvm_arch 中:
```c
struct kvm_arch {
  // ...
	u64 master_kernel_ns;
	u64 master_cycle_now;
```

kvm->arch.kvmclock_offset 记录虚拟机开机的时候 kernel_ns 的 offset

## MSR_KVM_SYSTEM_TIME 和 MSR_KVM_WALL_CLOCK
<!-- 4c9afc20-ab4c-46fb-aa05-ed5d69b72de2 -->

MSR_KVM_SYSTEM_TIME 中包含的是 pvti 的地址
MSR_KVM_WALL_CLOCK 处理墙上时间

Documentation/virt/kvm/x86/cpuid.rst
```txt
KVM_FEATURE_CLOCKSOURCE            0           kvmclock available at msrs
KVM_FEATURE_CLOCKSOURCE2           3           kvmclock available at msrs
```

在 arch/x86/include/uapi/asm/kvm_para.h 定义如下:

```c
#define MSR_KVM_WALL_CLOCK  0x11
#define MSR_KVM_SYSTEM_TIME 0x12

/* Custom MSRs falls in the range 0x4b564d00-0x4b564dff */
#define MSR_KVM_WALL_CLOCK_NEW  0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01
```

Documentation/virt/kvm/x86/msr.rst

MSR_KVM_WALL_CLOCK 的注册的位置为 kvm_register_clock

### 之所以有两个，细节可以看 boot_vcpu_runs_old_kvmclock 的 git blame

## pvti 是如何沟通的
MSR_KVM_WALL_CLOCK_NEW 存储了 pvti 的地址

## vmware 这个总结写的不错
https://www.cse.iitb.ac.in/~puru/courses/spring22/downloads/timekeeping.pdf

## 核心结构体
- struct clocksource
- struct clock_event_device
- struct tick_device

## 有趣的 trace clock
/sys/kernel/debug/tracing/README 中展示了 trace 的过程中也是可以使用多种时钟的:

```txt
  trace_clock           - change the clock used to order events
       local:   Per cpu clock but may not be synced across CPUs
      global:   Synced across CPUs but slows tracing down.
     counter:   Not a clock, but just an increment
      uptime:   Jiffy counter from time of boot
        perf:   Same clock that perf events use
     x86-tsc:   TSC cycle counter
```

## 这个整理的很好
![](../img/15-2.png)

## 四个组合都有，但是讨论他们没什么意义了
```txt
High-res Dynamic ticks | High-res Periodic ticks
-------------------------------------------------
Low-res Dynamic ticks  | Low-res Periodic ticks
```

## windows 在 kvm 环境中运行，也是使用的 kvmclock 吧?
```diff
diff --git a/target/i386/kvm/kvm.c b/target/i386/kvm/kvm.c
index 2f66e63b880a..726837c79955 100644
--- a/target/i386/kvm/kvm.c
+++ b/target/i386/kvm/kvm.c
@@ -4718,6 +4718,7 @@ static int kvm_get_msrs(X86CPU *cpu)
             break;
         case MSR_KVM_SYSTEM_TIME:
             env->system_time_msr = msrs[i].data;
+            printf("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, env->system_time_msr);
             break;
         case MSR_KVM_WALL_CLOCK:
             env->wall_clock_msr = msrs[i].data;
```

启动之后可以获取到如下日志:
```txt
[martins3:kvm_get_msrs:4721] e8601
```
不过这个似乎是 seabios 来初始化的，更多的证据是从外部直接修改 pvti ，看看 windows 的时间会不会跳变。

## 配置 timer 的方法

通过 MSR_IA32_TSC_DEADLINE
```txt
@[
    lapic_next_deadline+5
    clockevents_program_event+138
    hrtimer_interrupt+291
    __sysvec_apic_timer_interrupt+85
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+462
    cpu_startup_entry+41
    start_secondary+291
    common_startup_64+318
]: 3584
```

## 获取时间的经典路径
- sysvec_apic_timer_interrupt
  - irq_enter_rcu
    - tick_irq_enter
      - tick_nohz_irq_enter
        - ktime_get
          - timekeeping_get_ns
            - timekeeping_get_delta
              - tk_clock_read
                - kvm_clock_get_cycles

## 继续看看
1. 为什么 qemu 中也存在 kvm-clock 的支持
  - https://zhuanlan.zhihu.com/p/665543594
2. https://tcbbd.moe/linux/qemu-kvm/kvm-time/#more
3. https://github.com/GiantVM/KVM-Annotation/wiki/Kvmclock
4. https://rwmj.wordpress.com/2010/10/15/kvm-pvclock/
5. https://lkml.org/lkml/2010/4/15/355
5. https://github.com/WCharacter/RDTSC-KVM-Handler

See “Changes to Instruction Behavior in VMX Non-Root Operation” in Chapter 25 of the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3C, for more information about the behavior of this instruction in VMX non-root operation.

7. https://www.redhat.com/en/blog/avoiding-clock-drift-vms

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
