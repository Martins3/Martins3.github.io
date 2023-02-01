# QEMU 中的 trace 机制

## 关键参考
- https://qemu.readthedocs.io/en/latest/devel/tracing.html

## 用户态的 trace
```c
  --enable-trace-backends=CHOICES
                           Set available tracing backends [log] (choices:
                           dtrace/ftrace/log/nop/simple/syslog/ust)
```


## 如果什么都
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

## 使用 ftrace 的时候，是不是实际上将信息导入到 kernel 的 ftrace buffer 中?
- qemu/trace/ftrace.c

## --enable-trace-backends=simple,ftrace

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

## --enable-trace-backends=nop
```c
static inline void _nocheck__trace_memory_region_ops_read(int cpu_index, void * mr, uint64_t addr, uint64_t value, unsigned size, const char * name)
{
}
```

## dtrace 这个程序没有
