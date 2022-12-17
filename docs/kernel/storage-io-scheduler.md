# io scheduer

应该可以在磁盘中观测到很明显的 io scheduler 的吧！

## io scheduelr
blkcg_policy_register 的调用位置:

1. block/blk-iolatency.c
3. block/blk-throttle.c
4. block/blk-iocost.c (实际上，这一个没有被注册上去)

2. block/bfq-iosched.c (按道理来说，不应该，可能自己特有的)

很尴尬，从 Kconfig 上分析一共只有三个 io scheduler , 所以说明不是所有的调度器都需要这个。


## 检查一下当前的 io scheduler
🧀  cat /sys/block/sda/queue/scheduler
[mq-deadline] kyber none

🧀  cat /sys/block/nvme0n1/queue/scheduler
[none] mq-deadline kyber


对于 IO scheduler 我早就非常的不爽了，和当前的 SSD 的原理完全不符合，所以对于 ssd 之类 driver 是如何绕过这一个东西, 从 bio 层次开始分析。

## 当 io scheduler 是 none 的时候，过程是如何的


## io scheduler 和 multiqueue 的层次关系是什么?

#### (block) io scheduler

提供给 io scheduler 的 interface 是什么？
1. request queue ?
