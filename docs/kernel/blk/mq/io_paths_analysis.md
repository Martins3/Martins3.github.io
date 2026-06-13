# Block Layer IO 请求路径详解 - Bypass 机制全梳理

在 Linux blk-mq 中，IO 请求有多种可能的流动路径，取决于多种因素：
- 是否有 **plug** 机制启用
- 是否有 **scheduler** (elevator)
- 请求类型 (**passthrough**, **flush**, **normal**)
- 是否支持 **direct issue**
- **hctx->dispatch_busy** 状态


Linux blk-mq 通过多种 bypass 机制优化 IO 路径：
1. **Merge**: 最理想的情况，不产生新 request
2. **Plug**: 批量处理，减少锁竞争和硬件中断
3. **Direct Issue**: 完全绕过队列，直接下发
4. **Scheduler**: 提供排序、优先级等高级功能
5. **Software Queue**: 通用路径，per-cpu 设计
6. **Special Bypass**: Passthrough 和 Flush 的特殊处理

 路径           触发条件        优势                适用场景
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Merge          相邻 sector     无 alloc/下发开销   顺序 IO
 Plug           current->plug   批量减少锁竞争      文件系统默认
 Direct Issue   none + !busy    零队列延迟          NVMe SSD
 blk_mq_ctx     通用            per-cpu 无锁        高并发
 Scheduler      elevator        排序/优先级         HDD/SATA

## 核心决策流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        blk_mq_submit_bio() 入口                                     │
│                        (所有 IO 的必经之路)                                         │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│  Step 1: 检查 blk_plug                                                             │
│  ────────────────────────────────────────                                           │
│  if (plug && plug->mq_list 有可用 request)                                          │
│      └── 使用 cached request (避免重新分配)                                          │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│  Step 2: 尝试 Merge                                                                 │
│  ────────────────────────────────────────                                           │
│  blk_mq_attempt_bio_merge()                                                         │
│      └── 尝试与前/后相邻请求合并 (front merge/back merge)                            │
│      └── 如果合并成功，直接返回，无需创建 request                                    │
│      └── [BYPASS 路径 1: 合并后无需下发新请求]                                       │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ (未合并，需要新 request)
┌─────────────────────────────────────────────────────────────────────────────────────┐
│  Step 3: 分配 Request 并初始化                                                       │
│  ────────────────────────────────────────                                           │
│  rq = blk_mq_get_new_requests()                                                     │
│      └── 获取当前 CPU 的 blk_mq_ctx                                                 │
│      └── 分配 tag                                                                   │
│      └── rq->mq_ctx = ctx (绑定到软件队列)                                           │
│      └── rq->mq_hctx = hctx (绑定到硬件队列)                                         │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────┴───────────────┐
                    │                               │
                    ▼                               ▼
┌────────────────────────────────┐  ┌──────────────────────────────────────────────────┐
│  路径 A: Plug 路径             │  │  路径 B: 非 Plug 路径                            │
│  ───────────────────────────── │  │  ────────────────────────────────                │
│                                │  │                                                  │
│  if (current->plug != NULL)    │  │  hctx = rq->mq_hctx                              │
│      └── blk_add_rq_to_plug()  │  │                                                  │
│      └── 请求放入 plug 列表    │  │  ┌──────────────────┬──────────────────────┐     │
│      └── 暂不处理，等待 unplug │  │  │                  │                      │     │
│      └── [BYPASS 路径 2]       │  │  ▼                  ▼                      │     │
│                                │  │  使用调度器         不使用调度器           │     │
└────────────────────────────────┘  │  (q->elevator)      (q->elevator == NULL)  │     │
                                    │                                            │     │
                                    │  if (RQF_USE_SCHED  │                      │     │
                                    │      || dispatch_busy│                     │     │
                                    │      || !is_sync)   │                      │     │
                                    │      └── 插入调度器 │                      │     │
                                    │          队列       │                      │     │
                                    │      └── 等待       │                      │     │
                                    │          调度       │                      │     │
                                    │                     │                      │     │
                                    │                     ▼                      │     │
                                    │              插入 blk_mq_ctx               │     │
                                    │              (软件队列)                    │     │
                                    │              spin_lock(&ctx->lock)         │     │
                                    │              list_add_tail()               │     │
                                    │                                            │     │
                                    └──────────────────┬────────────────────────────┘     │
                                                       │                                  │
                                                       ▼                                  │
                                    ┌──────────────────────────────────────────┐         │
                                    │  Step 4: 尝试 Direct Issue                │         │
                                    │  ───────────────────────────────────────  │         │
                                    │                                           │         │
                                    │  if (!RQF_USE_SCHED && !dispatch_busy      │◀────────┘
                                    │      && is_sync && 有 budget)             │
                                    │      └── blk_mq_try_issue_directly()      │
                                    │      └── 直接调用 driver->queue_rq()      │
                                    │      └── [BYPASS 路径 3: 完全绕过队列]     │
                                    │                                           │
                                    │  else                                     │
                                    │      └── blk_mq_insert_request()          │
                                    │      └── 根据情况选择:                     │
                                    │          - 插入 scheduler                 │
                                    │          - 插入 blk_mq_ctx                 │
                                    │          - 插入 hctx->dispatch (passthrough)│
                                    └──────────────────────────────────────────┘
                                                       │
                                                       ▼
                                    ┌──────────────────────────────────────────┐
                                    │  Step 5: 触发队列运行                     │
                                    │  ───────────────────────────────────────  │
                                    │                                           │
                                    │  blk_mq_run_hw_queue()                    │
                                    │      └── 调度 kblockd workqueue           │
                                    │      └── 或同步运行                       │
                                    │                                           │
                                    │  blk_mq_sched_dispatch_requests()         │
                                    │      └── 从 scheduler 取请求              │
                                    │      └── 或从 blk_mq_ctx 取请求           │
                                    │      └── blk_mq_dispatch_rq_list()        │
                                    │          └── driver->queue_rq()           │
                                    └──────────────────────────────────────────┘
```

---

## 详细路径分析

### 路径 1: Bio Merge Bypass

**触发条件**: 新 bio 可以与已存在的请求合并

```c
// blk-mq.c: 3192
if (blk_mq_attempt_bio_merge(q, bio, nr_segs))
    goto queue_exit;  // 合并成功，直接返回，无需创建 request
```

**流程**:
```
submit_bio(bio sector 0-7)
    └── 发现已有请求在 sector 8-15
    └── 可以 back merge 成 sector 0-15
    └── bio 被合并到现有 request
    └── 不创建新 request，不下发新 IO
```

**性能影响**: 最高效的 bypass，减少了 request 分配和下发开销。

---

### 路径 2: Plug Bypass

**触发条件**: `current->plug != NULL`

```c
// blk-mq.c: 3232-3234
if (plug) {
    blk_add_rq_to_plug(plug, rq);
    return;  // 请求放入 plug 列表，暂不处理
}
```

**代码实现**:
```c
// blk-mq.c: 1399-1420
static void blk_add_rq_to_plug(struct blk_plug *plug, struct request *rq)
{
    struct request *last = rq_list_peek(&plug->mq_list);

    if (!plug->rq_count) {
        trace_block_plug(rq->q);  // 记录 plug 事件
        // 添加 timer，超时后自动 unplug
    }

    // 尝试与 plug 列表中的最后一个请求合并
    if (last && ...)
        // merge 成功

    // 添加到 plug 列表
    rq_list_add_tail(&plug->mq_list, rq);
    plug->rq_count++;
}
```

**Plug 的触发时机**:
```c
// 用户代码显式使用
blk_start_plug(&plug);
... // 提交多个 IO
read(fd, buf, size);  // 内部使用 plug
write(fd, buf, size);
blk_finish_plug(&plug);  // 触发 unplug，批量下发

// 或文件系统内部使用 (ext4, xfs 等)
ext4_file_read_iter()
    └── blk_start_plug()
    └── 提交多个 bio
    └── blk_finish_plug()  // 批量下发
```

**性能优势**:
- 批量处理减少锁操作次数
- 增加请求合并机会
- 减少硬件中断次数

---

### 路径 3: Direct Issue Bypass

**触发条件**:
- 没有 scheduler (`!RQF_USE_SCHED`)
- hctx 不忙 (`!dispatch_busy`)
- 同步 IO (`is_sync`)
- 有 budget (tag 可用)

```c
// blk-mq.c: 3242-3244
blk_mq_run_dispatch_ops(q, blk_mq_try_issue_directly(hctx, rq));
```

**执行流程**:
```c
blk_mq_try_issue_directly()
    └── blk_mq_request_issue_directly()
        └── q->mq_ops->queue_rq()  // 直接调用驱动回调
            └── nvme_queue_rq() / virtio_queue_rq() / scsi_queue_rq()
    └── 如果成功 (BLK_STS_OK)
        └── 请求直接下发，无需经过任何队列
```

**特点**:
- 最快的路径，完全绕过了 blk_mq_ctx 和 scheduler
- 适用于 none 调度器 + 低负载场景
- 如果驱动返回 BLK_STS_RESOURCE，会回退到 insert 路径

---

### 路径 4: Scheduler 路径

**触发条件**: `q->elevator != NULL`

```c
// blk-mq.c: 2654-2660 (blk_mq_insert_request)
if (q->elevator) {
    LIST_HEAD(list);
    list_add(&rq->queuelist, &list);
    q->elevator->type->ops.insert_requests(hctx, &list, flags);
}
```

**流程**:
```
submit_bio()
    └── 创建 request
    └── rq->rq_flags |= RQF_USE_SCHED  // 标记使用调度器
    └── 插入到调度器的内部队列
    └── 等待调度器选择何时下发

blk_mq_sched_dispatch_requests()  // 异步执行
    └── scheduler->ops.dispatch_request()
        └── 例如 dd_dispatch_request() (mq-deadline)
        └── 根据策略选择下一个请求
    └── blk_mq_dispatch_rq_list()
        └── driver->queue_rq()
```

**调度器的作用**:
- 请求排序（按 sector，电梯算法）
- 优先级控制（RT/BE/IDLE）
- 延迟保证（deadline）
- 带宽控制（kyber）

---

### 路径 5: Software Queue (blk_mq_ctx) 路径

**触发条件**:
- 无 scheduler (`q->elevator == NULL`)
- 无法 direct issue (dispatch_busy 或 async)

```c
// blk-mq.c: 2664-2671
spin_lock(&ctx->lock);
list_add_tail(&rq->queuelist, &ctx->rq_lists[hctx->type]);
blk_mq_hctx_mark_pending(hctx, ctx);  // 标记 hctx 有此 ctx 的待处理请求
spin_unlock(&ctx->lock);
```

**特点**:
- 最通用的路径
- 利用 per-cpu 队列消除锁竞争
- hctx->ctx_map 位图跟踪哪些 ctx 有待处理请求

---

### 路径 6: Passthrough Bypass

**触发条件**: `blk_rq_is_passthrough(rq)` (如 SG_IO, IDE_CMD)

```c
// blk-mq.c: 2619-2630
if (blk_rq_is_passthrough(rq)) {
    // Passthrough 请求直接放入 hctx->dispatch
    blk_mq_request_bypass_insert(rq, flags);
}
```

**原因**: Passthrough 请求通常需要优先处理，不应被调度器延迟。

---

### 路径 7: Flush Request 特殊处理

**触发条件**: `req_op(rq) == REQ_OP_FLUSH`

```c
// blk-mq.c: 2631-2653
if (req_op(rq) == REQ_OP_FLUSH) {
    // Flush 请求直接放入 hctx->dispatch 头部
    blk_mq_request_bypass_insert(rq, BLK_MQ_INSERT_AT_HEAD);
}
```

**原因**: Flush 请求需要尽快执行，以保证数据一致性。

---

## 路径选择决策树

```
submit_bio()
    │
    ├─► [Merge?] ──YES──► [Return] (无新 request)
    │
    ├─► [Plug?] ──YES──► [Add to plug list] ──► [Return]
    │                      (等待 unplug 批量处理)
    │
    ├─► [Create Request]
    │
    ├─► [Flush?] ──YES──► [Insert to hctx->dispatch head] ──► [Run hwq]
    │
    ├─► [Passthrough?] ──YES──► [Insert to hctx->dispatch] ──► [Run hwq]
    │
    ├─► [Scheduler?] ──YES──► [Insert to scheduler] ──► [Run hwq]
    │                           (等待调度)
    │
    ├─► [Can Direct Issue?] ──YES──► [driver->queue_rq()] ──► [Return]
    │      (!busy && sync && budget)
    │
    └─► [Insert to blk_mq_ctx] ──► [Run hwq]
        (最通用路径)
```

## 五大 IO 路径详解

路径 1: Bio Merge Bypass (最高效)

submit_bio(bio)
    └── blk_mq_attempt_bio_merge()
        └── 与相邻请求合并
        └── [直接返回，无新 request，无下发]

触发: 顺序 IO 时，新 bio 可与已有请求合并

路径 2: Plug Bypass (批量优化)

blk_start_plug()  ← 显式或由文件系统调用
submit_bio() x N  ← 请求放入 current->plug->mq_list
blk_finish_plug() ← 批量下发，增加 merge 机会

路径 3: Direct Issue Bypass (最低延迟)

submit_bio()
    └── alloc_request()
    └── blk_mq_try_issue_directly()
        └── driver->queue_rq()  ← 直接调用驱动，绕过所有队列！

触发条件 (必须同时满足):

• 无 scheduler (none)
• !dispatch_busy
• 同步 IO (is_sync)
• 有可用 tag (budget)

路径 4: Software Queue (blk_mq_ctx)

submit_bio()
    └── alloc_request()
    └── spin_lock(&ctx->lock)       ← 只锁当前 CPU 的 ctx
    └── list_add_tail(&ctx->rq_lists[type])
    └── blk_mq_run_hw_queue()       ← 异步调度

路径 5: Scheduler Queue (mq-deadline/kyber)

submit_bio()
    └── alloc_request()
    └── rq->rq_flags |= RQF_USE_SCHED
    └── elevator->ops.insert_requests()  ← 插入调度器队列
    └── [等待调度器选择时机]
    └── blk_mq_sched_dispatch_requests()
        └── elevator->ops.dispatch_request()
        └── driver->queue_rq()

决策流程 (简化版)

submit_bio()
    │
    ├─► [Merge 成功?] ──YES──► [Return] (最高效)
    │
    ├─► [Plug 启用?] ──YES──► [Add to plug list] (批量优化)
    │
    ├─► [Direct Issue 条件满足?] ──YES──► [driver->queue_rq()] (最低延迟)
    │      (none 调度器 + !busy + sync + budget)
    │
    ├─► [Scheduler?] ──YES──► [Insert to scheduler] (排序/优先级)
    │
    └─► [Default] ──► [Insert to blk_mq_ctx] (通用路径)

## 关键问题: 如何离开软件队列

list_add(&rq->queuelist, &ctx->rq_lists[hctx->type]); 确实是放进 blk-mq 的 software queue，
也就是每个 ctx 的 rq_lists[type] 里，位置在 block/blk-mq.c:blk_mq_insert_request 中

它离开 software queue 的时机，是这个 hctx 被 run 起来做 dispatch 的时候，不是等 I/O 完成时才离开。

两种主要出队方式：
1. 普通 flush busy ctx 路径:
   在 block/blk-mq.c:1792 的 blk_mq_flush_busy_ctxs() 里，flush_busy_ctx() 直接把
   ctx->rq_lists[type] list_splice_tail_init() 到一个临时 list，这一步就已经从 software queue 移走了。
2. queue busy 时逐个取出:
   在 block/blk-mq-sched.c:213 的 blk_mq_do_dispatch_ctx() 里，调用 blk_mq_dequeue_from_ctx()；后者
   在 block/blk-mq.c:1822 的 dispatch_rq_from_ctx() 里执行
   list_del_init(&dispatch_data->rq->queuelist);
   这就是“逐个离开 software queue”。

之后 request 会进入 dispatch 流程：

- 在 block/blk-mq.c:2100 的 blk_mq_dispatch_rq_list() 里，request 从临时 list 再被取下，然后调用驱动的 queue_rq()。
- 如果驱动/资源不足，没发出去的 request 不会回到 ctx->rq_lists[]，而是被挂到 hctx->dispatch，见 block/blk-mq.c:2160。

所以更准确地说：

ctx->rq_lists[type] -> hctx 运行 dispatch 时被摘下 -> 尝试 queue_rq() -> 发不出去则转到 hctx->dispatch


block/blk-mq-sched.c 并不是给 scheduler 用的，但是的确如此:
```txt
@[
        blk_mq_dispatch_rq_list+5
        __blk_mq_sched_dispatch_requests+280
        blk_mq_sched_dispatch_requests+45
        blk_mq_run_hw_queue+622
        blk_mq_run_hw_queues+125
        blk_mq_requeue_work+415
        process_one_work+402
        worker_thread+406
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 13
@[
        blk_mq_dispatch_rq_list+5
        __blk_mq_sched_dispatch_requests+171
        blk_mq_sched_dispatch_requests+45
        blk_mq_run_hw_queue+622
        blk_mq_run_hw_queues+125
        blk_mq_requeue_work+415
        process_one_work+402
        worker_thread+406
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 193
```

### 从 struct request 的 union 的结构体

```txt
struct request {

  union {
      struct list_head queuelist;  // 双向链表节点
      struct request *rq_next;     // 单向链表指针
  };
```

两种链表结构用于不同场景

1. queuelist (双向链表) - 用于队列管理
- 使用场景：ctx->rq_lists[type], hctx->dispatch
- 需要双向遍历、插入、删除操作
- 例如：blk-mq.c:1832
dispatch_data->rq = list_entry_rq(ctx->rq_lists[type].next);
list_del_init(&dispatch_data->rq->queuelist);

2. rq_next (单向链表) - 用于批量处理
- 使用场景：struct rq_list (blkdev.h:1155-1158)
struct rq_list {
    struct request *head;
    struct request *tail;
};
- 用于高性能的批量完成、plug 机制
- 只需要单向遍历，更轻量
- 例如：blk-mq.h:278-289 中的 rq_list_pop()

为什么用 Union？

内存优化：
- struct list_head 占用 16 字节 (两个指针)
- struct request * 占用 8 字节 (一个指针)
- 两者不会同时使用，所以共享内存空间

生命周期分离：
1. 在队列中 → 使用 queuelist (双向链表)
2. 批量处理/完成 → 使用 rq_next (单向链表)

典型转换场景

blk-flush.c:358-361 的注释说明了这一点：
```txt
/*
 * May have been corrupted by rq->rq_next reuse, we need to
 * re-initialize rq->queuelist before reusing it here.
 */
INIT_LIST_HEAD(&rq->queuelist);
```

当 request 从 rq_list (使用 rq_next) 返回到队列时，需要重新初始化 queuelist。

批量完成 blk-mq.c:blk_mq_end_request_batch 中:
```c
while ((rq = rq_list_pop(&iob->req_list)) != NULL) {
    prefetch(rq->bio);
    prefetch(rq->rq_next);  // 预取下一个，提高性能
    blk_complete_request(rq);
}
```

## 再综合一下这个分析
<!-- d8b42a95-929b-4dcb-b40d-444885cd3332 -->

- 如何进入到软件队列，如何从软件队列离开的?

```c
static void blk_mq_insert_request(struct request *rq, blk_insert_t flags)
{
    struct request_queue *q = rq->q;
    struct blk_mq_ctx *ctx = rq->mq_ctx;
    struct blk_mq_hw_ctx *hctx = rq->mq_hctx;

    // 1. 将 request 标记为已使用调度器
    rq->rq_flags |= RQF_USE_SCHED;

    // 2. 根据是否有调度器选择路径
    if (q->elevator) {
        // 使用 IO 调度器
        q->elevator->type->ops.insert_requests(hctx, &rq->queuelist, flags);
    } else {
        // 无调度器，直接插入软件队列
        spin_lock(&ctx->lock);
        list_add_tail(&rq->queuelist, &ctx->rq_lists[hctx->type]);
        spin_unlock(&ctx->lock);

        // 标记该 ctx 有待处理请求
        blk_mq_hctx_mark_pending(hctx, ctx);
    }
}

// block/blk-mq-sched.c
void blk_mq_sched_dispatch_requests(struct blk_mq_hw_ctx *hctx)
{
    struct request_queue *q = hctx->queue;
    struct elevator_queue *e = q->elevator;
    LIST_HEAD(rq_list);

    // 1. 首先处理 dispatch list（之前失败的请求）
    if (!list_empty(&hctx->dispatch)) {
        list_splice_init(&hctx->dispatch, &rq_list);
        blk_mq_dispatch_rq_list(hctx, &rq_list, false);
        return;
    }

    // 2. 从调度器获取请求
    if (e && e->type->ops.dispatch_request) {
        e->type->ops.dispatch_request(hctx);
        return;
    }

    // 3. 无调度器，直接从软件队列获取
    blk_mq_flush_busy_ctxs(hctx, &rq_list);
    blk_mq_dispatch_rq_list(hctx, &rq_list, false);
}
```

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          IO 提交流程 (submit_bio)                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  1. 用户发起 IO                                                                  │
│     │                                                                           │
│     ▼                                                                           │
│  2. VFS -> Filesystem -> 构造 bio                                               │
│     │  (bi_opf: READ/WRITE, 目标 sector, bv_page...)                           │
│     ▼                                                                           │
│  3. submit_bio() -> blk_mq_submit_bio()                                         │
│     │                                                                           │
│     ├──▶ 尝试请求合并 (bio merge)                                               │
│     │     - 检查相邻 sector 是否可以合并                                         │
│     │     - 成功则更新现有 request，返回                                         │
│     │                                                                           │
│     ├──▶ 获取 request (blk_mq_get_new_requests)                                │
│     │     - 从 mempool 分配 request 结构                                         │
│     │     - 分配 tag（标识此请求）                                               │
│     │                                                                           │
│     ├──▶ 初始化 request                                                         │
│     │     - 关联 bio 链表                                                        │
│     │     - 设置 mq_ctx (当前 CPU 的软件队列)                                    │
│     │     - 计算 mq_hctx (目标硬件队列)                                          │
│     │                                                                           │
│     └──▶ 选择路径                                                               │
│           │                                                                     │
│           ├─[有调度器]─▶ 插入调度器队列 (elv->ops.insert_requests)             │
│           │                - 按调度策略排序                                      │
│           │                - mq-deadline: 按截止时间排序                         │
│           │                                                                     │
│           └─[无调度器]─▶ 直接发送或插入 ctx 队列                                │
│                            - 尝试直接 issue (__blk_mq_issue_directly)          │
│                            - 或插入软件队列等待批量处理                          │
│                                                                                 │
│  4. 触发队列运行 (blk_mq_run_hw_queue)                                          │
│     │                                                                           │
│     ├──▶ 同步运行（async=false）                                                │
│     │     - 当前上下文直接处理                                                   │
│     │                                                                           │
│     └──▶ 异步运行（async=true）                                                 │
│           - 调度 kblockd 工作队列                                                │
│           - blk_mq_dispatch_work() 异步执行                                      │
│                                                                                 │
│  5. 请求分发 (blk_mq_sched_dispatch_requests)                                   │
│     │                                                                           │
│     ├──▶ 先处理 hctx->dispatch（之前失败的请求）                                │
│     │                                                                           │
│     ├──▶ 从调度器获取请求（如果有调度器）                                       │
│     │     - deadline: 检查是否有过期请求                                         │
│     │     - 按优先级和扇区位置选择                                               │
│     │                                                                           │
│     └──▶ 从软件队列获取请求（无调度器）                                         │
│           - 遍历 ctx_map 找到有待处理请求的 CPU                                  │
│           - 批量获取请求                                                         │
│                                                                                 │
│  6. 驱动提交 (queue_rq)                                                         │
│     │                                                                           │
│     └──▶ 调用驱动的 ->queue_rq() 回调                                           │
│           - NVMe: 填充 submission queue entry                                   │
│           - 写 doorbell 通知控制器                                               │
│           - 返回 BLK_STS_OK / BLK_STS_RESOURCE                                  │
│                                                                                 │
│  7. 硬件执行                                                                     │
│     │                                                                           │
│     └──▶ SSD Controller 处理请求                                                │
│           - 从 NAND 读取/写入数据                                                │
│           - DMA 数据传输                                                         │
│           - 完成时写 completion queue                                            │
│                                                                                 │
│  8. 完成处理 (ISR -> blk_mq_complete_request)                                   │
│     │                                                                           │
│     ├──▶ 中断处理程序读取 CQ entry                                              │
│     │     - 通过 tag 找到对应 request                                            │
│     │                                                                           │
│     ├──▶ blk_mq_complete_request()                                              │
│     │     - 更新 request 状态为 MQ_RQ_COMPLETE                                  │
│     │                                                                           │
│     ├──▶ 软中断处理 (blk_done_softirq)                                          │
│     │     - 批量处理完成请求                                                     │
│     │     - 释放 tag                                                             │
│     │                                                                           │
│     └──▶ 回调通知上层                                                           │
│           - 调用 end_io()                                                        │
│           - bio_endio() -> 文件系统回调                                          │
│           - 唤醒等待的进程                                                       │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

阶段 1: 请求插入 (blk-mq.c:2613)

blk_mq_insert_request()
  ├─ 如果是 passthrough 请求 → 直接插入 hctx->dispatch
  ├─ 如果是 FLUSH 请求 → 插入 hctx->dispatch
  └─ 普通请求:
      └─ blk_mq_insert_requests()
          ├─ 如果硬件队列不忙 → 尝试直接下发 (blk_mq_try_issue_list_directly)
          └─ 否则 → 插入软件队列 ctx->rq_lists[type]└─ blk_mq_hctx_mark_pending() // 在 ctx_map 中标记

阶段 2: 请求调度

blk_mq_run_hw_queue()
  └─ blk_mq_sched_dispatch_requests()
      ├─ 从 hctx->dispatch 取出请求
      ├─ 从软件队列取出请求 (通过 ctx_map 查找)
      └─ 调用驱动的 queue_rq() 下发到硬件

CPU 到硬件队列映射 (blk-mq.h:473-477)
struct blk_mq_queue_map {
    unsigned int *mq_map;      // CPU ID → 硬件队列索引
    unsigned int nr_queues;    // 硬件队列数量
    unsigned int queue_offset; // 起始硬件队列偏移
};

ctx_map 位图 (blk-mq.c:71-86)
- 用于快速查找哪些软件队列有待处理请求
- 避免遍历所有软件队列
- 使用 sbitmap_set_bit() 标记，sbitmap_clear_bit() 清除

直接下发优化 (blk-mq.c:2587-2591)
if (!hctx->dispatch_busy && !run_queue_async) {
    blk_mq_try_issue_list_directly(hctx, list);
    // 如果成功，跳过软件队列，直接发送到硬件
}

5. 队列类型 (blk-mq.h:486-492)

enum hctx_type {
    HCTX_TYPE_DEFAULT,  // 默认 I/O
    HCTX_TYPE_READ,     // 只读 I/O
    HCTX_TYPE_POLL,     // 轮询 I/O
};


这个就是插入软件队列吗?
```sh
sudo bpftrace -e '
kprobe:blk_mq_insert_request {
    $rq = (struct request *)arg0;
    printf("Insert request: rq=%p, ctx=%p, hctx=%p, cpu=%d\n",
           $rq, $rq->mq_ctx, $rq->mq_hctx, $rq->mq_ctx->cpu);
}
'
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
