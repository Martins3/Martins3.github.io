# Linux 调度器完整测试计划
<!-- 6012962f-ab12-4a6f-9864-fd5b195f076b -->

> 针对 sched-questions.md 中的89个问题，设计完整的测试验证方案

---

## 测试环境

### 主机环境 (物理机)
- CPU: i9-13900K (32逻辑核)
- 内核: 6.18.9-100.fc42.x86_64
- 用途: 本地快速测试

### 虚拟机环境 (yyds)
- 通过 `./alpine/alpine-action.sh -a ssh -n yyds` 访问
- NFS共享: `/home/martins3/data/vn`
- 用途: 隔离环境测试、内核参数修改

---

## 测试分组与计划

### 第一组: 基础概念验证 (问题1-10)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 1 | load/priority/weight/share 关系 | 创建不同nice值进程，观察/proc/[pid]/sched | P0 |
| 2 | sched_class 和 policy 关系 | ps -eo cls 观察各类policy映射 | P0 |
| 3 | 四个priority字段区别 | 读取/proc/[pid]/sched对比字段 | P0 |
| 4 | MLFQ是什么 | 文档调研 + 对比CFS实现 | P1 |
| 5 | vruntime溢出处理 | 创建长期运行进程，观察vruntime变化 | P1 |
| 6 | time_slice和vruntime关系 | ftrace跟踪sched_slice计算 | P2 |
| 7 | sysctl_sched_latency保证 | 创建多进程，观察每个进程是否都能运行 | P1 |
| 8 | load和runnable_weight关系 | 读取/proc/[pid]/schedstat分析 | P2 |
| 9 | on_rq字段区别 | 内核代码分析 + ftrace验证 | P2 |
| 10 | group_node作用 | cgroup测试，观察task_group结构 | P2 |

### 第二组: PELT深入验证 (问题11-17)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 11 | PELT解决的问题 | 对比开关PELT的负载计算(如可行) | P1 |
| 12 | weight/share/load avg区别 | 同时测试cgroup weight和进程nice | P1 |
| 13 | sched_avg字段含义 | 编写程序读取并解释各字段 | P0 |
| 14 | load_sum在se和cfs_rq区别 | ftrace跟踪update_load_avg | P2 |
| 15 | update_load_avg调用位置 | bpftrace跟踪所有调用点 | P1 |
| 16 | propagate_entity_load_avg | ftrace跟踪负载传播路径 | P2 |
| 17 | CPU utilization获取 | 对比/proc/stat、/proc/[pid]/sched、perf | P0 |

### 第三组: Task Group/Cgroup (问题18-24)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 18 | shares和load_avg区别 | 创建cgroup，设置shares，观察变化 | P0 |
| 19 | calc_group_shares公式 | 编写测试程序验证计算公式 | P2 |
| 20 | update_cfs_group作用 | ftrace跟踪group weight更新 | P2 |
| 21 | grq是什么 | 代码分析 + 绘制结构图 | P2 |
| 22 | group间负载均衡 | 创建多级cgroup，测试CPU分配 | P1 |
| 23 | se_runnable和se_weight | 读取/proc/sched_debug分析 | P2 |
| 24 | tg->load_avg保证 | 编写验证脚本，检查一致性 | P2 |

### 第四组: SMP负载均衡 (问题25-34)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 25 | 负载均衡触发时机 | bpftrace跟踪load_balance入口 | P1 |
| 26 | find_busiest_group/queue | ftrace跟踪并分析选择逻辑 | P2 |
| 27 | balance衡量标准 | 分析/proc/schedstat指标 | P1 |
| 28 | softirq集中在CPU0 | 观察/proc/softirqs SCHED行 | P0 |
| 29 | sched_domain和sched_group | 读取/proc/sys/kernel/sched_domain | P0 |
| 30 | domain概念 | 分析domain0/domain1结构 | P1 |
| 31 | 大小核调度代码位置 | 搜索arch/x86/kernel/cpu/相关代码 | P2 |
| 32 | taskset实现 | 测试taskset命令，观察效果 | P0 |
| 33 | NUMA balancing | 检查/proc/sys/kernel/numa_balancing | P1 |
| 34 | load balancing跨NUMA | 如有多NUMA环境，测试跨节点迁移 | P2 |

### 第五组: 抢占机制 (问题35-43)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 35 | 抢占实现原理 | 查看/proc/sys/kernel/preempt_mode | P0 |
| 36 | schedule()关抢占原因 | 代码分析 + 测试preempt_count | P2 |
| 37 | cond_resched安全性 | 测试持锁时cond_resched行为 | P2 |
| 38 | 三种抢占模式区别 | 检查当前配置，测试不同模式(如支持动态) | P1 |
| 39 | PREEMPT_DYNAMIC | 检查/proc/sys/kernel/preempt | P0 |
| 40 | preempt_count检查位置 | bpftrace跟踪preempt_schedule | P2 |
| 41 | 关中断不安全原因 | 编写测试代码验证 | P2 |
| 42 | nr_involuntary_switches | 观察/proc/[pid]/status字段 | P0 |
| 43 | migrate_disable | 测试migrate_disable效果 | P2 |

### 第六组: 实时调度 (问题44-49)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 44 | SCHED_FIFO vs SCHED_RR | root权限测试两种策略行为 | P0 |
| 45 | RT throttling | 测试触发throttling的条件 | P0 |
| 46 | RT选核流程 | ftrace对比RT和CFS选核差异 | P2 |
| 47 | RT导致spinlock问题 | 编写优先级反转测试程序 | P2 |
| 48 | RT优先级范围 | 测试chrt -f 1-99各优先级 | P1 |
| 49 | RT的nice值 | 测试nice对RT进程是否无效 | P1 |

### 第七组: EEVDF验证 (问题50-55)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 50 | EEVDF解决CFS问题 | 对比EEVDF和CFS的延迟特性 | P0 |
| 51 | EEVDF三大基石 | 编写测试程序验证Lag/Deadline计算 | P1 |
| 52 | EEVDF调度决策流程 | ftrace跟踪evdf调度路径 | P1 |
| 53 | SCHED_BATCH被干掉? | 测试内核是否还支持BATCH | P0 |
| 54 | EEVDF vs CFS数据结构 | 对比fair.c代码结构变化 | P2 |
| 55 | latency_nice参数 | 测试sched_setattr设置latency_nice | P1 |

### 第八组: 进程管理 (问题56-61)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 56 | pid/ppid/sid/pgid区别 | 创建进程组，观察关系 | P0 |
| 57 | real_parent和parent | 测试孤儿进程被收养场景 | P1 |
| 58 | exit_signal | 测试不同exit_signal行为 | P2 |
| 59 | 父子树状关系 | pstree观察进程树结构 | P1 |
| 60 | TASK_INTERRUPTIBLE区别 | 编写程序测试两种睡眠状态 | P2 |
| 61 | on_rq含义 | 代码分析 + /proc/[pid]/sched分析 | P2 |

### 第九组: 内核机制 (问题62-67)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 62 | scheduler_tick作用 | bpftrace跟踪tick处理流程 | P1 |
| 63 | hrtick和schedule_tick | 检查hrtimer使用情况 | P2 |
| 64 | nohz影响 | 检查/proc/sys/kernel/nohz*配置 | P1 |
| 65 | schedule()同步机制 | ftrace跟踪rq->lock获取 | P2 |
| 66 | ttwu同步保证 | ftrace跟踪try_to_wake_up | P2 |
| 67 | smp_wmb作用 | 代码分析 + 内存序测试 | P3 |

### 第十组: 调试统计 (问题68-70)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 68 | taskstats/delayacct | 测试getdelays工具 | P1 |
| 69 | latencytop | 检查/proc/sys/kernel/latencytop | P2 |
| 70 | schedstat用途 | 编写程序解析/proc/[pid]/schedstat | P1 |

### 第十一组: 特定场景 (问题71-77)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 71 | update_blocked_averages警告 | 观察dmesg是否有相关警告 | P2 |
| 72 | 主动触发RT throttling | 编写程序触发throttling | P1 |
| 73 | CPU core乱跑原因 | 已完成: make -j4任务分布测试 | P0 |
| 74 | context switch代价 | perf测量上下文切换开销 | P1 |
| 75 | isolcpus和nohz_full | 检查当前内核启动参数 | P1 |
| 76 | cgroup cpu.weight | 测试weight和shares关系 | P1 |
| 77 | autogroup工作方式 | 检查/proc/[pid]/autogroup | P0 |

### 第十二组: 代码实现 (问题78-83)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 78 | select_task_rq和set_task_cpu | ftrace对比两个函数 | P2 |
| 79 | update_curr和update_cfs_group | ftrace跟踪调用时机 | P2 |
| 80 | attach_entity_cfs_rq | ftrace跟踪task attach流程 | P2 |
| 81 | rq中cfs_rq地位 | 代码分析 + 结构体分析 | P2 |
| 82 | stop/idle无rq原因 | 代码分析 | P3 |
| 83 | sched_class优先级 | 检查vmlinux中class排列 | P2 |

### 第十三组: 扩展学习 (问题84-89)

| 问题 | 测试内容 | 测试方法 | 优先级 |
|------|----------|----------|--------|
| 84 | sched_ext BPF | 尝试加载BPF scheduler | P1 |
| 85 | SCHED_PROXY_EXEC | 检查内核配置和文档 | P2 |
| 86 | PREEMPT_LAZY | 测试lazy preempt行为 | P1 |
| 87 | Documentation阅读 | 整理scheduler文档结构 | P2 |
| 88 | 参考文章理解 | 对比LoyenWang等文章 | P3 |
| 89 | EEVDF论文理解 | 阅读EEVDF论文关键章节 | P3 |

---

## 测试执行计划

### 阶段1: P0优先级测试 (基础验证)
- [ ] 1.1 load/priority/weight/share 关系
- [ ] 1.2 sched_class和policy关系  
- [ ] 1.3 四个priority字段
- [ ] 1.4 CPU utilization获取
- [ ] 1.5 sched_avg字段含义
- [ ] 1.6 shares和load_avg区别
- [ ] 1.7 softirq集中在CPU0
- [ ] 1.8 sched_domain和sched_group
- [ ] 1.9 taskset实现
- [ ] 1.10 抢占模式检查
- [ ] 1.11 involuntary/voluntary switches
- [ ] 1.12 RT策略测试
- [ ] 1.13 RT throttling
- [ ] 1.14 EEVDF vs CFS对比
- [ ] 1.15 SCHED_BATCH支持
- [ ] 1.16 pid/ppid/sid/pgid区别
- [ ] 1.17 CPU core乱跑原因
- [ ] 1.18 isolcpus检查
- [ ] 1.19 autogroup工作

### 阶段2: P1优先级测试 (深入理解)
- [ ] 2.1 PELT解决的问题
- [ ] 2.2 update_load_avg调用位置
- [ ] 2.3 group间负载均衡
- [ ] 2.4 负载均衡触发时机
- [ ] 2.5 balance衡量标准
- [ ] 2.6 NUMA balancing
- [ ] 2.7 抢占模式切换
- [ ] 2.8 RT优先级范围
- [ ] 2.9 RT的nice值
- [ ] 2.10 EEVDF三大基石
- [ ] 2.11 EEVDF调度决策
- [ ] 2.12 latency_nice参数
- [ ] 2.13 父子树状关系
- [ ] 2.14 scheduler_tick
- [ ] 2.15 nohz影响
- [ ] 2.16 taskstats/delayacct
- [ ] 2.17 schedstat用途
- [ ] 2.18 触发RT throttling
- [ ] 2.19 context switch代价
- [ ] 2.20 cgroup cpu.weight
- [ ] 2.21 sched_ext BPF
- [ ] 2.22 PREEMPT_LAZY

### 阶段3: P2优先级测试 (代码深入)
- [ ] 3.1 vruntime溢出
- [ ] 3.2 time_slice和vruntime
- [ ] 3.3 sysctl_sched_latency
- [ ] 3.4 load和runnable_weight
- [ ] 3.5 on_rq字段
- [ ] 3.6 group_node
- [ ] 3.7 weight/share/load区别
- [ ] 3.8 load_sum区别
- [ ] 3.9 propagate_entity_load_avg
- [ ] 3.10 calc_group_shares
- [ ] 3.11 update_cfs_group
- [ ] 3.12 grq是什么
- [ ] 3.13 se_runnable和se_weight
- [ ] 3.14 tg->load_avg保证
- [ ] 3.15 find_busiest_group/queue
- [ ] 3.16 domain概念
- [ ] 3.17 大小核调度代码
- [ ] 3.18 load balancing跨NUMA
- [ ] 3.19 schedule()关抢占
- [ ] 3.20 cond_resched安全性
- [ ] 3.21 preempt_count检查
- [ ] 3.22 关中断不安全
- [ ] 3.23 migrate_disable
- [ ] 3.24 RT选核流程
- [ ] 3.25 RT spinlock问题
- [ ] 3.26 EEVDF数据结构
- [ ] 3.27 real_parent和parent
- [ ] 3.28 exit_signal
- [ ] 3.29 TASK_INTERRUPTIBLE
- [ ] 3.30 on_rq含义
- [ ] 3.31 hrtick和schedule_tick
- [ ] 3.32 schedule()同步
- [ ] 3.33 ttwu同步
- [ ] 3.34 latencytop
- [ ] 3.35 update_blocked_averages警告
- [ ] 3.36 select_task_rq和set_task_cpu
- [ ] 3.37 update_curr和update_cfs_group
- [ ] 3.38 attach_entity_cfs_rq
- [ ] 3.39 rq中cfs_rq地位
- [ ] 3.40 sched_class优先级
- [ ] 3.41 SCHED_PROXY_EXEC
- [ ] 3.42 Documentation阅读

### 阶段4: P3优先级测试 (高级/文档)
- [ ] 4.1 MLFQ是什么
- [ ] 4.2 stop/idle无rq原因
- [ ] 4.3 smp_wmb作用
- [ ] 4.4 参考文章理解
- [ ] 4.5 EEVDF论文理解

---

## 测试工具准备

### 已准备
- [x] test_eevdf.sh - 基础调度器测试
- [x] test_load_balance.sh - 负载均衡测试

### 需要开发
- [ ] test_pelt.sh - PELT深度测试
- [ ] test_cgroup.sh - Cgroup调度测试
- [ ] test_rt.sh - 实时调度测试 (需root)
- [ ] test_preempt.sh - 抢占机制测试
- [ ] test_sched_ext.sh - BPF scheduler测试
- [ ] test_process_relationship.sh - 进程关系测试

---

## 当前状态

**已完成**: 
- test_eevdf.sh: 基础调度策略测试
- test_load_balance.sh: SMP负载均衡测试

**进行中**: 等待开始阶段1测试

**测试环境**: 
- 物理机: 已测试
- yyds VM: 待测试

---

*计划创建时间: 2026-03-04*  
*预计完成时间: 待定*

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
