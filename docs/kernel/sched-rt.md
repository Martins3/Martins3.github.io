# rt.c 分析


## sched_attr

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
