## sched debugfs
<!-- f3ccc663-5fc3-4e67-8e25-b5252cb99fa1 -->

关联的源码为 : kernel/sched/debug.c

其中的东西还不少，例如
```txt
echo NO_RT_RUNTIME_SHARE > /sys/kernel/debug/sched/features
```

整体的结构为:
```txt
root@martins3:/sys/kernel/debug/sched# tree
.
├── base_slice_ns
├── debug
├── fair_server
│   ├── cpu0
│   │   ├── period
│   │   └── runtime
│   ├── cpu1
│   │   ├── period
│   │   └── runtime
│   ├── cpu10
│   │   ├── period
│   │   └── runtime
│   ├── cpu11
│   │   ├── period
│   │   └── runtime
│   ├── cpu12
│   │   ├── period
│   │   └── runtime
│   ├── cpu13
│   │   ├── period
│   │   └── runtime
│   ├── cpu14
│   │   ├── period
│   │   └── runtime
│   ├── cpu15
│   │   ├── period
│   │   └── runtime
│   ├── cpu16
│   │   ├── period
│   │   └── runtime
│   ├── cpu17
│   │   ├── period
│   │   └── runtime
│   ├── cpu18
│   │   ├── period
│   │   └── runtime
│   ├── cpu19
│   │   ├── period
│   │   └── runtime
│   ├── cpu2
│   │   ├── period
│   │   └── runtime
│   ├── cpu20
│   │   ├── period
│   │   └── runtime
│   ├── cpu21
│   │   ├── period
│   │   └── runtime
│   ├── cpu22
│   │   ├── period
│   │   └── runtime
│   ├── cpu23
│   │   ├── period
│   │   └── runtime
│   ├── cpu24
│   │   ├── period
│   │   └── runtime
│   ├── cpu25
│   │   ├── period
│   │   └── runtime
│   ├── cpu26
│   │   ├── period
│   │   └── runtime
│   ├── cpu27
│   │   ├── period
│   │   └── runtime
│   ├── cpu28
│   │   ├── period
│   │   └── runtime
│   ├── cpu29
│   │   ├── period
│   │   └── runtime
│   ├── cpu3
│   │   ├── period
│   │   └── runtime
│   ├── cpu30
│   │   ├── period
│   │   └── runtime
│   ├── cpu31
│   │   ├── period
│   │   └── runtime
│   ├── cpu4
│   │   ├── period
│   │   └── runtime
│   ├── cpu5
│   │   ├── period
│   │   └── runtime
│   ├── cpu6
│   │   ├── period
│   │   └── runtime
│   ├── cpu7
│   │   ├── period
│   │   └── runtime
│   ├── cpu8
│   │   ├── period
│   │   └── runtime
│   └── cpu9
│       ├── period
│       └── runtime
├── features
├── latency_warn_ms
├── latency_warn_once
├── migration_cost_ns
├── nr_migrate
├── numa_balancing
│   ├── hot_threshold_ms
│   ├── scan_delay_ms
│   ├── scan_period_max_ms
│   ├── scan_period_min_ms
│   └── scan_size_mb
├── preempt
├── tunable_scaling
└── verbose
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
