## 使用 syscall
使用 CLOCK_MONOTONIC_RAW 和 CLOCK_MONOTONIC

```txt
update_vsyscall
timekeeping_update
timekeeping_advance
tick_sched_do_timer
tick_sched_timer
__hrtimer_run_queues
hrtimer_interrupt
smp_apic_timer_interrupt
apic_timer_interrupt
```

nsec		= tk->tkr_mono.xtime_nsec >> tk->tkr_mono.shift;

```txt
./trace -I linux/timekeeper_internal.h  'update_vsyscall(struct timekeeper *tk) "%ld %ld", tk->tkr_mono.xtime_nsec , tk->tkr_mono.cycle_last' | tee a
```

## 问题的点:
### timekeeping_forward_now 可能会导致时间回退的

timekeeping_forward_now 有这么多地方调用，都是有 lock 吗?

这里直接修改了 cycle_last ，可能导致 cycle_last 回退
```c
	tk->tkr_mono.cycle_last = cycle_now;
	tk->tkr_raw.cycle_last  = cycle_now;
```

而且这里有一个问题

### timekeeping_apply_adjustment

这里是关键


## 问题
### 为什么 clock_adjtime 也是需要通过调用  timekeeping_advance 来实现时间调整

### timekeeping_advance 中
为什么只是计算 	timekeeping_adjust(tk, offset); 中 offset 的部分的时候


的确，就是这里的

### 为什么开机之后，tk->tkr_raw.cycle_last 和 tk->tkr_mono.cycle_last 会回退
并不是同时的在这里的啊


## 材料
- https://bugzilla.kernel.org/show_bug.cgi?id=219295

## 跟踪一下修复过程
https://lore.kernel.org/lkml/CAKcXpBwvjrmoPnfFgaXs81XF5du-mWzLiJ+8YvvhM_1tQMiZBQ@mail.gmail.com/
commit 324a2219ba38 ("Revert "timekeeping: Fix possible inconsistencies in _COARSE clockids"")

https://patchew.org/linux/87h632wals.ffs@tglx/

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
