## EEVDF 把 SCHED_BATCH 给干没了
https://lore.kernel.org/all/20230531115839.089944915@infradead.org/


## 先看文档
https://www.kernel.org/doc/html/latest/scheduler/sched-eevdf.html


Similarly to CFS, EEVDF aims to distribute CPU time equally among all runnable tasks with the same priority. To do so, it assigns a virtual run time to each task, creating a “lag” value that can be used to determine whether a task has received its fair share of CPU time. In this way, a task with a positive lag is owed CPU time, while a negative lag means the task has exceeded its portion. EEVDF picks tasks with lag greater or equal to zero and calculates a virtual deadline (VD) for each, selecting the task with the earliest VD to execute next. It’s important to note that this allows latency-sensitive tasks with shorter time slices to be prioritized, which helps with their responsiveness.

There are ongoing discussions on how to manage lag, especially for sleeping tasks; but at the time of writing EEVDF uses a “decaying” mechanism based on virtual run time (VRT). This prevents tasks from exploiting the system by sleeping briefly to reset their negative lag: when a task sleeps, it remains on the run queue but marked for “deferred dequeue,” allowing its lag to decay over VRT. Hence, long-sleeping tasks eventually have their lag reset. Finally, tasks can preempt others if their VD is earlier, and tasks can request specific time slices using the new sched_setattr() system call, which further facilitates the job of latency-sensitive applications.


1. vruntime

vruntime 是 CFS/EEVDF 里最基础的“虚拟运行时间”。

它不是任务真实跑了多少纳秒，而是一个“按权重修正后的运行时间”：

- 任务实际运行一段时间后，vruntime 会增加
- 权重高的任务，vruntime 增长得更慢
- 权重低的任务，vruntime 增长得更快

直觉上就是：
调度器不是只看“谁跑了多久”，而是看“按优先级折算后，谁占了多少份额”。

所以：

- vruntime 小：说明这个任务相对“跑得少”
- vruntime 大：说明这个任务相对“跑得多”

在 CFS 里，通常就是尽量选 vruntime 最小的任务来跑。


2. lag

lag 表示“一个任务距离它应得份额，差了多少”。

可以粗略理解成：

lag = 应该得到的服务 - 实际得到的服务

所以：

- lag > 0：它还没拿够 CPU，调度器欠它
- lag < 0：它已经跑超了
- lag = 0：大致公平

这是 EEVDF 相比 CFS 更直接表达“公平性”的量。

你可以把它理解成“账户余额”：

- 正数：还有额度没用，应优先补给它
- 负数：已经透支，先别急着给它跑

EEVDF 通常先看 lag >= 0 的任务，因为这些任务才是“值得被补偿”的。

3. VRT

这里的 VRT 一般就是在说 Virtual Runtime Track / virtual runtime progression 这个语境里的“虚拟时间推进”，本质上还是围绕 vruntime 那套虚拟时间轴。

你可以把它简单理解成：

- 系统里存在一条“虚拟时间线”
- 这条线不是墙上时钟时间，而是调度器内部用来衡量公平性的时间
- 任务的 lag、是否衰减、是否已经“补偿够了”，都是相对这条虚拟时间线来看的

在你那段话里，“lag decays over VRT”的意思不是说有个完全独立的新时间概念，而是说：

- 任务睡眠后
- 随着调度器的虚拟运行时间继续向前推进
- 它原先的 lag 会逐渐失效/衰减

所以这里的 VRT 可以近似理解成：调度器内部的虚拟时间进度。

如果你看内核讨论，很多时候它和 vruntime 的大框架是同一个家族的概念，不必硬拆成完全不同的东西。

4. VD

VD 是 Virtual Deadline，虚拟截止时间。

这是 EEVDF 里决定“谁先跑”的关键量之一。

思路是：

- 先用 lag 看谁有资格跑
- 再用 VD 看这些任务里谁更紧急

所以：

- VD 越早，任务越应当优先执行
- VD 越晚，任务可以再等等

这就是 EEVDF 和 CFS 很不一样的地方。
CFS 更像“谁最亏谁先上”；
EEVDF 是“先保证没跑亏的人有资格，再看谁最早到截止点”。

这能更好照顾短时间片、低延迟任务，因为它们通常会有更早的虚拟截止时间。

## Gemini

EEVDF 的出现不仅仅是一次算法的替换，它是 Linux 调度器在**理论模型**上的一次重大回归和修正。
它解决了一个 CFS（完全公平调度器）在其 15 年生命周期中始终无法完美解决的根本性矛盾：
**延迟（Latency）与带宽（Bandwidth）的耦合**。

以下是关于 EEVDF 的深度技术总结：

### 1. 核心痛点：CFS 的“延迟-带宽”耦合

在深入 EEVDF 之前，必须理解 CFS 的缺陷：

* **CFS 的逻辑**：CFS 追求的是长期公平。它总是选择 `vruntime`（虚拟运行时间）最小的任务运行。
* **问题**：在 CFS 中，如果你想让一个任务**响应更快**（低延迟），你唯一的办法就是提高它的权重（降低 nice 值）。但是，提高权重意味着你不仅让它运行得更早，还必须给它**更多**的 CPU 时间片（带宽）。
* **场景**：假设有一个音频处理线程，它每秒只需要运行 1 毫秒，但必须立刻运行。在 CFS 中，为了保证它“立刻运行”，你必须给它很高的权重，结果它获得了过量的 CPU 时间配额，这对不需要那么多时间的其他任务是不公平的。

**EEVDF 的目标**：允许任务在**不增加**总 CPU 配额（带宽）的前提下，请求更早的执行权（低延迟）。

### 2. EEVDF 的三大数学基石

EEVDF 不再依赖 CFS 那些复杂的启发式补丁（Heuristics），而是建立在三个核心概念之上：

#### A. 虚拟运行时间 (Virtual Runtime)

和 CFS 一样，EEVDF 依然维护任务的运行进度。

vruntime += 物理运行时间 / 权重


#### B. 滞后量 (Lag) —— 衡量“公平”的尺子

这是 EEVDF 引入的关键状态。

* **定义**：一个任务理应获得的 CPU 时间与它实际获得的 CPU 时间之差。
* **公式**：
* **正 Lag (+)**：任务被“亏待”了，它有资格（Eligible）立即运行。
* **负 Lag (-)**：任务“透支”了时间，它应该等待。

#### C. 虚拟截止时间 (Virtual Deadline) —— 衡量“延迟”的尺子

这是 EEVDF 的杀手锏。每个任务在申请运行队列时，会带有一个“请求的时间片长度”（Request Slice）。

* **公式**：

Virtual Deadline = vruntime + 请求的时间片长度 / 权重

* **含义**：这个公式将“任务想跑多久”直接转化为了“任务应该在什么时候结束”。
* **关键点**：如果一个任务请求的**时间片很短**，那么算出来的 Deadline 就会很小（很早）。这使得**短任务可以天然获得更早的截止时间，从而被优先调度**，而无需提高它的权重。


### 3. 调度决策流程

当调度器需要选择下一个任务时，EEVDF 执行两步筛选：

1. **第一步：资格筛选 (Eligibility Check)**
* 调度器只看那些 `Lag >= 0` 的任务。
* 这意味着：只有那些“当前没有占便宜”的任务，才有资格参与竞争。这保证了**长期公平性**。


2. **第二步：截止时间排序 (Deadline Sorting)**
* 在所有“有资格”的任务中，选择 **Virtual Deadline 最小** 的那个。
* 这意味着：在都公平的前提下，谁承诺完成得最早（通常是请求时间片最短的任务），谁就先上。这保证了**低延迟响应**。

### 4. 详细对比：CFS vs EEVDF

| 维度               | CFS (Completely Fair Scheduler)                                                           | EEVDF (Earliest Eligible Virtual Deadline First)                                                                   |
| ---                | ---                                                                                       | ---                                                                                                                |
| **核心数据结构**   | 红黑树，以 `vruntime` 排序。总是挑最左边的节点。                                          | 增强型红黑树（WAVL树），同时索引 `vruntime` 和 `deadline`。                                                        |
| **延迟控制**       | **隐式**。通过调整 `sysctl_sched_latency` 等全局参数，或者调整 nice 值。                  | **显式**。任务可以通过 `sched_setattr` 显式声明其所需的切片大小（latency_nice）。                                  |
| **交互式任务**     | 依赖“唤醒抢占”（Wakeup Preemption）和“放置奖励”等复杂的启发式补丁。代码难维护，容易出错。 | **天然支持**。交互式任务睡眠时会积累正 Lag，唤醒时变为 Eligible，且通常请求短切片，Deadline 很早，因此会立即抢占。 |
| **对时间片的处理** | 动态计算，试图切碎时间片以模拟“理想多任务处理器”。                                        | 尊重任务请求的时间片。如果任务说“我要跑 5ms”，EEVDF 会基于这个承诺计算截止时间。                                   |
| **代码复杂度**     | 极高。充满了为了修补特殊场景（如 cgroup 节流、唤醒延迟）而加入的补丁。                    | **中等但清晰**。移除了大量 CFS 的补丁代码，逻辑回归数学模型。                                                      |

### 5. 总结：EEVDF 带来的改变

EEVDF 将 Linux 调度器带入了一个**“这也是有承诺的”**时代。

* **对于桌面用户**：视频播放、游戏操作等需要低延迟的任务，不再需要因为公平性而被强行打断，或者为了抢占 CPU 而伪装成高优先级进程。它们只需请求一个小的时间片，EEVDF 就会优先安排。
* **对于服务器/云**：它消除了 CFS 中许多为了平滑延迟而引入的性能抖动（Jitter），使得吞吐量和延迟的控制更加精确和可预测。
* **对于内核开发者**：Peter Zijlstra（EEVDF 的实现者）通过这次重构，删除了大量陈旧的 CFS 补丁代码，让调度器核心变得更易于维护和扩展。

## 为什么还是可以看到很多 cfs

```txt
CFS的遗产：什么还在？

1. 数据结构 100%保留

// cfs_rq - CFS运行队列
struct cfs_rq {
    struct load_weight  load;           // 负载权重
    struct rb_root_cached tasks_timeline;  // 红黑树任务队列
    struct sched_entity *curr;          // 当前运行的实体
    struct sched_avg    avg;            // PELT负载统计
    // ... 其他字段
};

// sched_entity - CFS调度实体
struct sched_entity {
    struct load_weight  load;           // 权重
    struct rb_node      run_node;       // 红黑树节点
    u64                 vruntime;       // 虚拟运行时间
    // ... 其他字段
};

2. 调度类名称还在

const struct sched_class fair_sched_class;  // 还是叫fair_sched_class

3. CFS特性配置都还在

 配置项                    状态
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 CONFIG_FAIR_GROUP_SCHED   ✅ 还在
 CONFIG_CFS_BANDWIDTH      ✅ 还在
 CONFIG_SCHED_HRTICK       ✅ 还在
 CONFIG_SCHED_AUTOGROUP    ✅ 还在
 CONFIG_SCHED_CORE         ✅ 还在
 CONFIG_SCHED_SMT          ✅ 还在
 CONFIG_SCHED_MC           ✅ 还在

4. PELT (Per-Entity Load Tracking) 还在

update_load_avg();  // PELT更新函数还在使用

5. CFS文档还在

Documentation/scheduler/sched-design-CFS.rst  # CFS设计文档
Documentation/scheduler/sched-eevdf.rst       # EEVDF新文档

──────────────────────────────────────────────────────────────────────────────────────────
什么被替换了？

核心调度算法：从 vruntime-based → deadline-based

 组件         CFS                                    EEVDF
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 主选择函数   pick_next_entity() (基于最小vruntime   __pick_eevdf() (基于virtual deadline
              )                                      )
 时间片计算   固定公式                               基于 lag 和 deadline
 抢占决策     tick驱动                               deadline驱动

关键变化：pick_next_task_fair() 内部实现

// CFS时代：选择vruntime最小的
pick_next_entity() -> 找最小vruntime的任务

// EEVDF时代：选择virtual deadline最小的
__pick_eevdf() -> 找 eligible 且 deadline 最小的任务

──────────────────────────────────────────────────────────────────────────────────────────
通俗比喻

想象CFS是一辆旧车：

• 🚗 车身框架 (cfs_rq, sched_entity) - 还在
• 🔧 变速箱系统 (PELT, group scheduling) - 还在
• 🎛️ 仪表盘 (fair_sched_class) - 还在
• 📖 使用手册 (文档) - 还在

但是：

• 🧠 引擎 (调度算法) - 换成了EEVDF
• 🎯 导航系统 (任务选择逻辑) - 换成了基于deadline

──────────────────────────────────────────────────────────────────────────────────────────
结论

CFS的"东西"大部分都在——数据结构、配置选项、文档、甚至代码注释里都还有"CFS"字样。但是核心的
调度算法从vruntime-based换成了deadline-based。

所以内核代码里同时存在：

• ✅ "CFS"的遗产（数据结构、带宽控制、组调度等）
• ✅ "EEVDF"的新实现（调度决策、deadline计算等）

这就是为什么fair_sched_class还叫fair，cfs_rq还叫cfs，只是pick_next的实现变了。
```

## codex 分析

eevdf 的核心考虑的事情只有三个:
1. 计算谁“该被服务”
   update_entity_lag() 和 entity_eligible()。
2. 在“该被服务”的任务里挑谁先跑
   __pick_eevdf() / pick_eevdf()。
3. 给每个任务维护一个虚拟 deadline，并决定何时触发 resched
   update_deadline()，再配合 protect_slice()。

CFS 时代你可以粗暴理解成：

- 红黑树按 vruntime 排
- 谁 vruntime 最小，谁最该跑

EEVDF 改成了两阶段判断，内核注释就在 kernel/sched/fair.c:903：

1. task 必须 eligible
2. 在 eligible 的 task 里，选 virtual deadline 最早的

也就是从“只看谁跑得最少”变成了：

- 先看你是不是“欠服务”
- 再看你这次请求的 deadline 谁更早

最核心的变化有这几个。

1. 新增 lag / eligible 这一层
CFS 的核心标尺基本是 vruntime。
EEVDF 引入了 lag，见 kernel/sched/fair.c
```c
/*
 * lag_i = S - s_i = w_i * (V - v_i)
 *
 * However, since V is approximated by the weighted average of all entities it
 * is possible -- by addition/removal/reweight to the tree -- to move V around
 * and end up with a larger lag than we started with.
 *
 * Limit this to either double the slice length with a minimum of TICK_NSEC
 * since that is the timing granularity.
 *
 * EEVDF gives the following limit for a steady state system:
 *
 *   -r_max < lag < max(r_max, q)
 *
 * XXX could add max_slice to the augmented data to track this.
 */
static void update_entity_lag(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	s64 vlag, limit;

	WARN_ON_ONCE(!se->on_rq);

	vlag = avg_vruntime(cfs_rq) - se->vruntime;
	limit = calc_delta_fair(max_t(u64, 2*se->slice, TICK_NSEC), se);

	se->vlag = clamp(vlag, -limit, limit);
}
```

直觉上：

- lag >= 0：这个任务还欠服务，eligible
- lag < 0：这个任务已经超前服务了，暂时不该优先

判断入口是 kernel/sched/fair.c:738。

2. 不再直接“挑最小 vruntime”，而是“挑最早 eligible deadline”
核心挑选函数是 kernel/sched/fair.c:918。

它不是简单拿左边最小 vruntime，而是：

- rb-tree 仍然有序
- 但树上的排序核心变成 deadline
- 同时借助 min_vruntime 做剪枝，判断子树里有没有 eligible entity

所以它本质上是：

- 先过滤掉不 eligible 的
- 再在剩下的里面找 earliest deadline

3. 每个 entity 都维护一个虚拟 deadline
见 kernel/sched/fair.c:1025。

公式在代码注释里：

vd_i = ve_i + r_i / w_i

也就是：

- ve_i 近似对应当前虚拟执行位置
- r_i 是这次请求的 slice
- w_i 是权重

所以 EEVDF 显式把“请求大小”放进了调度决策里。
这正是它相比老 CFS 更关键的地方之一。

4. 增加了 slice protection / run-to-parity 机制
见：

- kernel/sched/fair.c:866
- kernel/sched/fair.c:888

这部分是为了避免当前任务刚上 CPU 就被过早打断，给它一个最小受保护运行区间。
所以 EEVDF 不是单纯“谁 deadline 更早就立即抢”，还会考虑当前任务的受保护 slice。

一句话总结 CFS -> EEVDF 的核心变化

CFS 更像：

- 维护公平
- 按 vruntime 最小优先

EEVDF 更像：

- 先看谁欠服务
- 再按虚拟 deadline 排先后
- 让不同请求大小的任务有更好的延迟表现

所以不是把 CFS 全推翻了，而是在 fair scheduling 这层，从：

- minimum vruntime first

变成：

- eligible + earliest virtual deadline first

## eevdf 中的 lag 的含义
<!-- ff88f7ad-fcb2-4bbd-bd51-a0a3176150d5 -->

源码注释在 kernel/sched/fair.c:566 开始：

lag_i = S - s_i = w_i * (V - v_i)

你可以先别管符号，先理解量：

- S
  理想公平世界里，这个任务到现在应该得到的服务
- s_i
  它实际上得到的服务
- V
  系统整体的“平均虚拟时间”
- v_i
  这个任务自己的 vruntime
- w_i
  任务权重

所以：

- lag > 0：它拿少了，欠服务
- lag < 0：它拿多了，超前服务
- lag = 0：刚好公平

3. Linux 里怎么实现 lag

Linux 不会每次都真的精确算 S - s_i，而是用 avg_vruntime(cfs_rq) - se->vruntime 近似。

核心函数是 kernel/sched/fair.c:693：

vlag = avg_vruntime(cfs_rq) - se->vruntime;
se->vlag = clamp(vlag, -limit, limit);

这里的 se->vlag 就是内核里保存的 lag 近似值。


avg_vruntime(cfs_rq) 之所以不是普通平均，是因为调度器的公平目标本来就不是“每个 task 平均”，而是“按 weight 平均”。
所以它必须算：
```txt
Σ(vruntime * weight) / Σ(weight)
```
而 Linux 为了避免大数溢出，实际存成：
```txt
zero_vruntime + Σ((vruntime - zero_vruntime) * weight) / Σ(weight)
```

这里需要说明一下，其实 vruntime 已经有一层 weight 的考虑了，但是在计算 avg_vruntime 的时候，需要对于
vruntime 再来进行一次 weight


## eevdf 中的 virtual deadline 是什么意思
<!-- f214acea-6eb4-4212-9e56-8d9cbaeb1e33 -->

1. lag 决定你有没有资格上桌
2. virtual deadline 决定上桌的人里谁先吃饭

“如果这个任务这次请求的服务量按公平速度推进，它最晚应该在虚拟时间轴上的哪里完成。”

它不是墙上时钟的 deadline，不是 SCHED_DEADLINE 那种真实纳秒 deadline。
它是 CFS/EEVDF 自己内部虚拟时间坐标系里的 deadline。

1. 先看源码里的公式
```c
/*
 * XXX: strictly: vd_i += N*r_i/w_i such that: vd_i > ve_i
 * this is probably good enough.
 */
static bool update_deadline(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if ((s64)(se->vruntime - se->deadline) < 0)
		return false;

	/*
	 * For EEVDF the virtual time slope is determined by w_i (iow.
	 * nice) while the request time r_i is determined by
	 * sysctl_sched_base_slice.
	 */
	if (!se->custom_slice)
		se->slice = sysctl_sched_base_slice;

	/*
	 * EEVDF: vd_i = ve_i + r_i / w_i
	 */
	se->deadline = se->vruntime + calc_delta_fair(se->slice, se);

	/*
	 * The task has consumed its request, reschedule.
	 */
	return true;
}
```

se->deadline = se->vruntime + calc_delta_fair(se->slice, se);

注释写的是：

EEVDF: vd_i = ve_i + r_i / w_i

你可以把它读成：

- ve_i：这个任务当前的虚拟执行位置，也就是 vruntime
- r_i：这次请求的服务量，也就是 slice
- w_i：权重

所以：

- 当前虚拟位置
- 再加上“这次请求折算成虚拟时间要走多远”
- 得到这个任务的虚拟 deadline


4. 为什么 deadline 和 slice 有关

这是 EEVDF 很关键的味道。

如果一个任务这次“请求”很小，比如：

- 它只需要很短一段 CPU burst

那它的：

deadline = vruntime + 很小的一段虚拟增量

就会更早。

而一个需要更长 slice 的任务：

- deadline 会更靠后

所以 EEVDF 天然更容易让“小请求”低延迟完成。

这就是它相对老 CFS 一个非常重要的改进点：

- 不只是看谁更“落后”
- 还看这次请求“短不短”

———

5. 为什么要除以权重

你可以看源码里的 calc_delta_fair(se->slice, se)，它本质上就是按权重把真实 slice 转成虚拟增量。

直觉上：

- 权重大，虚拟时间走得慢
- 同样一个 slice，对重任务来说换算成的虚拟增量更小
- 所以它的 virtual deadline 会更近一些

这符合 fair share 的定义，因为高权重任务本来就应该更快拿到更多服务。

———

6. 一个直觉例子

假设两个任务都 eligible，现在都在同一个 vruntime 附近：

- A: vruntime = 100, slice = 5
- B: vruntime = 100, slice = 20

不管先不先考虑 weight，直觉上：

- A 这次请求更短
- A 的 virtual deadline 更早
- 所以 A 更容易先被选中

这就是 EEVDF 在交互任务、小 burst 任务上更有优势的根源。

———

7. 它和 vruntime 的区别

- vruntime
  表示“你现在已经跑到哪里了”
- virtual deadline
  表示“你这次请求如果按公平进度完成，终点应该在哪里”

所以：

- vruntime 是当前位置
- deadline 是这次服务窗口的终点

可以把它想成：

当前位置 + 本次请求长度 = 本次 deadline

只是这个“长度”是在虚拟时间轴上量出来的。

———

8. 为什么 rb-tree 按 deadline 排

在 EEVDF 里，树上的主排序关系是 deadline，见 kernel/sched/fair.c:544：

return (s64)(a->deadline - b->deadline) < 0;

也就是说，红黑树的左边是更早 deadline 的任务。

但并不是直接取最左边就完事，因为还要先过 eligible 这一关。
所以 kernel/sched/fair.c:918 才会：

- 树按 deadline 有序
- 同时用 min_vruntime 做 eligibility 剪枝
- 找到最早 eligible deadline

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
