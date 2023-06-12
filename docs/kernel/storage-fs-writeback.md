## [Flushing out pdflush](https://lwn.net/Articles/326552/)
The amount of dirty memory is listed in `/proc/meminfo`.

Jens Axboe in [his patch](http://lwn.net/Articles/324833/) set proposes a new idea of using flusher threads per **backing device info (BDI)**, as a replacement for pdflush threads. Unlike pdflush threads, per-BDI flusher threads focus on a single disk spindle. With per-BDI flushing, when the request_queue is congested, blocking happens on request allocation, avoiding request starvation and providing better fairness.
> BDI 相比 pdflush 到底有什么好处

As with pdflush, per-BDI writeback is controlled through the `writeback_control` data structure, which instructs the writeback code what to do, and how to perform the writeback. The important fields of this structure are:
1. `sync_mode`: defines the way synchronization should be performed with respect to inode locking. If set to `WB_SYNC_NONE`, the writeback will skip locked inodes, where as if set to WB_SYNC_ALL will wait for locked inodes to be unlocked to perform the writeback.

2. `nr_to_write`: the number of pages to write. This value is decremented as the pages are written.

3. `older_than_this`: If not NULL, all inodes older than the jiffies recorded in this field are flushed. This field takes precedence over `nr_to_write`.

```c
/*
 * A control structure which tells the writeback code what to do.  These are
 * always on the stack, and hence need no locking.  They are always initialised
 * in a manner such that unspecified fields are set to zero.
 */
struct writeback_control {
    long nr_to_write;       /* Write this many pages, and decrement
                       this for each page written */
    long pages_skipped;     /* Pages which were not written */

    /*
     * For a_ops->writepages(): if start or end are non-zero then this is
     * a hint that the filesystem need only write out the pages inside that
     * byterange.  The byte at `end' is included in the writeout request.
     */
    loff_t range_start;
    loff_t range_end;

    enum writeback_sync_modes sync_mode;

    unsigned for_kupdate:1;     /* A kupdate writeback */
    unsigned for_background:1;  /* A background writeback */
    unsigned tagged_writepages:1;   /* tag-and-write to avoid livelock */
    unsigned for_reclaim:1;     /* Invoked from the page allocator */
    unsigned range_cyclic:1;    /* range_start is cyclic */
    unsigned for_sync:1;        /* sync(2) WB_SYNC_ALL writeback */
#ifdef CONFIG_CGROUP_WRITEBACK
    struct bdi_writeback *wb;   /* wb this writeback is issued under */
    struct inode *inode;        /* inode being written out */

    /* foreign inode detection, see wbc_detach_inode() */
    int wb_id;          /* current wb id */
    int wb_lcand_id;        /* last foreign candidate wb id */
    int wb_tcand_id;        /* this foreign candidate wb id */
    size_t wb_bytes;        /* bytes written by current wb */
    size_t wb_lcand_bytes;      /* bytes written by last candidate */
    size_t wb_tcand_bytes;      /* bytes written by this candidate */
#endif
};
```

The struct `bdi_writeback` keeps all information required for flushing the dirty pages:

```c
struct bdi_writeback {
    struct backing_dev_info *bdi;
    unsigned int nr;
    struct task_struct  *task;
    wait_queue_head_t   wait;
    struct list_head    b_dirty;
    struct list_head    b_io;
    struct list_head    b_more_io;

    unsigned long       nr_pages;
    struct super_block  *sb;
};
```
The `bdi_writeback` structure is initialized when the device is registered through `bdi_register()`. The fields of the `bdi_writeback` are:


1. `bdi`: the `backing_device_info` associated with this `bdi_writeback`,
2. `task`: contains the pointer to the default flusher thread which is responsible for spawning threads for performing the flushing work,
3. `wait`: a wait queue for synchronizing with the flusher threads,
4. `b_dirty`: list of all the dirty inodes on this BDI to be flushed,
5. `b_io`: inodes parked for I/O,
6. `b_more_io`: more inodes parked for I/O; all inodes queued for flushing are inserted in this list, before being moved to b_io,
7. `nr_pages`: total number of pages to be flushed, and
8. `sb`: the pointer to the superblock of the filesystem which resides on this BDI.

The `bdi_writeback_task()` function waits for the `dirty_writeback_interval`, which by default is 5 seconds, and initiates `wb_do_writeback(wb)` periodically. If there are no pages written for five minutes, the flusher thread exits (with a grace period of `dirty_writeback_interval`). If a writeback work is later required (after exit), new flusher threads are spawned by the default writeback thread.

## https://medium.com/coccoc-engineering-blog/our-lessons-on-linux-writeback-do-dirty-jobs-the-right-way-fa1863a5a3dc
- https://github.com/iovisor/bpftrace/blob/master/tools/writeback_example.txt
    - [ ] 这个跟踪的到底是谁?

# fs-writeback.md
1. 这里并不是处理 inode sb 之类的写会，此处和 page-writeback.c 配合使用，共同完成 writeback 机制。


## TODO
1. blk_plug 机制
2. 如何找到所有的 dirty pages
3. 如何确定时间的 ?
4. 和 page-writeback.c 的沟通方式是什么 ?
  - fs-writeback 最后调用到 page-writeback.c 中间

## core struct

```c
struct backing_dev_info {

/*
 * Each wb (bdi_writeback) can perform writeback operations, is measured
 * and throttled, independently.  Without cgroup writeback, each bdi
 * (bdi_writeback) is served by its embedded bdi->wb.
 *
 * On the default hierarchy, blkcg implicitly enables memcg.  This allows
 * using memcg's page ownership for attributing writeback IOs, and every
 * memcg - blkcg combination can be served by its own wb by assigning a
 * dedicated wb to each memcg, which enables isolation across different
 * cgroups and propagation of IO back pressure down from the IO layer upto
 * the tasks which are generating the dirty pages to be written back.
 *
 * A cgroup wb is indexed on its bdi by the ID of the associated memcg,
 * refcounted with the number of inodes attached to it, and pins the memcg
 * and the corresponding blkcg.  As the corresponding blkcg for a memcg may
 * change as blkcg is disabled and enabled higher up in the hierarchy, a wb
 * is tested for blkcg after lookup and removed from index on mismatch so
 * that a new wb for the combination can be created.
 */
struct bdi_writeback {

/*
 * Passed into wb_writeback(), essentially a subset of writeback_control
 */
struct wb_writeback_work {
```

- [ ] 什么叫做 each wb ? 为什么存在多个 writeback ?


## wakeup_flusher_threads
1. vmscan.c:shrink_inactive_list 调用此处
2. 其内容和 wakeup_dirtytime_writeback 几乎没有区别
    1. 但是增加很多检查
    2. 对于三级关系不是很了解 : backing_dev_info, bdi_writeback, 和 work_struct


```txt
#0  wb_start_writeback (wb=wb@entry=0xffff8881040ef860, reason=reason@entry=WB_REASON_SYNC) at fs/fs-writeback.c:1193
#1  0xffffffff81406f22 in __wakeup_flusher_threads_bdi (reason=WB_REASON_SYNC, bdi=0xffff8881040ef800) at fs/fs-writeback.c:2279
#2  __wakeup_flusher_threads_bdi (reason=WB_REASON_SYNC, bdi=0xffff8881040ef800) at fs/fs-writeback.c:2270
#3  wakeup_flusher_threads (reason=reason@entry=WB_REASON_SYNC) at fs/fs-writeback.c:2304
#4  0xffffffff8140b0b1 in ksys_sync () at fs/sync.c:101
#5  0xffffffff8140b139 in __do_sys_sync (__unused=<optimized out>) at fs/sync.c:113
#6  0xffffffff82180c3f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9003fcdbf58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc9003fcdbf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

自动触发的基本的路径：
```txt
#0  wb_wakeup (wb=wb@entry=0xffff88811ce51000) at ./include/linux/spinlock.h:375
#1  0xffffffff814066e3 in wb_start_background_writeback (wb=wb@entry=0xffff88811ce51000) at fs/fs-writeback.c:1229
#2  0xffffffff812ebcb3 in balance_dirty_pages (wb=wb@entry=0xffff88811ce51000, pages_dirtied=512, flags=flags@entry=0) at mm/page-writeback.c:1754
#3  0xffffffff812ec2ed in balance_dirty_pages_ratelimited_flags (mapping=mapping@entry=0xffff888103d80508, flags=flags@entry=0) at ./arch/x86/include/asm/current.h:41
#4  0xffffffff812ec40b in balance_dirty_pages_ratelimited (mapping=mapping@entry=0xffff888103d80508) at mm/page-writeback.c:2064
#5  0xffffffff812df0c9 in generic_perform_write (iocb=<optimized out>, i=0xffffc9003fda3cb0) at mm/filemap.c:3806
#6  0xffffffff812e417f in __generic_file_write_iter (iocb=iocb@entry=0xffff888145066600, from=from@entry=0xffffc9003fda3cb0) at mm/filemap.c:3900
#7  0xffffffff816b9483 in blkdev_write_iter (iocb=0xffff888145066600, from=0xffffc9003fda3cb0) at block/fops.c:534
#8  0xffffffff817113ca in call_write_iter (iter=0xffffc9003fda3cb0, kio=0xffff888145066600, file=<optimized out>) at ./include/linux/fs.h:2186
#9  io_write (req=0xffff888145066600, issue_flags=10) at io_uring/rw.c:918
#10 0xffffffff81701d24 in io_issue_sqe (req=req@entry=0xffff888145066600, issue_flags=issue_flags@entry=10) at io_uring/io_uring.c:1836
#11 0xffffffff81702355 in io_wq_submit_work (work=0xffff8881450666c8) at io_uring/io_uring.c:1912
#12 0xffffffff81713089 in io_worker_handle_work (worker=worker@entry=0xffff8881034f7840) at io_uring/io-wq.c:587
#13 0xffffffff81713750 in io_wqe_worker (data=0xffff8881034f7840) at io_uring/io-wq.c:632
#14 0xffffffff8100265c in ret_from_fork () at arch/x86/entry/entry_64.S:308
```


```c
/*
 * Handle writeback of dirty data for the device backed by this bdi. Also
 * reschedules periodically and does kupdated style flushing.
 */
void wb_workfn(struct work_struct *work)
```

```txt
#0  wb_workfn (work=0xffff888109c4d988) at fs/fs-writeback.c:2230
#1  0xffffffff8114cd57 in process_one_work (worker=worker@entry=0xffff8881487da3c0, work=0xffff888109c4d988) at kernel/workqueue.c:2289
#2  0xffffffff8114cf7c in worker_thread (__worker=0xffff8881487da3c0) at kernel/workqueue.c:2436
#3  0xffffffff811556c7 in kthread (_create=0xffff8881037f3640) at kernel/kthread.c:376
#4  0xffffffff8100265c in ret_from_fork () at arch/x86/entry/entry_64.S:308
```
- wb_workfn
  - wb_do_writeback : 将 bdi_writeback::work_list 中的 work
    - wb_writeback
      - `__writeback_inodes_wb`
          -  writeback_sb_inodes

- wb_queue_work

## dirtytime_work : 周期 writeback 工作
```c
/*
 * Wake up bdi's periodically to make sure dirtytime inodes gets
 * written back periodically.  We deliberately do *not* check the
 * b_dirtytime list in wb_has_dirty_io(), since this would cause the
 * kernel to be constantly waking up once there are any dirtytime
 * inodes on the system.  So instead we define a separate delayed work
 * function which gets called much more rarely.  (By default, only
 * once every 12 hours.)
 *
 * If there is any other write activity going on in the file system,
 * this function won't be necessary.  But if the only thing that has
 * happened on the file system is a dirtytime inode caused by an atime
 * update, we need this infrastructure below to make sure that inode
 * eventually gets pushed out to disk.
 */
static void wakeup_dirtytime_writeback(struct work_struct *w);
static DECLARE_DELAYED_WORK(dirtytime_work, wakeup_dirtytime_writeback);
```

## 验证这个回答的正确性
https://stackoverflow.com/questions/27900221/difference-between-vm-dirty-ratio-and-vm-dirty-background-ratio
