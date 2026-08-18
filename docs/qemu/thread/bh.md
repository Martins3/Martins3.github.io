# qemu bh

简单来说，使用 bh 是因为一个 thread 上不能执行某一个任务，需要换另外一个 thread 的上下文来执行

```txt
  producer thread
      |
      | qemu_bh_schedule(bh)
      v
  ctx->bh_list
      |
      | aio_notify(ctx)，必要时写 eventfd
      v
  唤醒 AioContext 所属线程
      |
      | aio_poll(ctx)
      v
  aio_bh_poll(ctx)
      |
      v
  fn(arg)
```

qemu_bh_schedule() 本身不会执行回调，只做两件事：

1. 原子地将 BH 标记并放入 ctx->bh_list。
2. 调用 aio_notify(ctx)，必要时用 eventfd 唤醒事件循环。

## aio_bh_poll 的作用

aio_bh_poll 是 QEMU Bottom Half (BH) 调度系统的核心函数，用于执行所有已调度（scheduled）的 BH 回调。

核心功能

```c
/* Multiple occurrences of aio_bh_poll cannot be called concurrently. */
int aio_bh_poll(AioContext *ctx)
{
    // 1. 原子移动：将全局 bh_list 移动到本地 slice
    QSLIST_MOVE_ATOMIC(&slice.bh_list, &ctx->bh_list);

    // 2. 遍历并执行所有 BH 回调
    while ((bh = aio_bh_dequeue(...))) {
        if (scheduled and not deleted) {
            aio_bh_call(bh);      // 执行回调
        }
        if (deleted or oneshot) {
            g_free(bh);           // 释放 BH
        }
    }
    return ret;  // 返回是否有实际工作完成
}
```

BH 状态流转

 状态           含义
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 BH_PENDING     已加入队列，等待执行
 BH_SCHEDULED   需要调用回调函数
 BH_DELETED     标记删除，不执行回调
 BH_ONESHOT     执行后自动删除
 BH_IDLE        空闲时执行（不算作 progress）

什么时候调用

1. 主事件循环 aio_poll() 中

```c
// util/aio-posix.c
bool aio_poll(AioContext *ctx, bool blocking)
{
    qemu_lockcnt_inc(&ctx->list_lock);

    // ... 等待 fd 事件 ...
    ctx->fdmon_ops->wait(ctx, &ready_list, timeout);

    // 执行 BH（在 fd 事件 dispatch 之前）
    progress |= aio_bh_poll(ctx);

    // 执行 fd 回调
    progress |= aio_dispatch_ready_handlers(ctx, &ready_list, block_ns);
    ...
}
```

2. GSource dispatch 中

```c
// util/aio-posix.c
void aio_dispatch(AioContext *ctx)
{
    qemu_lockcnt_inc(&ctx->list_lock);

    aio_bh_poll(ctx);        // 先执行 BH

    ctx->fdmon_ops->gsource_dispatch(ctx, &ready_list);
    aio_dispatch_ready_handlers(ctx, &ready_list, 0);
    ...
}
```

(这么看，总是先去执行 bh ，然后去执行其他的内容?)

3. 何时触发调用

```txt
 触发方式                     说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 qemu_bh_schedule(bh)         调度 BH，自动触发 aio_notify 唤醒事件循环
 aio_bh_schedule_oneshot()    创建一次性 BH 并调度
 协程调度 aio_co_schedule()   内部使用 BH 实现
```

设计要点

线程安全

// 从任意线程安全地调度 BH
void qemu_bh_schedule(QEMUBH *bh) {
    aio_bh_enqueue(bh, BH_SCHEDULED);  // 原子操作
    aio_notify(ctx);                    // 唤醒事件循环
}

执行顺序保证

• BH 优先于 fd 事件：在 aio_poll 中，aio_bh_poll 在 aio_dispatch_ready_handlers 之前执行
• 切片隔离：每次 aio_bh_poll 创建一个切片，新调度的 BH 进入下一个切片（避免无限递归）

回调中可再调度

// BH 回调中调度新 BH 是安全的
void my_bh_callback(void *opaque) {
    qemu_bh_schedule(another_bh);  // 进入下一个切片，当前 poll 不执行
}

使用场景

1. 延迟执行工作：从非事件循环线程安全地提交工作到事件循环
2. 避免递归：将可能递归的调用推迟到事件循环的下一次迭代
3. 协程调度：aio_co_schedule() 内部使用 BH 实现跨 AioContext 的协程迁移

## 经典使用

### QEMU block layer 的 thread pool completion

当底层没有使用 Linux AIO/io_uring 的异步 flush 时，QEMU 会把阻塞的 fdatasync() 放进 thread pool：

AioContext thread / coroutine
    raw_co_flush_to_disk()
        raw_thread_pool_submit(handle_aiocb_flush)
            thread_pool_submit_co()
                提交给 worker
                qemu_coroutine_yield()

#### worker 完成之后

核心代码在 util/thread-pool.c:82：

```txt
ret = req->func(req->arg);       /* worker 中执行 fdatasync */

qatomic_set(&req->ret, ret);
qatomic_store_release(&req->state, THREAD_DONE);

qemu_bh_schedule(pool->completion_bh);
```

注意 worker 没有这样做：

```txt
/* 错误：不能在 worker 中直接调用 */
elem->common.cb(elem->common.opaque, elem->ret);
```

而是 schedule BH。

然后原来的 AioContext 线程醒来：

```txt
aio_poll(pool->ctx)
    aio_bh_poll()
        thread_pool_completion_bh()
            elem->common.cb()
                thread_pool_co_cb()
                    aio_co_wake(original_coroutine)
```

BH 创建时就绑定到了 thread pool 所属的 AioContext，见 util/thread-pool.c:322：

```txt
pool->ctx = ctx;
pool->completion_bh =
    aio_bh_new(ctx, thread_pool_completion_bh, pool);
```

最后 completion callback 恢复原来的 coroutine，见 util/thread-pool.c:278：

```c
static void thread_pool_co_cb(void *opaque, int ret)
{
    ThreadPoolCo *co = opaque;

    co->ret = ret;
    aio_co_wake(co->co);
}
```

#### 为什么这里不能直接调用 callback

因为 worker thread 只负责执行阻塞函数，它不是这个 block request 所属的 AioContext 线程。

如果直接在 worker 中调用 completion callback，可能出现：

- 在错误线程访问 BlockDriverState。
- 在错误线程操作 pool->head；源码明确说这个链表只能由 AioContext 的“mother thread”访问。
- completion callback 继续进入 block layer，而这些代码默认当前线程就是正确的 AioContext。
- 在错误线程恢复 coroutine。
- 多个 worker 同时执行 completion，破坏原本由单线程事件循环提供的串行语义。

因此这里必须存在：

worker thread
    └── 只发布结果
          └── 跨线程通知
                └── AioContext thread 执行完成回调

理论上也可以自己实现“线程安全队列 + eventfd + fd handler”，但那正是在重新实现 BH。QEMU 已经通过 BH 提供了这套机制。



### nvme
在 nvme 上随意触发一个 io ，就可以得到这样的结果:

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - nvme_process_sq
                            - nvme_update_sq_tail
                              - ldl_le_pci_dma
                                - ldl_le_dma
                                  - dma_memory_read
                                    - dma_memory_rw
                                      - dma_memory_rw_relaxed
                                        - address_space_rw
                                          - address_space_read_full
                                            - flatview_read
                                              - flatview_translate
                                                - flatview_do_translate
                                                  - address_space_translate_iommu
                                                    - amdvi_translate

### [ ] timer
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - qemu_clock_run_all_timers
          - qemu_clock_run_timers
            - timerlist_run_timers
              - timerlist_run_timers
                - qemu_bh_schedule

### [ ] scsi

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_scsi_handle_cmd
                            - virtio_scsi_handle_cmd_vq
                              - virtio_scsi_handle_cmd_req_submit
                                - defer_call_end
                                  - ioq_submit (这个在 linux 中的 block/linux-aio.c)
                                    - qemu_laio_process_completions
                                      - qemu_bh_schedule

### free page hint
aio bh 的例子:
```txt
        s->free_page_bh = aio_bh_new_guarded(iothread_get_aio_context(s->iothread),
                                             virtio_ballloon_get_free_page_hints, s,
                                             &dev->mem_reentrancy_guard);
```

virtio balloon 这个设备的消息处理还是在 qemu 的 main loop 中处理的
```txt
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_ready_handlers
                      - aio_dispatch_handler
                        - virtio_queue_host_notifier_read
                          - virtio_queue_notify_vq
                            - virtio_balloon_handle_free_page_vq
```

执行者总是在 balloon0 这个 io thread 中，这个 thread 有自己特殊的路径需要考虑，所以这个是非常合理的
```txt
- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_bh_poll
            - aio_bh_call
              - virtio_ballloon_get_free_page_hints
                - get_free_page_hints
                  - qemu_guest_free_page_hint
                    - migration_clear_memory_region_dirty_bitmap_rnge
                      - migration_clear_memory_region_dirty_bitmap
                        - migration_clear_memory_region_dirty_bitmap
                          - memory_region_clear_dirty_bitmap
```

类似的，例如执行获取 balloon 的统计量
```json
{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }
```

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - do_qmp_dispatch_bh
                            - qmp_marshal_qom_get
                              - qmp_qom_get
                                - object_property_get_qobject
                                  - object_property_get
                                    - property_get_alias
                                      - object_property_get
                                        - balloon_stats_get_all

## 如何理解 aio_bh_schedule_oneshot ?

QEMUBH *bh = aio_bh_new(ctx, cb, opaque);
qemu_bh_schedule(bh);

这是可复用对象：

- aio_bh_new()：只创建，不调度。
- qemu_bh_schedule()：调度。
- 执行后对象仍存在，可以再次 schedule。
- 多次 schedule 发生在回调执行前，通常合并为一次执行。

aio_bh_schedule_oneshot(ctx, cb, opaque);
- 创建；
- 立即 schedule；
- 执行后自动释放。

所以它很适合“向另一个 AioContext 投递一次函数调用”。

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
