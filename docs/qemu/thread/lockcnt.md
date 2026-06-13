## qemu lockcounters
<!-- 0ed52d78-10a1-4a7d-9930-b6dc4a0ba311 -->

https://qemu.readthedocs.io/en/v10.0.3/devel/lockcnt.html

这个的确是不容易的

```c
static bool fdmon_poll_gsource_check(AioContext *ctx)
{
    AioHandler *node;
    bool result = false;

    /*
     * We have to walk very carefully in case aio_set_fd_handler is
     * called while we're walking.
     */
    qemu_lockcnt_inc(&ctx->list_lock);

    QLIST_FOREACH_RCU(node, &ctx->aio_handlers, node) {
        int revents = node->pfd.revents & node->pfd.events;

        if (revents & (G_IO_IN | G_IO_HUP | G_IO_ERR) && node->io_read) {
            result = true;
            break;
        }
        if (revents & (G_IO_OUT | G_IO_ERR) && node->io_write) {
            result = true;
            break;
        }
    }

    qemu_lockcnt_dec(&ctx->list_lock);

    return result;
}
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(感觉还是很懵逼啊)

1. 类型定义

ctx->list_lock 是 QemuLockCnt 类型（定义在 include/qemu/lockcnt.h），这是一种计数锁（counting lock），特点：
• 支持两种状态：locked（互斥锁） 和 incremented（计数访问）

2. 核心用途

根据 include/qemu/aio.h

```txt
/* A lock to protect between QEMUBH and AioHandler adders and deleter,
 * and to ensure that no callbacks are removed while we're walking and
 * dispatching them.
 */
QemuLockCnt list_lock;
```

主要保护对象：

 数据结构                    说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 ctx->aio_handlers           已注册的 AIO 处理器链表
 ctx->deleted_aio_handlers   待删除的 AIO 处理器链表
 ctx->poll_aio_handlers      参与 userspace polling 的处理器链表

3. 使用模式分析

3.1 写操作（需 Exclusive Lock）

```txt
// aio_set_fd_handler() - 注册/修改 fd 处理器
qemu_lockcnt_lock(&ctx->list_lock);
// ... 修改 aio_handlers ...
qemu_lockcnt_unlock(&ctx->list_lock);

// aio_context_destroy()
qemu_lockcnt_lock(&ctx->list_lock);
fdmon_epoll_disable(ctx);
qemu_lockcnt_unlock(&ctx->list_lock);
```

3.2 读操作（Increment/Decrement 模式）

```txt
// aio_poll() - 主事件循环
qemu_lockcnt_inc(&ctx->list_lock);
// ... 遍历 handlers ...
aio_free_deleted_handlers(ctx);
qemu_lockcnt_dec(&ctx->list_lock);

// aio_dispatch() - GLib 集成
qemu_lockcnt_inc(&ctx->list_lock);
// ... dispatch handlers ...
qemu_lockcnt_dec(&ctx->list_lock);

// poll_set_started() - 轮询控制
qemu_lockcnt_inc(&ctx->list_lock);
// 遍历 poll_aio_handlers
qemu_lockcnt_dec(&ctx->list_lock);
```

3.3 延迟删除机制

aio_remove_fd_handler() 实现了延迟删除策略：

```txt
static bool aio_remove_fd_handler(AioContext *ctx, AioHandler *node)
{
    // 如果有读者正在遍历（count > 0），则标记为删除
    if (qemu_lockcnt_count(&ctx->list_lock)) {
        QLIST_INSERT_HEAD_RCU(&ctx->deleted_aio_handlers, node, node_deleted);
        return false;  // 未真正删除
    }
    // 否则立即删除
    QLIST_REMOVE(node, node);
    return true;
}
```

真正的删除在 aio_free_deleted_handlers() 中完成：

```txt
static void aio_free_deleted_handlers(AioContext *ctx)
{
    // 只有最外层的调用者才能执行释放（count == 1）
    if (!qemu_lockcnt_dec_if_lock(&ctx->list_lock)) {
        return;  // 嵌套状态，让外层处理
    }
    // 释放所有标记为删除的 handlers
    qemu_lockcnt_inc_and_unlock(&ctx->list_lock);
}
```

设计亮点
1. 读写分离：inc/dec 允许多个读者并发，lock/unlock 提供独占写访问
2. 嵌套安全：支持嵌套调用（如 aio_poll() 中调用 poll_set_started()）
3. 延迟删除：避免在遍历过程中修改链表，防止 use-after-free
4. 与 GLib 集成：gsource_dispatch 等回调在 list_lock incremented 状态下执行

## 额外发现，qemu 可以通过 list_lock 来实现 epoll_wait 和 epoll_ctl 的互斥的

通过:
```txt
   时间   Thread A (aio_poll)                Thread B (aio_set_fd_handler)         状态
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   t1     qemu_lockcnt_inc()                                                       count=1
   t2     epoll_wait() 阻塞...                                                     count=1, 阻塞中
   t3                                        qemu_lockcnt_lock() → 阻塞            count=1, B 等待
   t4     epoll_wait() 返回 (事件/timeout)                                         count=1
   t5     qemu_lockcnt_dec()                                                       count=0, B 唤醒
   t6                                        获取锁，epoll_ctl() 执行              count=0, locked
   t7                                        qemu_lockcnt_unlock(), aio_notify()   count=0, unlocked
   t8     处理事件或返回
```

你的怀疑得到验证：
1. epoll_ctl 和 epoll_wait 确实不会并发执行 - 这是 QEMU 通过 list_lock 强制保证的，不是 Linux 的限制
3. aio_notify 的作用 - 在 qemu_lockcnt_unlock 之后调用，用于：
	- 唤醒正在阻塞的 epoll_wait（让 Thread A 尽快释放锁）
	- 确保下次 aio_poll 循环能处理新注册的 fd
4. 修改并非"立即"生效 - 从 epoll_ctl 调用到 epoll_wait 能看到新状态，中间可能有延迟（取决于 Thread A 何时再次调用 epoll_wait）

不过，我的确认同这个说法:

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
QEMU 的 list_lock 不仅保护 epoll 操作，还保护：

1. ctx->aio_handlers 链表的一致性
2. ctx->deleted_aio_handlers 延迟删除机制
3. 防止 AioHandler 结构体在遍历期间被释放
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
