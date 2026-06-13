# uclamp
<!-- 5c0b7176-e7fe-4302-9044-a75432dbdd9e -->

这两个解释的比较清楚，主要是嵌入式领域的。

两个文档结合看:
Documentation/scheduler/sched-util-clamp.rst
Documentation/scheduler/schedutil.rst
- [Scheduler utilization clamping](https://lwn.net/Articles/762043/)

不是的吧，
docs/kernel/cgroup/cgroup-sched.md 中提到了

如何理解?

clamp 是希望在什么性能范围中执行?

## 先看 Documentation/scheduler/sched-util-clamp.rst 中的文档看看吧

## clamp 机制还是有点逆天

### 🎯 目标

*clamp* 机制的目的不是立即改变任务的实际 CPU 利用率数据，而是在调度行为中 **引入偏好约束（hints）**：
1. **影响任务放置**（选择在哪个 CPU 运行）
2. **影响调频（DVFS）决策**（在合适的频率下运行）

换句话说，它影响的是 *调度器和 DVFS governor 的决策逻辑*，并不直接 “钳制” PELT 利用率真实值，只是在需要的时候使用这个边界值来影响策略决策。

* **UCLAMP_MIN（利用率最小钳制）**
  * 表示愿意 *提升* 任务运行性能
  * 就像给任务一个 *boost* 提示
* **UCLAMP_MAX（利用率最大钳制）**
  * 表示愿意 *限制* 任务不在高性能点浪费能量
  * 表示 *不希望跑太高的频率*

每个值的范围是 [0 … 1024]，按比例映射到实际的 CPU 利用率和 DVFS 频率点

这些 clamp 值本身不会改变任务自身的真实 PELT 利用率：

> **任务的实际 util signal 始终保持真实、不被修改；只有在调度器需要做决策时，才会使用 *clamped* 值来影响结果。**

例如：

* 当一个任务唤醒后，调度器计算 *哪个 CPU* 更合适
* 调度器调用 `schedutil` 做频率更新时
* 能量感知调度（EAS/CAS）做放置决策时

这种约束才会被使用。

## ⚙️ 四、*uclamp* 在调度器内部是怎么聚合的？

调度器不是简单地遍历所有任务去算最大/最小值，而是使用 **bucket（桶）机制做高效聚合**：
- 每个任务的 uclamp 值属于一个桶
- 每个 CPU runqueue 只统计这些桶的计数
- 聚合策略是 **取所有任务中的最大值（max aggregation）**

举个例子：

```
任务0: uclamp_min=300，uclamp_max=900
任务1: uclamp_min=500，uclamp_max=500

=> CPU runqueue 实际 uclamp:
  uclamp_min = max(300, 500) = 500
  uclamp_max = max(900, 500) = 900
```

也就是说：

> 在同一个 CPU 中，任何任务要求更高的约束，都会覆盖其他任务的需求。

## 🧩 五、*uclamp* 与 *schedutil* 的关系

### 📌 schedutil

`schedutil` 是 Linux 内核调频 governor 的一种动态频率选择机制，它：

* 根据调度器的 *util_est*（利用率估计）
* 结合 *uclamp* 的请求范围

在每次调度事件（任务唤醒、迁移、运行态变化）后：

🏃 调度器会调用 schedutil 去更新 DVFS 频率
➡ 然后把 runqueue 上所有任务的 **uclamp min/max 聚合值** 带给 schedutil
➡ 用于判断接下来应该设定什么频率。([内核.org][2])

简单来说：

> uclamp 是一种 *约束 hint*
> schedutil 是把约束和实际利用率 *转化为频率决策* 的机制

## 🧨 六、使用 *uclamp* 的系统和应用场景

### ✔ 适合用于：

✅ 需要 *提升 UI/交互任务体验*
通过设置较高的 UCLAMP_MIN

✅ 限制后台任务的能耗占用
通过设置较低的 UCLAMP_MAX

✅ 系统进入省电模式
调整系统级的 sched_util_clamp_max
/proc/sys/kernel/sched_util_clamp_max

## 📉 七、限制和注意点

虽然机制很灵活，但存在一些注意事项：

###  1) *UCLAMP_MAX 很难真正限制频率*

因为 *max 聚合规则* 会导致：

> 如果其他任务没有限制，那么 UCLAMP_MAX 的约束可能被**跑掉**。([Linux内核文档][1])

### 2) 可能影响 util_est 和频率策略

极端钳制有可能导致：

* PELT util_avg 信号波动异常
* 频率策略响应不符合预期
* 调度延迟与频率切换延迟叠加造成抖动

这些限制是内核设计和硬件限制共同导致的，而不是 *uclamp* 本身的bug。([Linux内核文档][1])

## 🧠 八、总结对比

| 机制                 | 作用          | 是否直接改变 util 信号 | 与 freq 的关系               |
| ------------------ | ----------- | -------------- | ------------------------ |
| **uclamp / clamp** | 性能约束 hint   | ❌ 否            | 影响 freq 决策               |
| **schedutil**      | 调频 governor | ❌              | 根据 util + uclamp 决定 freq |
| **PELT util_avg**  | 真实利用率       | ✅              | uclamp *影响逻辑，但不改变真实值*    |


*utilization clamping*（通常简称 uclamp）是 Linux 内核从 5.3 版本开始引入的调度 hint 机制，允许任务通过 UCLAMP_MIN / UCLAMP_MAX 提供性能需求约束，这些约束会通过最大聚合逻辑影响调度器做 CPU 选择和 DVFS 频率策略，但不会改变实际 PELT 利用率数据。

## 为什么 Android 大量依赖 uclamp，而服务器很少用

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
