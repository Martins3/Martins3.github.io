## Multi-Queue Block IO Queueing Mechanism (blk-mq)
<!-- 9172b450-19c2-4c72-821a-8a10dc569fb2 -->

https://docs.kernel.org/block/blk-mq.html

The former design had a single queue to store block IO requests with a single
lock. That did not scale well in SMP systems due to dirty data in cache and the
bottleneck of having a single lock for multiple processors.

(才意识到，当多个 core 同时发送命令给 nvme 驱动，那么 nvme 控制器和 pcie 都是需要
有能力来同时处理多个任务的)

blk-mq has two group of queues:
1. software staging queues
2. hardware dispatch queues.

When the request arrives at the block layer, it will try the shortest
path possible: send it directly to the hardware queue. However, there are **two
cases** that it might not do that:
1. if there’s an IO scheduler attached at the layer or
2. if we want to try to merge requests.
In both cases, requests will be sent to the software queue.
(默认情况是直接发送给硬件队列，除非可以 plug 或者有 scheduler)

Then, after the requests are processed by software queues, they will be placed
at the hardware queue, a second stage queue where the hardware has direct access
to process those requests. However, if the hardware does not have enough
resources to accept more requests, blk-mq will place requests on a temporary
queue, to be sent in the future, when the hardware is able.

**If it’s not possible to send the requests directly to hardware, they will be
added to a linked list (hctx->dispatch) of requests.**

Along with that, the requests can be reordered to ensure fairness of system
resources (e.g. to ensure that no application suffers from starvation) and/or
to improve IO performance, by an IO scheduler.

(wbt 和 io scheduler 的功能这么看有点重复? 或者他们的层次结构是什么)

![](./img/multiqueue.png)

## [lwn : The multiqueue block layer](https://lwn.net/Articles/552904/)

While requests are in the submission queue, they can be operated on by the block layer in the usual manner. Reordering of requests for locality offers **little** or no benefit on solid-state devices;
indeed, spreading requests out across the device might help with the parallel processing of requests.
So reordering will not be done, but coalescing requests will reduce the total number of I/O operations, improving performance somewhat.
Since the submission queues are **per-CPU**, there is no way to coalesce requests submitted to different queues.
With no empirical evidence whatsoever, your editor would guess that adjacent requests are most likely to come from the same process and,
thus, will automatically find their way into the same submission queue, so the lack of cross-CPU coalescing is probably not a big problem.

The block layer will move requests from the submission queues into the hardware queues up to the maximum number specified by the driver

- [ ] per cpu 内部可以合并，但是外部不可以
- [ ] 超过设置的数值，将会一次性提交

It offers two ways for a block driver to hook into the system, one of which is the "request" interface.

The second block driver mode — the "make request" interface — allows a driver to do exactly that. It hooks the driver into a much higher part of the stack, shorting out the request queue and handing I/O requests directly to the driver.

The multiqueue block layer work tries to fix this problem by adding a third mode for drivers to use. In this mode, the request queue is split into a number of separate queues:
  - Submission queues are set up on a per-CPU or per-node basis. Each CPU submits I/O operations into its own queue, with no interaction with the other CPUs. Contention for the submission queue lock is thus eliminated (when per-CPU queues are used) or greatly reduced (for per-node queues).
  - One or more hardware dispatch queues simply buffer I/O requests for the driver.

## [Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems](https://kernel.dk/systor13-final18.pdf)
Why we need block layer:
1. It is a convenience library to hide the complexity and diversity of storage devices from the application while providing common services that are valuable to applications.
2. In addition, the block layer implements IO-fairness, IO-error handling, IO-statistics, and IO-scheduling that improve performance and help protect end-users from poor or malicious implementations of other applications or device drivers.

Specifically, we identified three main problems:
1. Request Queue Locking
2. Hardware Interrupts
3. Remote Memory Accesses

reducing lock contention and remote memory accesses are key challenges when redesigning the block layer to scale on high NUMA-factor architectures.
Dealing efficiently with the high number of hardware interrupts is beyond the control of the block layer (more on this below) as the block layer cannot dictate how a
device driver interacts with its hardware.

Based on our analysis of the Linux block layer, we identify three major requirements for a block layer:
1. **Single Device Fairness** :  Without a centralized arbiter of device access, applications must either coordinate among themselves for fairness or rely on the fairness policies implemented in device drivers (which rarely exist).
2. **Single and Multiple Device Accounting** : Having a uniform interface for system performance monitoring and accounting enables applications and other operating system components to make intelligent decisions about application scheduling, load balancing, and performance.
3. **Single Device IO Staging Area** :
    - To do this, the block layer requires a staging area, where IOs may be buffered before they are sent down into the device driver.
    - Using a staging area, the block layer can reorder IOs, typically to promote sequential accesses over random ones, or it can group IOs, to submit larger IOs to the underlying device.
    - In addition, the staging area allows the block layer to adjust its submission rate for quality of service or due to device back-pressure indicating the OS should not send down additional IO or risk overflowing the device’s buffering capability.


**Our two level queuing strategy relies on the fact that modern SSD’s have random read and write latency that is as fast
as their sequential access. Thus interleaving IOs from multiple software dispatch queues into a single hardware dispatch
queue does not hurt device performance. Also, by inserting
requests into the local software request queue, our design
respects thread locality for IOs and their completion.**

In our design we have moved IO-scheduling functionality into the software queues only, thus even legacy devices that implement just a single
dispatch queue see improved scaling from the new multiqueue block layer.

**In addition** to introducing a **two-level queue based model**,
our design incoporates several other implementation improvements.
1. First, we introduce tag-based completions within the block layer. Device command tagging was first introduced
with hardware supporting native command queuing. A tag is an integer value that uniquely identifies the position of the
block IO in the driver submission queue, so when completed the tag is passed back from the device indicating which IO has been completed.
2. Second, to support fine grained IO accounting we have
modified the internal Linux accounting library to provide
statistics for the states of both the software queues and dispatch queues. We have also modified the existing tracing
and profiling mechanisms in blktrace, to support IO tracing
for future devices that are multi-queue aware.

While the basic mechanisms for driver registration and IO submission/completion remain
unchanged, our design introduces these following requirements:
- HW dispatch queue registration: The device driver must export the number of submission queues that it supports as well as the size of these queues, so that the
block layer can allocate the matching hardware dispatch queues.
- HW submission queue mapping function: The device driver must export a function that returns a mapping
between a given software level queue (associated to core i or NUMA node i), and the appropriate hardware dispatch queue.
- IO tag handling: The device driver tag management mechanism must be revised so that it accepts tags generated by the block layer.
While not strictly required, using a single data tag will result in optimal CPU usage between the device driver and block layer.

Large internal data parallelism in SSDs disks enables many
concurrent IO operations which, in turn, allows single devices to achieve close to a million IOs per second (IOPS)
for random accesses, as opposed to just hundreds on traditional magnetic hard drives.

sockets 在此处是什么 ?

Scalability problem of block layer:
1. Request Queue Locking:
2. Hardware Interrupts
3. Remote Memory Accesses

Based on our analysis of the Linux block layer, we identify three major requirements for a block layer:
- Single Device Fairness
- Single and Multiple Device Accounting
- Single Device IO Staging Area

The number of entries in the software level queue can dynamically grow and shrink as needed to support the outstanding queue depth maintained by the application, though
queue expansion and contraction is a relatively costly operation compared to the memory overhead of maintaining
enough free IO slots to support most application use. Conversely, the size of the hardware dispatch queue is bounded
and correspond to the maximum queue depth that is supported by the device driver and hardware.

## Multi-queue 架构分析 (内核工匠)
<!-- 672a4e0e-c926-49eb-81c6-0a8d6fac8395 -->

https://www.cnblogs.com/Linux-tech/p/12961279.html

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
