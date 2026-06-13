# rt.c 分析

## rt 如何区分 fifo rr

`rt_sched_class`就对应 RR 算法或者 FIFO 算法的调度策略，具体调度策略由进程的`task_struct->policy`指定；

SCHED_FIFO

SCHED_RR

https://mp.weixin.qq.com/s/EqKU052H5LxRjE68jeiaDg

> /proc/sys/kernel/sched_rt_runtime_us ,sched_rt_runtime_us 的默认值为 950000，代表 RT 进程可以占用 95% 的 CPU 时间片，剩余 5% 用于响应其他请求

这种现象的出现其实涉及到 Linux 的系统调度，它主要分为两大类：实时调度类和非实时调度类。
- 实时调度类：Linux 上实时调度类主要有 SCHED_RR、SCHED_FIFO 两种，采用 RT 调度算法。调度策略 SCHED_FIFO 和 SCHED_RR 是实时策略，数值越大优先级越高，另外实时调度策略的线程总是比前面三种通常的调度策略优先级更高。通常，调度器会为每个可能的调度优先级(sched_priority value)维护一个可运行的线程列表，并且是以最高静态优先级列表头部的线程作为下次调度的线程。所有的调度都是抢占式的：如果一个具有更高静态优先级的线程转换为可以运行了，那么当前运行的线程会被强制进入其等待的队列中。
- 非实时调度类：非实时调度类就是完全公平调度 CFS(Completely Fair Scheduler)，在内核定义中是 SCHED_OTHER 或者 SCHED_NORMAL，**这两个代表同样的含义**。

SCHED_FIFO 是一种先入先出的调度策略（First In First Out）。该策略简单来说就是一旦进程占用 CPU 则一直运行，直到有更高优先级的任务达到或者自己放弃运行，并且运行期间不响应软中断。

## rt 选核
[Linux Scheduler 之 rt 选核流程](https://mp.weixin.qq.com/s/3lNF_o8_RvD8kGxxo5VCfw)

## 似乎 rt 的 scheduler 让用户态的 spinlock 出现问题

## rt throttling
[What does "sched: RT throttling activated" mean?](https://access.redhat.com/solutions/2988101)

interrupt handlers run as threads with a SCHED_FIFO priority (default: 50). A cpu-hog thread with a SCHED_FIFO or SCHED_RR policy higher than the interrupt handler threads can prevent interrupt handlers from running and cause programs waiting for data signaled by those interrupts to be starved and fail.

一下内容在 kernel 中分析:

task_tick_rt 被调用的比较少:
```txt
@[
    update_curr_rt+5
    task_tick_rt+28
    scheduler_tick+170
    update_process_times+147
    tick_nohz_handler+143
    __hrtimer_run_queues+271
    hrtimer_interrupt+250
    __sysvec_apic_timer_interrupt+85
    sysvec_apic_timer_interrupt+57
    asm_sysvec_apic_timer_interrupt+26
    inet_gro_receive+67
    dev_gro_receive+1746
    napi_gro_receive+124
    ieee80211_rx_napi+157
    iwl_mvm_rx_mpdu_mq+5120
    iwl_pcie_rx_handle+958
    iwl_pcie_napi_poll_msix+45
    __napi_poll+40
    net_rx_action+710
    handle_softirqs+228
    do_softirq.part.0+59
    __local_bh_enable_ip+102
    iwl_pcie_irq_rx_msix_handler+205
    irq_thread_fn+32
    irq_thread+399
    kthread+205
    ret_from_fork+49
    ret_from_fork_asm+26
]: 1
```

但是 update_curr_rt 的调用位置就很多了:
```txt
@[
    update_curr_rt+5
    dequeue_task_rt+44
    __schedule+1446
    schedule+39
    irq_thread+332
    kthread+205
    ret_from_fork+49
    ret_from_fork_asm+26
]: 123
@[
    update_curr_rt+5
    put_prev_task_rt+35
    __schedule+3965
    schedule+39
    irq_thread+332
    kthread+205
    ret_from_fork+49
    ret_from_fork_asm+26
]: 123
```

一个是 timer 的 tick 正好命中，一个是 SCHED_FIFO 的程序任务做完了。

### 制作一个现场出来

## 时至今日，还是不懂各种 priority 的变换关系
https://stackoverflow.com/questions/8887531/which-real-time-priority-is-the-highest-priority-in-linux

## 两个 priority 是分开的
https://unix.stackexchange.com/questions/154728/why-highest-priority-threads-are-displayed-with-rt-in-top-and-htop

```sh
ps ax --format uname,pid,ppid,tty,cmd,cls,pri,rtprio
```

## cgroup 可以配置 rt_runtime_us

https://wiki.linuxfoundation.org/%20realtime/start

https://news.ycombinator.com/item?id=41594862

## 从 ubuntu 的 low-laterncy kernel 到底是什么说起
https://www.phoronix.com/news/Ubuntu-Low-Lat-Generic-Kernel

https://unix.stackexchange.com/questions/553980/why-would-anyone-choose-not-to-use-the-lowlatency-kernel



## rt 相关的几个变量
cd  /proc/sys/kernel/

sched_rt_period_us
sched_rt_runtime_us
sched_util_clamp_min_rt_default


> [!NOTE]
> 参考神奇海螺的意见，有待验证

  sched_rt_period_us
  它定义 RT bandwidth 控制的统计周期，也就是“多少时间算一个 100% CPU 带宽窗口”。

  源码里直接写了默认值 1s，见 kernel/sched/rt.c:14 和 kernel/sched/rt.c:34。文档说明在 Documentation/scheduler/sched-rt-group.rst:89。

  直觉上：

  - sched_rt_period_us = 1000000
  - 表示 1 秒是一个预算周期
  - 这一秒里的 RT 任务最多能用掉多少时间，由 sched_rt_runtime_us 决定

  sched_rt_runtime_us
  它定义在上面那个周期里，RT 调度类最多能跑多久。

  文档在 Documentation/scheduler/sched-rt-group.rst:92 说得很直接：

  - 它必须 <= sched_rt_period_us
  - 默认是 950000
  - -1 表示不限制，相当于 runtime == period

  默认组合是：

  - sched_rt_period_us = 1000000
  - sched_rt_runtime_us = 950000

  也就是：

  - 每 1 秒里，RT 最多跑 0.95 秒
  - 留 0.05 秒给非 RT 工作恢复系统
  - 防止 runaway SCHED_FIFO/RR 把机器完全锁死

  所以这两个变量是一对，控制的是 RT throttling / RT CPU 带宽限制。

  可以粗暴记成：

  RT 可用比例 = sched_rt_runtime_us / sched_rt_period_us

  例如默认就是 95%。

  `sched_util_clamp_min_rt_default`
  这个不是 RT throttling，而是 uclamp 默认 boost。

  文档在 Documentation/admin-guide/sysctl/kernel.rst:1300：

  - 默认 Linux 偏性能
  - RT 任务默认会有 uclamp.min = 1024
  - 这会让它们倾向于跑在最高频率、最高 capacity 的 CPU 上
  - 这个 sysctl 允许你把“RT 任务默认最小 util clamp”改小

  它只对这类 RT 任务生效：

  - 用户没有显式通过 sched_setattr() 自己改过 uclamp.min

  所以它不是硬实时优先级，不是时间片，也不是 bandwidth。它控制的是：

  - RT 任务默认要求的最低性能档位
  - 进而影响放核和提频

  比如：

  - 1024：尽量拉满
  - 800：仍然偏高，但没那么激进
  - 更低：更省电，但 RT 任务可能不再默认冲最高频和大核

  一句话区分

  - sched_rt_period_us
    RT 带宽统计周期
  - sched_rt_runtime_us
    这个周期里 RT 最多能跑多久
    主要是防止 RT 饿死普通任务
  - sched_util_clamp_min_rt_default
    RT 任务默认最低性能请求
    主要影响提频和选核，不直接限制 RT 能跑多久

  最容易混淆的点

  很多人会把 sched_rt_runtime_us 和 sched_util_clamp_min_rt_default 混成“RT 性能强弱”。

  其实不是一回事：

  - sched_rt_runtime_us
    控的是“能跑多”
  - sched_util_clamp_min_rt_default
    控的是“跑的时候要求多高性能”

## sched: RT throttling activated

感觉这个很容易遇到:
update_curr_rt

```txt
			printk_deferred_once("sched: RT throttling activated\n");
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
