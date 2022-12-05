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
