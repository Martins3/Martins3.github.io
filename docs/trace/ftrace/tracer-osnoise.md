# osnoise
Documentation/trace/osnoise-tracer.rst

```txt
echo timerlat > current_tracer
```
没太搞懂实现的原理

https://research.redhat.com/blog/article/osnoise-for-fine-tuning-operating-system-noise-in-linux-kernel/

这个在 HPC 领域的时候很有用的，可以检查出来程序被操作系统中什么干扰了

## CLK 2023 的一个报告

- New kernel tracer(osnoise, timerlat) to locate the latency spike

What can consume the cpu time?
- Linux task abstractions
  - NMI
  - IRQs
  - Softirqs
  - Threads
- Hardware can interfere your task
  - SMIs
  - VMs

## 和 hwlat 什么关系?
osnoise 应该是一个更加全面的工具

## 看看这个
https://lpc.events/event/16/contributions/1217/attachments/1041/2080/2022_lpc_osnoise.pdf
```sh
echo osnoise > current_tracer
echo osnoise > set_event # 这个是有必要的吗?
echo 8 > osnoise/stop_tracing_us
cat trace
```

## 测试
当启动 osnoise 之后，会立刻把所有的 CPU 都运行满
```txt
# tracer: osnoise
#
#                                _-----=> irqs-off
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable                         MAX
#                              |||| /     delay                                   SINGLE      Interference counters:
#                              |||||               RUNTIME      NOISE  %% OF CPU  NOISE    +-----------------------------+
#           TASK-PID      CPU# |||||   TIMESTAMP    IN US       IN US  AVAILABLE  IN US     HW    NMI    IRQ   SIRQ THREAD
#              | |         |   |||||      |           |             |    |            |      |      |      |      |      |
           <...>-6172    [031] ....1   342.580948: 1000000       3767  99.62330      19   1026      0   1000     31      4
           <...>-6141    [000] ....1   343.580041: 1000000       5311  99.46890     106   1032      0   1000     19      1
           <...>-6142    [001] ....1   343.580079: 1000000       6149  99.38510      18   1001      0   1000     13      0
           <...>-6143    [002] ....1   343.580082: 1000000       4721  99.52790      30   1008      0   1000     20      0
           <...>-6144    [003] ....1   343.580121: 1000000       9147  99.08530      23   1002      0   1000     25      1
           <...>-6145    [004] ....1   343.580144: 1000000       4699  99.53010      28   1011      0   1000     22      0
           <...>-6147    [006] ....1   343.580184: 1000000      31086  96.89140     454   1496      0   1000     21      0
           <...>-6146    [005] ....1   343.580194: 1000000       4936  99.50640      25   1013      0   1008     23     10
           <...>-6148    [007] ....1   343.580277: 1000000       7193  99.28070      37   1008      0   1000     12      0
           <...>-6150    [009] ....1   343.580326: 1000000       6333  99.36670      37   1001      0   1000     17      1
           <...>-6149    [008] ....1   343.580337: 1000000       8585  99.14150     771   1008      0   1030     20     34
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
