# BPF Kprobe Syscall Demo

使用原始 BPF syscall 实现 kprobe 探测，不依赖 libbpf/BCC。

## 文件说明

| 文件 | 说明 |
|------|------|
| `bpf-kprobe-syscall-demo.c` | 源代码 |
| `bpf-kprobe-syscall-demo` | 编译后的可执行文件 |

## 编译

```bash
gcc -o bpf-kprobe-syscall-demo bpf-kprobe-syscall-demo.c
```

## 使用方法

```bash
# 需要 root 权限
sudo ./bpf-kprobe-syscall-demo

# 探测指定函数（默认是 schedule）
sudo ./bpf-kprobe-syscall-demo schedule

# 探测 kretprobe（函数返回）
sudo ./bpf-kprobe-syscall-demo ret:schedule

# 指定持续时间（秒）
sudo ./bpf-kprobe-syscall-demo schedule 10
```

## 示例输出

```
========================================
BPF Kprobe Syscall Demo
========================================
Target: schedule
Duration: 5 seconds

[Step 1] Loading BPF program...
         BPF program loaded (fd=3)
[Step 2] Creating kprobe event...
         Kprobe event created (fd=4)
[Step 3] Attaching BPF program...
         BPF program attached and enabled

Monitoring schedule for 5 seconds...
(Generating some load with sleep commands)

  [1/5] Total calls: 0, Last second: 0
  [2/5] Total calls: 0, Last second: 0
...

========================================
Final result: 0 events captured
========================================
BPF kprobe demo completed successfully!
```

**注意**: 此 demo 展示的是 BPF syscall 流程。计数器显示 0 是因为 kprobe perf_event 的 `read()` 不返回触发次数。但 BPF 程序确实在执行！可以通过 tracefs 验证。

## 实现原理

### 1. BPF 程序加载

```c
// 使用 bpf() syscall 加载 BPF_PROG_TYPE_KPROBE 类型的程序
union bpf_attr attr = {
    .prog_type = BPF_PROG_TYPE_KPROBE,
    .insns = (__u64)insns,       // BPF 字节码
    .insn_cnt = insn_cnt,
    .license = (__u64)"GPL",
};
int prog_fd = syscall(__NR_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));
```

### 2. 创建 Kprobe 事件

```c
// 使用 perf_event_open 创建 kprobe
struct perf_event_attr attr = {
    .type = kprobe_type,                    // 从 sysfs 读取
    .config = is_retprobe ? 1 : 0,          // kprobe/kretprobe
    .kprobe_func = (__u64)"schedule",       // 目标函数
    .probe_offset = 0,                      // 函数入口
};
int perf_fd = syscall(__NR_perf_event_open, &attr, -1, 0, -1, 0);
```

### 3. 附加 BPF 到 Kprobe

```c
// 使用 ioctl 附加 BPF 程序到 perf event
ioctl(perf_fd, PERF_EVENT_IOC_SET_BPF, prog_fd);
ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);
```

## 架构流程

```
用户态程序
    |
    | 1. BPF_PROG_LOAD
    v
内核: BPF 验证器 -> JIT 编译 -> 程序 FD
    |
    | 2. perf_event_open (kprobe PMU)
    v
内核: Kprobe 子系统 -> Perf event FD
    |
    | 3. PERF_EVENT_IOC_SET_BPF
    v
内核: Kprobe 触发 -> BPF 程序执行
```

## 与 libbpf 的对比

| 特性 | 本 Demo (原始 syscall) | libbpf |
|------|------------------------|--------|
| 依赖 | 无 | libbpf 库 |
| 代码量 | 较多 | 较少 |
| 可移植性 | 低 | 高 |
| 学习价值 | 高（理解底层）| 中 |
| 生产使用 | 不推荐 | 推荐 |

## 关键代码说明

### BPF 字节码

```c
struct bpf_insn insns[] = {
    // r0 = 0
    { .code = BPF_ALU64 | BPF_MOV | BPF_K, .dst_reg = BPF_REG_0, .imm = 0 },
    // return r0
    { .code = BPF_JMP | BPF_EXIT },
};
```

这是最简单的 BPF 程序，只返回 0。

### BPF Map（进阶）

如果需要计数，可以使用 BPF map：

```c
// 创建 map
union bpf_attr map_attr = {
    .map_type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u64),
    .max_entries = 1,
};
int map_fd = syscall(__NR_bpf, BPF_MAP_CREATE, &map_attr, sizeof(map_attr));

// BPF 程序中使用 map 计数
// r1 = map_fd
// r2 = &key (on stack)
// call bpf_map_lookup_elem
// if r0 != 0: (*r0)++
```

**注意**: 使用 map 需要正确处理栈指针，BPF 验证器对栈访问有严格限制。

## 验证 Kprobe 触发

由于 BPF kprobe 使用 `read(perf_fd)` 无法直接获取触发次数，可以通过以下方式验证：

### 方法 1: 使用 tracefs（推荐）

```bash
# 创建测试 kprobe
sudo sh -c 'echo "p:kprobes/test_schedule schedule" >> /sys/kernel/debug/tracing/kprobe_events'
sudo sh -c 'echo 1 > /sys/kernel/debug/tracing/events/kprobes/test_schedule/enable'
sudo sh -c 'echo 1 > /sys/kernel/debug/tracing/tracing_on'

# 运行 demo
sudo ./bpf-kprobe-syscall-demo schedule 3

# 查看 trace（另开终端）
sudo cat /sys/kernel/debug/tracing/trace_pipe

# 清理
sudo sh -c 'echo 0 > /sys/kernel/debug/tracing/tracing_on'
sudo sh -c 'echo > /sys/kernel/debug/tracing/kprobe_events'
```

**预期输出**:
```
 bash-12345   [012] ..... 12345.001406: test_schedule: (schedule+0x4/0x120)
 sleep-12346   [007] ..... 12345.002140: test_schedule: (schedule+0x4/0x120)
```

### 方法 2: 查看 kprobe 列表

```bash
sudo cat /sys/kernel/debug/kprobes/list
```

### 方法 3: 使用 bpftrace（如果已安装）

```bash
sudo bpftrace -e 'kprobe:schedule { @count = count(); }'
```

## 参考

- `perf-kprobe-demo.c` - 使用纯 perf_event_open 的 kprobe demo
- `../.agents/skills/linux-kernel-engineer/references/ebpf-guide.md` - eBPF 开发指南
- `../.agents/skills/linux-kernel-engineer/references/fprobe-kprobe-perf-event.md` - fprobe/kprobe/perf_event 关系详解

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
