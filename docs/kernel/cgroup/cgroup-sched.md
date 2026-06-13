# cgroup sched

## cgroup cpu 接口

<!-- 8424aad4-f20d-407a-8e41-0b064a7975d7 -->
| 接口            | 说明                                                                                        |
|-----------------|---------------------------------------------------------------------------------------------|
| cpu.idle        | 只有在 idle 的时候才可以使用                                                                |
| cpu.max         |
| cpu.max.burst   |
| cpu.pressure    | psi 机制                                                                                    |
| cpu.stat        |
| cpu.stat.local  | 仅仅关心本层级的，不用关心下层级的                                                          |
| cpu.uclamp.max  | ?
| cpu.uclamp.min  |
| cpu.weight      | CFS 调度权重
| cpu.weight.nice | (chatgpt) 使用 nice 值映射为 cpu.weight ，为兼容传统 Linux 调度语义，实际上是对 cpu.weight 的一层抽象 |

```txt
🧀  cat cpu.stat
usage_usec 0
user_usec 0
system_usec 0
nice_usec 0
core_sched.force_idle_usec 0
nr_periods 0
nr_throttled 0
throttled_usec 0
nr_bursts 0
burst_usec 0
```

CPU controller 实际上同时处理四类完全不同的问题：

1. **公平性**
   谁和谁竞争时应该多拿一点 CPU。对应 `cpu.weight` / `cpu.weight.nice` / `cpu.idle`。
2. **隔离性**
   某个 cgroup 最多只能吃多少 CPU。对应 `cpu.max` / `cpu.max.burst`。
3. **可观测性**
   到底是“没跑到”还是“被 quota 卡住了”。对应 `cpu.pressure` / `cpu.stat` / `cpu.stat.local`。
4. **性能意图**
   调度器做 util/freq 放大或压制时，应该按什么上下限估计。对应 `cpu.uclamp.min` / `cpu.uclamp.max`。

如果把这几类东西混起来，就会出现两个常见误判：

- 把 `cpu.weight` 当成上限用。实际上它只在竞争时起作用。
- 把 `cpu.pressure` 当成 throttle 统计。实际上它只说明“在等 CPU”，并不等于 quota 触发。

1. `cpu.weight` vs `cpu.weight.nice`

这两个本质上是同一个旋钮，只是单位不同：

- `cpu.weight` 范围大，细粒度，区间是 `[1, 10000]`
- `cpu.weight.nice` 用 `nice` 的 `[-20, 19]` 映射到权重，粒度粗很多

实战上：

- 需要精确比例时，用 `cpu.weight`
- 想和进程级 nice 概念统一时，用 `cpu.weight.nice`

我在这台机器上测试：

- `low` 设置 `cpu.weight=100`
- `high` 设置 `cpu.weight.nice=-5`
- 两个 busy loop 绑同一个 CPU 跑 3 秒

结果 `high` 的 `usage_usec` 明显更高，说明它们确实是同一类“竞争权重”控制，而不是硬限制。

2. `cpu.idle` vs `cpu.weight`

`cpu.idle=1` 不是“把 weight 调低一点”这么简单，而是把整个 cgroup 变成类似 `SCHED_IDLE` 的组。

区别：

- `cpu.weight` 仍然参与正常公平竞争
- `cpu.idle=1` 表示“只有别人都不急着跑时，我再跑”

实测同一 CPU 上一个 normal 组、一个 idle 组同时跑 3 秒，idle 组几乎拿不到 CPU。

注意一点：当前机器内核 `6.19.8-100.fc42.x86_64` 上，`cpu.idle=1` 之后读回：

- `cpu.weight=1`
- `cpu.weight.nice=19`

而 `Documentation/admin-guide/cgroup-v2.rst` 里还写“idle group 的 `cpu.weight` 读出来是 0”。这说明文档和当前实现已经有差异，不能只靠记忆判断。

3. `cpu.max` vs `cpu.max.burst`

`cpu.max` 是硬 quota，格式是：

```txt
$MAX $PERIOD
```

例如：

```txt
20000 100000
```

表示每 100ms 最多只能跑 20ms 。
**cpu.max 限制的是整个 cgroup 在所有 CPU 上的累计 CPU 时间，不是每个 CPU 单独限制。**

`cpu.max.burst` 则是“临时可借的额度”：

- `0` 表示完全不借
- 正值表示允许短时超额运行，但长期平均仍受 `cpu.max` 约束

它们的关系不是替代，而是：

- `cpu.max` 决定长期硬上限
- `cpu.max.burst` 决定短期能不能约过上限

4. `cpu.pressure` vs `cpu.stat`

这是最容易混的两个观察接口。

`cpu.pressure` 看的是：

- 任务已经 runnable
- 但因为 CPU 忙，没能立刻被调度

所以它回答的是：**“排队严重吗？”**

`cpu.stat` 看的是：

- 已经实际跑掉的 CPU 时间：`usage_usec` / `user_usec` / `system_usec`
- quota/throttle 统计：`nr_periods` / `nr_throttled` / `throttled_usec` / `nr_bursts` / `burst_usec`

所以它回答的是：**“执行了多少？被 quota 卡了多少次？”**

经验上：

- `cpu.pressure` 高，不一定有 `nr_throttled`
- `nr_throttled` 高，也不一定代表外部系统整体 CPU 忙，只是这个 cgroup 被 quota 卡住了

5. `cpu.stat` vs `cpu.stat.local`

// cpu.stat 的 throttled_usec 来源（kernel/sched/core.c）
throttled_usec = cfs_b->throttled_time;  // 总计值，包含所有子 cgroup

// cpu.stat.local 的 throttled_usec 来源（kernel/sched/core.c）
throttled_self_usec = throttled_time_self(tg);  // 仅自身，遍历 throttled_clock_self_time


 接口             统计范围                                包含内容
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 cpu.stat         总计值（包含子 cgroup）                 完整的 CPU 统计信息
 cpu.stat.local   仅当前 cgroup 自身（不包含子 cgroup）   仅节流相关统计

1. cpu.stat : 该文件包含两部分数据：
	1. 基础 CPU 时间（始终存在）：
	  • usage_usec - 总 CPU 使用时间
	  • user_usec - 用户态时间
	  • system_usec - 内核态时间
	2. CFS 带宽控制统计（启用 CPU 控制器时）：
	  • nr_periods - 经过的周期数
	  • nr_throttled - 被节流的周期数
	  • throttled_usec - 总计节流时间（含子 cgroup）
	  • nr_bursts, burst_usec - 突发相关统计
2. cpu.stat.local
仅包含当前 cgroup 自身的节流时间：
throttled_usec - 仅当前 cgroup 自身的节流时间（不含子 cgroup）

6. `cpu.uclamp.min` vs `cpu.uclamp.max`

这俩不是 quota，也不是 share。

它们控制的是 **utilization clamp**，即调度器在做：

- capacity fitting
- energy aware placement
- schedutil 频率选择

时，任务利用率估计值的上下边界。

可以这样理解：

- `cpu.uclamp.min`：保底，不要把我估计得太低
- `cpu.uclamp.max`：封顶，不要把我估计得太高

它影响的是调度器“怎么看这个 cgroup 的任务”，不是“这个 cgroup 最多只能跑多少微秒”。

因此：

- 想做 CPU 时间硬隔离，用 `cpu.max`
- 想做频率/性能意图提示，用 `cpu.uclamp.*`

### 总结
1. cpu.max : 对应 bandwidth
2. cpu.weight : 对应 share

### 实现
配置参数，最后会调用到
- tg_set_cfs_bandwidth
- sched_group_set_shares

```txt
@[
        tg_set_cfs_bandwidth+1
        tg_set_bandwidth+133
        cpu_max_write+218
        kernfs_fop_write_iter+369
        vfs_write+641
        ksys_write+123
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 74

@[
        sched_group_set_shares+5
        cpu_weight_write_u64+74
        cgroup_file_write+367
        kernfs_fop_write_iter+369
        vfs_write+641
        ksys_write+123
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 71
```

## 两个基本问题

### cgroup cpu.weight和 setpriority/nice 的关系

他们是分层叠加的，不是互相覆盖。

sched_init 中注释:
```c
		/*
		 * How much CPU bandwidth does root_task_group get?
		 *
		 * In case of task-groups formed through the cgroup filesystem, it
		 * gets 100% of the CPU resources in the system. This overall
		 * system CPU resource is divided among the tasks of
		 * root_task_group and its child task-groups in a fair manner,
		 * based on each entity's (task or task-group's) weight
		 * (se->load.weight).
		 *
		 * In other words, if root_task_group has 10 tasks of weight
		 * 1024) and two child groups A0 and A1 (of weight 1024 each),
		 * then A0's share of the CPU resource is:
		 *
		 *	A0's bandwidth = 1024 / (10*1024 + 1024 + 1024) = 8.33%
		 *
		 * We achieve this by letting root_task_group's tasks sit
		 * directly in rq->cfs (i.e root_task_group->se[] = NULL).
		 */
```
root_task_group 包含：
   ├─ 10 个普通任务，每个权重 1024
   ├─ 子任务组 A0，权重 1024
   └─ 子任务组 A1，权重 1024

总权重 = 10×1024 + 1024 + 1024 = 12288
A0 的 CPU 带宽 = 1024 / 12288 = 1/12 ≈ 8.33%

cpu.weight 决定整个 cgroup 能从父级抢到多少 CPU 时间；nice 决定这些 CPU 时间在 cgroup 内部 怎么分给各个进程。两者是乘法关系。

例如对这个例子：
```txt
  root rq->cfs
    task A
    group se for cgroup X        ← cpu.weight 决定这个 group se 的权重
      cgroup X cfs_rq
        task B                    ← nice 决定 task B 的 se 权重
        task C                    ← nice 决定 task C 的 se 权重
```

第一层：cgroup vs cgroup（cpu.weight 生效的地方）

cpu.weight 影响的是 group sched_entity 在父级 cfs_rq 里的权重：

```c
  struct task_group {
      unsigned long shares;   // 来自 cpu.weight
  };
```

父级调度器看到的是一个 group entity，权重是 shares。兄弟 cgroup 之间按 shares 比例分配父级时间。

第二层：进程 vs 进程（nice 生效的地方）

当调度器进入某个 cgroup 的 cfs_rq 后，面对的是这个 cgroup 内部的 tasks。此时：

```c
  struct sched_entity {
      struct load_weight load;   // 来自进程的 nice
  };
```

nice -20 ~ 19 会映射成一个 load.weight：
- nice 0 → weight ≈ 1024
- nice 越低，weight 越大
- nice 越高，weight 越小

cgroup 内部的 tasks 按各自的 load.weight 比例分配该 cgroup 抢到的时间。

假设 CPU 上只有两个 cgroup：

┌────────┬────────────┬──────────┬────────────┐
│ cgroup │ cpu.weight │ 内部进程 │ nice       │
├────────┼────────────┼──────────┼────────────┤
│ A      │ 100        │ a1, a2   │ a1=0, a2=5 │
├────────┼────────────┼──────────┼────────────┤
│ B      │ 300        │ b1       │ 0          │
└────────┴────────────┴──────────┴────────────┘

运行结果大概是：

1. A 和 B 之间：按 weight 100:300 = 1:3 分配 CPU 时间。
    - A 拿到 25%
    - B 拿到 75%
2. A 内部：a1 (nice 0, weight 1024) 和 a2 (nice 5, weight 335) 分 A 的 25%。
    - a1 ≈ 25% × 1024/(1024+335) ≈ 18.8%
    - a2 ≈ 25% × 335/(1024+335) ≈ 6.2%
3. B 内部：b1 独享 B 的 75%。

边界情况

1. SCHED_FIFO/RR/DEADLINE：实时调度器。文档里明确写了：
   - cpu.weight、cpu.max 主要影响 fair-class 任务；SCHED_FIFO/RR/DEADLINE 不按这套 CFS group bandwidth 运行。
2. autogroup：如果内核开了 CONFIG_SCHED_AUTOGROUP，同 TTY 的进程会被自动归为一个 autogroup，有自己的 task_group。这时 nice
   会先作用到 autogroup 的 shares 上，而不是直接作用到进程。


### scheduler 如何处理多个 core
<!-- b80bc0b9-014d-49e7-afbc-54b4907d9b45 -->

32 core
A: 30 threads, nice 0
B: 20 threads, nice 10
全部 CPU-bound / runnable
无 cgroup、无绑核限制

那么总权重大致是：
A = 30 * 1024 = 30720
B = 20 * 110  = 2200

总权重 = 32920
所以理想长期 CPU 份额：
A = 30720 / 32920 = 93.3%
B = 2200  / 32920 = 6.7%

1. balance 机制来负责任务调度到哪一个 core 上的
2. 调度器关注的是 thread，而不是 process ，所以的确存在一个情况，process 越多，那么占用的 CPU 越多
    但是这不是一个严重问题，因为可以使用 cgroup 来限制
3. 当计算 rbtree 的时候，都是仅仅考虑一个 CPU 上排序


现在来思考一个 cgroup ，其实和普通的 thread 相同，rbtree 只是多了一个层级而已
加入 cgroup 为:

root
    - A
        - a b c
    - B
        - d e f
    - C
        - x y

对于 cpu 12 ，首先 balance 选择 cgroup A B C 谁来，如果选择了 A ，然后在其中计算 a b c
不过，假设一个 CPU 17 上有 A 和 B ，如下两个情况:

A : a b
B : d

和
A : a
B : d e f

显然在 CPU 17 的 rbtree 中， A B 的权重不可以简单按照 A 和 B 的 tg->shares 来计算

#### cgroup 的 share 如何计算

calc_group_shares 上有一个非常长的注释，讲解计算方法，也是一直没看懂的东西了:
```txt
                         tg->shares * grq->load.weight
    ge->load.weight = --------------------------------------
                      sum_over_cpus(grq->load.weight)
```
这个公式简单来说，就是在一个 cpu 上排序的时候，要看当前 cgroup 有多少个 process 在这个 cpu 上:

grq->load.weight
  是 group 内部这个 CPU 上的 runnable entity 权重总和。

ge->load.weight
  是这个 group 在父 cfs_rq 里参与竞争时使用的权重，也就是

可以把它们看成“内部负载”和“对外代表权重”。

比如 group A 在 CPU0 上有两个 nice 0 task：

A 的 grq on CPU0:
  task1 weight = 1024
  task2 weight = 1024

grq->load.weight = 2048

但父层 root cfs_rq 里不会直接看到 task1/task2，而是看到一个代表 group A
的 ge：

root cfs_rq on CPU0:
  group A 的 ge
  group B 的 ge

这个 ge->load.weight 不是简单等于 2048，而是由 calc_group_shares() 算出
来：

ge->load.weight = tg->shares * 当前 CPU 上 group 的负载占比

所以关系是：

grq->load.weight 决定这个 CPU 应该分到 tg->shares 的多少比例；
ge->load.weight 是分配后的结果，用来让父层调度器调度这个 group。

如果 group A 只在 CPU0 有 runnable load，其他 CPU 上没有：

sum_over_cpus(grq->load.weight) = CPU0 的 grq->load.weight

那么：

ge->load.weight = tg->shares

也就是说，哪怕 grq->load.weight = 2048，只要它是这个 group 唯一活跃的
CPU，ge->load.weight 也会接近整个 tg->shares，比如 1024。

如果 group A 在两个 CPU 上负载一样：

CPU0 grq->load.weight = 2048
CPU1 grq->load.weight = 2048
tg->shares = 1024

那么：

CPU0 ge->load.weight = 1024 * 2048 / (2048 + 2048) = 512
CPU1 ge->load.weight = 512

所以不是：

ge->load.weight == grq->load.weight

而是：

ge->load.weight = tg->shares 在各 CPU 的 grq->load.weight 间按比例切出来的一份

grq->load.weight 是“这个 CPU 上 group 内部有多重”；ge->load.weight 是“这个 CPU 上这个 group 对父层表现出多重”。

## 关键结构体

```c
struct task_group {
	struct cgroup_subsys_state css;
	struct sched_entity **se;
	struct cfs_rq **cfs_rq;
	unsigned long shares;
	atomic_long_t load_avg;
	struct task_group *parent;
	struct list_head children;
	struct cfs_bandwidth cfs_bandwidth;
};
```
- `css` 把 cgroup core 的对象连接到调度器对象。
- `se[cpu]` 是这个 group 在父级 `cfs_rq` 中的调度实体。
- `cfs_rq[cpu]` 是这个 group 在该 CPU 上拥有的 CFS runqueue。
- `shares` 来自 `cpu.weight`，是静态配置。
- `load_avg` 是 PELT 聚合出来的动态负载。
- `cfs_bandwidth` 保存 `cpu.max`、`cpu.max.burst`、period timer、throttle list 和统计。

```c
struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
    raw_spinlock_t      lock;
    ktime_t         period;
    u64         quota;
    u64         runtime; // 记录限额剩余时间，会使用quota值来周期性赋值；
    s64         hierarchical_quota;

    u8          idle;
    u8          period_active; // 周期性计时已经启动；
    u8          slack_started;
    struct hrtimer      period_timer;
    struct hrtimer      slack_timer; // 延迟定时器，在任务出列时，将剩余的运行时间返回到全局池里；
    struct list_head    throttled_cfs_rq;

    /* Statistics: */
    int         nr_periods;
    int         nr_throttled;
    u64         throttled_time;
#endif
};
```

1. throttle 是按 CPU 的 cfs_rq 执行的。全局池空的那一刻，正在扣费的那个 CPU 的 cfs_rq 被挂到 cfs_bandwidth.throttled_cfs_rq 链表。

## CONFIG_FAIR_GROUP_SCHED

CONFIG_FAIR_GROUP_SCHED 是 CFS 组调度（group scheduling） 的总开关。
它控制内核是否支持 把一组进程当作一个整体调度单元。

- 开启时：CFS 的 rbtree 里可以有两种 sched_entity：
    - task entity：普通进程
    - group entity：代表一个 cgroup 的 task_group

  调度器 pick_next_task_fair 遇到 group entity 时，会递归进入它的
  cfs_rq，继续向下挑选。这就是你文档里画的那个层级树能工作的前提。
- 关闭时：每个 CPU 只有一个 root cfs_rq，所有进程直接在里面按 nice/vruntime 竞争。不存在 task_group 层级，cpu.weight 和 cpu.max 等 cgroup CPU controller 完全失效。

┌─────────────────────────────────┬────────────────────────────────────┐
│ 配置                            │ 依赖 CONFIG_FAIR_GROUP_SCHED       │
├─────────────────────────────────┼────────────────────────────────────┤
│ CONFIG_CGROUP_SCHED             │ 是                                 │
├─────────────────────────────────┼────────────────────────────────────┤
│ CONFIG_CFS_BANDWIDTH（cpu.max） │ 是                                 │
├─────────────────────────────────┼────────────────────────────────────┤
│ CONFIG_RT_GROUP_SCHED           │ 通常一起开，但 RT 组调度是独立选项 │
└─────────────────────────────────┴────────────────────────────────────┘

简单说：如果没有 CONFIG_FAIR_GROUP_SCHED，你之前 dump 的那个 task_group -> cfs_rq -> rbtree 层级结构根本不存在，cgroup v2 的 CPU
controller 就只剩一个空壳。

## bandwidth 实现细节

### cfs_bandwidth::period_timer 和 cfs_bandwidth::slack_timer

1. period_timer — 周期配额刷新定时器

作用：周期性为任务组重新填充带宽配额，并解限流（unthrottle）被限流的 cfs_rq。

• 这是一个绝对时间的周期定时器（HRTIMER_MODE_ABS_PINNED），按 cfs_period_us 周期触发。
• 每次到期时调用 do_sched_cfs_period_timer()，主要工作包括：
    1. 配额充值：调用 __refill_cfs_bandwidth_runtime() 重新填充本周期可用 runtime（包括 burst 配额）。
    2. 统计周期：递增 nr_periods；如果存在被限流的队列，递增 nr_throttled。
    3. 分发 runtime 并解限流：如果有 cfs_rq 因耗尽配额而被限流（throttled）且当前有可用 runtime，则调用
       distribute_cfs_runtime() 将 runtime 分发给它们，使其恢复调度。
    4. 空闲优化：如果该任务组在上个周期没有任何活动（idle），且当前也没有被限流的队列，则停止定时器以减少开销；待有新活动时再
       重新启动。

这是带宽控制的"心跳"，决定了配额何时重置。

2. slack_timer — 剩余配额回收定时器

作用：延迟一小段时间，将本周期内未用完的剩余 runtime（slack）收集起来，分发给其他被限流的 cfs_rq。

• 这是一个相对时间定时器（HRTIMER_MODE_REL），默认延迟 5ms（cfs_bandwidth_slack_period）。
• 它不是周期性的，而是在特定条件下由 start_cfs_slack_bandwidth() 按需启动：
    • 当某个 cfs_rq 变空闲或交出剩余 runtime 时，如果检测到当前周期即将结束（快到 period_timer
      刷新时间），与其让这些剩余配额随周期重置浪费掉，不如启动 slack_timer 等待一小段时间后分发出去。
• 到期时调用 do_sched_cfs_slack_timer()，主要工作：
    1. 检查是否快到周期刷新时间了（runtime_refresh_within()），如果是则放弃，因为新周期马上会重新充值。
    2. 如果当前剩余 runtime 大于一个 slice（sched_cfs_bandwidth_slice()），调用 distribute_cfs_runtime() 分发出去解限流。
    3. 重置 slack_started 标志。

它的目的是提高带宽利用率：避免周期末尾剩余的一点点配额被浪费，优先用于解限流等待的队列。

### unthrottle_cfs_rq 和 throttle_cfs_rq

```txt
@[
        throttle_cfs_rq+1
        put_prev_entity+113
        pick_next_task_fair+337
        pick_next_task+106
        __schedule+368
        schedule+39
        irqentry_exit+193
        asm_sysvec_apic_timer_interrupt+26
]: 43
```


unthrottle_cfs_rq 和 throttle_cfs_rq
cfs_bandwidth::period_timer 和 cfs_bandwidth::slack_timer

## cgroup 中的 cpuset cpu 和 cpuacct
<!-- decd39a3-6b36-4f9e-bb89-b398a51df8a0 -->

```c
#if IS_ENABLED(CONFIG_CPUSETS)
SUBSYS(cpuset)
#endif

#if IS_ENABLED(CONFIG_CGROUP_SCHED)
SUBSYS(cpu)
#endif

#if IS_ENABLED(CONFIG_CGROUP_CPUACCT)
SUBSYS(cpuacct)
#endif
```

cgroup v2 没有单独的 `cpuacct` controller，不表示 CPU accounting 消失了；`usage_usec`、`user_usec`、`system_usec` 被合入 `cpu.stat`，由 cgroup core 和 CPU controller 暴露。

cpuacct：“用了多少 CPU”
cpuacct 是一个**纯统计（accounting）**控制器，负责：
- 统计 cgroup 消耗的 CPU 时间
- 不参与调度决策
- 常用于监控、计费、资源观测


观察 CONFIG_CGROUP_CPUACCT 起作用的三个位置为:
```txt
include/linux/cgroup.h:#ifdef CONFIG_CGROUP_CPUACCT
kernel/sched/build_utility.c:#ifdef CONFIG_CGROUP_CPUACCT
include/linux/cgroup_subsys.h:#if IS_ENABLED(CONFIG_CGROUP_CPUACCT)
```


```c
struct cgroup_subsys cpuacct_cgrp_subsys = {
	.css_alloc	= cpuacct_css_alloc,
	.css_free	= cpuacct_css_free, // 定义的都是 v1 中的文件
	.legacy_cftypes	= files,
	.early_init	= true,
};
```

## 关键参考
1. CFS 调度器（3）-组调度 : http://www.wowotech.net/process_management/449.html

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
