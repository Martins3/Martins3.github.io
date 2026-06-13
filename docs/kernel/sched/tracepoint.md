# 先把玩一下 sched 中各个 trace 点

## runq
- runqlat
- runqlen
- runqslower


单核虚拟机中跑
stress-ng --vm-bytes 800m --vm-keep --vm 8
```txt
🧀  sudo runqlen
Sampling run queue length... Hit Ctrl-C to end.
^C
     runqlen       : count     distribution
        0          : 0        |                                        |
        1          : 0        |                                        |
        2          : 1        |                                        |
        3          : 0        |                                        |
        4          : 0        |                                        |
        5          : 1        |                                        |
        6          : 3        |                                        |
        7          : 472      |****************************************|
```
- cpudist : 真的只能运行这么点时间吗？

```txt
sudo cpudist
Tracing on-CPU time... Hit Ctrl-C to end.
^C
     usecs               : count     distribution
         0 -> 1          : 11017    |****                                    |
         2 -> 3          : 2639     |                                        |
         4 -> 7          : 66552    |************************                |
         8 -> 15         : 110165   |****************************************|
        16 -> 31         : 48474    |*****************                       |
        32 -> 63         : 5249     |*                                       |
        64 -> 127        : 4536     |*                                       |
       128 -> 255        : 19311    |*******                                 |
       256 -> 511        : 2694     |                                        |
       512 -> 1023       : 1369     |                                        |
      1024 -> 2047       : 609      |                                        |
      2048 -> 4095       : 116      |                                        |
      4096 -> 8191       : 76       |                                        |
      8192 -> 16383      : 55       |                                        |
     16384 -> 32767      : 30       |                                        |
     32768 -> 65535      : 11       |                                        |
     65536 -> 131071     : 4        |                                        |
```
```txt
🧀   sudo cpudist
Tracing on-CPU time... Hit Ctrl-C to end.
^C
     usecs               : count     distribution
         0 -> 1          : 7565     |**                                      |
         2 -> 3          : 4660     |*                                       |
         4 -> 7          : 4782     |*                                       |
         8 -> 15         : 5322     |**                                      |
        16 -> 31         : 105994   |****************************************|
        32 -> 63         : 103194   |**************************************  |
        64 -> 127        : 20629    |*******                                 |
       128 -> 255        : 2902     |*                                       |
       256 -> 511        : 1247     |                                        |
       512 -> 1023       : 638      |                                        |
      1024 -> 2047       : 387      |                                        |
      2048 -> 4095       : 305      |                                        |
      4096 -> 8191       : 278      |                                        |
      8192 -> 16383      : 158      |                                        |
     16384 -> 32767      : 143      |                                        |
     32768 -> 65535      : 91       |                                        |
     65536 -> 131071     : 71       |                                        |
    131072 -> 262143     : 35       |                                        |
    262144 -> 524287     : 2        |                                        |
    524288 -> 1048575    : 5        |                                        |
```

- cpuunclaimed : 当一个 thread 可以运行，但是由于 cpu binding 的问题无法运行

- offcputime  : This combines the summaries from both the offcputime and wakeuptime  tools.
- offwaketime :
- wakeuptime  : 记录谁唤醒了谁，并且统计时间

- offcputime

This program shows stack traces and task names that were blocked and "off-CPU", and the total duration they were not running:
their  "off-CPU time".  It works by tracing when threads block and when they return to CPU, measuring both the time they were
off-CPU and the blocked stack trace and the task name.  This data is summarized in the kernel using an eBPF map, and by  sum‐
ming the off-CPU time by unique stack trace and task name.

The  output  summary  will help you identify reasons why threads were blocking, and quantify the time they were off-CPU. This
spans all types of blocking activity: disk I/O, network I/O, locks, page faults, involuntary context switches, etc.

This is complementary to CPU profiling (e.g., CPU flame graphs) which shows the time spent on-CPU. This shows the time  spent
off-CPU, and the output, especially the -f format, can be used to generate an "off-CPU time flame graph".

- offwaketime

This program shows kernel stack traces and task names that were  blocked
and  "off-CPU",  along  with  the  stack  traces  and task names for the
threads that woke them, and  the  total  elapsed  time  from  when  they
blocked  to  when  they were woken up.

This combines the summaries from both the offcputime and wakeuptime  tools.   The
time  measurement will be very similar to off-CPU time, however, off-CPU time may
include a little extra time spent waiting on a run queue to  be  scheduled.

- wakeuptime

It works by tracing when threads block  and  when  they
were  then woken up, and measuring the time delta. This time measurement
will be very similar to off-CPU time, however, off-CPU time may  include
a  little  extra  time spent waiting on a run queue to be scheduled.

做做实验来看看，例如:

offcputime : 就是当时离开 cpu 的时间
```txt
    finish_task_switch.isra.0
    __schedule
    schedule
    io_schedule
    bit_wait_io
    __wait_on_bit_lock
    out_of_line_wait_on_bit_lock
    do_get_write_access
    jbd2_journal_get_write_access
    __ext4_journal_get_write_access
    ext4_reserve_inode_write
    __ext4_mark_inode_dirty
    ext4_ext_insert_extent
    ext4_ext_map_blocks
    ext4_map_blocks
    ext4_do_writepages
    ext4_writepages
    do_writepages
    __writeback_single_inode
    writeback_sb_inodes
    __writeback_inodes_wb
    wb_writeback
    wb_workfn
    process_one_work
    worker_thread
    kthread
    ret_from_fork
    ret_from_fork_asm
    -                kworker/u256:6 (194406)
        5848
```

offwaketime
```txt
    waker:           kworker/u256:16 198967
    ret_from_fork_asm
    ret_from_fork
    kthread
    worker_thread
    process_one_work
    flush_to_ldisc
    tty_port_default_receive_buf
    n_tty_receive_buf_common
    __process_echoes
    tty_put_char
    pty_write
    tty_insert_flip_string_and_push_buffer
    queue_work_on
    __queue_work
    kick_pool
    --               --
    finish_task_switch.isra.0
    __schedule
    schedule
    worker_thread
    kthread
    ret_from_fork
    ret_from_fork_asm
    target:          kworker/u256:14 196701
        722317
```

所以，他们的区别:
1. offcputime 展示 target
2. offwaketime 展示 target 和 waker 的 backtrace
3. wakeuptime 展示 target

此外，offcputime 就是 target 不在 CPU 上的时间，而 wakeuptime 和 offwaketime 没有 queue 中等待的时间

## [ ] offcpu 的分析
- https://www.brendangregg.com/blog/2016-02-01/linux-wakeup-offwake-profiling.html
- https://www.brendangregg.com/FlameGraphs/offcpuflamegraphs.html

## trace_sched_waking

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
