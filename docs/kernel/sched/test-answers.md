# Linux 调度器问题回答与测试验证
<!-- 7f5c5ed2-605b-4137-945f-9c4bd92fd580 -->

> 本文档回答 [sched-questions.md](./sched-questions.md) 中整理的问题，并提供测试验证。

---

## 测试环境准备

### 虚拟机快速操作

根据 AGENTS.md，使用 `yyds` 虚拟机进行测试。**不要手动构造 SSH 命令**，使用封装好的工具：

```bash
# 1. SSH 连接到 yyds 虚拟机（使用 -n 参数直接指定，非交互式）
./alpine/alpine-action.sh -a ssh -n yyds

# 2. 或获取 SSH 连接信息（用于自动化脚本）
./alpine/alpine-action.sh -a ssh_auto -n yyds
# 输出: ssh -p 50584 root@localhost

# 3. 启动 yyds 虚拟机
./alpine/alpine-action.sh -a run -n yyds

# 4. 杀死 yyds 虚拟机
./alpine/alpine-action.sh -a kill -n yyds

# 对应的 zsh 别名（在 code/zsh 中定义）:
# ge  → SSH 到默认 VM
# qdr → 启动默认 VM
# k   → 快速杀死当前 VM
```

### NFS 共享目录（重要！）

**yyds 虚拟机配置了 NFS 共享**：
- 主机目录：`/home/martins3/data/vn` 已挂载到 VM 的 `/home/martins3/data/vn`
- **直接在主机修改文件，VM 中实时可见，无需 scp 拷贝**

**检查共享是否生效**:
```bash
# 在 VM 中执行
mount | grep /home/martins3/data/vn
# 预期输出: 10.0.0.2:/home/martins3/data/vn on /home/martins3/data/vn type nfs4 (...)
```

**文件操作策略**:
| 场景 | 操作方式 |
|------|----------|
| **有 NFS 共享**（yyds） | 直接在主机编辑，VM 实时可见 |
| **无共享** | 使用 `scp -P <port>` 或 `./alpine/alpine-action.sh -a rsync` |

### 基础环境检查

```bash
# 检查当前环境（使用 check-vm.sh）
if ./check-vm.sh; then
    echo "当前是虚拟机，可直接执行任何命令"
else
    echo "当前是物理机，需通过 VM 测试"
fi

# 检查当前调度器类型
cat /proc/sys/kernel/sched_schedstats 2>/dev/null || echo "schedstats not enabled"

# 查看当前 CFS 调度器参数
sysctl kernel.sched | grep -E "(latency|granularity|shares)"

# 查看内核版本和配置
uname -r
grep CONFIG_SCHED_ /boot/config-$(uname -r) 2>/dev/null | head -10
```

---

## 1. 基础概念问题回答

### 1.1 load / priority / weight / share 的关系

**一句话总结**: 这是调度器在不同层次使用的"权重"概念，最终都转换为 weight 参与计算。

| 概念 | 层次 | 说明 |
|------|------|------|
| nice | 用户空间 | [-20, 19]，数值越小优先级越高 |
| priority | 内核表示 | [0, 139]，0-99 为 RT，100-139 为 NORMAL |
| weight | sched_entity | 通过 `sched_prio_to_weight[]` 表转换，nice 0 = weight 1024 |
| load | cfs_rq | 该 runqueue 上所有 entity weight 之和 |
| share | task_group | cgroup 的 cpu.weight，默认 100，范围 [1, 10000] |

**转换关系**:
```c
// NICE_TO_PRIO: [-20, 19] -> [100, 139]
#define NICE_TO_PRIO(nice)  ((nice) + DEFAULT_PRIO)  // DEFAULT_PRIO = 120

// weight: 通过 sched_prio_to_weight[40] 表查找
// nice 0 -> weight 1024
// nice -20 -> weight 88761
// nice 19 -> weight 15
```

**测试验证**:
```bash
# 查看当前进程的 nice 和 priority
ps -eo pid,nice,pri,comm | head -10

# 查看调度器权重表（如果内核编译了 debugfs）
cat /sys/kernel/debug/sched/debug 2>/dev/null | head -50

# 测试 nice 对 CPU 使用的影响
# 终端1: nice -n 19 yes > /dev/null
# 终端2: nice -n -20 yes > /dev/null  (需要 root)
# 终端3: top 观察 CPU 使用率比例
```

### 1.2 sched_class 和 policy 的关系

**一句话总结**: policy 是用户空间概念，sched_class 是内核实现；一个 sched_class 可以实现多个 policy。

```
用户空间 policy          内核 sched_class
-----------------        ----------------
SCHED_FIFO      ------>  rt_sched_class
SCHED_RR        ------>  rt_sched_class
SCHED_NORMAL    ------>  fair_sched_class (CFS/EEVDF)
SCHED_BATCH     ------>  fair_sched_class
SCHED_IDLE      ------>  fair_sched_class  (注意：不是 idle_sched_class!)
SCHED_DEADLINE  ------>  dl_sched_class
```

**关键区别**:
- `idle_sched_class`: 内核内部使用，当没有任何任务时运行 idle 线程
- `SCHED_IDLE`: 用户空间 policy，表示最低优先级的普通任务

**测试验证**:
```bash
# 查看进程的 sched class 和 policy
# CLS 列: TS=OTHER, FF=FIFO, RR=RR, B=BATCH, IDL=IDLE, DLN=DEADLINE
ps -eo pid,cmd,cls,pri,rtprio | head -20

# 使用 chrt 修改 policy
chrt -r 50 sleep 100 &  # SCHED_RR, priority 50
chrt -f 50 sleep 100 &  # SCHED_FIFO, priority 50
chrt -b -p 0 $$         # 将当前 shell 改为 BATCH
```

### 1.3 task_struct 中的四个 priority

| 字段 | 范围 | 说明 |
|------|------|------|
| static_prio | [100, 139] | 通过 nice 设置，NICE_TO_PRIO(nice) |
| normal_prio | 动态 | 根据 policy 计算，RT 任务使用 rt_priority |
| prio | 动态 | 实际使用的优先级，可能因优先级继承临时调整 |
| rt_priority | [1, 99] | RT 专用，数值越大优先级越高 |

**计算流程**:
```c
// 1. 用户设置 nice -> static_prio
static_prio = NICE_TO_PRIO(nice);  // 120 + nice

// 2. 根据 policy 计算 normal_prio
if (dl_policy(policy))       normal_prio = MAX_DL_PRIO - 1;      // -1
else if (rt_policy(policy))  normal_prio = MAX_RT_PRIO - 1 - rt_priority;  // 99 - rt_prio
else                         normal_prio = static_prio;

// 3. prio = normal_prio (可能被优先级继承临时修改)
prio = normal_prio;
```

**测试验证**:
```bash
# 查看进程的调度信息
cat /proc/self/sched | grep -E "prio|policy"

# 输出示例:
# policy                       : 0       (0=SCHED_NORMAL)
# prio                         : 120     (normal_prio)
# static_prio                  : 120     (nice 0)
# normal_prio                  : 120
# rt_priority                  : 0
```

---

## 2. PELT 问题回答

### 2.1 PELT 解决了什么问题

**背景**: 在 PELT 之前，调度器使用 per-runqueue 的统计，无法准确反映单个任务的行为特征。

**问题**:
1. 无法区分 CPU 密集型任务和 I/O 密集型任务
2. 无法区分突发型任务和稳定型任务
3. 跨 CPU 负载均衡时，只能根据优先级估算，无法根据实际行为

**PELT 核心思想**:
```
load_avg = runnable% * weight
util_avg = running% * SCHED_CAPACITY_SCALE
```

通过几何级数衰减，平滑统计过去一段时间的负载情况。

### 2.2 sched_avg 字段含义

```c
struct sched_avg {
    u64  last_update_time;      // 上次更新时间
    u64  load_sum;              // 带衰减的 load 累加值 (原始值)
    u64  runnable_load_sum;     // 带衰减的 runnable load 累加值
    u32  util_sum;              // 带衰减的 util 累加值
    u32  period_contrib;        // 当前 period 的贡献
    unsigned long load_avg;     // 平均 load (已乘 weight)
    unsigned long runnable_load_avg;  // 平均 runnable load
    unsigned long util_avg;     // 平均 util (CPU 利用率)
};
```

**区别**:
- `*_sum`: 几何级数累加值，用于计算平均
- `*_avg`: 实际的平均值，用于调度决策

**测试验证**:
```bash
# 查看进程的 PELT 统计
cat /proc/self/sched | grep -E "avg|sum"

# 输出:
# se.avg.load_sum                            : 0
# se.avg.runnable_sum                        : 0
# se.avg.util_sum                            : 0
# se.avg.load_avg                            : 0
# se.avg.runnable_avg                        : 0
# se.avg.util_avg                            : 0
```

### 2.3 update_load_avg 调用时机

```
enqueue_entity    -> update_load_avg (ENQUEUE)    # 任务入队
update_curr       -> update_load_avg (UPDATE)     # 当前任务运行中
dequeue_entity    -> update_load_avg (DEQUEUE)    # 任务出队
entity_tick       -> update_load_avg (UPDATE)     # 时钟 tick
attach_entity_cfs_rq -> update_load_avg (ATTACH)  # 迁移附加
```

---

## 3. Task Group 问题回答

### 3.1 shares vs load_avg

| 字段 | 类型 | 含义 |
|------|------|------|
| shares | 配置 | 用户设置的权重，通过 cgroup cpu.weight 或 autogroup 设置 |
| load_avg | 统计 | 该 task_group 在所有 CPU 上的实际负载之和 |

**关系**:
```
tg->load_avg ~= sum(tg->cfs_rq[cpu]->avg.load_avg)
```

**group entity weight 计算**:
```
se_weight(se) = tg->shares * grq->load_avg / tg->load_avg
```

### 3.2 层级调度示意

```
                 root_task_group (shares=unlimited)
                      /          \
               tg_A (shares=1000)  tg_B (shares=500)
                 /    \              /    \
            task1    task2       task3    task4
```

CPU 0 上：
- tg_A->cfs_rq[0] 包含 task1, task2
- tg_B->cfs_rq[0] 包含 task3, task4
- root_task_group->cfs_rq[0] 包含 tg_A 的 se, tg_B 的 se

**测试验证**:
```bash
# 创建 cgroup v2 并设置 cpu.weight
sudo mkdir -p /sys/fs/cgroup/test_a /sys/fs/cgroup/test_b

# 设置权重 (范围 1-10000)
echo 1000 | sudo tee /sys/fs/cgroup/test_a/cpu.weight
echo 500 | sudo tee /sys/fs/cgroup/test_b/cpu.weight

# 将进程加入 cgroup
echo $$ | sudo tee /sys/fs/cgroup/test_a/cgroup.procs

# 查看当前 cgroup
cat /proc/self/cgroup

# 清理
sudo rmdir /sys/fs/cgroup/test_a /sys/fs/cgroup/test_b
```

---

## 4. SMP 负载均衡问题回答

### 4.1 负载均衡触发时机

| 函数 | 触发条件 | 说明 |
|------|----------|------|
| idle_balance | schedule() 时当前 rq 为空 | CPU 空闲时尝试拉取任务 |
| rebalance_domains | SCHED_SOFTIRQ | 周期性负载均衡 |
| nohz_idle_balance | nohz 空闲 CPU | tickless CPU 的均衡 |
| newidle_balance | 进入 idle 前 | 新空闲 CPU 的均衡 |

**流程**:
```
scheduler_tick()
  -> trigger_load_balance()
    -> raise_softirq(SCHED_SOFTIRQ)  # 触发软中断

run_rebalance_domains()  # 软中断处理
  -> rebalance_domains()
    -> load_balance()     # 实际负载均衡
      -> find_busiest_group()  # 找最忙的组
      -> find_busiest_queue()  # 找最忙的队列
      -> detach_tasks()        # 迁移任务
```

### 4.2 调度域层级

```
DIE (所有核心)
  |
MC (多核心，共享 LLC)
  |
SMT (超线程，共享 L1)
```

**测试验证**:
```bash
# 查看 CPU 拓扑
cat /proc/cpuinfo | grep -E "physical id|core id|processor" | head -20

# 查看调度域信息
cat /proc/sys/kernel/sched_domain/cpu0/domain*/name 2>/dev/null

# 查看当前调度域配置
sysctl kernel.sched_domain | head -20
```

### 4.3 CPU 核心乱跑问题

**问题**: 为什么 make -j4 时，负载不集中在 4 个 core，而是分散到所有 core？

**原因**:
1. **负载均衡器会尽量均匀分布负载**，而不是固定 core
2. **缓存亲和性 vs 全局公平性** 的权衡
3. 默认情况下，负载均衡器会尝试让任务"回家" (wake_affine)

**控制方法**:
```bash
# 方法1: 使用 taskset 绑定到特定 core
taskset -c 0-3 make -j4

# 方法2: 使用 cgroup cpuset
sudo mkdir /sys/fs/cgroup/cpuset/myset
sudo sh -c 'echo 0-3 > /sys/fs/cgroup/cpuset/myset/cpuset.cpus'
echo $$ | sudo tee /sys/fs/cgroup/cpuset/myset/cgroup.procs
```

---

## 5. 抢占问题回答

### 5.1 抢占模式和检查点

| 模式 | 配置 | 抢占点 |
|------|------|--------|
| NONE | CONFIG_PREEMPT_NONE | 仅 cond_resched() |
| VOLUNTARY | CONFIG_PREEMPT_VOLUNTARY | cond_resched() + might_sleep() |
| FULL | CONFIG_PREEMPT_FULL | 任意可抢占点 |

**抢占检查位置**:
```
1. preempt_enable() 时
2. irqentry_exit_cond_resched() 从中断返回时
3. cond_resched() 显式检查
4. 系统调用/异常返回用户空间时
```

**测试验证**:
```bash
# 查看当前抢占模式
zgrep CONFIG_PREEMPT /proc/config.gz 2>/dev/null || grep CONFIG_PREEMPT /boot/config-$(uname -r)

# 查看动态抢占配置（如果支持）
cat /sys/kernel/debug/preempt 2>/dev/null || echo "Dynamic preempt not available"

# 如果支持 PREEMPT_DYNAMIC
echo preempt=full | sudo tee /proc/cmdline  # 需要重启生效
```

### 5.2 自愿切换 vs 强制切换

| 类型 | 计数器 | 触发场景 |
|------|--------|----------|
| 自愿 | nr_voluntary_switches | 调用 schedule() 主动让出 |
| 强制 | nr_involuntary_switches | 时间片用完被抢占 |

**代码逻辑**:
```c
// __schedule()
if (!preempt && prev_state) {
    // 主动睡眠，自愿切换
    switch_count = &prev->nvcsw;
} else {
    // 被抢占，强制切换
    switch_count = &prev->nivcsw;
}
```

**测试验证**:
```bash
# 查看进程的切换统计
cat /proc/self/status | grep -E "voluntary|nonvoluntary"

# 输出:
# voluntary_ctxt_switches:        1
# nonvoluntary_ctxt_switches:     0
```

---

## 6. 实时调度问题回答

### 6.1 SCHED_FIFO vs SCHED_RR

| 特性 | SCHED_FIFO | SCHED_RR |
|------|------------|----------|
| 调度方式 | 先进先出 | 时间片轮询 |
| 时间片 | 无，一直运行 | 有，默认 100ms |
| 抢占条件 | 更高优先级 RT 任务 | 同优先级任务或更高优先级 |
| 实现 | rt_sched_class | rt_sched_class + task_tick_rt |

**核心区别代码**:
```c
// task_tick_rt: SCHED_RR 的 tick 处理
if (--p->rt.time_slice)  // 时间片减一
    return;  // 还有时间片，继续运行

// 时间片用完，放到队列尾部
requeue_task_rt(rq, p, 0);
```

### 6.2 RT Throttling

**机制**: 防止 RT 任务占用全部 CPU，保证系统可以响应非 RT 任务。

```
sched_rt_period_us  = 1000000  (1秒)
sched_rt_runtime_us = 950000   (0.95秒)
```

RT 任务最多占用 95% 的 CPU 时间，剩下 5% 给其他任务。

**测试验证**:
```bash
# 查看当前 RT throttling 配置
cat /proc/sys/kernel/sched_rt_period_us
cat /proc/sys/kernel/sched_rt_runtime_us

# 测试 RT throttling
# 终端1: 运行一个 SCHED_FIFO 的 CPU 密集型任务
sudo chrt -f 99 yes > /dev/null &
PID=$!

# 终端2: 观察 throttling 计数
watch -n 1 "cat /proc/$PID/sched | grep -E throttle"

# 清理
kill $PID
```

---

## 7. EEVDF 问题回答

### 7.1 EEVDF 解决的核心问题

**CFS 的缺陷**: 延迟和带宽耦合
- 要让任务响应更快，只能提高权重
- 但提高权重意味着获得更多 CPU 时间（带宽）
- 无法做到"低延迟但少带宽"

**EEVDF 的解耦**:
```
Virtual Deadline = vruntime + (request_slice / weight)
```

- 任务可以请求小的时间片 (slice)
- 小 slice -> 早 deadline -> 优先调度
- 但 weight 不变，长期公平性保证

### 7.2 EEVDF vs CFS 对比

| 维度 | CFS | EEVDF |
|------|-----|-------|
| 排序依据 | vruntime | virtual deadline |
| 延迟控制 | 隐式（启发式） | 显式（slice 参数） |
| 交互式任务 | 依赖唤醒抢占补丁 | 天然支持（小 slice） |
| 代码复杂度 | 高（补丁多） | 中等（回归数学模型） |

**测试验证**:
```bash
# 检查内核是否使用 EEVDF
grep CONFIG_SCHED_EEVDF /boot/config-$(uname -r) 2>/dev/null

# 查看当前调度器信息
cat /sys/kernel/debug/sched/debug 2>/dev/null | head -20

# 使用 sched_setattr 设置 latency_nice (需要较新内核)
# 参见下方的测试脚本
```

---

## 8. 测试脚本

### 8.1 EEVDF 调度器测试脚本

```bash
#!/usr/bin/env bash
# test_eevdf.sh - 测试 EEVDF 调度器特性

set -E -e -u -o pipefail

echo "=== EEVDF 调度器测试 ==="

# 检查内核版本和配置
echo -e "\n1. 内核信息:"
uname -r

# 检查调度器相关配置
if [[ -f /boot/config-$(uname -r) ]]; then
    echo -e "\n2. 调度器配置:"
    grep -E "CONFIG_SCHED_|CONFIG_PREEMPT" /boot/config-$(uname -r) | head -20 || true
fi

# 检查当前调度器参数
echo -e "\n3. 调度器参数:"
sysctl kernel.sched 2>/dev/null | head -20 || true

# 创建测试程序
cat > /tmp/eevdf_test.c << 'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <string.h>

// 使用 sched_setattr 设置 latency_nice (如果支持)
struct sched_attr {
    __u32 size;
    __u32 sched_policy;
    __u64 sched_flags;
    __s32 sched_nice;
    __u32 sched_priority;
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
    __u32 sched_util_min;
    __u32 sched_util_max;
    __u32 latency_nice;  // EEVDF 新增
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void cpu_intensive_task(const char *name, int duration_sec) {
    printf("[%s] Starting CPU intensive task on CPU %d\n", name, sched_getcpu());
    volatile long long counter = 0;
    time_t start = time(NULL);
    while (time(NULL) - start < duration_sec) {
        counter++;
    }
    printf("[%s] Finished, counter=%lld\n", name, counter);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <test_case>\n", argv[0]);
        printf("Test cases: normal, batch, fifo, rr\n");
        return 1;
    }

    const char *test = argv[1];

    if (strcmp(test, "normal") == 0) {
        // 普通任务
        cpu_intensive_task("NORMAL", 5);
    }
    else if (strcmp(test, "batch") == 0) {
        // BATCH 任务
        struct sched_param param = { .sched_priority = 0 };
        if (sched_setscheduler(0, SCHED_BATCH, &param) < 0) {
            perror("sched_setscheduler BATCH");
        }
        cpu_intensive_task("BATCH", 5);
    }
    else if (strcmp(test, "fifo") == 0) {
        // FIFO 任务
        struct sched_param param = { .sched_priority = 50 };
        if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
            perror("sched_setscheduler FIFO (need root)");
            return 1;
        }
        cpu_intensive_task("FIFO", 5);
    }
    else if (strcmp(test, "rr") == 0) {
        // RR 任务
        struct sched_param param = { .sched_priority = 50 };
        if (sched_setscheduler(0, SCHED_RR, &param) < 0) {
            perror("sched_setscheduler RR (need root)");
            return 1;
        }
        cpu_intensive_task("RR", 5);
    }
    else {
        printf("Unknown test case: %s\n", test);
        return 1;
    }

    return 0;
}
EOF

# 编译测试程序
echo -e "\n4. 编译测试程序..."
if gcc -o /tmp/eevdf_test /tmp/eevdf_test.c 2>/dev/null; then
    echo "编译成功"
else
    echo "编译失败，使用简化测试"
fi

# 运行测试
echo -e "\n5. 运行调度测试:"

# 测试1: 查看不同 policy 的进程
echo -e "\n--- 当前进程调度信息 ---"
ps -eo pid,cmd,cls,pri,nice,rtprio | head -15

# 测试2: 普通任务竞争
echo -e "\n--- CFS 任务竞争测试 ---"
echo "启动 4 个普通任务，观察 CPU 分配..."
for i in 1 2 3 4; do
    if [[ -x /tmp/eevdf_test ]]; then
        /tmp/eevdf_test normal &
    else
        yes > /dev/null &
    fi
done
BG_PIDS=$!
sleep 3
echo "CPU 使用情况:"
top -bn1 | grep -E "^(%Cpu|PID)" | head -10 || true
kill %1 %2 %3 %4 2>/dev/null || true
wait 2>/dev/null || true

# 测试3: nice 值效果
echo -e "\n--- Nice 值效果测试 ---"
echo "同时运行 nice -20 和 nice 19 的任务，观察 CPU 分配比"
nice -n 19 yes > /dev/null &
LOW_PID=$!
nice -n -20 yes > /dev/null &
HIGH_PID=$!
sleep 3
echo "CPU 使用情况 (应该大约是 88:1 的比例):"
top -bn1 -p $LOW_PID -p $HIGH_PID | tail -5 || true
kill $LOW_PID $HIGH_PID 2>/dev/null || true
wait 2>/dev/null || true

echo -e "\n=== 测试完成 ==="
```

### 8.2 负载均衡测试脚本

```bash
#!/usr/bin/env bash
# test_load_balance.sh - 测试 SMP 负载均衡

set -E -e -u -o pipefail

echo "=== 负载均衡测试 ==="

# 查看 CPU 信息
echo -e "\n1. CPU 信息:"
nproc
cat /proc/cpuinfo | grep "model name" | head -1

# 查看调度域
echo -e "\n2. 调度域信息:"
for f in /proc/sys/kernel/sched_domain/cpu0/domain*/name; do
    if [[ -f "$f" ]]; then
        echo "  $f: $(cat $f)"
    fi
done 2>/dev/null || echo "  无法获取调度域信息"

# 测试1: 任务分布
echo -e "\n3. 任务分布测试:"
echo "创建 4 个 CPU 密集型任务，观察分布"
for i in 1 2 3 4; do
    taskset -c 0-3 sh -c 'while :; do :; done' &
done
sleep 2
echo "任务分布:"
ps -eo pid,psr,cmd | grep "while" | grep -v grep || true
kill %1 %2 %3 %4 2>/dev/null || true
wait 2>/dev/null || true

# 测试2: 绑定到特定 CPU
echo -e "\n4. CPU 绑定测试:"
taskset -c 0 yes > /dev/null &
PID=$!
sleep 1
echo "绑定到 CPU 0 的进程 $PID 实际运行 CPU:"
ps -o pid,psr -p $PID || true
kill $PID 2>/dev/null || true
wait 2>/dev/null || true

echo -e "\n=== 测试完成 ==="
```

### 8.3 运行测试的方法

**方法1: yyds 虚拟机（推荐，有 NFS 共享）**

由于 yyds 配置了 NFS 共享，主机上的 `~/data/vn` 目录在 VM 中实时可见：

```bash
# 1. 直接在主机上执行（文件已在 VM 中可见）
./alpine/alpine-action.sh -a ssh -n yyds

# 2. 在 VM 中运行测试（VM 内路径与主机相同）
cd /home/martins3/data/vn/docs/kernel/sched
bash test_eevdf.sh
bash test_load_balance.sh
```

**方法2: 物理机直接运行**

```bash
# 如果当前是物理机，直接运行
bash docs/kernel/sched/test_eevdf.sh
bash docs/kernel/sched/test_load_balance.sh
```

**方法3: 其他 VM（无 NFS 共享时使用）**

```bash
# 获取 SSH 连接信息
./alpine/alpine-action.sh -a ssh_auto -n <vm_name>
# 输出: ssh -p <port> root@localhost

# 拷贝脚本到 VM（使用对应端口）
scp -P <port> docs/kernel/sched/test_*.sh root@localhost:/tmp/

# 在 VM 中运行
./alpine/alpine-action.sh -a ssh -n <vm_name>
bash /tmp/test_eevdf.sh
```

---

## 9. 总结

### 核心概念速查

| 概念 | 一句话解释 |
|------|-----------|
| vruntime | 虚拟运行时间，越小越应该被调度 |
| load_avg | 平均负载，反映 CPU 需求 |
| util_avg | CPU 利用率，反映实际使用情况 |
| shares | cgroup 权重，决定资源分配比例 |
| min_vruntime | cfs_rq 中最小的 vruntime，用于防止新任务饿死 |
| virtual deadline | EEVDF 中的概念，决定调度顺序 |

### 关键路径

```
1. 时钟 tick -> scheduler_tick() -> task_tick_fair() -> update_curr()
2. 任务唤醒 -> try_to_wake_up() -> enqueue_task_fair() -> enqueue_entity()
3. 调度选择 -> schedule() -> pick_next_task() -> pick_next_task_fair()
4. 负载均衡 -> run_rebalance_domains() -> load_balance()
```

### 学习建议

1. **不要陷入函数调用链**，关注设计思想
2. **多使用工具观察**: `perf`, `ftrace`, `bpftrace`, `schedtool`
3. **从现象出发**: 先看到问题，再理解代码
4. **关注边界条件**: 调度器代码充满了边界处理

---

*最后更新: 2026-03-04*

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
