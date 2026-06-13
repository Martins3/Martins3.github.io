## 阅读代码
> libs/skew_heap.h: 提供了基本的优先队列数据结构，为本次实验提供了抽象数据结构方面的支持。

> kern/process/proc.[ch]：proc.h中扩展了proc_struct的成员变量，用于RR和stride调度算法。proc.c中实现了lab6_set_priority，用于设置进程的优先级。

> kern/schedule/{sched.h,sched.c}: 定义了 ucore 的调度器框架，其中包括相关的数据结构（包括调度器的接口和运行队列的结构），和具体的运行时机制。

lab5 中间wakeup_proc似乎在本地的简单设置函数，而在lab6 进入sched.c 中间。含有测试，然后调用其中各种函数的接口。

> kern/schedule/{default_sched.h,default_sched.c}: 具体的 round-robin 算法，在本次实验中你需要了解其实现。

> kern/schedule/default_sched_stride_c: Stride Scheduling调度器的基本框架，在此次实验中你需要填充其中的空白部分以实现一个完整的 Stride 调度器。



## 文档
则我们有可能希望每个进程得到的时间资源与他们的优先级成正比关系。Stride调度是基于这种想法的一个较为典型和简单的算法

该算法的基本思想可以考虑如下：
1. 为每个runnable的进程设置一个当前状态stride，表示该进程当前的调度权。另外定义其对应的pass值，表示对应进程在调度后，stride 需要进行的累加值。
1. 每次需要调度时，从当前 runnable 态的进程中选择 stride最小的进程调度。
1. 对于获得调度的进程P，将对应的stride加上其对应的步长pass（只与进程的优先权有关系）。
1. 在一段固定的时间之后，回到 2.步骤，重新调度当前stride最小的进程。
1. 可以证明，如果令 P.pass =BigStride / P.priority 其中 P.priority 表示进程的优先权（大于 1），而 BigStride 表示一个预先定义的大常数，则该调度方案为每个进程分配的时间将与其优先级成正比。证明过程我们在这里略去，有兴趣的同学可以在网上查找相关资料。将该调度器应用到 ucore 的调度器框架中来，则需要将调度器接口实现如下：

stride pass
pass 是 stride 在调用之后需要被添加的数值
stride最小的会被调用



## 问题
4. 时钟是如何逐步触发让当前进程被强制切换的。
https://chyyuu.gitbooks.io/ucore_os_docs/content/lab6/lab6_3_4_1_kernel_preempt_point.html

5. 为什么用户程序中间总是需要写入大量的yeild。

9. enqueue的实现中间，为什么需要添加`proc-> rq = rq;`

10. 在alloc_proc 中间添加两行代码
```
    proc->lab6_priority = 2;
    proc->lab6_stride = 0;
```
可以修复spin的问题

## 笔记
而且，当调度器比较复杂时，schedule 函数本身也会比较复杂，这样的设计结构很不清晰而且难免会出现错误。所以在此次实验中，ucore建立了一个单独的进程(kern/process/proc.c 中的 idleproc)作为 cpu 空闲时的 idle 进程，这个程序是通常一个死循环。你需要了解这个程序的实现。




，在具体实现中，ucore定义的进程控制块struct proc_struct包含了成员变量state,用于描述进程的运行状态，*而running和runnable共享同一个状态(state)值(PROC_RUNNABLE*。不同之处在于处于running态的进程不会放在运行队列中。

内核启动第一个用户进程的过程，实际上是从进程启动时的内核状态切换到该用户进程的内核状态的过程，而且该用户进程在**用户态的起始入口应该是forkret**。

*run_timer_list函数在每次timer中断处理过程中被调用，从而可用来调用调度算法所需的timer时间事件感知操作，调整相关进程的进程调度相关的属性值。通过调用调度类接口函数sched_class_proc_tick使得此操作与具体调度算法无关*

## 附录
附录文档:
http://web.eecs.umich.edu/~mosharaf/Readings/Stride.pdf


#### 调度算法的架构是什么
会发现其中只涉及了**三个关键调度相关函数**：wakup_proc、shedule、run_timer_list。如果我们能够让这三个调度相关函数的实现与具体调度算法无关，那么就可以认为ucore实现了一个与调度算法无关的调度框架。


#### 具体的调用算法内容实现是什么
1. lab6_stride 作为比较，初始值为0
    2. 更新算法：
          proc->lab6_stride += BIG_STRIDE / proc->lab6_priority;
    3. 可以保证所有的 lab6_priority 总是在 BIG_STRIDE 中间，从而即使溢出，也不会由此没有办法比较大小!

2. time_slice

3. rq
    1. max_time_slice 其实是一个常数
    2. run_list 和 heap 对应

1. 进程规划中间，为什么进程选择和进程离开划分成为两个部分:
```c
RR_dequeue(struct run_queue *rq, struct proc_struct *proc)
static struct proc_struct * RR_pick_next(struct run_queue *rq) {


```

```c
  if ((next = sched_class_pick_next()) != NULL) {
      sched_class_dequeue(next);
  }
```
> 实际上，这些函数的位置

#### default_sched 的算法内容是什么？
1. 没有什么意义，不管了!


## 总结一下
lab7 内容很少，从shedule 和 wakeup_proc 中间发生变化，使用

```c
sched_class_enqueue(proc);

if ((next = sched_class_pick_next()) != NULL) {
    sched_class_dequeue(next);
```
来管理proc 而已。

