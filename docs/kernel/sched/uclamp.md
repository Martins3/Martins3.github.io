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

emmmm，怎么似乎看了 ai ，还是没有完全搞懂

## clamp 机制分析

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

# uclamp：让调度器按“性能意图”做决定

*Utilization clamping*（uclamp）是 Linux 5.3 引入的调度提示机制，cgroup 支持在 5.4 合入。

它回答的不是“任务能拿多少 CPU 时间”，而是：**任务运行时，调度器应把它当作需要处于哪个性能档位。**

- `UCLAMP_MIN`：性能下限。任务即使刚唤醒、PELT 利用率还很低，也请求至少按这个性能点考虑；常用于 boost。
- `UCLAMP_MAX`：性能上限。任务的有效利用率不应高于这个性能点；常用于限制后台工作耗电。

取值范围为 `[0, 1024]`。它是归一化的**性能点**，不是某个固定 MHz，也不是百分之多少 CPU 时间。`1024` 表示 CPU 的最大调度容量；在大小核系统中，相同数值还会影响任务更偏向哪一类 CPU。

```text
“我至少需要 512 的性能”        -> UCLAMP_MIN = 512
“我最多按 512 的性能来考虑”     -> UCLAMP_MAX = 512
```

因此，`uclamp` 不是 quota，也不是优先级：

| 想解决的问题                     | 应使用的机制         |
| -------------------------------- | -------------------- |
| 任务/组最多实际运行多少 CPU 时间 | `cpu.max`            |
| 多个可运行任务怎样按比例分 CPU   | `nice`、`cpu.weight` |
| 任务运行时应请求什么性能点       | `uclamp`             |

相关背景可结合 [cgroup CPU 调度接口](../cgroup/cgroup-sched.md)
和 [schedutil](../power/schedutil.md) 阅读。

## 一句话模型

对某个调度决策而言，可以把 uclamp 理解为对利用率输入施加的边界：

```text
真实 PELT util_avg
        │
        ├─ 用于这次决策时，抬到 UCLAMP_MIN 以上
        └─ 用于这次决策时，压到 UCLAMP_MAX 以下
        │
有效 util ──> 选 CPU（EAS/CAS）和/或 schedutil 选频率
```

更接近伪代码的写法是：

```c
effective_util = clamp(util, effective_uclamp_min, effective_uclamp_max);
```

这里的重点是“**用于这次决策时**”：uclamp 不会直接改写任务保存的 PELT `util_avg`。读 PELT 信号时，看到的仍是任务真实的历史运行情况。

不过，`UCLAMP_MAX` 若长期把频率压得过低，任务可能一直跑不完、CPU 没有空闲时间，PELT 会因这一**间接结果**升高甚至饱和。它不是 uclamp 把 `util_avg` 字段改掉了，而是工作在较低性能下实际变得更忙。

## 它在哪些决策中生效

uclamp 要影响“选核”和“选频”才有价值，但具体路径有条件：

1. **任务放置**：当前主要是异构 CPU 上的 EAS/CAS 路径使用它。唤醒任务时，调度器比较任务放到各个 runqueue 后的有效利用率和能耗，因而较高的 `UCLAMP_MIN` 可使任务更倾向高容量 CPU。
2. **频率选择**：使用 `schedutil` governor 时，runqueue 的有效 uclamp 会参与下一次 DVFS 请求。低 PELT 的交互任务可以借由 `UCLAMP_MIN` 更快请求较高性能点。

它是 scheduler 的 hint，不是绕过硬件、驱动或 governor 限制的强制命令。例如平台不使用 `schedutil` 时，uclamp 不会通过该路径直接决定频率；频率切换延迟和 `schedutil` 的 rate limit 也仍然存在。

## 从任务请求到实际效果

一次请求会经过三层约束，最后才参与 CPU runqueue 的决策：

```text
任务请求（sched_setattr）
        │
        ▼
cgroup 限制：cpu.uclamp.min / cpu.uclamp.max
        │
        ▼
系统全局限制：sched_util_clamp_min / sched_util_clamp_max
        │
        ▼
任务的 effective uclamp
        │
        ▼
runqueue 聚合 ──> 选核 / schedutil 选频
```

### 任务、cgroup 和系统级接口

| 作用域 | 接口 | 含义 |
| --- | --- | --- |
| 单个任务 | `sched_setattr()` 的 `sched_util_min` / `sched_util_max` | 请求该任务的性能范围 |
| cgroup v2 | `cpu.uclamp.min` / `cpu.uclamp.max` | 给组内任务设置保护下限和限制上限；值用百分比或 `max` 表示 |
| 系统全局 | `/proc/sys/kernel/sched_util_clamp_min` / `/proc/sys/kernel/sched_util_clamp_max` | 限制系统允许的有效范围，单位为 `[0, 1024]` |
| RT 默认值 | `sched_util_clamp_min_rt_default` | 未显式设置时，调整 `SCHED_FIFO` / `SCHED_RR` 任务的默认最小性能请求 |

cgroup 的层级规则和名字一致：

- `cpu.uclamp.min` 是保护值；祖先和子组的有效值取更高者。
- `cpu.uclamp.max` 是限制值；祖先和子组的有效值取更低者。

因此，任务的请求可以被系统接受，却暂时无法满足：任务换到另一个 cgroup，或管理员修改全局限制后，effective 值可能随之变化。

普通 CFS 任务默认是 `min = 0, max = 1024`；RT 任务默认是 `min = 1024, max = 1024`，以保留传统的“RT 唤醒即请求最高性能”行为。后者可用 `sched_util_clamp_min_rt_default` 调低，详见 [RT 调度说明](rt.md)。

## runqueue 为何按最大值聚合

uclamp 是任务属性，但频率和放置决策通常针对一个 CPU 的 runqueue。为了不在调度热路径遍历所有任务，内核把 `[0, 1024]` 划为若干 bucket；任务入队和出队时只更新对应 bucket 的计数，从最高的非空 bucket 取得该 runqueue 的有效值。

对 `UCLAMP_MIN` 和 `UCLAMP_MAX`，runqueue 都采用 **max aggregation**：

```text
task A: min = 300, max = 900
task B: min = 500, max = 500

rq:     min = max(300, 500) = 500
        max = max(900, 500) = 900
```

这看起来有些反直觉，尤其是 `UCLAMP_MAX`。原因是同一个 CPU 上只要有一个任务被允许请求较高性能，就不能为了另一个受限任务而把它一并限制住。优先满足“需要/允许最高性能点”的任务，是这一聚合规则的选择。

### `UCLAMP_MAX` 的关键限制

下面的场景中，`UCLAMP_MAX=512` **不能保证**任务 A 所在 CPU 的频率被限制在 512：

```text
task A: max = 512，CPU 密集
task B: max = 1024，即使它本身很轻

rq max = max(512, 1024) = 1024
```

task B 的存在就足以让该 runqueue 解除上限；随后 task A 的真实高 util 可能推动 CPU 提到很高的频率。这是 uclamp 的设计限制，不应把每任务 `UCLAMP_MAX` 当作严格的频率或功耗隔离。

若目标是系统整体省电，系统级 `sched_util_clamp_max` 更可靠，因为它限制了所有任务的有效上限；不过它仍不同于直接设置 cpufreq 的最大频率。若目标是硬性 CPU 时间隔离，应使用 `cpu.max`。

## 典型使用方式

### 提升交互或 deadline 敏感任务

短小的 UI/渲染任务刚唤醒时，PELT 会有约 200 ms 的爬升过程；等 `util_avg` 自己反映出负载，可能已经错过一帧。为它设置合适的 `UCLAMP_MIN`，可以在下一次唤醒时更早请求足够的性能，减少 DVFS 爬升造成的延迟。

不要把数值写死为 `1024`。不同机器上“512”代表的实际工作能力不同；较好的做法是根据帧率、deadline 或延迟是否达标形成反馈回路，只提高到刚好满足目标的值。

### 约束后台工作

可为不关心响应时间的后台任务或 cgroup 设置较低的 `UCLAMP_MAX`，使其不主动请求过高性能，并在大小核设备上偏向低容量核心。这适合在移动设备中为前台应用预留性能和电量。

但要记住前节的 runqueue 聚合限制：若后台任务频繁与未受限任务共用 CPU，其实际节能效果可能不稳定。

### 省电模式

在低电量、熄屏等系统级策略中，可以降低 `sched_util_clamp_max`，使整个系统不再请求更高的性能点。这是一个方便的策略接口；直接限制 cpufreq 最大频率也能实现类似目标。

## 为什么 Android 常用，而服务器较少用

这不是“服务器不能用”，而是收益和控制面不同。

| Android / 嵌入式 | 典型服务器 |
| --- | --- |
| 大小核与 DVFS 差异明显，选核和选频都有显著能耗差 | CPU 往往更同质，性能策略更多交给硬件 P-state 或平台策略 |
| UI、音频、渲染有短 deadline，PELT/DVFS 爬升延迟肉眼可见 | 更常关注吞吐、尾延迟、隔离和资源计费 |
| 前台、后台、top-app 等产品语义清晰，系统能集中调参 | 通常用 affinity、`cpu.weight`、`cpu.max`、NUMA 和应用自身并发控制 |

因此 Android 很适合把 uclamp 作为产品层的性能/功耗提示；服务器也可用于特定的延迟敏感或节能 workload，但不能把它替代为 CPU 配额或严格的性能隔离。

## 容易混淆的结论

1. uclamp 约束的是调度器用于决策的**有效 util/性能点**，不直接修改保存的 PELT `util_avg`。
2. `UCLAMP_MIN` 是 boost 下限；`UCLAMP_MAX` 是性能上限。两者都不是 CPU 时间配额。
3. 选核效果主要见于 EAS/CAS 的异构 CPU 路径；选频效果需要 `schedutil` 路径。
4. 一个 CPU runqueue 上的 min 和 max 都按最大值聚合，所以单任务 `UCLAMP_MAX` 不是强制频率上限。
5. 要验证效果，应同时观察任务/cgroup 的 effective 设置、任务实际落在哪个 CPU、所用 cpufreq driver/governor，以及频率切换延迟。

## 参考

- [Linux 内核文档：Utilization Clamping](https://docs.kernel.org/scheduler/sched-util-clamp.html)
- [Linux 内核文档：cgroup v2 CPU controller](https://docs.kernel.org/admin-guide/cgroup-v2.html#cpu)
- [LWN: Scheduler utilization clamping](https://lwn.net/Articles/762043/)

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
