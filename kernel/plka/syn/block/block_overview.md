| name                   | blank | comment | code | desc                                                                       |
|------------------------|-------|---------|------|----------------------------------------------------------------------------|
| bfq-iosched.c          | 679   | 3326    | 2852 | bfq io scheduler                                                           |
| blk-mq.c               | 575   | 589     | 2368 | 几乎所有的函数都有 request_queue, mq 指的是 multiple queue                 |
| sed-opal.c             | 501   | 104     | 2090 | opal ?                                                                     |
| blk-throttle.c         | 392   | 407     | 1742 | wawawa                                                                     |
| blk-iocost.c           | 362   | 478     | 1632 | wawawa                                                                     |
| genhd.c                | 317   | 413     | 1354 |                                                                            |
| bio.c                  | 332   | 596     | 1300 | 干                                                                         |
| blk-core.c             | 262   | 487     | 1068 |                                                                            |
| blk-cgroup.c           | 252   | 401     | 1067 |                                                                            |
| bfq-cgroup.c           | 207   | 248     | 960  |                                                                            |
| partitions/ldm.c       | 162   | 411     | 925  |                                                                            |
| blk-sysfs.c            | 197   | 68      | 795  |                                                                            |
| blk-mq-debugfs.c       | 171   | 28      | 791  |                                                                            |
| bfq-wf2q.c             | 213   | 746     | 753  |                                                                            |
| kyber-iosched.c        | 144   | 160     | 746  |                                                                            |
| blk-iolatency.c        | 151   | 162     | 739  | wawawa                                                                     |
| scsi_ioctl.c           | 116   | 82      | 646  | https://en.wikipedia.org/wiki/SCSI                                         |
| blk-wbt.c              | 131   | 163     | 566  |                                                                            |
| elevator.c             | 156   | 127     | 560  | 管理各种 io scheduler ，似乎由于历史原因，io scheduler 被叫做elevator_type |
| blk-merge.c            | 144   | 221     | 550  | @todo 猜测各种调用 `elevator_type->ops` 实现将 request merge               |
| mq-deadline.c          | 124   | 148     | 545  |                                                                            |
| ioctl.c                | 69    | 36      | 480  |                                                                            |
| partition-generic.c    | 90    | 65      | 468  |                                                                            |
| partitions/efi.c       | 62    | 253     | 415  |                                                                            |
| bfq-iosched.h          | 142   | 549     | 412  |                                                                            |
| blk-settings.c         | 90    | 380     | 410  | 神奇的一个文件                                                             |
| partitions/msdos.c     | 65    | 112     | 410  |                                                                            |
| blk-mq-sched.c         | 88    | 85      | 408  |                                                                            |
| bsg.c                  | 99    | 22      | 398  |                                                                            |
| badblocks.c            | 60    | 176     | 367  |                                                                            |
| partitions/acorn.c     | 90    | 98      | 363  |                                                                            |
| compat_ioctl.c         | 38    | 34      | 355  |                                                                            |
| blk-mq-tag.c           | 76    | 156     | 321  |                                                                            |
| blk-integrity.c        | 76    | 54      | 313  |                                                                            |
| blk-zoned.c            | 66    | 102     | 312  |                                                                            |
| blk-mq-sysfs.c         | 79    | 2       | 303  |                                                                            |
| bsg-lib.c              | 64    | 56      | 292  |                                                                            |
| bio-integrity.c        | 72    | 107     | 288  |                                                                            |
| partitions/ibm.c       | 25    | 62      | 278  |                                                                            |
| blk-lib.c              | 54    | 91      | 262  |                                                                            |
| blk-flush.c            | 74    | 193     | 262  |                                                                            |
| blk.h                  | 48    | 48      | 261  |                                                                            |
| bounce.c               | 64    | 70      | 254  |                                                                            |
| opal_proto.h           | 34    | 177     | 253  |                                                                            |
| blk-ioc.c              | 63    | 119     | 233  |                                                                            |
| partitions/aix.c       | 29    | 44      | 226  |                                                                            |
| blk-rq-qos.c           | 36    | 62      | 206  |                                                                            |
| ioprio.c               | 25    | 24      | 203  |                                                                            |
| t10-pi.c               | 47    | 35      | 200  |                                                                            |
| cmdline-parser.c       | 50    | 12      | 193  |                                                                            |
| blk-stat.c             | 41    | 7       | 168  |                                                                            |
| blk-rq-qos.h           | 34    | 5       | 168  |                                                                            |
| blk-map.c              | 39    | 56      | 165  |                                                                            |
| blk-mq.h               | 41    | 65      | 153  |                                                                            |
| partitions/check.c     | 20    | 29      | 149  |                                                                            |
| partitions/ldm.h       | 32    | 21      | 146  |                                                                            |
| partitions/mac.c       | 16    | 16      | 111  |                                                                            |
| partitions/atari.c     | 16    | 30      | 111  |                                                                            |
| partitions/amiga.c     | 16    | 16      | 110  |                                                                            |
| partitions/cmdline.c   | 30    | 23      | 104  |                                                                            |
| blk-softirq.c          | 25    | 32      | 99   |                                                                            |
| partitions/sun.c       | 8     | 18      | 97   |                                                                            |
| blk-wbt.h              | 28    | 12      | 97   |                                                                            |
| blk-pm.c               | 18    | 109     | 91   |                                                                            |
| partitions/efi.h       | 15    | 15      | 89   |                                                                            |
| blk-cgroup-rwstat.h    | 22    | 43      | 84   |                                                                            |
| blk-timeout.c          | 28    | 42      | 81   |                                                                            |
| blk-mq-debugfs.h       | 21    | 2       | 80   |                                                                            |
| blk-cgroup-rwstat.c    | 16    | 35      | 78   |                                                                            |
| partitions/osf.c       | 5     | 9       | 73   |                                                                            |
| partitions/sgi.c       | 4     | 13      | 66   |                                                                            |
| blk-mq-sched.h         | 21    | 1       | 66   |                                                                            |
| partitions/sysv68.c    | 15    | 18      | 63   |                                                                            |
| blk-mq-tag.h           | 18    | 10      | 61   |                                                                            |
| blk-mq-cpumap.c        | 13    | 24      | 59   |                                                                            |
| blk-stat.h             | 23    | 95      | 53   |                                                                            |
| blk-pm.h               | 15    | 1       | 53   |                                                                            |
| partitions/karma.c     | 7     | 8       | 44   |                                                                            |
| partitions/check.h     | 8     | 5       | 42   |                                                                            |
| blk-exec.c             | 11    | 44      | 40   |                                                                            |
| partitions/ultrix.c    | 6     | 8       | 35   |                                                                            |
| Makefile               | 3     | 4       | 32   |                                                                            |
| partitions/mac.h       | 7     | 8       | 30   |                                                                            |
| blk-mq-pci.c           | 6     | 16      | 26   |                                                                            |
| blk-mq-virtio.c        | 5     | 16      | 25   |                                                                            |
| partitions/atari.h     | 4     | 13      | 20   |                                                                            |
| blk-mq-rdma.c          | 5     | 19      | 20   |                                                                            |
| partitions/Makefile    | 2     | 4       | 17   |                                                                            |
| blk-mq-debugfs-zoned.c | 5     | 4       | 13   |                                                                            |
| partitions/acorn.h     | 1     | 9       | 5    |                                                                            |
| partitions/sun.h       | 2     | 4       | 3    |                                                                            |
| partitions/osf.h       | 2     | 4       | 2    |                                                                            |
| partitions/sgi.h       | 3     | 4       | 2    |                                                                            |
| partitions/msdos.h     | 3     | 4       | 2    |                                                                            |
| partitions/karma.h     | 3     | 4       | 2    |                                                                            |
| partitions/ibm.h       | 0     | 1       | 1    |                                                                            |
| partitions/ultrix.h    | 1     | 4       | 1    |                                                                            |
| partitions/aix.h       | 0     | 1       | 1    |                                                                            |
| partitions/amiga.h     | 2     | 4       | 1    |                                                                            |
| partitions/sysv68.h    | 0     | 1       | 1    |                                                                            |
| partitions/cmdline.h   | 1     | 1       | 1    |                                                                            |

## TODO
1. ldk
2. plka
3. Documentaion
5. 注意: block layer 是可以被 disable 掉的，当其被 disable 掉之后，那么
6. 统计模块在什么地方 ?
7. io control 的位置似乎所有的地方都没有分析过 !
8. blk-flush 和 blk-core 的作用是什么 ?
8. submit_bio 开始分析

对于 IO scheduler 我早就非常的不爽了，和当前的 SSD 的原理完全不符合，所以对于 ssd 之类 driver 是如何绕过这一个东西, 从 bio 层次开始分析。

## question
1. 为什么需要 io scheduler ?
    1. bfq 之外的还有什么类型的
    2. bfq 是一个模块啊，如何动态注册 ?
2.  那些是 policy ，那些具体任务的执行，那些是用于 debug 的东西 ?
3. part one 和 part two
4. 谁知道 gendisk 以及 blk_dev 和这些关系是什么 ?

```c
	elv_unregister(&iosched_bfq_mq);
	blkcg_policy_unregister(&blkcg_policy_bfq);
```
elv_unregister 和 blkcg_policy_unregister : elv 的才是真正的管理吧  blkcg 显然又是处理 cgroup 的!

## cgroup 注入影响是什么 ?

## io scheduelr
blkcg_policy_register 的调用位置:

1. block/blk-iolatency.c
3. block/blk-throttle.c
4. block/blk-iocost.c (实际上，这一个没有被注册上去)

2. block/bfq-iosched.c (按道理来说，不应该，可能自己特有的)

很尴尬，从 Kconfig 上分析一共只有三个 io scheduler , 所以说明不是所有的调度器都需要这个。

## bio layer
- [A block layer introduction part 1: the bio layer](https://lwn.net/Articles/736534/) https://yq.aliyun.com/articles/609907

- [Block layer introduction part 2: the request layer](https://lwn.net/Articles/738449/)


http://byteliu.com/2019/05/10/Linux-The-block-I-O-layer/

http://byteliu.com/2019/05/21/What-is-the-major-difference-between-the-buffer-cache-and-the-page-cache-Why-were-they-separate-entities-in-older-kernels-Why-were-they-merged-later-on/

1. bio 给上下两个层次提供的接口是什么 ?
    1. FA



## all kinds of doc
1. https://zhuanlan.zhihu.com/p/39199521
    1. bio 机制核心 : 合并请求
2.



## multi queue
- [](https://www.thomas-krenn.com/en/wiki/Linux_Multi-Queue_Block_IO_Queueing_Mechanism_(blk-mq))

To use a device with blk-mq, the device must support the respective driver.

- [](https://kernel.dk/blk-mq.pdf)

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
