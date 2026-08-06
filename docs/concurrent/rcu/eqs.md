# extended quiescent state

也许先理解了 eqs 才可以理解 context_tracking 吧 ?

```c
/*
 * Is the current CPU in an extended quiescent state?
 *
 * No ordering, as we are sampling CPU-local information.
 */
static __always_inline bool rcu_dynticks_curr_cpu_in_eqs(void)
{
	return !(raw_atomic_read(this_cpu_ptr(&context_tracking.state)) & RCU_DYNTICKS_IDX);
}
```

https://www.suse.com/c/cpu-isolation-full-dynticks-part2/

其中的 3.2 RCU quiescent states reporting 解释的很好:

1. 如果有 tick ，就没有必要使用这个概念，因为 : Because that would inflict a costly atomic operation with a full memory barrier on every user/kernel round trip. Also the duty to report quiescent states eventually falls to other CPUs so one must be aware of the exported cost that implies.
2. 如果没有，那么利用进入到 idle 和 userspace 可以知道，CPU 一定不是在 rcu critical region 中的

## 解释
2026-08-05 codex 的解释，其实结果是很不错的

在 RCU 里，extended quiescent state（EQS，扩展静止状态）可以理解为：

> CPU 在一段较长时间内确定不会执行普通的 RCU 读侧临界区，因此 RCU 暂时不需要“盯着”这个 CPU。

典型场景：

- CPU 进入深度 idle；
- NO_HZ_FULL CPU 长时间运行用户态代码；
- CPU offline。

例如：

内核态运行        EQS：idle / userspace          内核态运行
-----------|----------------------------------|-----------
           rcu_eqs_enter()                     rcu_eqs_exit()
        RCU 不再等待该 CPU                  RCU 重新监视该 CPU

### 和普通 quiescent state 的区别

普通 quiescent state（QS）是一个“时间点”性质的证据：

RCU 读侧临界区结束 -> 经过一次上下文切换 -> RCU 知道旧 reader 已结束

它只能说明：CPU 已经经过了一个静止点。

EQS 则是一整个“时间区间”：

进入 EQS ================================= 离开 EQS
          整段时间都不会出现普通 RCU reader

因此称为 extended——静止状态被延伸成了一段持续时间。

### 它对 grace period 有什么意义

RCU grace period 要确认所有可能持有旧引用的 CPU，都经过至少一次 quiescent state。

如果某 CPU 已经处于 EQS：

- RCU 不需要等待它；
- 即使它长时间 idle、不开调度 tick，也不会阻塞 grace period；
- 这正是 NO_HZ/tickless 系统能与 RCU 配合的关键。

### RCU “不监视 CPU”是什么意思

内核通常把 EQS 描述为 RCU not watching：

- CPU 不应执行普通的 RCU 读侧临界区；
- RCU 的 dynticks 状态会标记它处于 EQS；
- 其他 CPU 检查其状态时，可以认定它已经提供了 quiescent-state 证据。

CPU 从 idle 或用户态重新进入内核、准备执行可能包含 RCU reader 的代码前，必须先退出 EQS。

### 中断和 NMI 怎么办

CPU 在 EQS 中仍可能收到中断或 NMI。进入中断上下文时，RCU 会暂时恢复 watching 状态；中断结束后，如果返回 idle/用户态，再重新回到 EQS。

所以 EQS 并不是“CPU 完全不执行代码”，而是：

> 在 RCU 的状态机保证下，CPU 不会在未通知 RCU 的情况下执行需要被 grace period 跟踪的读侧临界区。

一句话总结：普通 QS 是“这个 CPU 已经过了安全点”，EQS 是“这个 CPU 从现在起持续处于安全区，等它回来时再通知 RCU”。

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
