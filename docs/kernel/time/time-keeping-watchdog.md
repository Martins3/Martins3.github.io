# clocksource watchdog

当前(v7.1)实现把 watchdog timer 固定在 boot CPU 上。

每 0.5S 发起一次

clocksource_watchdog 中周期执行，完成两个工作
1. watchdog_check_freq(cs, reset_pending) : 和标准 CPU 对别，是否存在差别，如果出现区别，直接失败:
2. watchdog_check_cpu_skew(cs) : 各个 CPU 之间的时间是否是差别，用 boot CPU 和每一个 CPU 来对比

挑选规则就是，用 rate 最高的 clocksource 来评测其他的 clocksource

只有虚拟机中有 kvm-clock tsc 才存在这个校验的，物理机中往往都看不到这个调用，应该是 tsc 就是唯一可用的时钟，
其他时钟需要测试
```txt
@[
        clocksource_watchdog+5
        call_timer_fn+38
        __run_timers+501
        run_timer_softirq+73
        handle_softirqs+240
        __irq_exit_rcu+203
        sysvec_apic_timer_interrupt+113
        asm_sysvec_apic_timer_interrupt+26
        pv_native_safe_halt+15
        default_idle+9
        default_idle_call+40
        cpuidle_idle_call+346
        do_idle+142
        cpu_startup_entry+41
        start_secondary+294
        common_startup_64+318
]: 8
```

偶尔虚拟机中存在，应该是 ipi 阻塞了:
```txt
clocksource: Watchdog remote CPU 1 read timed out
```

## 触发方法

gdb attach qemu ，然后暂停 qemu ，那么就可以看到:
```txt
[   27.541839] clocksource: Marking clocksource tsc unstable due to frequency skew
[   27.542401] clocksource: Watchdog               kvm-clock interval:             5868ns
[   27.542696] clocksource: Clocksource                  tsc interval:             5775ns
[   27.542954] tsc: Marking TSC unstable due to clocksource watchdog
```

## 跨 CPU skew 检查

频率检查通过后，进入 watchdog_check_cpu_skew() (kernel/time/clocksource.c:423)。

每轮选择一个 remote CPU：

cpu = cpumask_next_wrap(curr_cpu, cpu_online_mask);

然后通过异步 IPI 让 boot CPU 和 remote CPU 交替读取同一个 clocksource。

大致时序：

```txt
boot CPU                         CPU28
   │                               │
   ├─ read TSC: B0                 │
   ├─ seq++ ──────────────────────►│
   │                               ├─ read TSC: R0
   │◄────────────────────── seq++ ─┤
   ├─ read TSC: B1                 │
   ├─ 检查 B1 >= R0                │
   ├─ seq++ ──────────────────────►│
   │                               ├─ read TSC: R1
   │                               ├─ 检查 R1 >= B1
   │◄────────────────────── seq++ ─┤
   └─ 重复数轮                     └─
```

核心检查：

prev = wd->cpu_ts[remote];
delta = (now - prev) & cs->mask;

if (delta > cs->max_raw_delta)
	WD_CPU_SKEWED;

它不是直接判断：

CPU28 与 CPU0 相差超过 N ns

而是检查因果顺序：

> 后发生的读取，其计数值不能比前一个 CPU 上已经发生的读取还小。

如果 CPU28 的 TSC 落后很多，就会出现：

真实顺序：CPU0 先读，CPU28 后读
计数结果：TSC_CPU28 < TSC_CPU0

无符号减法会得到一个接近 U64_MAX 的巨大 delta，从而超过 max_raw_delta，判定 inter-CPU skew。

这种方法：

- 不需要 kvm-clock/HPET 参与；
- 不把 per-CPU offset 混入频率检查；
- 能检测 TSC 跨 CPU 倒退；
- 小于 IPI/缓存通信延迟的 offset 不一定立即检测出来；
- 若 offset 随时间缓慢扩大，之后仍可能被检测。

remote CPU 超时只产生 WD_CPU_TIMEOUT，本轮跳过，不会直接误杀 clocksource。

## suspend、暂停等场景处理

resume、kgdb，以及 KVM 的 PVCLOCK_GUEST_STOPPED 会调用：

clocksource_touch_watchdog(); -> atomic_inc(&watchdog_reset_pending);

下一轮 watchdog 会读取新值，使用新的基线

相关代码在 clocksource_resume() (kernel/time/clocksource.c) 和 pvclock_touch_watchdogs() (arch/x86/kernel/pvclock.c)。

## -s -S qemu 来暂停虚拟机观察到的结果

```txt
clocksource: watchdog debug CPU0: failed frequency check: cs=tsc wd=kvm-clock cs_delta=60191798821 wd_delta=504972841 skew=59686825980 limit=235124573
  wd_seq=359 ppm_shift=8 jiffies=4294970305 expires=4294970296 overdue=9 reset=0
  clocksource: Marking clocksource tsc unstable due to frequency skew
  clocksource: Watchdog               kvm-clock interval:        504972841ns
  clocksource: Clocksource                  tsc interval:      60191798821ns
  tsc: Marking TSC unstable due to clocksource watchdog
```

```txt
  ━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   cs=tsc                  被检查的时钟源
  ──────────────────────  ────────────────────────────────────────
   wd=kvm-clock            作为参照的时钟源
  ──────────────────────  ────────────────────────────────────────
   cs_delta=60191798821    两次采样之间，TSC 换算后增加 60.192 秒
  ──────────────────────  ────────────────────────────────────────
   wd_delta=504972841      同期间，kvm-clock 增加 0.505 秒
  ──────────────────────  ────────────────────────────────────────
   skew=59686825980        两者相差 59.687 秒
  ──────────────────────  ────────────────────────────────────────
   limit=235124573         本轮允许误差约 0.235 秒
```
这就是经典的 tsc 没有被同步修正，所以会出现很大的误差导致的

### GDB 暂停 guest 的实际路径

按 Ctrl-C 时，GDB 发来 0x03。QEMU 在 gdb_read_byte() (/home/martins3/data/qemu/gdbstub/gdbstub.c:2357) 中处理：

if (runstate_is_running()) {
    /* 注释明确说明这里处理 gdb client 的 Ctrl-C */
    ...
    vm_stop(RUN_STATE_PAUSED);
}

命中 guest 断点 时，则走：

cpu_handle_guest_debug()
  → qemu_system_debug_request()
    → QEMU 主循环
      → vm_stop(RUN_STATE_DEBUG)

对应 断点处理 (/home/martins3/data/qemu/system/cpus.c:298) 和 主循环 (/home/martins3/data/qemu/system/runstate.c:1045)。

### GDB 继续执行的实际路径

GDB 的 continue 最终进入 gdb_continue() (/home/martins3/data/qemu/gdbstub/system.c:550)：

void gdb_continue(void)
{
    if (!runstate_needs_reset()) {
        trace_gdbstub_op_continue();
        vm_start();
    }
}

所以完整关系是：

GDB Ctrl-C / 命中断点
  → 内部 vm_stop()
    → 状态通知
      → kvmclock 保存时间

GDB continue
  → 内部 vm_start()
    → 状态通知
      → kvmclock 通过 KVM_SET_CLOCK 恢复时间

这条调用链不经过 QMP 命令处理。 因此，上一条关于“kvm-clock 恢复保存值，而普通继续执行不写回 TSC”的解释，适用于你这个 -s/-S 调试 guest 的场景。

## 这个东西的触发远远到底是什么来着?
clocksource: Watchdog remote CPU 5 read timed out

## 对于非 stable 的场景，如何分析的

guest 机器:
```txt
➜  ~ cat /sys/devices/system/clocksource/clocksource0/current_clocksource
kvm-clock
➜  ~ cat /sys/devices/system/clocksource/clocksource0/available_clocksource
kvm-clock hpet acpi_pm
```

物理机器:
```txt
🧀  cat /sys/devices/system/clocksource/clocksource0/current_clocksource
hpet
code/build/qemu on  master [$] amd
🧀   cat /sys/devices/system/clocksource/clocksource0/available_clocksource
hpet acpi_pm
```


### 休眠问题从这个函数分析起来
-__timekeeping_inject_sleeptime

## 如果虚拟机暂停 5s ，内部的时钟是如何维持正确的

打开了 kvm-clock 的情况:

如果出现了 stop / cont ，那么


很快的 stop / cont ，出现了大约 1s 的误差:
```txt
[  360.661925] clocksource: Marking clocksource tsc unstable due to frequency skew
[  360.662225] clocksource: Watchdog               kvm-clock interval:        495949066ns
[  360.662458] clocksource: Clocksource                  tsc interval:       1343945643ns
[  360.662674] tsc: Marking TSC unstable due to clocksource watchdog
```

等一会， 出现 109s 的误差:
```txt
[  395.156799] clocksource: Marking clocksource tsc unstable due to frequency skew
[  395.157081] clocksource: Watchdog               kvm-clock interval:        503984674ns
[  395.157270] clocksource: Clocksource                  tsc interval:     109568537443ns
```

可以发现，kvm-clock interval 总是 0.5s 的差别

用 date 观察，发现时间是连续的，也就是落后了现实世界的时间

```txt
for i in {1..100} ; do
	echo "$i"
	sleep 1
	date
done
```

## 如果机器 suspend 了 5s ，如何维护的?

- 之前睡眠 10s 的任务还是需要睡眠 10s ，而不是睡眠 5s 。

( clock type 应该可以解释的)

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
