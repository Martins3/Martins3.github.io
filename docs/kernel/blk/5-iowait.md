# iowait

## aio 的 iodepth 如何影响 iowait 的输出

测试脚本
```ini
[global]
time_based=1
runtime=1000
ioengine=io_uring
iodepth=2
direct=1
numjobs=1
bs=4k

[trash]
rw=randread
filename=/dev/sda
; /dev/sda 是一个 HDD
```

```sh
sudo taskset -ac 1 fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
```

然后观察 cpu1 中的输出，发现是否有 wa 的取决于盘的

关键的区别是没有调用 io_schedule

`_blk_mq_alloc_requests` 中，如果获取 blk_mq_get_tag 失败，
那么睡眠 3 ms ，然后 retry ，当然其实可以使用一个 wait queue ，
但是睡眠 3 ms 也不错。

如果 wa 接近 100% 的时候，程序实际上卡在 blk_mq_get_tag 上。
```txt
@[
    schedule+5
    io_schedule+70
    blk_mq_get_tag+348
    __blk_mq_alloc_requests+437
    blk_mq_submit_bio+424
    __submit_bio+149
    submit_bio_noacct_nocheck+666
    blkdev_direct_IO.part.0+586
    blkdev_read_iter+184
    aio_read+312
    io_submit_one+392
    __x64_sys_io_submit+173
    do_syscall_64+57
    entry_SYSCALL_64_after_hwframe+120
]: 593
@[
    schedule+5
    worker_thread+413
    kthread+244
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1364
```

当 iodepth 足够小，程序等待的位置在 do_io_getevent ，
```txt
@[
    schedule+0
    do_io_getevents+152
    __arm64_sys_io_getevents+100
    invoke_syscall+80
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+68
    el0t_64_sync_handler+256
    el0t_64_sync+392
]: 900
@[
    schedule+0
    kthread+236
    ret_from_fork+16
]: 2082
```



io uring 是可以正确处理这个问题 ，但是不是通过 io_schedule 来实现的
也就是无论 iodepth 是多少，例如是 1 ，面对这个 HDD 盘，io uring 总是
可以让这个 CPU 的 wa 接近 100% (有待分析这个如何实现的)

### 进一步深入的思考

发现默认情况下，只有 iodepth 大于 64 的时候，才会实现 wa 是 100%
```txt
cat /sys/block/sda/queue/nr_requests
64

/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/queue_depth 32
32

root@localhost:/sys/kernel/debug/block/sda/hctx0# cat tags | grep nr_tags
nr_tags=32

root@localhost:/sys/kernel/debug/block/sda/hctx0# cat sched_tags | grep nr_tags
nr_tags=64
```

当修改盘的 scheduler 是 none 之后，iodepth 大于 32 就可以实现 wa 为 100% ，

此时，sched_tags 的内容为空，也就是说，io scheduler 可以持有更多的 request ，
这是合理的，因为 io scheduler 本来就需要更多的 io 来合并。



## 细节分析
fs/proc/stat.c:show_stat

文档: https://man7.org/linux/man-pages/man5/proc_stat.5.html

```txt
                     iowait (since Linux 2.5.41)
                            (5) Time waiting for I/O to complete.  This
                            value is not reliable, for the following
                            reasons:

                            •  The CPU will not wait for I/O to complete;
                               iowait is the time that a task is waiting
                               for I/O to complete.  When a CPU goes into
                               idle state for outstanding task I/O,
                               another task will be scheduled on this
                               CPU.

                            •  On a multi-core CPU, the task waiting for
                               I/O to complete is not running on any CPU,
                               so the iowait of each CPU is difficult to
                               calculate.

                            •  The value in this field may decrease in
                               certain conditions.
```
1. 第一条，如果 fio 一个 HDD ，通过 top 可以看到 wa 接近 100% ，但是如果此时
stress-ng --vm-bytes 40M --vm-keep --vm 32 可以发现 wa 会变为 0 。
2. 第二条，不太理解
3. 第三条，可能是和这个有关
```c
/**
 * get_cpu_iowait_time_us - get the total iowait time of a CPU
 * @cpu: CPU number to query
 * @last_update_time: variable to store update time in. Do not update
 * counters if NULL.
 *
 * Return the cumulative iowait time (since boot) for a given
 * CPU, in microseconds. Note this is partially broken due to
 * the counter of iowait tasks that can be remotely updated without
 * any synchronization. Therefore it is possible to observe backward
 * values within two consecutive reads.
 *
 * This time is measured via accounting rather than sampling,
 * and is as accurate as ktime_get() is.
 *
 * Return: -1 if NOHZ is not enabled, else total iowait time of @cpu
 */
u64 get_cpu_iowait_time_us(int cpu, u64 *last_update_time)
{
	struct tick_sched *ts = &per_cpu(tick_cpu_sched, cpu);

	return get_cpu_sleep_time_us(ts, &ts->iowait_sleeptime,
				     nr_iowait_cpu(cpu), last_update_time);
}
EXPORT_SYMBOL_GPL(get_cpu_iowait_time_us);
```

### iodepth

### 进一步的思考
- 那么，是不是应该修改 read_events 中的 schedule 为 io_schedule
  - 4.19 内核也是如此的，之前使用的是 wait_event_interruptible_hrtimeout
    - 但是，似乎各种 wait event 函数都是不可以描述 io_schedule 效果的
  - aio 可以用于其他的东西，例如网络或者事件，修改为 io_schedule 会导致语义变化?

## 看看
https://www.kawabangga.com/posts/5903

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
