## 从上到达此处

blk-core.c 核心:

```txt
#0  submit_bio (bio=0xffff88822baa1f00) at block/blk-cgroup.h:398
#1  0xffffffff813bc63d in iomap_readahead (rac=<optimized out>, ops=0xffffffff8246d090 <xfs_read_iomap_ops>) at fs/iomap/buffered-io.c:422
#2  0xffffffff8128a334 in read_pages (rac=rac@entry=0xffffc900020afad0) at mm/readahead.c:158
#3  0xffffffff8128a6b0 in page_cache_ra_unbounded (ractl=ractl@entry=0xffffc900020afad0, nr_to_read=35, lookahead_size=<optimized out>) at mm/readahead.c:263
#4  0xffffffff8128ac39 in do_page_cache_ra (lookahead_size=<optimized out>, nr_to_read=<optimized out>, ractl=0xffffc900020afad0) at mm/readahead.c:293
#5  0xffffffff8127fc77 in do_sync_mmap_readahead (vmf=0xffffc900020afb98) at mm/filemap.c:3028
#6  filemap_fault (vmf=0xffffc900020afb98) at mm/filemap.c:3120
#7  0xffffffff812bacaf in __do_fault (vmf=vmf@entry=0xffffc900020afb98) at mm/memory.c:4173
#8  0xffffffff812bf1bf in do_cow_fault (vmf=0xffffc900020afb98) at mm/memory.c:4548
#9  do_fault (vmf=vmf@entry=0xffffc900020afb98) at mm/memory.c:4649
#10 0xffffffff812c3ba0 in handle_pte_fault (vmf=0xffffc900020afb98) at mm/memory.c:4911
#11 __handle_mm_fault (vma=vma@entry=0xffff8882235f1000, address=address@entry=93979750306376, flags=flags@entry=533) at mm/memory.c:5053
#12 0xffffffff812c4440 in handle_mm_fault (vma=0xffff8882235f1000, address=address@entry=93979750306376, flags=flags@entry=533, regs=regs@entry=0xffffc900020afcf8) at mm/memory.c:5151
#13 0xffffffff810f2983 in do_user_addr_fault (regs=regs@entry=0xffffc900020afcf8, error_code=error_code@entry=2, address=address@entry=93979750306376) at arch/x86/mm/fault.c:1397
#14 0xffffffff81edffa2 in handle_page_fault (address=93979750306376, error_code=2, regs=0xffffc900020afcf8) at arch/x86/mm/fault.c:1488
#15 exc_page_fault (regs=0xffffc900020afcf8, error_code=2) at arch/x86/mm/fault.c:1544
#16 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

- blk_start_plug : 用于积累，当执行对应的 blk_finish_plug 的时候，再将 io 提交上去

## write
- submit_bio
  - submit_bio_noacct
    - submit_bio_noacct_nocheck
      - __submit_bio_noacct
        - __submit_bio : 有点看不懂，好几个循环
          - blk_mq_submit_bio : 进入到 blk-mq.c 中了
            - blk_mq_get_cached_request
            - blk_insert_flush
              - blk_flush_queue_rq
                - blk_mq_add_to_requeue_list
                  - blk_mq_kick_requeue_list

```txt
#0  virtio_queue_rq (hctx=0xffff8880056b5e00, bd=0xffffc900011d7d70) at drivers/block/virtio_blk.c:342
#1  0xffffffff816ce470 in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff8880056b5e00, list=list@entry=0xffffc900011d7dc0, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:2056
#2  0xffffffff816d4d46 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880056b5e00) at block/blk-mq-sched.c:306
#3  0xffffffff816d4e54 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880056b5e00) at block/blk-mq-sched.c:339
#4  0xffffffff816ca8ec in __blk_mq_run_hw_queue (hctx=0xffff8880056b5e00) at block/blk-mq.c:2174
#5  0xffffffff816cad70 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2250
#6  0xffffffff816cb018 in blk_mq_run_hw_queue (hctx=<optimized out>, async=async@entry=false) at block/blk-mq.c:2298
#7  0xffffffff816cb2b4 in blk_mq_run_hw_queues (q=q@entry=0xffff888005748368, async=async@entry=false) at block/blk-mq.c:2346
#8  0xffffffff816cc20f in blk_mq_requeue_work (work=0xffff888005748578) at block/blk-mq.c:1481
#9  0xffffffff8114bd04 in process_one_work (worker=worker@entry=0xffff888004b20000, work=0xffff888005748578) at kernel/workqueue.c:2289
#10 0xffffffff8114bf2c in worker_thread (__worker=0xffff888004b20000) at kernel/workqueue.c:2436
#11 0xffffffff81154674 in kthread (_create=0xffff8881007f38c0) at kernel/kthread.c:376
#12 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#13 0x0000000000000000 in ?? ()
```

kick queue 的来源:
```txt
#0  0xffffffff816cd803 in blk_mq_kick_requeue_list (q=0xffff888005748368) at block/blk-mq.c:1511
#1  blk_mq_add_to_requeue_list (rq=<optimized out>, at_head=at_head@entry=true, kick_requeue_list=kick_requeue_list@entry=true) at block/blk-mq.c:1506
#2  0xffffffff816c2fe7 in blk_flush_queue_rq (add_front=true, rq=<optimized out>) at block/blk-flush.c:143
#3  blk_flush_complete_seq (rq=<optimized out>, fq=fq@entry=0xffff8880055f5240, seq=<optimized out>, error=error@entry=0 '\000') at block/blk-flush.c:198
#4  0xffffffff816c34ae in flush_end_io (flush_rq=<optimized out>, error=0 '\000') at block/blk-flush.c:268
#5  0xffffffff816cbaa6 in __blk_mq_end_request (rq=0xffff8880056b6000, error=<optimized out>) at block/blk-mq.c:1043
#6  0xffffffff81a99ca9 in virtblk_done (vq=0xffff88810067d000) at drivers/block/virtio_blk.c:291
#7  0xffffffff818035c6 in vring_interrupt (irq=<optimized out>, _vq=0xffffffff) at drivers/virtio/virtio_ring.c:2470
#8  vring_interrupt (irq=<optimized out>, _vq=0xffffffff) at drivers/virtio/virtio_ring.c:2445
#9  0xffffffff811a3c72 in __handle_irq_event_percpu (desc=desc@entry=0xffff8880056b4400) at kernel/irq/handle.c:158
#10 0xffffffff811a3e53 in handle_irq_event_percpu (desc=0xffff8880056b4400) at kernel/irq/handle.c:193
#11 handle_irq_event (desc=desc@entry=0xffff8880056b4400) at kernel/irq/handle.c:210
#12 0xffffffff811a8b2e in handle_edge_irq (desc=0xffff8880056b4400) at kernel/irq/chip.c:819
#13 0xffffffff810ce1d5 in generic_handle_irq_desc (desc=0xffff8880056b4400) at ./include/linux/irqdesc.h:158
#14 handle_irq (regs=<optimized out>, desc=0xffff8880056b4400) at arch/x86/kernel/irq.c:231
#15 __common_interrupt (regs=<optimized out>, vector=35) at arch/x86/kernel/irq.c:250
#16 0xffffffff82178827 in common_interrupt (regs=0xffffffff82c03df8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

## 关键参考
- [IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)

# blk-core.c
request_queue

## TODO
1. 希望搞清楚 bio request 和 request_queue 的关系

## 对于 queue 的各种操作
1. 从 280 行之后的一段位置，前面的内容并不知道是干什么的

```c
/**
 * blk_sync_queue - cancel any pending callbacks on a queue
 * @q: the queue
 *
 * Description:
 *     The block layer may perform asynchronous callback activity
 *     on a queue, such as calling the unplug function after a timeout.
 *     A block device may call blk_sync_queue to ensure that any
 *     such activity is cancelled, thus allowing it to release resources
 *     that the callbacks might use. The caller must already have made sure
 *     that its ->make_request_fn will not re-add plugging prior to calling
 *     this function.
 *
 *     This function does not cancel any asynchronous activity arising
 *     out of elevator or throttling code. That would require elevator_exit()
 *     and blkcg_exit_queue() to be called with queue lock initialized.
 *
 */
void blk_sync_queue(struct request_queue *q)
{
	del_timer_sync(&q->timeout);
	cancel_work_sync(&q->timeout_work);
}
```

```c
bool blk_get_queue(struct request_queue *q)
{
	if (likely(!blk_queue_dying(q))) {
		__blk_get_queue(q);
		return true;
	}

	return false;
}

void blk_put_queue(struct request_queue *q)
{
	kobject_put(&q->kobj);
}
```
1. 我是万万没有想到 : 居然我们可以使用 kobject 管理 request_queue
2. 这些调用位置也是异常的神奇。


```c
/**
 * blk_cleanup_queue - shutdown a request queue
 * @q: request queue to shutdown
 *
 * Mark @q DYING, drain all pending requests, mark @q DEAD, destroy and
 * put it.  All future requests will be failed immediately with -ENODEV.
 */
void blk_cleanup_queue(struct request_queue *q)
```

```c
struct request_queue *blk_alloc_queue(gfp_t gfp_mask)  // 对于简单的封装 driver md 是唯一调用者
struct request_queue *blk_mq_init_queue(struct blk_mq_tag_set *set) // mq 是其唯一调用者
      /**
       * blk_alloc_queue_node - allocate a request queue
       * @gfp_mask: memory allocation flags
       * @node_id: NUMA node to allocate memory from
       */
      struct request_queue *blk_alloc_queue_node(gfp_t gfp_mask, int node_id) // 唯一创建 request_queue 的函数
```

## merge
1. 各种 try merge 函数，其使用位置都是各种 mq 了


## submit_bio
1. 这是 fs 和 dev 进行 io 的唯一的接口吗 ?
2. submit_bio 和 generic_make_request 的关系是什么 ? 两者都是类似接口，但是 submit_bio 做出部分检查，最后调用 generic_make_request

iomap 直接来调用 submit_bio，有趣:

## account

## 代码浏览
- block/partitions : 处理各种分区
- `*-iosched.c` : 集中 scheduler
- `blk-mq.c` : multiqueue

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
| elevator.c             | 156   | 127     | 560  | 管理各种 io scheduler ，似乎由于历史原因，io scheduler 被叫做 elevator_type |
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

## question
1. 为什么需要 io scheduler ?
    1. bfq 之外的还有什么类型的
    2. bfq 是一个模块啊，如何动态注册 ?
2.  那些是 policy ，那些具体任务的执行，那些是用于 debug 的东西 ?
3. part one 和 part two

```c
	elv_unregister(&iosched_bfq_mq);
	blkcg_policy_unregister(&blkcg_policy_bfq);
```
elv_unregister 和 blkcg_policy_unregister : elv 的才是真正的管理吧  blkcg 显然又是处理 cgroup 的!
