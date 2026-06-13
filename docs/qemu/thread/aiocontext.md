## AioContext

通过 aio_set_fd_handler /  qemu_set_fd_handler => g_source_add_poll 可以将 fd 添加到 AioContext::source 上，
而 AioContext::source 总是会进一步地通过 g_source_attach 被关联到 GMainContext 上。

- create_aio_contexts
    - iothread_new
      - 使用 qemu_thread_create 创建一个 thread，在该线程中间执行 iothread_run
      - iothread_run 中间创建出来两个 context 来:
          - aio_context_new :
          - iothread_init_gcontext : 创建 GMainContext
    - aio_bh_schedule_oneshot : 其实就是前面的 aio bh 添加到 queue 上的操作
- join_aio_contexts : io thread 可以动态销毁的

```c
static void iothread_init_gcontext(IOThread *iothread)
{
    GSource *source;

    iothread->worker_context = g_main_context_new();                        // 创建 GMainContext
    source = aio_get_g_source(iothread_get_aio_context(iothread));          // 获取 AioContext 里面的 GSource
    g_source_attach(source, iothread->worker_context);                      // 将 GSource 关联到 GMainContext 上
    g_source_unref(source);
    iothread->main_loop = g_main_loop_new(iothread->worker_context, TRUE);  // GMainLoop 和这个创建的 GSource 来放到一起的
}
```

- aio_add_ready_handler 等机制不是遍历所有的 fd 而只是遍历 ready 的 fd

## AioContext 中到底包含了什么东西

- `aio_context_new`
  - `g_source_new`
  - `aio_set_event_notifier`
    - `aio_set_fd_handler`


```c
AioContext *aio_context_new(Error **errp)
{
    // ...

    // TODO 的确需要理解下这个
    ctx->co_schedule_bh = aio_bh_new(ctx, co_schedule_bh_cb, ctx);
    QSLIST_INIT(&ctx->scheduled_coroutines);

    aio_set_event_notifier(ctx, &ctx->notifier,
                           aio_context_notifier_cb,
                           aio_context_notifier_poll,
                           aio_context_notifier_poll_ready);

```


## unit test 还是需要看看的

tests/unit/iothread.c

tests/unit/test-aio.c
tests/unit/test-aio-multithread.c
tests/unit/test-block-iothread.c

这么看，QEMUBH 和 coroutine 没有关系:

- test_bh_schedule
  - aio_poll
    - aio_bh_poll
      - bh_test_cb

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
                    - aio_bh_poll


## aio 基本工作原理

```txt
🧀  qemu-system-x86_64 -object iothread,help
iothread options:
  aio-max-batch=<int>
  poll-grow=<int>
  poll-max-ns=<int>
  poll-shrink=<int>
  thread-pool-max=<int>
  thread-pool-min=<int>
```

似乎 demo 就是这么简单的?
```txt
AioContext *ctx = aio_context_new(&error_fatal);
int fd = open("/dev/sda", O_RDWR);
aio_set_fd_handler(ctx, fd, my_read_handler, my_write_handler, NULL);
aio_poll(ctx, true); // 运行事件循环
```

block/io_uring.c 和 block/linux-aio.c 中，总是通过 aio_set_fd_handler 来注册:
```c
void luring_attach_aio_context(LuringState *s, AioContext *new_context)
{
    s->aio_context = new_context;
    s->completion_bh = aio_bh_new(new_context, qemu_luring_completion_bh, s);
    aio_set_fd_handler(s->aio_context, s->ring.ring_fd,
                       qemu_luring_completion_cb, NULL,
                       qemu_luring_poll_cb, qemu_luring_poll_ready, s);
}
```

先看看两个 callback 的执行原理:

- thread_start
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - qemu_luring_completion_cb : 只有 fd handler 的工作

luring_process_completions 中

真的很经典的问题:
```sh
	arg_virtio_blk+="-object iothread,id=io0 "
	arg_virtio_blk+="-device virtio-blk,drive=virtio-blk1,id=virt-blk1,iothread=io0 "
	arg_virtio_blk+="-drive file=${virtio_blk_1},format=qcow2,if=none,id=virtio-blk1,aio=io_uring "
```

- coroutine_trampoline
  - blk_aio_write_entry
    - blk_co_do_pwritev_part
      - bdrv_co_pwritev_part
        - bdrv_aligned_pwritev
          - bdrv_driver_pwritev
            - qcow2_co_pwritev_part
              - qcow2_alloc_host_offset
                - handle_copied
                  - get_cluster_table
                    - l2_allocate
                      - qcow2_alloc_clusters
                        - alloc_clusters_noref
                          - qcow2_get_refcount
                            - qcow2_cache_get
                              - qcow2_cache_do_get
                                - bdrv_pread
                                  - bdrv_co_pread
                                    - bdrv_co_preadv
                                      - bdrv_co_preadv_part
                                        - bdrv_aligned_preadv
                                          - bdrv_driver_preadv
                                            - raw_co_preadv
                                              - raw_co_prw
                                                - raw_check_linux_io_uring
                                                  - aio_setup_linux_io_uring
                                                    - luring_attach_aio_context

- __clone3
  - start_thread
    - qemu_thread_start,
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - qemu_luring_poll_ready
              - qemu_luring_completion_cb

如果不去使用 iothread ，结果为:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked,
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_handlers
                      - aio_dispatch_handler
                        - qemu_luring_completion_cb

那么这里的 luring 到底是做什么的？

```txt
-   60.89%     0.00%  qemu-system-x86  libc.so.6                             [.] 0x00007ffff654c94f                                               ▒
   - 0x7ffff654c94f                                                                                                                               ▒
      - 60.87% coroutine_trampoline                                                                                                               ▒
         - 59.97% blk_aio_write_entry                                                                                                             ▒
            - 53.26% blk_co_do_pwritev_part                                                                                                       ▒
               - 51.64% bdrv_co_pwritev_part                                                                                                      ▒
                  - 48.61% bdrv_aligned_pwritev                                                                                                   ▒
                     - 47.83% bdrv_driver_pwritev                                                                                                 ▒
                        - 47.65% qcow2_co_pwritev_part                                                                                            ▒
                           - 42.92% qcow2_add_task                                                                                                ▒
                              - 42.44% qcow2_co_pwritev_task_entry                                                                                ▒
                                 - 42.38% qcow2_co_pwritev_task (inlined)                                                                         ▒
                                    - 41.31% bdrv_co_pwritev_part                                                                                 ▒
                                       - 37.68% bdrv_aligned_pwritev                                                                              ▒
                                          - 36.26% bdrv_driver_pwritev                                                                            ▒
                                             - 35.66% raw_co_pwritev                                                                              ▒
                                                - 35.58% raw_co_prw                                                                               ▒
                                                   - 35.02% luring_co_submit                                                                      ▒
                                                      - 34.16% luring_do_submit (inlined)                                                         ▒
                                                         - 33.95% ioq_submit                                                                      ▒
                                                            - 33.33% io_uring_submit                                                              ▒
                                         0.86% tracked_request_end                                                                                ▒
                                         0.54% tracked_request_begin                                                                              ▒
                           - 2.98% qcow2_alloc_host_offset
                              - 2.75% handle_copied (inlined)                                                                                     ▒
                                   1.10% get_l2_entry (inlined)                                                                                   ▒
                                 - 0.93% get_cluster_table                                                                                        ▒
                                      0.71% qcow2_cache_do_get                                                                                    ▒
                    0.71% tracked_request_end                                                                                                     ▒
            - 6.55% blk_aio_complete (inlined)                                                                                                    ▒
               - blk_aio_complete (inlined)                                                                                                       ▒
                  - 5.99% virtio_blk_rw_complete                                                                                                  ▒
                     - 3.19% virtio_blk_req_complete                                                                                              ▒
                        - 2.91% virtqueue_push                                                                                                    ▒
                           - 1.82% virtqueue_fill                                                                                                 ▒
                              - 1.50% virtqueue_unmap_sg (inlined)                                                                                ▒
                                 - 1.42% dma_memory_unmap (inlined)                                                                               ▒
                                    - 1.36% address_space_unmap                                                                                   ▒
                                         0.59% object_unref                                                                                       ▒
                     - 1.27% block_account_one_io                                                                                                 ▒
                        - 0.57% qemu_clock_get_ns                                                                                                 ▒
                           - get_clock (inlined)                                                                                                  ▒
                                0.52% clock_gettime@@GLIBC_2.17                                                                                   ▒
                     - 0.55% g_free                                                                                                               ▒
                          0.52% cfree@GLIBC_2.2.5
```
利用上，qemu gdb 脚本，也就是 qemu bt ，我们可以找到如下的 backtrace

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - virtio_queue_notify_vq
                - virtio_blk_handle_vq
                  - virtio_blk_submit_multireq
                    - blk_aio_pwritev
                      - blk_aio_prwv
                        - qemu_aio_coroutine_enter
                          - qemu_coroutine_switch
                            - ??
                              - ?? (也就是在这个等待过程)
                                - ??
                                  - coroutine_trampoline
                                    - blk_aio_write_entry
                                      - blk_aio_complete

之所以切换回来，是由于这个原因:

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - qemu_luring_poll_ready
                - luring_process_completions
                  - qemu_aio_coroutine_enter
                    - qemu_coroutine_switch



## 移除掉 AioContext 中的 lock

- aio context 中这两个函数被去掉了:
    - `aio_context_acquire`
    - `aio_context_release`
- `cpu_synchronize_all_post_init`


- http://blog.vmsplice.net/2024/01/qemu-aiocontext-removal-and-how-it-was.html
- https://kvm-forum.qemu.org/2023/Multiqueue_in_the_block_layer_wLom4Bt.pdf
  - graph 什么意思?
    - 原来是在 block/graph-lock.c 这个文件里面
- https://news.ycombinator.com/item?id=38847732
  - 关于 blog 的讨论

移除过程发生在，包含多个 commit
commit b6948ab01df0 ("virtio-blk: add iothread-vq-mapping parameter")

原始的 commit message
- https://lore.kernel.org/all/20231129195553.942921-6-stefanha@redhat.com/

### IOThread Virtqueue Mapping: Improving virtio-blk SMP scalability in QEMU
<!-- 139b7d9d-2b24-49b0-8a38-36ef6d11eb85 -->
- https://pretalx.com/kvm-forum-2024/talk/WATK7U/

https://developers.redhat.com/articles/2024/09/05/scaling-virtio-blk-disk-io-iothread-virtqueue-mapping#how_iothread_virtqueue_mapping_works


## main-loop.c 中的两个 AioContext 和一个 glib
<!-- 44da9544-dced-4fa1-897f-f84cf4a714f2 -->
```c
static AioContext *qemu_aio_context;
static AioContext *iohandler_ctx;
```

http://blog.vmsplice.net/2020/08/qemu-internals-event-loops.html

The reason why the main loop has two AioContexts is because
1. one, called **iohandler_ctx**, is used to implement older `qemu_set_fd_handler()` APIs whose handlers should
not run when the other AioContext, called **qemu_aio_context**, is run using aio_poll().
The QEMU block layer and newer code uses **qemu_aio_context** while older code uses **iohandler_ctx**.
Over time it may be possible to unify the two by converting iohandler_ctx handlers to safely execute in **qemu_aio_context**.

IOThreads have an AioContext and a glib GMainContext.
The AioContext is run using the aio_poll() API, which enables the advanced features of the event loop.
If a glib event loop is needed then the GMainContext can be run using g_main_loop_run() and the AioContext event sources will be included.

Code that relies on the AioContext `aio_*()` APIs will work with both the main loop and IOThreads.
Older code using `qemu_*()` APIs only works with the main loop. glib code works with both the main loop and IOThreads.

总结就是，由于各种历史兼容性原因，导致有两个 AioContext 和 glib

## iothread

IOThread 的关联文件为 iothread.c, 内容非常短，IOThread 的实现也很容易:
```c
struct IOThread {
    AioContext *ctx;
    GMainContext *worker_context;
    GMainLoop *main_loop;

    QemuThread thread;
    QemuMutex init_done_lock;
    QemuCond init_done_cond;    /* is thread initialization done? */
    bool stopping;
};
```
IOThread 的核心流程 iothread_run 主要就是通过 aio_poll 进行 listen，如果有 fd ready, 那么
那么执行对应 AioHandler::io_read 和 AioHandler::io_write 这些注册的 hook

iothread_run 中实际上会首先使用 aio_poll 然后 g_main_loop_run 来监听。
```c
/*
 * Note: from functional-wise the g_main_loop_run() below can
 * already cover the aio_poll() events, but we can't run the
 * main loop unconditionally because explicit aio_poll() here
 * is faster than g_main_loop_run() when we do not need the
 * gcontext at all (e.g., pure block layer iothreads).  In
 * other words, when we want to run the gcontext with the
 * iothread we need to pay some performance for functionality.
 */
```

```c
static bool virtio_blk_vq_aio_context_init(VirtIOBlock *s, Error **errp)
{
    // ...
    if (conf->iothread_vq_mapping_list) {
        if (!iothread_vq_mapping_apply(conf->iothread_vq_mapping_list,
                                       s->vq_aio_context,
                                       conf->num_queues,
                                       errp)) {
            g_free(s->vq_aio_context);
            s->vq_aio_context = NULL;
            return false;
        }
    } else if (conf->iothread) {
        AioContext *ctx = iothread_get_aio_context(conf->iothread);
        for (unsigned i = 0; i < conf->num_queues; i++) {
            s->vq_aio_context[i] = ctx;
        }

        /* Released in virtio_blk_vq_aio_context_cleanup() */
        object_ref(OBJECT(conf->iothread));
    } else {
        AioContext *ctx = qemu_get_aio_context();
        for (unsigned i = 0; i < conf->num_queues; i++) {
            s->vq_aio_context[i] = ctx;
        }
    }

}
```

VirtIOBlockDataPlane::ctx 的赋值根据 conf 是否有 iothread
- 有 : 通过 iothread_get_aio_context 获取 IOThread::ctx
- 没有 : 通过 qemu_get_aio_context 得到的是 main loop thread 中 qemu_aio_context

aio_set_fd_handler 设置 fd 的时候，使用 VirtIOBlockDataPlane::ctx 作为参数，
所以如果配置了 IOThread, 那么这些 fd 将别 IOThread 来监听。


## Question
- [ ] 似乎 io 是可以划分为 external 的，例如 aio_node_check


## chatgpt 关于 AioContext 的理解
<!-- 1a994729-c16a-43d9-941f-1148a97956a3 -->

在 QEMU 中引入 **AioContext**，本质上是为了解决 **“异步 I/O 的并发、安全与可扩展性”** 这一组长期存在且相互耦合的问题。它不是一个单点优化，而是一个**并发模型与事件驱动架构的重构核心**。

下面从“问题 → 设计目标 → AioContext 如何解决 → 没有它会怎样”四个层面系统说明。

---

## 一、QEMU 在引入 AioContext 之前面临的核心问题

### 1. I/O 回调在“错误的线程”执行

早期 QEMU 的模型是：

* 设备模型、Block layer、网络后端
* 都假定运行在 **主线程（main loop / iothread）**
* 异步 I/O（libaio、POSIX AIO、fd ready）完成后：

  * 回调被投递到**全局的主事件循环**

问题在于：

* 多个 vCPU 线程
* 多个 block backend
* 多个 network backend

都在**争夺同一个事件循环和 BQL（Big QEMU Lock）**

结果：

* 回调顺序不可控
* 容易发生死锁
* 性能被主线程串行化

---

### 2. Block I/O 无法安全地多线程化

典型矛盾：

* **Block layer 想并行**
* **设备模型和状态访问不允许并行**

例如：

```
vCPU thread
  └── 发起磁盘请求
        └── libaio 提交
              └── 完成回调
                    └── 修改 BlockDriverState
```

如果回调在 **任意线程** 执行：

* 必须加大量锁
* 或退化为全部回到主线程

两种都不可接受。

---

### 3. 事件源过多、生命周期复杂

QEMU 有大量异步事件源：

* fd 可读/可写
* timer
* bh（bottom half）
* aio completion
* coroutine resume

在旧模型中：

* 这些事件散落在全局 main loop
* 无法清晰表达：

  > “**这组事件属于哪个线程、哪个 I/O 域？**”

---

### 4. iothread / vCPU / 主线程模型不可扩展

随着需求演进：

* 多磁盘 → 多 iothread
* vCPU 与 I/O 解耦
* 热迁移 / block job / mirror

需要一种机制来表达：

> **“这个 I/O 对象，只能在某个线程中被访问和回调”**

旧模型无法表达这个约束。

---

## 二、AioContext 的设计目标（它“想解决什么”）

AioContext 不是“异步 I/O 本身”，而是：

> **一个“异步事件 + 回调的执行域（Execution Context）”**

设计目标可以总结为 5 点：

1. **明确 I/O 回调在哪个线程执行**
2. **让 block layer 可以真正并行**
3. **减少全局锁（尤其是 BQL）**
4. **把事件循环“分区”**
5. **为 coroutine / block job / iothread 提供统一语义**

---

## 三、AioContext 是什么（一句话定义）

> **AioContext 是 QEMU 中“绑定到某个线程的事件循环 + 异步 I/O 完成域”。**

更准确地说：

* 每个 AioContext：

  * 有自己的 event loop
  * 有自己的 fd / timer / bh / aio completion
  * **只能被一个线程 drive**
* 一个 BlockBackend / device / job：

  * **绑定到某一个 AioContext**
  * 所有回调 **保证在该 context 所在线程执行**

---

## 四、AioContext 具体解决了哪些问题

### 1. 明确“回调在哪个线程跑”

关键保证：

```
提交 I/O 的线程 ≠ 回调执行线程
但：
回调一定在 AioContext 所属线程执行
```

这带来的直接好处：

* Block layer 可以：

  * 假设“我在自己的线程”
  * 不需要为每个字段加锁
* coroutine resume 是安全的
* 设备模型不再被随机线程打断

---

### 2. Block layer 真正实现并行

现在的结构是：

```
vCPU thread
  └── submit → BlockBackend
        └── aio_submit → AioContext_X
              └── completion → AioContext_X thread
                    └── 回调 / coroutine resume
```

如果你有：

* 多个磁盘
* 多个 iothread
* 每个 iothread 一个 AioContext

那么：

* I/O completion 并行
* block job 并行
* 不再全部回到主线程

---

### 3. 事件源被“域化”（scoping）

AioContext 内部统一管理：

* aio handlers
* fd handlers
* timers
* bh
* coroutine scheduling

这意味着：

* 事件**不再是全局的**
* 生命周期与 I/O 对象一致
* 销毁 BlockBackend 时可以安全清理所有 pending 事件

---

### 4. iothread 成为“有意义的实体”

iothread 本质上是：

```
Thread
  └── run loop
        └── aio_poll(ctx)
```

没有 AioContext：

* iothread 只是个空壳
* 还得回 main loop

有了 AioContext：

* iothread == 独立 I/O 世界
* 可以完全不碰主线程

---

### 5. 为 coroutine 提供“合法的调度点”

QEMU coroutine 的一个硬约束是：

> **resume 必须在创建它的 AioContext 中发生**

AioContext 明确了：

* coroutine 属于哪个执行域
* yield / resume 的合法线程

没有它：

* coroutine 会变成不可维护的隐式假设集合

---

## 五、如果没有 AioContext，会怎样？

如果没有 AioContext：

1. block layer 必须：

   * 要么全局锁
   * 要么回主线程
2. iothread 没有实际隔离意义
3. coroutine resume 不安全
4. block job / mirror / migration 的并发性不可实现
5. QEMU 不可能扩展到今天的规模

可以说：

> **没有 AioContext，就没有现代 QEMU 的 block / iothread / coroutine 架构。**

---

## 六、一句话总结（给内核 / 虚拟化开发者的版本）

> **AioContext 的引入，是为了解决 QEMU 在多线程环境下“异步 I/O 回调归属不明确、事件循环全局化、block layer 无法并行”的根本性架构问题。它将 I/O 完成、事件调度和线程执行域绑定在一起，是 QEMU 并发模型的基石。**

如果你愿意，下一步我可以从源码角度拆：

* `aio_context_acquire/release` 为什么存在
* AioContext 与 BQL 的边界
* block job 为什么频繁切换 AioContext
* AioContext 与 io_uring / libaio 的关系


## gdb qemu 的辅助脚本
<!-- 6f7f18f4-345b-4ccb-9dc4-40bb5977dc5a -->

在 QEMU 中存在对应的脚本，在 gdb 的 cmdline 中使用 help qemu 可以有:
```txt
qemu bt -- Display backtrace including coroutine switches
qemu coroutine -- Display coroutine backtrace
qemu handlers -- Display aio handlers
qemu mtree -- Display the memory tree hierarchy
qemu tcg-lock-status -- Display TCG Execution Status
qemu timers -- Display the current QEMU timers
```
除掉 qemu bt ，其他的几个在调试 qemu 的 thread 也是有价值的

qemu mtree 的使用有 bug
```txt
qemu mtree
Traceback (most recent call last):
  File "/home/martins3/data/qemu/scripts/qemugdb/mtree.py", line 40, in invoke
    self.process_queue()
    ~~~~~~~~~~~~~~~~~~^^
  File "/home/martins3/data/qemu/scripts/qemugdb/mtree.py", line 49, in process_queue
    self.print_item(ptr)
    ~~~~~~~~~~~~~~~^^^^^
  File "/home/martins3/data/qemu/scripts/qemugdb/mtree.py", line 66, in print_item
    int(addr + (size - 1)),
        ~~~~~^~~~~~~~~~~~
OverflowError: int too big to convert
Error occurred in Python: int too big to convert
```

原理参考：https://mp.weixin.qq.com/s/R0Ja-0HXdZyNSkpM2Y6Gsg

qemu handlers 的意义非凡:

简单来说，就是 `struct AioHandler` 的 dump
但是为什么我总是有一个理解，那就是实现监听的方法不只是有这一个的，或者说，
所有的被 epoll 监听的 fd 都是需要定义一个 `struct AioHandler` 吗?
```txt
{
  pfd = {
    fd = 177,
    events = 25,
    revents = 0
  },
  io_read = 0x555555d52570 <virtio_queue_host_notifier_read>,
  io_write = 0x0,
  io_poll = 0x555555d53e20 <virtio_queue_host_notifier_aio_poll>,
  io_poll_ready = 0x555555d525b0 <virtio_queue_host_notifier_aio_poll_ready>,
  io_poll_begin = 0x555555d53be0 <virtio_queue_host_notifier_aio_poll_begin>,
  io_poll_end = 0x555555d53bb0 <virtio_queue_host_notifier_aio_poll_end>,
  opaque = 0x55555853dbbc,
  node = {
    le_next = 0x7fff0c4ba1d0,
    le_prev = 0x7fff0c4ba3b0
  },
  node_ready = {
    le_next = 0x0,
    le_prev = 0x0
  },
  node_deleted = {
    le_next = 0x0,
    le_prev = 0x0
  },
  node_poll = {
    le_next = 0x7fff0c4ba1d0,
    le_prev = 0x7fff0c4ba3e0
  },
  node_submitted = {
    sle_next = 0x0
  },
  flags = 0,
  internal_cqe_handler = {
    cb = 0x555555f14120 <fdmon_special_cqe_handler>,
    next = {
      sqe_next = 0x0
    },
    cqe = {
      user_data = 0,
      res = 0,
      flags = 0,
      big_cqe = 0x7fff0c4ba350
    }
  },
  poll_idle_timeout = 0,
  poll_ready = false,
  poll = {
    ns = 0
  }
}
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
