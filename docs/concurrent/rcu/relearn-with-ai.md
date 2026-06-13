# rcu by kimi
## 什么可以导致进入  quiescent state
<!-- 231d1717-a1ac-4645-ac24-58993fae3fa7 -->

可以的:
- 用户态执行
- idle 循环
- 从 RCU 读临界区退出

不可以的:
- 执行硬件中断/软中断（可能持有 RCU）
- 在 `rcu_read_lock()` / `rcu_read_unlock()` 之间


## 细节
```c
// 核心检查函数: kernel/rcu/tree_stall.h
static void check_cpu_stall(struct rcu_data *rdp)
{
    // 1. 读取当前 GP 序列号（用于检测 GP 是否已推进）
    gs1 = READ_ONCE(rcu_state.gp_seq);

    // 2. 读取预期的 stall 检测时间点
    js = READ_ONCE(rcu_state.jiffies_stall);

    // 3. 读取 GP 开始时间
    gps = READ_ONCE(rcu_state.gp_start);

    // 4. 再次读取 GP 序列号（检测一致性）
    gs2 = READ_ONCE(rcu_state.gp_seq);

    // 5. 如果以下任一条件满足，则不认为是 stall：
    //    - GP 序列号已变化（GP 在推进）
    //    - 当前时间未到预期的检测点
    //    - GP 开始后重新设置了检测点
    //    - 没有正在进行的 GP
    if (gs1 != gs2 ||
        ULONG_CMP_LT(jiffies, js) ||
        ULONG_CMP_GE(gps, js) ||
        !rcu_seq_state(gs2))
        return;  // 无 stall，或 GP 已完成

    // 6. 检测该 CPU 是否处于 QS 状态
    rnp = rdp->mynode;
    self_detected = READ_ONCE(rnp->qsmask) & rdp->grpmask;

    // 7. 如果本 CPU 的 QS 位仍在 qsmask 中，
    //    说明本 CPU 尚未报告 QS，可能正在阻塞 GP
    if (self_detected || /* ... */) {
        print_cpu_stall();  // 打印 stall 信息
    }
}
```

RCU Stall 触发原因分类

1. CPU 被长时间禁用中断
	- 如：在硬中断上下文执行长循环、spin_lock_irqsave 持有过久
2. 调度延迟
	- 如：高优先级任务抢占、RT 任务 starving RCU 内核线程
3. 长时间关闭抢占
	- 如：`preempt_disable()` 区间执行复杂操作
4. 虚拟机时间跳变
	- 如：VM 被 pause/restore、vMotion、调试断点
5. 死锁或活锁
	- 如：内核死锁导致特定 CPU 无法前进
6. RCU 内核线程饥饿
	- 如：`rcu_preempt` kthread 无法获得 CPU 时间

(那么，这里有一个问题，这里的 preempt_disable 如果在 preempt kernel ，估计有很大的变化)

### 3.1 Stall 检测调用链

```
update_process_times()           [kernel/time/timer.c]
    └── rcu_sched_clock_irq()    [kernel/rcu/tree.c]
            └── rcu_pending()    [kernel/rcu/tree.c]
                    ├── check_cpu_stall()      [kernel/rcu/tree_stall.h]
                    │       └── print_cpu_stall()
                    │               └── print_cpu_stall_info()  <-- 关键输出函数
                    └── invoke_rcu_core()
                            └── raise_softirq(RCU_SOFTIRQ)
                                    └── rcu_core()
```

### 3.2 关键数据结构

```c
// RCU 全局状态（每个 RCU flavor 一个，如 rcu_sched、rcu_preempt）
struct rcu_state {
    // GP 序列号（高2位是状态，低30位是计数）
    unsigned long gp_seq;

    // GP 开始时间（jiffies）
    unsigned long gp_start;

    // 下次 stall 检测时间
    unsigned long jiffies_stall;

    // 强制 QS 计数器
    unsigned long n_force_qs;
    unsigned long n_force_qs_gpstart;

    // RCU 节点树（层次结构）
    struct rcu_node node[NUM_RCU_NODES];

    // GP 内核线程
    struct task_struct *gp_kthread;
};

// 每 CPU RCU 数据
struct rcu_data {
    // 当前 CPU 持有的 callback 列表
    struct rcu_segcblist cblist;

    // 本 CPU 已知的 GP 序列号
    unsigned long gp_seq;

    // 本 CPU 当前 GP 中的 ticks 计数
    unsigned long ticks_this_gp;

    // 软中断快照（用于检测 softirq 是否前进）
    unsigned long softirq_snap;

    // 所属的 rcu_node
    struct rcu_node *mynode;

    // 在 rcu_node->qsmask 中的位掩码
    unsigned long grpmask;

    // 该 CPU 是否在 dynticks idle 模式
    int dynticks;
};

// RCU 节点（树结构）
struct rcu_node {
    // 哪些 CPU 还未报告 QS（位掩码）
    unsigned long qsmask;

    // 下次 GP 启动时的 qsmask
    unsigned long qsmaskinitnext;

    // 当前 GP 序列号
    unsigned long gp_seq;

    // 被抢占的 RCU 读临界区任务（用于 PREEMPT_RCU）
    struct list_head blkd_tasks;
};
```

### 3.3 报错字段解析源码

```c
// kernel/rcu/tree_stall.h: print_cpu_stall_info()
static void print_cpu_stall_info(int cpu)
{
    struct rcu_data *rdp = per_cpu_ptr(&rcu_data, cpu);
    unsigned long ticks_value;
    char *ticks_title;

    // 1. 计算落后 GP 数或当前 GP ticks
    ticks_value = rcu_seq_ctr(rcu_state.gp_seq - rdp->gp_seq);
    if (ticks_value) {
        ticks_title = "GPs behind";  // 落后多个 GP
    } else {
        ticks_title = "ticks this GP";  // 在当前 GP 中
        ticks_value = rdp->ticks_this_gp;
    }

    // 2. 计算从上次 irq_work 以来的 GP 差值
    delta = rcu_seq_ctr(rdp->mynode->gp_seq - rdp->rcu_iw_gp_seq);

    // 3. 检测是否可能是假阳性
    falsepositive = rcu_is_gp_kthread_starving(NULL) &&
                    rcu_watching_snap_in_eqs(ct_rcu_watching_cpu(cpu));

    // 4. 检测 rcuc kthread 是否饥饿
    rcuc_starved = rcu_is_rcuc_kthread_starving(rdp, &j);
    if (rcuc_starved)
        snprintf(buf, sizeof(buf), " rcuc=%ld jiffies(starved)", j);

    // 5. 打印核心信息行
    pr_err("\t%d-%c%c%c%c: (%lu %s) idle=%04x/%ld/%#lx softirq=%u/%u fqs=%ld%s%s\n",
           cpu,
           "O."[!!cpu_online(cpu)],           // O=online, .=offline
           "o."[!!(rdp->grpmask & rdp->mynode->qsmaskinit)],
           "N."[!!(rdp->grpmask & rdp->mynode->qsmaskinitnext)],
           !IS_ENABLED(CONFIG_IRQ_WORK) ? '?' :
               rdp->rcu_iw_pending ? (int)min(delta, 9UL) + '0' : "!."[!delta],
           ticks_value, ticks_title,
           ct_rcu_watching_cpu(cpu) & 0xffff,   // idle 状态低16位
           ct_nesting_cpu(cpu),                 // 嵌套计数
           ct_nmi_nesting_cpu(cpu),             // NMI 嵌套
           rdp->softirq_snap,                   // softirq 快照
           kstat_softirqs_cpu(RCU_SOFTIRQ, cpu), // 当前 softirq 计数
           data_race(rcu_state.n_force_qs) - rcu_state.n_force_qs_gpstart, // fqs
           rcuc_starved ? buf : "",
           falsepositive ? " (false positive?)" : "");
}
```

### 3.4 报错字段详细含义

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  典型 RCU Stall 报错格式解析                                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [  173.584884] rcu:     15-...0: (1 GPs behind) idle=aa24/1/0x4000000000000000 softirq=1113/1114 fqs=6014 │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │ 字段 1: "15-...0"                                                       ││
│  │                                                                         ││
│  │   15     - CPU 编号                                                     ││
│  │   .      - CPU online（. = online, 数字位置被省略）                     ││
│  │   .      - 在当前 GP 的 qsmaskinit 中（. = 在）                         ││
│  │   .      - 在下次 GP 的 qsmaskinitnext 中（. = 在）                     ││
│  │   0      - irq_work 响应延迟（数字 0-9 表示延迟的 GP 数, ! = 无延迟）   ││
│  │            0 表示有 irq_work 且未延迟                                   ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │ 字段 2: "(1 GPs behind)"                                                ││
│  │                                                                         ││
│  │   该 CPU 落后全局 GP 的数量。如果为 0，则显示 "(X ticks this GP)"       ││
│  │   表示该 CPU 在当前 GP 中已经历的时钟 ticks                             ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │ 字段 3: "idle=aa24/1/0x4000000000000000"                                 ││
│  │                                                                         ││
│  │   aa24           - dynticks 计数器（16进制）                             ││
│  │                    反映该 CPU 进出 idle 的次数                          ││
│  │   1              - RCU 读临界区嵌套深度                                  ││
│  │                    0 = 不在临界区, 1+ = 在临界区（嵌套层数）            ││
│  │   0x400...       - NMI 嵌套和其他状态位                                  ││
│  │                    0x4000000000000000 = 在用户态                        ││
│  │                    0x0000000000000000 = 在内核态                        ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │ 字段 4: "softirq=1113/1114"                                             ││
│  │                                                                         ││
│  │   1113  - 上次检查时记录的 RCU softirq 计数（快照）                      ││
│  │   1114  - 当前 RCU softirq 计数                                         ││
│  │                                                                         ││
│  │   如果两者相等，说明 RCU softirq 已停止处理                             ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │ 字段 5: "fqs=6014"                                                      ││
│  │                                                                         ││
│  │   Forced Quiescent State（强制静止状态）尝试次数                         ││
│  │   该 GP 期间，RCU 尝试通过 IPI 强制其他 CPU 报告 QS 的次数              ││
│  │   数值越大，说明 stall 持续时间越长                                     ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

第二行系统级信息：

```
[  173.585294] rcu:     (detected by 4, t=26002 jiffies, g=20189, q=340 ncpus=32)

┌─────────────────────────────────────────────────────────────────────────────┐
│ 字段解析:                                                                    │
│                                                                             │
│   detected by 4  - CPU 4 检测到该 stall                                    │
│   t=26002        - stall 持续时间（jiffies）                               │
│                    HZ=1000 → 26秒; HZ=250 → 104秒                          │
│   g=20189        - 当前 Grace Period 编号                                  │
│   q=340          - 等待的 callback 队列长度                                │
│   ncpus=32       - 系统 CPU 总数                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```


"If you are using synchronize_rcu_expedited() in a loop, please restructure your code to batch your updates, and then use a single synchronize_rcu() instead."


```
┌─────────────────────────────────────────────────────────────────────────┐
│  普通 synchronize_rcu() 工作流程                                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. 发起 GP 请求                                                         │
│        ↓                                                                │
│  2. 等待各 CPU "自然"报告 QS                                              │
│        ↓                                                                │
│     CPU 0        CPU 1        CPU 2        CPU 3                        │
│   [用户态]      [irq_exit]   [schedule]   [用户态]                      │
│      ↓            ↓            ↓            ↓                           │
│    报告QS       报告QS       报告QS       报告QS                         │
│        ↓                                                                │
│  3. GP 完成，唤醒等待者                                                  │
│                                                                         │
│  特点：被动等待，依赖正常调度路径                                        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│  synchronize_rcu_expedited() 工作流程                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. 发起 Expedited GP 请求                                               │
│        ↓                                                                │
│  2. 向所有在线 CPU 发送 IPI                                             │
│        ↓                                                                │
│     CPU 0        CPU 1        CPU 2        CPU 3                        │
│   [被IPI打断]  [被IPI打断]  [被IPI打断]  [被IPI打断]                    │
│      ↓            ↓            ↓            ↓                           │
│   IPI handler: 检查是否在读临界区                                        │
│      ├─ 是 → 设置 urgent 标志，等待读解锁时报告 QS                      │
│      └─ 否 → 立即报告 QS                                                │
│        ↓                                                                │
│  3. 所有 CPU 快速报告 QS，GP 完成                                        │
│                                                                         │
│  特点：主动打断，强制快速推进                                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

```txt
┌─────────────────────────────────────────────────────────────────────┐
│                    Expedited GP IPI Handler                          │
│                    (rcu_exp_handler 系列)                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  smp_call_function_single_interrupt()                                │
│        ↓                                                            │
│  rcu_exp_handler() / rcu_exp_handler_bh()                           │
│        ↓                                                            │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  1. 检查当前 CPU 是否在 RCU 读临界区                          │   │
│  │     └── 读取 ->rcu_read_lock_nesting                         │   │
│  │                                                              │   │
│  │  2. 如果不在临界区                                            │   │
│  │     └── 立即调用 rcu_report_exp_rdp() 报告 QS                │   │
│  │                                                              │   │
│  │  3. 如果在临界区                                              │   │
│  │     └── 设置 urgent 标志 (rcu_urgent_qs)                     │   │
│  │     └── 外层 rcu_read_unlock() 会检测到标志并报告 QS         │   │
│  │                                                              │   │
│  │  4. 如果 PREEMPT_RCU 且任务被抢占                             │   │
│  │     └── 请求调度器帮助 (set_tsk_need_resched)                │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

快速 QS 报告机制

```c
// 普通 rcu_read_unlock() 路径
void rcu_read_unlock(void)
{
    // ... 常规递减嵌套计数
    if (READ_ONCE(current->rcu_read_lock_nesting) == 0) {
        // 退出最外层临界区
        barrier();
        // 普通路径：等待自然 QS 检测
    }
}

// Expedited 加速路径
void rcu_read_unlock(void)
{
    // ... 常规递减
    if (READ_ONCE(current->rcu_read_lock_nesting) == 0) {
        barrier();
        // 检查是否有 urgent QS 请求
        if (unlikely(READ_ONCE(__this_cpu_read(rcu_data.rcu_urgent_qs)))) {
            __this_cpu_write(rcu_data.rcu_urgent_qs, false);
            // 立即报告 QS，加速 GP 完成
            rcu_report_exp_rdp();
        }
    }
}
```

2.4 分层 IPI 优化

```
┌─────────────────────────────────────────────────────────────────────┐
│              分层 IPI 机制（大 CPU 数量优化）                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  在 64+ CPU 系统上，向所有 CPU 发送 IPI 开销很大                      │
│                                                                     │
│  解决方案：利用 RCU 树结构分层发送                                    │
│                                                                     │
│                 rcu_node (root)                                     │
│                      │                                              │
│        ┌─────────────┼─────────────┐                               │
│        ▼             ▼             ▼                               │
│    rcu_node 0    rcu_node 1    rcu_node 2                          │
│    (CPU 0-7)     (CPU 8-15)    (CPU 16-23)                         │
│        │             │             │                               │
│    ┌───┴───┐    ┌───┴───┐    ┌───┴───┐                            │
│    ▼       ▼    ▼       ▼    ▼       ▼                            │
│  rdp0    rdp1  rdp2   rdp3  rdp4   rdp5  ...                       │
│                                                                     │
│  1. 首先向每个 leaf rcu_node 的一个代表 CPU 发送 IPI                │
│  2. 收到 IPI 的 CPU 负责唤醒同 node 的其他 CPU                      │
│  3. 减少总 IPI 数量，降低总线压力                                   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

3.1 核心函数调用链

```
synchronize_rcu_expedited()
    └── wait_rcu_gp(call_rcu_expedited)
            └── __wait_rcu_gp()
                    └── exp_funnel_lock()          [获取分层锁]
                            └── rcu_exp_gp_seq_start()  [启动 GP]
                                    └── sync_rcu_exp_select_cpus()  [选择目标 CPU]
                                            └── rcu_exp_select_node_cpus()  [按 node 选择]
                                                    └── smp_call_function_single_async()  [发送 IPI]
                                                            └── rcu_exp_handler()  [IPI handler]
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
