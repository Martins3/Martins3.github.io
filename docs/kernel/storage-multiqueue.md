# multiqueue

## 收集一点 backtrace
```txt
#0  blk_mq_start_request (rq=rq@entry=0xffff888140d83180) at block/blk-mq.c:1249
#1  0xffffffff81a9ba13 in virtblk_prep_rq (vblk=<optimized out>, vblk=<optimized out>, vbr=0xffff888140d83288, req=0xffff888140d83180, hctx=0xffff888140646000) at drivers/block/virtio_blk.c:335
#2  virtblk_prep_rq_batch (req=0xffff888140d83180) at drivers/block/virtio_blk.c:394
#3  virtio_queue_rqs (rqlist=0xffffc900001cfd90) at drivers/block/virtio_blk.c:433
#4  0xffffffff816cf01c in __blk_mq_flush_plug_list (q=<optimized out>, q=<optimized out>, plug=0xffffc900001cfd90) at block/blk-mq.c:2731
#5  __blk_mq_flush_plug_list (plug=0xffffc900001cfd90, q=0xffff888004499ea8) at block/blk-mq.c:2726
#6  blk_mq_flush_plug_list (plug=plug@entry=0xffffc900001cfd90, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2787
#7  0xffffffff816c1245 in __blk_flush_plug (plug=0xffffc900001cfd90, plug@entry=0xffffc900001cfd20, from_schedule=from_schedule@entry=false) at block/blk-core.c:1139
#8  0xffffffff816c1504 in blk_finish_plug (plug=0xffffc900001cfd20) at block/blk-core.c:1163
#9  blk_finish_plug (plug=plug@entry=0xffffc900001cfd90) at block/blk-core.c:1160
#10 0xffffffff8140488c in wb_writeback (wb=wb@entry=0xffff88815664d800, work=work@entry=0xffffc900001cfe30) at fs/fs-writeback.c:2097
#11 0xffffffff81405b49 in wb_check_background_flush (wb=0xffff88815664d800) at fs/fs-writeback.c:2131
#12 wb_do_writeback (wb=0xffff88815664d800) at fs/fs-writeback.c:2219
#13 wb_workfn (work=0xffff88815664d988) at fs/fs-writeback.c:2246
#14 0xffffffff8114bd04 in process_one_work (worker=worker@entry=0xffff88814066d000, work=0xffff88815664d988) at kernel/workqueue.c:2289
#15 0xffffffff8114bf2c in worker_thread (__worker=0xffff88814066d000) at kernel/workqueue.c:2436
#16 0xffffffff81154674 in kthread (_create=0xffff88814066e000) at kernel/kthread.c:376
#17 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

```txt
#0  blk_account_io_completion (req=0xffff888100f00300, bytes=12288) at block/blk-mq.c:799
#1  0xffffffff816ccae5 in blk_update_request (req=req@entry=0xffff888100f00300, error=error@entry=0 '\000', nr_bytes=12288) at block/blk-mq.c:914
#2  0xffffffff816cceb9 in blk_mq_end_request (rq=0xffff888100f00300, error=0 '\000') at block/blk-mq.c:1053
#3  0xffffffff81a99ca9 in virtblk_done (vq=0xffff888140cedb00) at drivers/block/virtio_blk.c:291
#4  0xffffffff818035c6 in vring_interrupt (irq=<optimized out>, _vq=0xffff888100f00300) at drivers/virtio/virtio_ring.c:2470
#5  vring_interrupt (irq=<optimized out>, _vq=0xffff888100f00300) at drivers/virtio/virtio_ring.c:2445
#6  0xffffffff811a3c72 in __handle_irq_event_percpu (desc=desc@entry=0xffff88810018d800) at kernel/irq/handle.c:158
#7  0xffffffff811a3e53 in handle_irq_event_percpu (desc=0xffff88810018d800) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff88810018d800) at kernel/irq/handle.c:210
#9  0xffffffff811a8b2e in handle_edge_irq (desc=0xffff88810018d800) at kernel/irq/chip.c:819
#10 0xffffffff810ce1d5 in generic_handle_irq_desc (desc=0xffff88810018d800) at ./include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff88810018d800) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=34) at arch/x86/kernel/irq.c:250
#13 0xffffffff82178827 in common_interrupt (regs=0xffffc900402bbd68, error_code=<optimize
```

## multiqueue
[lwn : The multiqueue block layer](https://lwn.net/Articles/552904/)
> While requests are in the submission queue, they can be operated on by the block layer in the usual manner. Reordering of requests for locality offers **little** or no benefit on solid-state devices;
> indeed, spreading requests out across the device might help with the parallel processing of requests.
> So reordering will not be done, but coalescing requests will reduce the total number of I/O operations, improving performance somewhat.
> Since the submission queues are **per-CPU**, there is no way to coalesce requests submitted to different queues.
> With no empirical evidence whatsoever, your editor would guess that adjacent requests are most likely to come from the same process and,
> thus, will automatically find their way into the same submission queue, so the lack of cross-CPU coalescing is probably not a big problem.

[Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems](https://kernel.dk/systor13-final18.pdf)
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
statistics for the states of both the software queues and dis-
patch queues. We have also modified the existing tracing
and profiling mechanisms in blktrace, to support IO tracing
for future devices that are multi-queue aware.

While the basic mechanisms for driver registration and IO submission/completion remain
unchanged, our design introduces these following requirements:
- HW dispatch queue registration: The device driver must export the number of submission queues that it supports as well as the size of these queues, so that the
block layer can allocate the matching hardware dispatch queues.
- HW submission queue mapping function: The device driver must export a function that returns a mapping
between a given software level queue (associated to core i or NUMA node i), and the appropriate hardware dispatch queue.
- IO tag handling: The device driver tag management mechanism must be revised so that it accepts tags generated by the block layer. While not strictly required,
using a single data tag will result in optimal CPU usage between the device driver and block layer.

## [x] io scheduler 和 multiqueue
- Kernel Documentaion : https://www.kernel.org/doc/html/latest/block/blk-mq.html

> blk-mq has two group of queues: software staging queues and hardware dispatch queues. When the request arrives at the block layer, it will try the shortest path possible: send it directly to the hardware queue. However, there are two cases that it might not do that: if there’s an IO scheduler attached at the layer or if we want to try to merge requests. In both cases, requests will be sent to the software queue.

> The block IO subsystem adds requests in the software staging queues (represented by struct blk_mq_ctx) in case that they weren’t sent directly to the driver. A request is one or more BIOs. They arrived at the block layer through the data structure struct bio. The block layer will then build a new structure from it, the struct request that will be used to communicate with the device driver. Each queue has its own lock and the number of queues is defined by a per-CPU or per-node basis.

```c
struct blk_mq_hw_ctx
```
原来还是可以修改 scheduler 的:
- https://linuxhint.com/change-i-o-scheduler-linux/
- https://askubuntu.com/questions/78682/how-do-i-change-to-the-noop-scheduler

检查了一下自己的机器的:
```c
➜ cat /sys/block/nvme0n1/queue/scheduler

[none] mq-deadline
```
应该只是支持 block/mq-deadline.c ，但是实际上并不会采用任何的 scheduler 的。

从函数 `blk_start_plug` 的注释说，plug 表示向 block layer 添加数据。

## multi queue
- [](https://www.thomas-krenn.com/en/wiki/Linux_Multi-Queue_Block_IO_Queueing_Mechanism_(blk-mq))

To use a device with blk-mq, the device must support the respective driver.

- [Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems](https://kernel.dk/blk-mq.pdf)

Large internal data parallelism in SSDs disks enables many
concurrent IO operations which, in turn, allows single devices to achieve close to a million IOs per second (IOPS)
for random accesses, as opposed to just hundreds on traditional magnetic hard drives.
> 1. 高速的设备需要多核维持生活吗，不是说好的 dma 之类的不需要 CPU 处理 ? 不是，由于每一个 block 的处理可能是需要处理的 ?
> 2. 那为什么需要多核啊 ? 难道 CPU 的速度已经赶不上 ssd 了

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

- [](https://lwn.net/Articles/552904/)

It offers two ways for a block driver to hook into the system, one of which is the "request" interface.

The second block driver mode — the "make request" interface — allows a driver to do exactly that. It hooks the driver into a much higher part of the stack, shorting out the request queue and handing I/O requests directly to the driver.

The multiqueue block layer work tries to fix this problem by adding a third mode for drivers to use. In this mode, the request queue is split into a number of separate queues:
  - Submission queues are set up on a per-CPU or per-node basis. Each CPU submits I/O operations into its own queue, with no interaction with the other CPUs. Contention for the submission queue lock is thus eliminated (when per-CPU queues are used) or greatly reduced (for per-node queues).
  - One or more hardware dispatch queues simply buffer I/O requests for the driver.

## core struct : software and hardware queue

```c
/**
 * struct blk_mq_hw_ctx - State for a hardware queue facing the hardware
 * block device
 */
struct blk_mq_hw_ctx ;
```

```c
/**
 * struct blk_mq_ctx - State for a software queue facing the submitting CPUs
 */
struct blk_mq_ctx {
```
