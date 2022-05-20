## perf
1. kernel/events/core.c 中间定义了 `perf_event_open`
2. 输出放到 buffer ring 中间

```c
/*
 * Callers need to ensure there can be no nesting of this function, otherwise
 * the seqlock logic goes bad. We can not serialize this because the arch
 * code calls this from NMI context.
 */
void perf_event_update_userpage(struct perf_event *event)
```

[^1] https://jvns.ca/blog/2016/03/12/how-does-perf-work-and-some-questions/
