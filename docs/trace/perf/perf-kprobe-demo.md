# perf_event_open kprobe 演示

本目录包含一个使用 `perf_event_open` 系统调用直接创建 kprobe 的演示程序，展示了 kprobe 如何被封装为 perf_event 的 PMU 类型。

## 文件说明

| 文件 | 说明 |
|------|------|
| `perf-kprobe-demo.c` | 源代码 |
| `perf-kprobe-demo` | 编译后的可执行文件 |
| `perf-kprobe-demo.md` | 本文档 |

## 使用方法

### 基本用法

```bash
# 需要 root 权限
sudo ./perf-kprobe-demo

# 探测指定的内核函数 (默认探测 schedule)
sudo ./perf-kprobe-demo schedule

# 探测 do_nanosleep (需要生成 sleep 调用才能看到事件)
sudo ./perf-kprobe-demo do_nanosleep

# 探测 kretprobe (函数返回)
sudo ./perf-kprobe-demo ret:schedule
```

### 示例输出

```
$ sudo ./perf-kprobe-demo schedule
kprobe PMU type = 8
探测函数: schedule
(使用 Ctrl+C 停止)

kprobe 已启用，等待事件...

[     1] time=18446744071592549493 ip=0x3cdb700f6b8 --> entry
[     2] time=18446744071592549493 ip=0x3cdbb9ca581 --> entry
[     3] time=18446744071592549493 ip=0x3cdc1a0bab9 --> entry
[     4] time=18446744071592549493 ip=0x3cdc24aaafa --> entry
[     5] time=18446744071592549493 ip=0x3cdc79b200e --> entry
         -------- (count=5)
...

总计捕获 1234 个事件
kprobe 已停止.
```

**kretprobe 示例:**

```
$ sudo ./perf-kprobe-demo ret:schedule
kprobe PMU type = 8
探测函数: ret:schedule
(使用 Ctrl+C 停止)

kprobe 已启用，等待事件...

[     1] time=18446744071592594748 ip=0x3d2b5bcad8e <-- return
[     2] time=18446744071592594748 ip=0x3d2bbc543dc <-- return
[     3] time=18446744071592594748 ip=0x3d2c1cd50b8 <-- return
...

总计捕获 5678 个事件
kprobe 已停止.
```

## 原理说明

### 1. kprobe PMU

内核将 kprobe 封装为一种 PMU (Performance Monitoring Unit) 类型：

```
/sys/bus/event_source/devices/kprobe/type
```

读取该文件得到 PMU 类型 ID（通常是 6 或 8），用于 `perf_event_attr.type`。

### 2. perf_event_attr 配置

关键字段：
- `type`: kprobe PMU 类型（从 sysfs 读取）
- `config`: bit 0 设置是否为 kretprobe（1 = kretprobe, 0 = kprobe）
- `kprobe_func`: 要探测的内核函数名（与 `config1` 是 union）
- `probe_offset`: 函数内偏移（与 `config2` 是 union，0 表示函数入口）

```c
struct perf_event_attr attr = {
    .type = kprobe_type,        // 从 /sys/bus/event_source/devices/kprobe/type 读取
    .config = is_kretprobe ? 1 : 0,
    .kprobe_func = (uint64_t)"do_nanosleep",  // union with config1
    .probe_offset = 0,           // union with config2
    // ...
};
```

### 3. 数据通路

```
内核函数被调用
    ↓
kprobe 触发
    ↓
perf_event 子系统捕获
    ↓
写入 mmap ring buffer
    ↓
用户态从 ring buffer 读取
```

### 4. 与传统 kprobe API 的区别

| 方式 | API | 数据传递 | 典型用户 |
|------|-----|----------|----------|
| **perf_event_open** | `syscall(__NR_perf_event_open, ...)` | mmap ring buffer | `perf` 工具, eBPF |
| **直接 kprobe** | `register_kprobe()` | 直接回调 | 内核模块 |

## 相关内核代码

- `kernel/events/core.c` - perf_event 核心，定义 `perf_kprobe` PMU
- `kernel/trace/trace_event_perf.c` - `perf_kprobe_init()` 实现
- `kernel/kprobes.c` - kprobe 核心实现
- `tools/lib/bpf/libbpf.c` - libbpf 中的 `perf_event_open_probe()` 函数

## 常见问题

### 1. Permission denied

需要 root 权限或 `CAP_PERFMON` 能力：

```bash
sudo ./perf-kprobe-demo
```

### 2. 找不到 kprobe PMU

检查内核配置：

```bash
# 检查是否启用了 kprobe 事件
cat /boot/config-$(uname -r) | grep CONFIG_KPROBE_EVENTS

# 检查 sysfs 路径是否存在
ls /sys/bus/event_source/devices/kprobe/
```

### 3. 函数不存在

检查函数名是否在内核符号表中：

```bash
grep do_nanosleep /proc/kallsyms
```

### 4. 没有捕获到事件

- 确保目标函数确实被调用
- 对于低频函数（如 `do_nanosleep`），需要生成对应的调用才能看到事件
- 可以尝试探测高频函数如 `schedule` 来验证功能正常

## 参考

- `bpf-kprobe-syscall-demo.c` - 使用 BPF syscall 的 kprobe demo
- `bpf-kprobe-syscall-demo.md` - BPF kprobe 文档
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
