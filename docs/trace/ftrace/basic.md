## 原来 trace_pipe 会自动的清理掉 trace 中内容

```txt
root@martins3:/sys/kernel/tracing# cat trace_pipe
    kworker/10:0-1653074 [010] ..... 288658.063521: sync_hw_clock <-process_one_work
    kworker/10:0-1653074 [010] ..... 289317.061835: sync_hw_clock <-process_one_work
    kworker/10:2-1676855 [010] ..... 289976.060155: sync_hw_clock <-process_one_work
    kworker/10:1-1679443 [010] ..... 290635.058468: sync_hw_clock <-process_one_work
    kworker/10:0-1681849 [010] ..... 291294.056784: sync_hw_clock <-process_one_work
    kworker/10:0-1681849 [010] ..... 291953.055112: sync_hw_clock <-process_one_work
   kworker/10:73-1706703 [010] ..... 292612.053439: sync_hw_clock <-process_one_work
^C
root@martins3:/sys/kernel/tracing# cat trace
# tracer: function
#
# entries-in-buffer/entries-written: 0/0   #P:32
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
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
