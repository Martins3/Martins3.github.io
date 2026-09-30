2026-09 实测给出了确定答案，完整推导和实测见
[kvmclock/guest-stopped.md](../../kernel/time/kvmclock/guest-stopped.md)：

- qmp/hmp `stop` + `cont`：qemu 在 runstate 变化时调 `KVM_SET_CLOCK`（把 guest 的 kvmclock 拨回暂停时刻）
  和每个 vcpu 一次 `KVM_KVMCLOCK_CTRL`（置 `PVCLOCK_GUEST_STOPPED`），
  所以 `check_cpu_stall()` 里那句 `kvm_check_and_clear_guest_paused()` 会 return 掉；
  对 qemu 进程 `strace -f -e trace=ioctl` 能直接看到 `KVM_GET_CLOCK` / `KVM_SET_CLOCK` / `KVM_KVMCLOCK_CTRL`。
- `kill -STOP $qemu_pid`：完全绕过 runstate（`--trace runstate_set` 里没有任何事件，
  strace 里也没有那三个 ioctl），guest 的 jiffies 会一次性跳过整个暂停时长
  （`tick_do_update_jiffies64()` 的 slow path），于是报出 `t=` 恰好等于暂停时长的 stall。
- 前置条件是**暂停瞬间有一个 grace period 在飞**（`check_cpu_stall()` 开头就有 `!rcu_gp_in_progress()` 直接 return），
  这就是"有时候能触发、有时候不能"的原因。复现 workload 用
  `membarrier(MEMBARRIER_CMD_GLOBAL)`（内核里就是 `synchronize_rcu()`），
  但注意它在 1 个 online CPU 的 guest 上是空操作。

## 利用 gdb 暂停的确存在本质的不同:
```txt
runstate_set current_run_state 9 (running) new_state 4 (paused)
runstate_set current_run_state 4 (paused) new_state 9 (running)
clocksource: Marking clocksource tsc unstable due to frequency skew
clocksource: Watchdog               kvm-clock interval:        503995129ns
clocksource: Clocksource                  tsc interval:       1894661885ns
tsc: Marking TSC unstable due to clocksource watchdog
```
date 观察，时间存在跳变，

stop / cont 涉及到的 ioctl
```txt
11:51:51.725783 ioctl(30, KVM_GET_CLOCK, 0x7ffe138434a0) = 0        <- stop
11:51:56.732436 ioctl(30, KVM_SET_CLOCK, 0x7ffe13843450) = 0        <- cont
11:51:56.732489 ioctl(42, KVM_KVMCLOCK_CTRL, 0) = 0                 <- vcpu0 线程
11:51:56.732544 ioctl(44, KVM_KVMCLOCK_CTRL, 0) = 0                 <- vcpu1 线程
```

## soft lockup

暂停时有可运行的 current task（例子中是阻塞在 `synchronize_rcu()` 里的 workload，当时脚本里还叫 `wl.out`）时，
还会出现：

```txt
[ 3369.353121] watchdog: BUG: soft lockup - CPU#0 stuck for 42s! [wl.out:272]
```

## clocksource watchdog 把 TSC 标成 unstable

7.2 的 clocksource watchdog 是重写过的（`kernel/time/clocksource.c` 的 `watchdog_check_freq()`，
按 ppm 判断 skew），SIGSTOP 恢复后大概率能看到：

```txt
[  203.647577] clocksource: Marking clocksource tsc unstable due to frequency skew
[  203.647813] clocksource: Watchdog               kvm-clock interval:        495999854ns
[  203.648001] clocksource: Clocksource                  tsc interval:      45509207014ns
```

qmp 那一侧很直白：kvmclock 被 `KVM_SET_CLOCK` 拨回去了（0.496s），
TSC 还在走（45.5s），两者对不上，于是 tsc 被判 unstable。

有意思的是 SIGSTOP 那一侧**两个时钟都跳**，间隔却打印成微秒级：

```txt
[ 3369.359875] clocksource: Watchdog               kvm-clock interval:             2255ns
[ 3369.360058] clocksource: Clocksource                  tsc interval:             2075ns
```

2µs 的窗口下 `(max_delta >> ppm_shift)` 退化成 0，读数噪声（几百 ns）就足以判成 skew。
8 个单 CPU 的 vm 做 SIGSTOP 全部稳定复现这条 warn，看起来更像 watchdog 自身的假阳性，
值得单独再看一次（`watchdog_check_freq()` 的 retry 逻辑）。


暂停很久之后，的确会出现跳变，但是，注意观察，这里跳变量很小的:
```txt
[10833.672095] clocksource: Marking clocksource tsc unstable due to frequency skew
[10833.672602] clocksource: Watchdog               kvm-clock interval:            14958ns
[10833.672787] clocksource: Clocksource                  tsc interval:            14780ns
[10833.672974] tsc: Marking TSC unstable due to clocksource watchdog
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
