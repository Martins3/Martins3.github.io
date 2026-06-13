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

aio bh 的例子:
```txt
        s->free_page_bh = aio_bh_new_guarded(iothread_get_aio_context(s->iothread),
                                             virtio_ballloon_get_free_page_hints, s,
                                             &dev->mem_reentrancy_guard);
```

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
                    - migration_clear_memory_region_dirty_bitmap_range
                      - migration_clear_memory_region_dirty_bitmap
                        - migration_clear_memory_region_dirty_bitmap
                          - memory_region_clear_dirty_bitmap
```

## 如何理解 aio_bh_schedule_oneshot ?




## bh 的 api 似乎正好是两个 api 的
aio_bh_schedule_oneshot
qemu_bh_schedule

```txt
void qemu_bh_schedule(QEMUBH *bh)
{
    aio_bh_enqueue(bh, BH_SCHEDULED);
}
```

## 和 defer.md 的关系是什么?


## 为什么需要 bh 机制

> [!NOTE]
> 参考神奇海螺的意见，有待验证

简单来说:
```txt
1. 事件循环架构的要求

QEMU 是事件驱动架构，所有工作必须在事件循环（AioContext）的上下文中执行：

/* Called concurrently from any thread */
static void aio_bh_enqueue(QEMUBH *bh, unsigned new_flags)
{
    QSLIST_INSERT_HEAD_ATOMIC(&ctx->bh_list, bh, next);
    aio_notify(ctx);  // 唤醒事件循环，而不是立即执行
}

关键设计：qemu_bh_schedule() 只是将 BH 加入队列，然后通过 aio_notify() 通知事件循环，实际的回
调执行被推迟到事件循环的下一次迭代。

2. 线程安全（从任意线程调度）

源码明确说明 aio_bh_enqueue() 可以从任意线程并发调用：

/* Called concurrently from any thread */
static void aio_bh_enqueue(QEMUBH *bh, unsigned new_flags)

但 aio_bh_poll() 有约束：

/* Multiple occurrences of aio_bh_poll cannot be called concurrently. */
int aio_bh_poll(AioContext *ctx)

延迟回调的原因：Worker 线程不能直接执行回调（可能涉及线程不安全操作），必须通过 aio_notify()
→ 主事件循环 → aio_bh_poll() 的路径。

3. 防止同一次 poll 中的递归（Slice 机制）

int aio_bh_poll(AioContext *ctx)
{
    /* 1. 原子地将当前队列移动到本地 slice */
    QSLIST_MOVE_ATOMIC(&slice.bh_list, &ctx->bh_list);

    /* 2. 只处理 slice 中的 BH */
    while ((bh = aio_bh_dequeue(&slice->bh_list, &flags))) {
        aio_bh_call(bh);  // 如果回调中调用 qemu_bh_schedule()，
                          // 新 BH 会进入 ctx->bh_list，而不是 slice
    }
}

效果：当前 aio_bh_poll() 中调度的 BH 不会在本次执行，必须等待下一次 poll。这防止了：

• 单个 BH 无限递归导致的栈溢出
• 回调打断当前 poll 的执行流程

4. 支持嵌套 poll（bh_slice_list）

struct BHListSlice {
    BHList bh_list;
    QSIMPLEQ_ENTRY(BHListSlice) next;
};

当回调中调用 aio_poll()（嵌套事件循环）时，新的 slice 会被追加到 bh_slice_list 队列，确保嵌套
调用也能处理所有待执行的 BH。

─────────────────────────────────────────────────────────────────────────────────────────────
总结：延迟回调是 QEMU 事件循环架构的核心设计，提供线程安全（任意线程可调度）、上下文隔离（在
主循环中执行）、防止递归（slice 机制）和支持嵌套 poll 的能力。
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
