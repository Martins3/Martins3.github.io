# QEMU AIO 事件循环架构分析

## 概述

QEMU 提供了两种事件循环机制来处理异步 I/O 事件：

1. **直接调用 `aio_poll()`** - 独立高效的事件循环
2. **GSource dispatch** - 与 GLib 生态系统集成的事件循环

这两种机制底层共享相同的核心逻辑，但适用于不同的使用场景。

## 1. 机制一：直接 aio_poll()

### 代码路径

```c
// util/aio-posix.c
bool aio_poll(AioContext *ctx, bool blocking)
{
    qemu_lockcnt_inc(&ctx->list_lock);

    // 1. Userspace polling (可选优化)
    try_poll_mode(ctx, &ready_list, &timeout);

    // 2. 阻塞等待 I/O 事件 (epoll_wait/ppoll)
    ctx->fdmon_ops->wait(ctx, &ready_list, timeout);

    // 3. 执行 Bottom Halves
    aio_bh_poll(ctx);

    // 4. 分发 fd 事件
    aio_dispatch_ready_handlers(ctx, &ready_list, block_ns);

    qemu_lockcnt_dec(&ctx->list_lock);
}
```

### 使用场景

- **IOThread**: 专门的 I/O 线程使用 `aio_poll()` 独立运行事件循环
- **高性能路径**: 需要精确控制 poll 行为，避免额外开销
- **嵌套调用**: 支持在回调中递归调用 `aio_poll()`

## 2. 机制二：GSource Dispatch

### 代码路径

```c
// AioContext 本身就是一个 GSource
static GSourceFuncs aio_source_funcs = {
    .prepare = aio_ctx_prepare,      // 设置超时
    .check   = aio_ctx_check,        // 检查是否有事件
    .dispatch= aio_ctx_dispatch,     // 分发事件
    .finalize= aio_ctx_finalize
};

// 被 GLib 主循环调用
static gboolean aio_ctx_dispatch(GSource *source, ...)
{
    aio_dispatch((AioContext *)source);
    return true;
}

void aio_dispatch(AioContext *ctx)
{
    qemu_lockcnt_inc(&ctx->list_lock);

    aio_bh_poll(ctx);                    // 执行 BH

    ctx->fdmon_ops->gsource_dispatch(...); // 收集就绪 fd

    aio_dispatch_ready_handlers(...);    // 分发回调

    qemu_lockcnt_dec(&ctx->list_lock);
}
```

### 使用场景

- **QEMU 主线程**: 使用 `g_main_context_iteration()` 运行主循环
- **GLib 集成**: 与 GTK、DBus 等 GLib 组件共存
- **UI 应用**: 需要图形界面的事件循环集成

## 4. 为什么需要两个机制？

### 4.1 GLib 集成需求（主线程）

QEMU 主线程需要与 **GLib 生态系统** 深度集成：

- **GTK/UI 集成**: QEMU 的图形界面依赖 GLib 的主循环
- **网络协议**: 许多网络库使用 GLib 事件循环
- **DBus/系统服务**: 系统级集成需要 GLib
- **统一事件源**: 将 QEMU 的 AIO 事件与 GLib 的其他事件源统一管理

```c
// util/main-loop.c
static int os_host_main_loop_wait(int64_t timeout)
{
    GMainContext *context = g_main_context_default();
    g_main_context_acquire(context);
    glib_pollfds_fill(&timeout);
    // ...
}
```

## 5. 架构示意图

```
┌─────────────────────────────────────────────────────────────┐
│                      QEMU 进程                              │
├─────────────────────────────────────────────────────────────┤
│  主线程 (vl.c)                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  g_main_loop_run()                                  │    │
│  │       ↓                                             │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐   │    │
│  │  │  GTK 事件   │  │  DBus 事件  │  │ AioContext │   │    │
│  │  │   GSource   │  │   GSource   │  │  GSource   │   │    │
│  │  └─────────────┘  └─────────────┘  └────────────┘   │    │
│  │       ↑___________________________________________  │    │
│  │                    统一由 GLib 调度                 │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│  IOThread 1 (block/io.c)                                    │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  while (true) {                                     │    │
│  │      aio_poll(ctx, true);  // 直接调用，无 GLib    │    │
│  │  }                                                  │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│  IOThread 2                                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  while (true) {                                     │    │
│  │      aio_poll(ctx, true);  // 独立事件循环          │    │
│  │  }                                                  │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
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
