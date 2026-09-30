# QEMU 中的 trace 机制
<!-- 9e98b48a-6b73-48b8-aeca-3f5ee4e04347 -->

## 关键参考
https://github.com/qemu/qemu/blob/master/docs/devel/tracing.rst
这就是全部内容了

qemu 的 trace 默认 backend 是可以动态添加 trace 的

发现即便是 backend 是 nop ，还是有如下的内容:
```txt
(qemu) info trace-events
handle_qmp_command : state 0
monitor_qmp_respond : state 0
monitor_qmp_cmd_out_of_band : state 0
monitor_qmp_err_in_band : state 0
monitor_qmp_cmd_in_band : state 0
monitor_qmp_in_band_dequeue : state 0
monitor_qmp_in_band_enqueue : state 0
monitor_suspend : state 0
monitor_protocol_event_queue : state 0
monitor_protocol_event_emit : state 0
monitor_protocol_event_handler : state 0
handle_hmp_command : state 0
```

## 相关文件
这个脚本 scripts/qemu-trace-stap
这个目录 : trace/

## 各种配置的展开

```txt
  --enable-trace-backends=CHOICES
                           Set available tracing backends [log] (choices:
                           dtrace/ftrace/log/nop/simple/syslog/ust)
```
### 默认配置

默认配置不会导致性能下降?

/home/martins3/core/qemu/build/trace/trace-softmmu.h
```c
static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    if (trace_event_get_state(TRACE_MEMORY_REGION_OPS_READ) && qemu_loglevel_mask(LOG_TRACE)) {
        if (message_with_timestamp) {
            struct timeval _now;
            gettimeofday(&_now, NULL);
#line 12 "/home/martins3/core/qemu/softmmu/trace-events"
            qemu_log("%d@%zu.%06zu:memory_region_ops_read " "cpu %d mr %p addr 0x%"PRIx64" value 0x%"PRIx64" size %u name '%s'" "\n",
                     qemu_get_thread_id(),
                     (size_t)_now.tv_sec, (size_t)_now.tv_usec
                     , cpu_index, mr, addr, value, size, name);
#line 199 "trace/trace-softmmu.h"
        } else {
#line 12 "/home/martins3/core/qemu/softmmu/trace-events"
            qemu_log("memory_region_ops_read " "cpu %d mr %p addr 0x%"PRIx64" value 0x%"PRIx64" size %u name '%s'" "\n", cpu_index, mr, addr, value, size, name);
#line 203 "trace/trace-softmmu.h"
        }
    }
}
```

### --enable-trace-backends=simple

```c

static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    _simple_trace_memory_region_ops_read(cpu_index, mr, addr, value, size, name);
}

void _simple_trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    TraceBufferRecord rec;
    size_t argname_len = name ? MIN(strlen(name), MAX_TRACE_STRLEN) : 0;

    if (!trace_event_get_state(TRACE_MEMORY_REGION_OPS_READ)) {
        return;
    }

    if (trace_record_start(&rec, _TRACE_MEMORY_REGION_OPS_READ_EVENT.id, 8 + 8 + 8 + 8 + 8 + 4 + argname_len)) {
        return; /* Trace Buffer Full, Event Dropped ! */
    }
    trace_record_write_u64(&rec, (uint64_t)cpu_index);
    trace_record_write_u64(&rec, (uintptr_t)(uint64_t *)mr);
    trace_record_write_u64(&rec, (uint64_t)addr);
    trace_record_write_u64(&rec, (uint64_t)value);
    trace_record_write_u64(&rec, (uint64_t)size);
    trace_record_write_str(&rec, name, argname_len);
    trace_record_finish(&rec);
}

```

### --enable-trace-backends=simple,ftrace

如果配置了两个日志，那么日志就会写入到两个地方中去:
```c
static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    _simple_trace_memory_region_ops_read(cpu_index, mr, addr, value, size, name);
    {
        char ftrace_buf[MAX_TRACE_STRLEN];
        int unused __attribute__ ((unused));
        int trlen;
        if (trace_event_get_state(TRACE_MEMORY_REGION_OPS_READ)) {
#line 12 "/home/martins3/core/qemu/softmmu/trace-events"
            trlen = snprintf(ftrace_buf, MAX_TRACE_STRLEN,
                             "memory_region_ops_read " "cpu %d mr %p addr 0x%"PRIx64" value 0x%"PRIx64" size %u name '%s'" "\n" , cpu_index, mr, addr, value, size, name);
#line 223 "trace/trace-softmmu.h"
            trlen = MIN(trlen, MAX_TRACE_STRLEN - 1);
            unused = write(trace_marker_fd, ftrace_buf, trlen);
        }
    }
}
```

### --enable-trace-backends=ftrace

似乎真的就是 ftrace 只是作用于 ftrace 了:
```c
static inline void _nocheck__trace_memory_region_ops_write(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    {
        char ftrace_buf[MAX_TRACE_STRLEN];
        int unused __attribute__ ((unused));
        int trlen;
        if (trace_event_get_state(TRACE_MEMORY_REGION_OPS_WRITE)) {
#line 20 "../system/trace-events"
            trlen = snprintf(ftrace_buf, MAX_TRACE_STRLEN,
                             "memory_region_ops_write " "cpu %d mr %p addr 0x%"PRIx64" value 0x%"PRIx64" size %u name '%s'" "\n" , cpu_index, mr, addr, value, size, name);
#line 386 "trace/trace-system.h"
            trlen = MIN(trlen, MAX_TRACE_STRLEN - 1);
            unused = write(trace_marker_fd, ftrace_buf, trlen);
        }
    }
}
```

### --enable-trace-backends=nop
```c
static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
}
```

### --enable-trace-backends=ust


似乎这个就是我想要的，没有任何性能损失的
```c
static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
    tracepoint(qemu, memory_region_ops_read, cpu_index, mr, addr, value, size, name);
}
```

## 使用 ftrace 的时候，是不是实际上将信息导入到 kernel 的 ftrace buffer 中?
- qemu/trace/ftrace.c

```sh
qemu-system-x86_64 -trace help
```

## 性能影响

测试一下，如果添加 trace 对于性能的影响有多少?

使用  fio 4k randread virtio-blk 来测试
875k
894k

大约 2% 的性能差别，其实勉强可以接受吧



## qemu 中的 dtrace 影响
实现上的重叠”，但 DTrace 并不等价于 SystemTap。QEMU 这里的命名容易误导，因为 dtrace backend 实际表达的是：

> 生成兼容 DTrace SDT/USDT 格式的静态探针。

而在 Linux 上，这套探针通常由 SystemTap 提供工具链并负责消费。

整个路径大致是：

trace-events
    ↓ tracetool --format=d
DTrace provider 描述文件
    ↓ dtrace -h / -G
QEMU ELF 中的 SDT/USDT 探针
    ↓ stap
SystemTap 脚本捕获探针

具体来说：

- QEMU 先生成 provider qemu { probe ... } 形式的 DTrace provider 定义，见 scripts/tracetool/format/d.py:42。
- 构建时调用 dtrace -h 和 dtrace -G 生成头文件及对象，见 trace/meson.build:43。
- 在 Linux 上，这个 dtrace 命令通常不是完整的原生 DTrace，而是 SystemTap 提供的 DTrace/SDT 兼容工具。
- QEMU 再生成 .stp tapset，把 SystemTap probe 映射到这些 USDT mark：

probe qemu.system.x86_64.some_event =
    process("/path/to/qemu-system-x86_64").mark("some_event")

对应代码在 scripts/tracetool/format/stap.py:44。

QEMU 文档其实直接说明了这种关系：

> "dtrace" backend uses DTrace sdt probes but has only been tested with SystemTap

见 docs/devel/tracing.rst:349。

所以更准确的理解是：

 名称                    在这里扮演的角色
━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 QEMU dtrace backend     在 QEMU 二进制里埋入 SDT/USDT 静态探针
──────────────────────  ─────────────────────────────────────────────────
 dtrace provider 格式    描述 probe 名称和参数的兼容接口
──────────────────────  ─────────────────────────────────────────────────
 SystemTap               Linux 上注册、启用和读取这些探针的 tracing 系统
──────────────────────  ─────────────────────────────────────────────────
 原生 DTrace             Solaris/macOS 等系统上的完整 tracing 框架

之所以看起来“等价”，是因为 DTrace 这个词在代码里主要指 SDT 探针格式/ABI，不是完整的 DTrace tracing 系统。SystemTap 实现了对这个格式的兼容，因此同一个
QEMU backend 同时服务于二者。甚至 backend 文件自己就写着 DTrace/SystemTAP backend，见 scripts/tracetool/backend/dtrace.py:3。

如果重新命名，它叫 usdt 或 sdt backend 可能更直观。

## ust 和 systemtap 的关系

dtrace 和 ust 是两个并列、相互独立的 QEMU trace backend，共享同一份 trace-events 定义，但使用不同的 tracing 生态。

其中 UST 明确指 LTTng Userspace Tracer，不是 SystemTap。

 QEMU backend    探针机制                    运行时工具                      数据格式/用途
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 dtrace          DTrace SDT/USDT 静态探针    SystemTap stap 或原生 DTrace    灵活脚本、临时诊断、按进程附加
──────────────  ──────────────────────────  ──────────────────────────────  ──────────────────────────────────
 ust             LTTng-UST tracepoint        lttng、lttng-sessiond           高吞吐采集、结构化 CTF、长期分析

### 编译时区别

对于同一个事件，例如：

qemu_malloc(void *ptr, size_t size)

dtrace backend 大致生成：

QEMU_QEMU_MALLOC(ptr, size);

并在 ELF 中放入 SDT/USDT probe。SystemTap 通过：

process("/path/to/qemu").mark("qemu_malloc")

捕获它。实现见 scripts/tracetool/backend/dtrace.py:37。

而 ust backend 生成：

tracepoint(qemu, qemu_malloc, ptr, size);

实现见 scripts/tracetool/backend/ust.py:33。它还会生成：

TRACEPOINT_EVENT(
    qemu,
    qemu_malloc,
    TP_ARGS(...),
    TP_FIELDS(...)
)

字段会编码为 LTTng 的 CTF 数据类型，见 scripts/tracetool/format/ust_events_h.py:61。

选择 ust 还会让 QEMU 链接 lttng-ust：

dependency('lttng-ust', required: true)

见 meson.build:1145。

### 运行时区别

dtrace/SystemTap 的典型操作是：

qemu-trace-stap run qemu-system-x86_64 'kvm_*'

或者自己编写 .stp 脚本。它很适合现场调查，因为 SystemTap 还能把 QEMU USDT probe、内核 probe、函数 probe 等组合起来。

ust 则采用 LTTng session 工作流：

```txt
lttng create qemu-test
lttng enable-event 'qemu:*' -u
lttng start

# 运行 QEMU

lttng stop
lttng view
lttng destroy
```
见 docs/devel/tracing.rst:306。

### 可以同时启用

可以这样构建：

./configure --enable-trace-backends=dtrace,ust

QEMU 会为每个 trace_foo() 调用同时生成两套埋点路径。文档明确说明：启用多个 backend 时，trace 会发送给所有 backend，见 docs/devel/tracing.rst:214。

不过只有被外部 tracing 工具启用的探针才会真正记录数据：

trace_foo()
 ├─ DTrace/USDT probe  → stap/DTrace 是否启用
 └─ LTTng tracepoint   → LTTng session 是否启用

所以它们的关系可以概括为：

> dtrace 是面向 DTrace/SystemTap USDT 生态的输出适配器；ust 是面向 LTTng-UST 生态的输出适配器。两者共享 QEMU 事件定义，但探针 ABI、运行时、缓冲区和输出
> 格式都不同。

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
