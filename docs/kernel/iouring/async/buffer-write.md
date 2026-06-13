# buffered write
## 为什么 iouring buffered write 没有对称的回调机制
<!-- e2b2aea9-f4a6-4f29-adf7-a73baad94ab4 -->

io_async_buf_func 是 read 专用的回调机制，用于处理异步缓冲区读取（buffered read）时的页缓存未命中。
对于 write，没有对称的回调机制。我认为根本原因在于，write 的写是内核中的 page wirteback 机制来做的，
想做回调也可以，但是太复杂了。

io_async_buf_func 的作用（仅用于 Read）

```c
static int io_async_buf_func(struct wait_queue_entry *wait, unsigned mode,
                             int sync, void *arg)
{
    struct wait_page_queue *wpq;
    struct io_kiocb *req = wait->private;
    struct wait_page_key *key = arg;

    wpq = container_of(wait, struct wait_page_queue, wait);

    if (!wake_page_match(wpq, key))      // 检查是否是目标 page
        return 0;

    rw->kiocb.ki_flags &= ~IOCB_WAITQ;   // 清除等待标志
    list_del_init(&wait->entry);         // 从 waitqueue 移除
    io_req_task_queue(req);              // ← 调度 task_work 重试读操作
    return 1;
}
```

触发条件（io_rw_should_retry）：

```c
static bool io_rw_should_retry(struct io_kiocb *req)
{
    // 1. 仅限 buffered IO（非 DIRECT）
    if (kiocb->ki_flags & (IOCB_DIRECT | IOCB_HIPRI))
        return false;

    // 2. 文件系统必须支持 FOP_BUFFER_RASYNC
    if (!(req->file->f_op->fop_flags & FOP_BUFFER_RASYNC))
        return false;

    // 3. 设置回调并启用 IOCB_WAITQ 标志
    wait->wait.func = io_async_buf_func;
    kiocb->ki_flags |= IOCB_WAITQ;
    kiocb->ki_waitq = wait;
}
```

要记住，buffer write 的写回在这里:
```txt
@[
        nvme_queue_rqs+5
        blk_mq_dispatch_queue_requests+359
        blk_mq_flush_plug_list+120
        blk_add_rq_to_plug+170
        blk_mq_submit_bio+1556
        __submit_bio+116
        __submit_bio_noacct+144
        iomap_ioend_writeback_submit+92
        iomap_add_to_ioend+306
        xfs_writeback_range+93
        iomap_writeback_folio+503
        iomap_writepages+87
        xfs_vm_writepages+145
        do_writepages+211
        __writeback_single_inode+65
        writeback_sb_inodes+535
        __writeback_inodes_wb+76
        wb_writeback+691
        wb_workfn+691
        process_one_work+402
        worker_thread+602
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 699
```

## io_uring 的 buffer write 的 worker 上限
<!-- 97b6c107-d933-4c5f-ab07-6933329461e1 -->

核心问题：Hash 序列化
```txt
if (req->file && (req->flags & REQ_F_ISREG)) {
    bool should_hash = def->hash_reg_file;  // ← 普通文件写需要 hash

    if (should_hash || (ctx->flags & IORING_SETUP_IOPOLL))
        io_wq_hash_work(&req->work, file_inode(req->file));  // ← 按 inode has
h
}

void io_wq_hash_work(struct io_wq_work *work, void *val)
{
    bit = hash_ptr(val, IO_WQ_HASH_ORDER);  // 同一文件的所有请求 hash 相同
    atomic_or(IO_WQ_WORK_HASHED | (bit << IO_WQ_HASH_SHIFT), &work->flags);
}

```

为什么只有 2 个 Worker

```txt
hash = __io_get_work_hash(work_flags);
/* hashed, can run if not already running */
if (!test_and_set_bit(hash, &wq->hash->map)) {  // ← 原子操作检查
    // 可以执行（第一个 worker）
} else {
    // hash 冲突，跳过（其他 worker 无法并行处理同一文件）
    stall_hash = hash;
}
```

关键限制：
• 同一 inode 的多个写请求被 hash 到 同一个 bucket
• test_and_set_bit 确保同一时间只能有一个 worker处理该文件的请求
• 其他 worker 即使空闲，看到 bit 被设置也会跳过（IO_ACCT_STALLED_BIT）

实际测试，没那么简单，即便这个测试，也不是稳定的有 iou-wrk 出现:
```txt
fio --name=test --ioengine=io_uring --direct=0 --iodepth=1024 --rw=randwrite --bs=4k --directory=$HOME/hack --nrfiles=16 --size=40G --runtime=100 --time_based

pstree -t -p  $(pgrep fio | tail -1)
```

## FOP_BUFFER_WASYNC 的影响
<!-- cada88a7-315f-4697-83e7-e6496259d6e5 -->

```txt
FOP_BUFFER_WASYNC 是文件系统向 io_uring 声明："我支持非阻塞（NOWAIT）的 buffered write"。

核心作用

// io_uring/rw.c:1150-1156
if (!(kiocb->ki_flags & IOCB_DIRECT) &&           // buffered IO
    !(req->file->f_op->fop_flags & FOP_BUFFER_WASYNC) &&  // 不支持 WASYNC
    (req->flags & REQ_F_ISREG))                   // 普通文件
    goto ret_eagain;                              // 直接进 io-wq，不尝试非
阻塞

 文件系统     支持 WASYNC   行为差异
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 XFS, Btrfs   ✓             尝试 IOCB_NOWAIT 写入，失败才进 io-wq
 ext4, f2fs   ✗             直接进 io-wq，不尝试非阻塞

为什么需要这个标志

Buffered write 通常需要：

1. 查找/分配 page cache 页
2. 等待页面锁定
3. 可能触发写回（writeback）

这些操作都可能阻塞。如果文件系统不能保证在资源不可用时快速返回 -EAGAIN，就
不应该设置 FOP_BUFFER_WASYNC。

// fs/read_write.c:1750-1753
if ((iocb->ki_flags & IOCB_NOWAIT) &&
    !((iocb->ki_flags & IOCB_DIRECT) ||
      (file->f_op->fop_flags & FOP_BUFFER_WASYNC)))
    return -EINVAL;  // 如果不支持，直接拒绝 NOWAIT 请求

实际效果对比

支持 WASYNC (XFS/Btrfs)

用户提交 buffered write
        │
        ▼
┌───────────────┐
│ 尝试 NOWAIT   │  ← 在提交线程快速尝试
│ 写入          │
└───────┬───────┘
        │
   ┌────┴────┐
   ▼         ▼
 成功      -EAGAIN
   │         │
   ▼         ▼
 完成    进 io-wq

不支持 WASYNC (ext4/f2fs)

用户提交 buffered write
        │
        ▼
┌───────────────┐
│ 直接返回      │  ← 不尝试，避免不确定的阻塞
│ -EAGAIN       │
└───────┬───────┘
        │
        ▼
     进 io-wq
   （worker 线程处理）

关键区别

 特性           支持 WASYNC                不支持 WASYNC
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 延迟           可能更低（同步路径命中）   总是 io-wq 开销
 CPU 效率       提交线程可能自旋/重试      直接交给 worker
 文件系统要求   必须正确处理 IOCB_NOWAIT   无特殊要求

总结

FOP_BUFFER_WASYNC 是文件系统的能力声明：

▌ "我可以在 buffered write 时处理 IOCB_NOWAIT 标志，如果无法立即完成，我
▌ 会返回 -EAGAIN 而不会阻塞。"

这让 io_uring 有机会在提交线程中快速完成 buffered write，避免进入 io-wq 的
上下文切换开销。对于不支持此标志的文件系统，io_uring 保守地直接交给 io-wq
处理，确保不会阻塞提交路径。
```

如果我们使用 ext4 测试，就可以发现所以的 io 都是串行的
```txt
@retval[io_write, 0]: 10391822
@retval[io_write, -11]: 10391823
```

## iouring buffered write 会产生 io-wq

对于 aio direct=0 测试结果 11k，而 io_uring direct=0 可以达到 180k 的性能。
很显然，__do_sys_io_uring_enter 只是提交，而存在另外一个线程负责等待硬件响应。

通过 syscall ，将任务放到 io workqueue 上:
```txt
   - 44.41% entry_SYSCALL_64_after_hwframe
      - 44.30% do_syscall_64
         - 23.83% __do_sys_io_uring_enter
            - 16.64% io_submit_sqes
               - 12.90% io_queue_async
                  - 12.59% io_queue_iowq
                     - 10.88% io_wq_enqueue
                        - 9.66% io_wq_activate_free_worker
                           - 9.26% try_to_wake_up
                              - 7.02% __task_rq_lock
                                 - 6.77% _raw_spin_lock
                                      6.58% native_queued_spin_lock_slowpath
                                0.65% ttwu_queue_wakelist
                                0.52% select_task_rq
                     - 1.59% io_prep_async_link
                          1.07% io_wq_hash_work
                          0.50% io_prep_async_work
               - 1.90% io_issue_sqe
                  - 0.94% io_file_get_normal
                       0.85% fget
                    0.74% io_write
               - 1.24% io_prep_rw
                    1.06% io_prep_rw_setup
            - 3.02% __io_run_local_work
               - 1.71% __io_submit_flush_completions
                  - 1.60% io_free_batch_list
                     - 0.72% io_clean_op
                          0.55% kfree
                       0.67% fput
                 0.58% llist_reverse_order
            - 1.51% io_cqring_wait
               - 0.81% schedule
```

然后让内核中的 io workqueue 来完成工作:
```txt
   - io_wq_worker
      - 36.46% io_worker_handle_work
         - 34.64% io_wq_submit_work
            - 34.45% io_issue_sqe
               - 31.33% io_write
                  - 29.08% ext4_buffered_write_iter
                     - 26.20% generic_perform_write
                        - 15.67% ext4_da_write_begin
                           - 11.04% __filemap_get_folio
                              - 6.25% filemap_add_folio
                                 - 2.62% __filemap_add_folio
                                 - 1.63% __folio_batch_add_and_move
                                 - 1.57% __mem_cgroup_charge
                              - 3.27% folio_alloc_noprof
                              - 0.94% filemap_get_entry
                                   0.77% xas_load
                           - 4.06% ext4_block_write_begin
                              - 2.36% create_empty_buffers
                                 - 1.99% folio_alloc_buffers
                              - 1.09% ext4_da_get_block_prep
                                 - 0.67% ext4_da_map_blocks.constprop.0
                        - 4.81% copy_page_from_iter_atomic
                             4.12% rep_movs_alternative
                        - 3.44% ext4_da_do_write_end
                           - 3.10% block_write_end
                              - 3.02% __block_commit_write
                          1.34% balance_dirty_pages_ratelimited_flags
                       0.81% file_modified
                    0.63% rw_verify_area
               - 2.37% __io_req_task_work_add
                  - 0.65% try_to_wake_up
                 0.52% kiocb_done
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
