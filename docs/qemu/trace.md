# QEMU 中的 trace 机制

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

## dtrace 还没有测试过

## 使用 ftrace 的时候，是不是实际上将信息导入到 kernel 的 ftrace buffer 中?
- qemu/trace/ftrace.c

```sh
qemu-system-x86_64 -trace help
```

## 测试一下，如果添加 trace 对于性能的影响有多少?

使用  fio 4k randread virtio-blk 来测试
875k
894k

大约 2% 的性能差别，其实勉强可以接受吧

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
