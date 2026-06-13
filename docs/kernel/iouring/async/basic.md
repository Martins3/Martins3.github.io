# 基础
## qemu 参数 ,discard=on,aio=native,cache.direct=on
<!-- 47c0cb04-e42f-4a32-bbbc-7d2abc029682 -->

原来，cache.direct=on 的含义是由于 aio 无法处理 buffer io ？

qemu-system-x86_64: -drive file=/home/martins3/hack/vm/2403-nix/img/virtio_blk_1,format=qcow2,if=none,id=virtio-blk1,aio=native:
aio=native was specified, but it requires cache.direct=on, which was not specified.

```c
     /* Currently Linux does AIO only for files opened with O_DIRECT */
    if (s->use_linux_aio && !(s->open_flags & O_DIRECT)) {
        error_setg(errp, "aio=native was specified, but it requires "
                         "cache.direct=on, which was not specified.");
        ret = -EINVAL;
        goto fail;
    }
    if (s->use_linux_aio) {
        s->has_laio_fdsync = laio_has_fdsync(s->fd);
    }
```
也就是 iouring 可以读写 buffer ，但是 aio 必须使用 O_DIRECT 的模式

## 进行 io 的时候，页面是什么时候 pin 住的
<!-- 234162cf-0620-4162-ba9b-f82d9d0be116 -->

如果是 direct io ，那么需要在提交的时候，将**用户态**的页面 pin 住
```txt
@[
        bio_iov_iter_get_pages+5
        iomap_dio_bio_iter+587
        __iomap_dio_rw+937
        iomap_dio_rw+18
        xfs_file_dio_read+185
        xfs_file_read_iter+265
        __io_read+175
        io_read+63
        __io_issue_sqe+59
        io_issue_sqe+55
        io_submit_sqes+274
        __do_sys_io_uring_enter+516
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1200961
```

```txt
@[
        bio_iov_iter_get_pages+5
        iomap_dio_bio_iter+587
        __iomap_dio_rw+937
        iomap_dio_rw+18
        xfs_file_dio_read+185
        xfs_file_read_iter+265
        aio_read+306
        io_submit_one+239
        __x64_sys_io_submit+148
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 659511
```

如果是 buffer io read ，需要在提交的时候 pin 页面用于 dma

```txt
@[
        bio_iov_iter_get_pages+5
        iomap_dio_bio_iter+485
        __iomap_dio_rw+937
        iomap_dio_rw+18
        xfs_file_dio_read+185
        xfs_file_read_iter+265
        __io_read+175
        io_read+63
        __io_issue_sqe+59
        io_issue_sqe+55
        io_submit_sqes+274
        __do_sys_io_uring_enter+456
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 55964
```
在完成的时候，来拷贝页面，而且需要注意到，如果提交的时候，内存不足，那么是在提交的过程中
来分配，不会由于没有内存，就返回 EAGAIN 的
```txt
6.69% __do_sys_io_uring_enter
- 69.55% io_submit_sqes
   - 68.71% io_issue_sqe
      - 68.16% __io_issue_sqe
         - io_read
            - __io_read
               - 67.42% 0xffffffffc10b411f
                  - 66.17% 0xffffffffc10b3444
                     - 66.13% filemap_read
                        - 65.07% filemap_get_pages
                           - 59.00% force_page_cache_ra
                              - 58.89% page_cache_ra_unbounded
                                 - 33.80% filemap_add_folio
                                    - 24.68% __mem_cgroup_charge
                                       - 23.76% charge_memcg
                                          + 23.22% try_charge_memcg
                                         0.78% get_mem_cgroup_from_mm
                                    + 7.80% __filemap_add_folio
                                    + 1.15% __folio_batch_add_and_move
                                 - 20.45% read_pages
                                    + 18.42% iomap_readahead
                                    - 1.85% blk_finish_plug
                                       - __blk_flush_plug
                                          - blk_mq_flush_plug_list
                                             - blk_mq_dispatch_queue_requests
                                                  0.81% 0xffffffffc04659c0
                                   2.31% up_read
                                 + 1.73% folio_alloc_noprof
                           - 3.28% filemap_get_read_batch
                                2.58% xas_load
                             1.88% filemap_update_page
                        - 0.60% touch_atime
                             atime_needs_update
                    0.57% 0xffffffffc10b3454
- 14.42% __io_run_local_work
   - 13.70% __io_run_local_work_loop
      - 13.65% io_req_task_submit
         - io_issue_sqe
            - 13.46% __io_issue_sqe
               - 13.41% io_read
                  - 12.91% __io_read
                     - 12.36% 0xffffffffc10b411f
                        - 11.90% 0xffffffffc10b3444
                           - 11.86% filemap_read
                              - 6.82% copy_page_to_iter
                                   _copy_to_iter
                              - 3.36% filemap_get_pages
                                 - filemap_get_read_batch
                                      2.66% xas_load
```

buffer write 的 perf 结果，可以看到大多数工作都是在 io_wq_worker 中完成的，
而且工作主要是拷贝内存，提交任务在另外系统中:
```txt
-   45.84%     0.00%  iou-wrk-128624  [unknown]          [k] 0x0000000004365b10
     0x4365b10
     0
     ret_from_fork_asm
     ret_from_fork
     io_wq_worker
   - io_worker_handle_work
      - 45.02% io_wq_submit_work
         - io_issue_sqe
            - 39.81% __io_issue_sqe
               - 39.56% io_write
                  - 38.71% ext4_buffered_write_iter
                     - 37.56% generic_perform_write
                        + 27.76% ext4_da_write_begin
                          4.98% copy_folio_from_iter_atomic
                        + 4.14% ext4_da_do_write_end.isra.0
            - 4.94% __io_req_task_work_add
               + 4.43% try_to_wake_up
```

## io_cqring_wait
<!-- 380e053a-3fe2-45db-a4c1-38afa514dd54 -->

当 iouring 等待动作完成的时候，就是等待在这里，所以经常可以看到 process 卡到这里:
```txt
[<0>] io_cqring_wait+0x674/0x6a0 # 具体在 __io_cqring_wait_schedule
[<0>] __do_sys_io_uring_enter+0x141/0x410
[<0>] do_syscall_64+0x84/0x250
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

例如使用 direct=0 ext4 10G 的文件 1000s 然后观察到的结果:
```txt
   - 87.20% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 84.85% __do_sys_io_uring_enter
            - 60.97% io_submit_sqes
               - 60.14% io_issue_sqe
                  - 59.54% __io_issue_sqe
                     - io_read
                        - __io_read
                           - 58.76% filemap_read
                              - 58.07% filemap_get_pages
                                 - 52.92% force_page_cache_ra
                                    - 52.81% page_cache_ra_unbounded
                                       - 42.89% read_pages
                                          - 27.04% ext4_mpage_readpages
                                             - 21.12% submit_bio_noacct_nocheck
                                                + 20.61% __submit_bio
                                             - 2.30% bio_alloc_bioset
                                                - 1.09% mempool_alloc_noprof
                                                     0.95% kmem_cache_alloc_noprof
                                                - 1.02% bio_associate_blkg
                                                     0.92% bio_associate_blkg_from_css
                                             - 2.14% ext4_map_blocks
                                                - 1.19% __check_block_validity.constprop.0
                                                     1.01% ext4_sb_block_valid
                                                  0.82% ext4_es_lookup_extent
                                          + 15.52% blk_finish_plug
                                       + 6.32% filemap_add_folio
                                       + 2.66% folio_alloc_noprof
                                 - 2.95% filemap_get_read_batch
                                      2.50% xas_load
                                   1.40% filemap_update_page
            - 21.16% io_cqring_wait
               - 9.73% schedule
                  + 9.57% __schedule
               - 9.18% __io_run_local_work
                  - 8.13% __io_run_local_work_loop
                     - io_req_task_submit
                        - 7.91% io_issue_sqe
                           - __io_issue_sqe
                              - 7.46% io_read
                                 - __io_read
                                    - 6.19% filemap_read
                                       - 2.34% filemap_get_pages
                                          - filemap_get_read_batch
                                               1.33% xas_load
                                       - 2.32% copy_page_to_iter
                                            2.27% _copy_to_iter
                                       - 0.63% touch_atime
                                            0.51% atime_needs_update
                    0.59% llist_reverse_order
            - 1.46% __io_run_local_work
               - 1.31% __io_run_local_work_loop
                  - io_req_task_submit
                     - io_issue_sqe
                        - __io_issue_sqe
                           - 1.23% io_read
                              - __io_read
                                 - 1.14% filemap_read
                                    - 0.56% copy_page_to_iter
                                         _copy_to_iter
         + 1.14% exit_to_user_mode_loop
           0.63% arch_exit_to_user_mode_prepare.isra.0
```

## struct kiocb  和 struct io_kiocb
<!-- 04c2c932-084d-44b7-b405-331bdd644b8e -->

组装的位置在 `__kernel_read`，然后传递给这种注册的 read_iter 和 write_iter
```txt
	ret = file->f_op->read_iter(&kiocb, &iter);
```


简而言之 kiocb 就是为了为了让为了执行 callback
```c
struct kiocb {
    struct file     *ki_filp;       // 文件指针
    loff_t          ki_pos;         // 文件偏移
    void (*ki_complete)(struct kiocb *iocb, long ret);  // 完成回调
    void            *private;       // 私有数据
    int             ki_flags;       // 标志位
    u16             ki_ioprio;      // IO 优先级
    // ...
};

struct io_kiocb {
  union {
      struct file     *file;      // 文件指针
      struct io_cmd_data cmd;
  };
  u8              opcode;         // 操作码 (如 IORING_OP_READ, IORING_OP_WRITE)
  io_req_flags_t  flags;          // REQ_F_* 标志
  struct io_cqe   cqe;            // 完成队列条目
  struct io_ring_ctx *ctx;        // io_uring 上下文
  struct io_uring_task *tctx;     // per-task 上下文
  atomic_t        refs;           // 引用计数
  struct io_kiocb *link;          // 链接的请求（用于链式操作）
  void            *async_data;    // 异步数据
  // ...
};
```
常见的就是 aio 和 io uring 了，但是也可以看到一些其他的

1. erofs
经典的数据流的过程，这个 rq_submit 已经到了驱动了，但是下层又是文件系统，所以是 rq submit 到 iocb
```c
static void erofs_fileio_rq_submit(struct erofs_fileio_rq *rq)
{
	struct iov_iter iter;
	int ret;

	if (!rq)
		return;
	rq->iocb.ki_pos = rq->bio.bi_iter.bi_sector << SECTOR_SHIFT;
	rq->iocb.ki_ioprio = get_current_ioprio();
	rq->iocb.ki_complete = erofs_fileio_ki_complete;
	if (test_opt(&EROFS_SB(rq->sb)->opt, DIRECT_IO) &&
	    rq->iocb.ki_filp->f_mode & FMODE_CAN_ODIRECT)
		rq->iocb.ki_flags = IOCB_DIRECT;
	iov_iter_bvec(&iter, ITER_DEST, rq->bvecs, rq->bio.bi_vcnt,
		      rq->bio.bi_iter.bi_size);
	ret = vfs_iocb_iter_read(rq->iocb.ki_filp, &rq->iocb, &iter);
	if (ret != -EIOCBQUEUED)
		erofs_fileio_ki_complete(&rq->iocb, ret);
}
```
2. mm/page_io.c:swap_writepage_fs

## buffered io 和 direct io 的 callback 是什么?
<!-- 8eb1e42f-8cca-4908-9a95-06d5aba6872b -->

- blk_update_request 中来处理 bi_end_io 的 callback
  - blkdev_bio_end_io_async (direct io : 如果直接在 blkdev 上)
	- io_complete_rw
  - iomap_dio_bio_end_io (direct io : 如果在 xfs 上)
	- io_complete_rw
  - mpage_end_io (buffer io)
    - __read_end_io
      - folio_wake_bit
        - io_async_buf_func : read buffer io 的 callback
          - io_req_task_queue (将完成的任务挂上去)

direct io 经典 callback :
```txt
@[
        io_complete_rw+5
        blkdev_bio_end_io_async+78
        blk_update_request+415
        scsi_end_request+39
        scsi_io_completion+83
        blk_done_softirq+74
        handle_softirqs+241
        __irq_exit_rcu+194
        common_interrupt+133
        asm_common_interrupt+38
        cpuidle_enter_state+211
        cpuidle_enter+45
        cpuidle_idle_call+241
        do_idle+120
        cpu_startup_entry+41
        start_secondary+296
        common_startup_64+318
]: 301
```
buffer io 经典的 callback : io_async_buf_func

```txt
@[
        folio_wake_bit+1
        __read_end_io+282
        blk_update_request+415
        scsi_end_request+39
        scsi_io_completion+83
        blk_done_softirq+74
        handle_softirqs+241
        __irq_exit_rcu+194
        common_interrupt+133
        asm_common_interrupt+38
        cpuidle_enter_state+211
        cpuidle_enter+45
        cpuidle_idle_call+241
        do_idle+120
        cpu_startup_entry+41
        start_secondary+296
        common_startup_64+318
]: 867
```

```txt
 31)               |              folio_wake_bit() {
 31)   0.107 us    |                _raw_spin_lock_irqsave();
 31)               |                __wake_up_locked_key() {
 31)               |                  __wake_up_common() {
 31)               |                    io_async_buf_func) {
 31)               |                      io_req_task_queue() {
 31)               |                        __io_req_task_work_add() {
 31)   0.087 us    |                          __rcu_read_lock();
 31)   0.077 us    |                          __rcu_read_unlock();
 31)   0.490 us    |                        }
 31)   0.688 us    |                      }
 31)   0.911 us    |                    }
 31)   1.165 us    |                  }
 31)   1.336 us    |                }
 31)   0.090 us    |                _raw_spin_unlock_irqrestore();
 31)   1.948 us    |              }
 31)   2.124 us    |            }
```

### 存储系统中断返回的 callback 分析

- blk_mq_end_request
  - blk_update_request
    - bio_endio
      - end_bio_bh_io_sync
        - bio_put
    - blk_stat_add
    - blk_mq_free_request

可以看到，同时会释放 request / bio ，然后统计数据。

总体来说，这是符合预期的，做出来的功能: 释放，统计，唤醒等待的进程
```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 2)               |  dummy_complete_rq [virtio_dummy]() {
 2)               |    blk_mq_end_request() {
 2)               |      blk_update_request() {
 2)               |        blk_account_io_completion.part.0() {
 2)   0.120 us    |          preempt_count_add();
 2)   0.087 us    |          preempt_count_sub();
 2)   0.755 us    |        }
 2)               |        bio_endio() {
 2)   0.476 us    |          blk_throtl_bio_endio();
 2)               |          bio_uninit() {
 2)   0.082 us    |            __rcu_read_lock();
 2)   0.090 us    |            __rcu_read_unlock();
 2)   0.587 us    |          }
 2)               |          end_bio_bh_io_sync() {
 2)               |            end_buffer_read_sync() {
 2)               |              wake_up_bit() { // 省掉一大片
 2)   6.722 us    |              }
 2)   6.914 us    |            }
 2)               |            bio_put() {
 2)               |              bio_free() {
 2)   0.083 us    |                bio_uninit();
 2)   0.084 us    |                bvec_free();
 2)               |                mempool_free() {
 2)               |                  mempool_free_slab() {
 2)               |                    kmem_cache_free() {
 2)   0.118 us    |                      __slab_free();
 2)   0.437 us    |                    }
 2)   0.611 us    |                  }
 2)   0.774 us    |                }
 2)   1.359 us    |              }
 2)   1.590 us    |            }
 2)   8.833 us    |          }
 2) + 10.441 us   |        }
 2) + 11.684 us   |      }
 2)   0.173 us    |      ktime_get();
 2)               |      blk_stat_add() {
 2)               |        blk_throtl_stat_add() {
 2)   0.083 us    |          throtl_track_latency();
 2)   0.317 us    |        }
 2)   0.081 us    |        __rcu_read_lock();
 2)   0.082 us    |        preempt_count_add();
 2)   0.082 us    |        preempt_count_sub();
 2)   0.082 us    |        __rcu_read_unlock();
 2)   1.642 us    |      }
 2)   0.081 us    |      preempt_count_add();
 2)   0.093 us    |      update_io_ticks(); // TODO 为什么这里需要 preempt 的操作
 2)   0.082 us    |      preempt_count_sub();
 2)               |      blk_mq_free_request() {
 2)               |        __blk_mq_free_request() {
 2)   0.262 us    |          blk_mq_put_tag();
 2)               |          blk_queue_exit() {
 2)   0.083 us    |            __rcu_read_lock();
 2)   0.075 us    |            __rcu_read_unlock();
 2)   0.472 us    |          }
 2)   0.075 us    |            __rcu_read_unlock();
 2)   0.472 us    |          }
 2)   1.374 us    |        }
 2)   1.600 us    |      }
 2) + 16.309 us   |    }
 2) + 17.881 us   |  }
```

## aio 提交 buffer io 等待位置
<!-- 56c4dbc9-8b33-4550-8ab5-0952fa3247d6 -->

```txt
[<0>] folio_wait_bit_common+0x12b/0x320
[<0>] filemap_update_page+0x2fe/0x310
[<0>] filemap_get_pages+0x2ba/0x470
[<0>] filemap_read+0xff/0x440
[<0>] aio_read+0x133/0x210
[<0>] io_submit_one+0xee/0x370
[<0>] __x64_sys_io_submit+0x94/0x1e0
[<0>] do_syscall_64+0x84/0x250
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

```txt
   - 86.83% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 49.49% __x64_sys_io_submit
            - 48.07% io_submit_one
               - 43.62% aio_write
                  - 41.73% ext4_buffered_write_iter
                     - 40.49% generic_perform_write
                        - 32.87% ext4_da_write_begin
                           - 28.05% __filemap_get_folio
                              - 18.73% filemap_add_folio
                                 + 8.39% __filemap_add_folio
                                 + 8.20% __mem_cgroup_charge
                                 - 1.95% __folio_batch_add_and_move
                                    - folio_batch_move_lru
                                       - 1.35% lru_add
                                            0.95% lru_gen_add_folio
                              - 6.08% filemap_get_entry
                                   5.92% xas_load
                              + 2.92% folio_alloc_noprof
                           + 4.47% ext4_block_write_begin
                        + 4.16% ext4_da_do_write_end.isra.0
                          2.44% copy_folio_from_iter_atomic
                          0.63% balance_dirty_pages_ratelimited_flags
                 1.03% aio_complete
                 0.88% __io_submit_one.isra.0
                 0.62% _copy_from_user
                 0.56% kmem_cache_alloc_noprof
              0.80% lookup_ioctx
         + 33.08% __x64_sys_fadvise64
         + 3.22% __x64_sys_io_getevents
           0.58% arch_exit_to_user_mode_prepare.isra.0
```

## 然后分析对比一下 ./aio.md 和 ./aio-buffer-io.md

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
