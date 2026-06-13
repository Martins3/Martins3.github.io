# Linux Trace 技术整理报告

**整理时间**: 2026-02-18  
**目标目录**: `~/data/vn/docs/trace/` 和 `~/data/vn/code/src/ebpf/`  
**原则**: 不动原文件，只做梳理和补充

---

## 📁 目录结构

```
~/data/vn/docs/trace/           # Trace 文档总目录
├── overview.md                 # 总体概览
├── todo.md                     # 待办事项
├── ftrace/                     # Ftrace 详细文档
│   ├── ftrace.md               # 基本用法
│   ├── ftrace-internals.md     # 内部实现
│   ├── fprobe.md               # Fprobe 机制
│   ├── trace-cmd.md            # trace-cmd 工具
│   └── tracer-*.md             # 各种 tracer
├── tools/                      # 工具文档
│   └── bpftrace/               # bpftrace 相关
├── code/                       # 代码示例
├── perf/                       # Perf 相关
├── kprobe.md                   # Kprobe 文档
├── kallsyms.md                 # Kallsyms
├── tracepoint.md               # Tracepoint
├── mce.md                      # MCE (Machine Check Exception)
├── rasdaemon.md                # RAS daemon
└── ...

~/data/vn/code/src/ebpf/        # eBPF 代码
├── minimal.bpf.c               # 最小示例
├── bootstrap.bpf.c             # 引导程序
├── tc.bpf.c                    # TC (Traffic Control)
├── cg.bpf.c                    # Cgroup
├── ds.bpf.c                    # Data structure
├── task_iter.bpf.c             # Task iterator
├── mapwriter.bpf.c             # Map 操作
├── bpf_cubic.bpf.c             # TCP Congestion Control
├── iter_*.bpf.c                # Iterator 示例
├── bcc/                        # BCC 相关
├── ext4/                       # Ext4 文件系统
├── ra/                         # Read-ahead
└── ...

~/data/vn/code/src/m/tracer/    # 内核模块 tracer
├── trace_martins3.c            # 自定义 tracer
├── extra.diff                  # 内核补丁
└── README.md                   # 说明

~/data/vn/docs/ebpf/            # eBPF 文档
├── overview.md                 # 概览
├── libbpf.md                   # libbpf
├── bpftrace.md                 # bpftrace
├── bcc.md                      # BCC
├── bpftool.md                  # bpftool
├── internal.md                 # 内部机制
├── verifier.md                 # Verifier
├── arena.md                    # Arena
├── btf.md                      # BTF
├── core.md                     # CO-RE
├── todo.md                     # 待办
└── ...
```

---

## ✅ 已知的知识

### 1. Ftrace 体系

**基础概念**:
- Ftrace 是内核内置的跟踪框架
- 不需要额外安装，通过 `/sys/kernel/debug/tracing/` 访问
- 支持 function tracer、function_graph、event trace 等

**可用的 Tracers**:
```
nop          - 无操作（默认）
function     - 函数跟踪
function_graph - 函数调用图
blk          - 块设备跟踪
mmiotrace    - MMIO 跟踪
timerlat     - 定时器延迟
osnoise      - 操作系统噪声
wakeup       - 唤醒延迟
wakeup_rt    - RT 任务唤醒
wakeup_dl    - Deadline 任务唤醒
irqsoff      - 中断关闭时间
preemptoff   - 抢占关闭时间
preemptirqsoff - 中断+抢占关闭时间
```

**使用方法**:
```bash
# 查看可用 tracer
cat /sys/kernel/debug/tracing/available_tracers

# 启用 function tracer
echo function > /sys/kernel/debug/tracing/current_tracer
echo do_nanosleep > /sys/kernel/debug/tracing/set_ftrace_filter

# 查看结果
cat /sys/kernel/debug/tracing/trace
```

**Ftrace vs Kprobe**:
| 特性 | Ftrace | Kprobe |
|------|--------|--------|
| 侵入性 | 低 | 高 |
| 可编程 | 否 | 是 |
| 性能 | 极高 | 高 |
| 用途 | 函数跟踪 | 动态插桩 |

### 2. eBPF 体系

**核心组件**:
- **BPF Program**: 运行在内核虚拟机中的代码
- **Map**: 内核和用户态共享的数据结构
- **Helper**: 内核提供的辅助函数
- **Verifier**: 代码安全检查器

**Program Types** (部分):
```
kprobe/tracepoint - 内核跟踪
xdp               - 网络包处理
tc                - 流量控制
socket            - Socket 过滤
cgroup_skb        - Cgroup 网络
perf_event        - 性能事件
lsm               - 安全钩子 (LSM)
struct_ops        - 结构体操作 (TCP CC)
iter              - 内核迭代器
```

**开发流程**:
1. 编写 `.bpf.c` 程序
2. 使用 clang 编译为 BPF 字节码
3. 使用 libbpf 加载到内核
4. 用户态程序通过 map 与 BPF 通信

**工具链**:
- **libbpf**: 加载和管理 BPF 程序
- **bpftool**: 查看和调试 BPF
- **bpftrace**: 高级跟踪语言
- **BCC**: Python/C 框架

### 3. Perf 体系

**基本用法**:
```bash
# 统计性能事件
perf stat -e cycles,instructions ./program

# 记录性能数据
perf record -g ./program

# 生成报告
perf report

# 实时查看
perf top

# Probe 跟踪
perf probe --add do_nanosleep
perf record -e probe:do_nanosleep -aR -- sleep 1
```

### 4. Tracepoint

**概念**: 内核中预定义的静态跟踪点

**查看可用 tracepoints**:
```bash
ls /sys/kernel/debug/tracing/events/
```

**启用 tracepoint**:
```bash
echo 1 > /sys/kernel/debug/tracing/events/sched/sched_switch/enable
cat /sys/kernel/debug/tracing/trace
```

### 5. 前端工具

| 工具 | 类型 | 用途 |
|------|------|------|
| trace-cmd | Ftrace 前端 | 命令行工具 |
| KernelShark | GUI | 可视化分析 |
| bpftrace | 高级语言 | 快速编写跟踪脚本 |
| BCC | 框架 | Python/C 开发 |
| hotspot | GUI | Perf 数据可视化 |
| sysdig | 系统调用 | 全系统监控 |
| uftrace | 用户态 | 函数调用图 |

---

## ❓ 还不知道的问题 (待研究)

### Ftrace 相关问题

1. **Ftrace 如何防止自跟踪？**
   - 内核 trace 目录下的函数有 `notrace` 标记
   - `within_notrace_func()` 检查
   - 但具体实现细节还需深入

2. **Blktrace 为什么需要单独模块？**
   - 文档中提出的问题
   - 可能与块设备特殊性有关

3. **Fprobe 是什么？**
   - 新的探测机制？
   - 与 kprobe/fprobe 的关系？

4. **Perf 输出为什么不是 100%？**
   - 例如 96.45% 后直接跳到 36.36%
   - 采样精度问题？

5. **timerlat/osnoise tracer 的详细用法**
   - 文档已存在，但需要实践

### eBPF 相关问题

1. **libbpf 到底提供了哪些功能？**
   - 具体 API 列表
   - Skeleton 自动生成机制

2. **为什么 minimal 在 host 中无输出？**
   - `trace_pipe` 没有数据
   - Guest 中 tc.bpf.c 不能运行

3. **不同 eBPF program type 的 helper 差异**
   - 每个 type 有哪些专属 helper？

4. **BPF Arena 的使用**
   - 新特性，mmap 共享内存
   - 与 ring buffer/hash map 的区别

5. **CO-RE (Compile Once, Run Everywhere)**
   - 如何实现跨内核版本兼容？
   - BTF 的作用？

6. **Verfier 的详细规则**
   - 如何写出能通过验证的代码？
   - 常见错误模式？

7. **Socket hook 的触发时机**
   - `SEC("socket")` 什么时候调用？

8. **BCC vs libbpf-tools 的选择**
   - 各自适用场景？
   - 为什么 NixOS 上 BCC 有困难？

### 工具对比问题

1. **bpftrace/bcc/ftrace 实现 tracepoint 的差别**
   - 内部机制有何不同？

2. **Ftrace 的 kprobe 和 function tracer 区别**
   - 文档中提到的关系

3. **Tracee 为什么无法在 NixOS 使用？**
   - Docker 执行失败

4. **pprof vs gperftools 的关系？**
   - 两者如何配合使用？

### 实际应用问题

1. **如何实现 tiptop 的功能？**
   - 显示 IPC、cache miss 等 per-process

2. **rtla 工具的具体用法**
   - 实时 Linux 分析

3. **perf 的 `-R` 参数含义**
   - `perf record -e probe:xxx -g -aR`

4. **kunpeng 中 perf 输出粗糙的原因**
   - 只有粗略的调用栈

---

## 🛠️ 可以写成的工具

### 1. 快速 trace 脚本

**需求**: 一键启用常见 trace 场景

```bash
# 提议工具: quick-trace
quick-trace function do_nanosleep      # 跟踪函数
quick-trace event sched/sched_switch   # 跟踪事件
quick-trace graph do_nanosleep         # 函数调用图
quick-trace clean                      # 清理
```

**实现思路**:
```bash
#!/bin/bash
# quick-trace.sh

case "$1" in
    function)
        echo function > /sys/kernel/debug/tracing/current_tracer
        echo "$2" > /sys/kernel/debug/tracing/set_ftrace_filter
        echo "Tracing function: $2"
        ;;
    event)
        echo 1 > /sys/kernel/debug/tracing/events/$2/enable
        echo "Tracing event: $2"
        ;;
    graph)
        echo function_graph > /sys/kernel/debug/tracing/current_tracer
        echo "$2" > /sys/kernel/debug/tracing/set_graph_function
        ;;
    clean)
        echo nop > /sys/kernel/debug/tracing/current_tracer
        echo > /sys/kernel/debug/tracing/trace
        echo > /sys/kernel/debug/tracing/set_ftrace_filter
        ;;
esac
```

### 2. eBPF 快速生成器

**需求**: 根据模板生成 eBPF 程序框架

```bash
# 提议工具: ebpf-template
ebpf-template kprobe do_nanosleep   # 生成 kprobe 模板
ebpf-template tracepoint sched/sched_switch  # 生成 tracepoint 模板
ebpf-template xdp                   # 生成 XDP 模板
```

### 3. Perf 数据分析助手

**需求**: 解析 perf.data 并输出结构化报告

```bash
# 提议工具: perf-analyze
perf record -g ./program
perf-analyze --hot-functions    # 最热的函数
perf-analyze --call-graph       # 调用图分析
perf-analyze --flamegraph       # 生成火焰图
```

### 4. Trace 数据转换器

**需求**: 在不同 trace 格式间转换

```bash
# 提议工具: trace-convert
trace-convert ftrace-to-chrome   # Ftrace 转 Chrome 格式
trace-convert perf-to-flame      # Perf 转火焰图
trace-convert bpftrace-to-pcap   # 网络跟踪转 pcap
```

### 5. eBPF Map 查看器

**需求**: 实时查看 eBPF map 内容

```bash
# 提议工具: bpftool-gui / bpfmap-view
bpfmap-view --map-name my_map    # 查看 map
bpfmap-view --watch              # 实时刷新
bpfmap-view --export csv         # 导出数据
```

### 6. 跟踪事件浏览器

**需求**: 浏览和搜索可用跟踪点

```bash
# 提议工具: trace-ls
trace-ls                         # 列出所有 tracepoints
trace-ls --category sched        # 按类别过滤
trace-ls --search "switch"       # 搜索关键词
trace-ls --detail sched/sched_switch  # 详细信息
```

---

## 📚 需要补充的 Demo

### 1. Ftrace Demo

**位置**: `~/data/vn/code/src/m/ftrace_demo.c` (新文件)

**内容**:
```c
// 通过 sysfs 接口演示 ftrace 编程
// - 启用/禁用 tracer
// - 设置 filter
// - 读取 trace 结果
```

### 2. Kprobe Demo

**已有**: `~/data/vn/code/src/m/tracepoint.c`

**补充**:
- kprobe 使用示例
- kretprobe 返回值跟踪

### 3. eBPF 完整示例

**已有**: `~/data/vn/code/src/ebpf/minimal.bpf.c`

**补充更多类型**:
- `kprobe_demo.bpf.c` - Kprobe 跟踪
- `tracepoint_demo.bpf.c` - Tracepoint 跟踪
- `xdp_demo.bpf.c` - XDP 包处理
- `tc_demo.bpf.c` - TC 流量控制
- `iter_demo.bpf.c` - 内核迭代器
- `lsm_demo.bpf.c` - LSM 安全钩子

### 4. Perf Event Demo

**位置**: `~/data/vn/code/src/c/perf_event/` (新目录)

**内容**:
```c
// 使用 perf_event_open 系统调用
// - 硬件事件 (cycles, instructions)
// - 软件事件 (page faults, context switches)
// - Tracepoint 事件
```

### 5. Ring Buffer Demo

**已有**: `~/data/vn/code/src/ebpf/bootstrap.bpf.c`

**补充**:
- 生产者-消费者模式
- 大数据量处理
- 多 CPU 支持

### 6. Map Types Demo

**位置**: `~/data/vn/code/src/ebpf/map_demos/` (新目录)

**内容**:
- `hash_map.bpf.c` - 键值对存储
- `array_map.bpf.c` - 数组访问
- `ringbuf.bpf.c` - 环形缓冲区
- `stack_trace.bpf.c` - 堆栈跟踪
- `sk_storage.bpf.c` - Socket 存储

### 7. CO-RE Demo

**位置**: `~/data/vn/code/src/ebpf/core_demo/` (新目录)

**内容**:
- BTF 使用
- 字段重定位
- 跨内核版本兼容

### 8. BTF 和调试 Demo

**内容**:
- 生成 BTF
- 使用 bpftool 查看 BTF
- 调试技巧

---

## 📝 整理建议

### 立即可以做的

1. **创建 quick-trace 脚本** - 最常用的 Ftrace 快捷操作
2. **补充 eBPF demo** - 每种 program type 一个最小示例
3. **整理 todo.md** - 将问题分类为"已知"、"待研究"、"低优先级"

### 中期目标

1. **完善文档** - 为每个 tracer 编写详细使用指南
2. **工具开发** - 开发上述提议的工具
3. **示例丰富** - 每个概念都有可运行的代码

### 长期目标

1. **自动化测试** - 验证所有 demo 在最新内核可运行
2. **性能基准** - 对比不同 trace 方法的性能
3. **可视化** - 开发 trace 数据可视化工具

---

## 📊 当前进度评估

| 领域 | 文档完整性 | 代码示例 | 工具化程度 |
|------|-----------|----------|-----------|
| Ftrace | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ |
| eBPF/libbpf | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Perf | ⭐⭐⭐ | ⭐⭐ | ⭐ |
| Tracepoint | ⭐⭐⭐ | ⭐⭐ | ⭐ |
| BCC/bpftrace | ⭐⭐ | ⭐ | ⭐ |

**整体评价**: 文档较全，但工具化和 demo 还有提升空间。

---

*本文档仅做梳理，未修改原文件*

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
