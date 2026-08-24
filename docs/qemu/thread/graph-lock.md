## block/graph-lock.c

block/graph-lock.c 解决的是：QEMU Block Graph 在主线程动态修改时，
可能与 IOThread 中的 I/O 协程并发遍历，导致拓扑不一致、越界访问甚至 UAF（use-after-free）的问题。

### Block Graph 是什么

QEMU 块设备不是简单的一层磁盘，而是一张图：

virtio-blk
    │
    ▼
qcow2
    ├── file ──> raw/file/rbd
    └── backing ──> base image

其中：

- BlockDriverState 是节点。
- BdrvChild 是边。
- bs->children、bs->parents 保存拓扑关系。
- snapshot、mirror、commit、热插拔、替换 backing file 等操作会修改这张图。

源码也明确说明它保护节点和边的添加/删除：include/block/graph-lock.h:23。

### 原来的并发问题

Block Graph 的修改通常由持有 BQL 的主循环完成，但 I/O 协程可以在其他 IOThread/AioContext 中运行，它们并不受 BQL 串行化。

例如：

`txt
IOThread                         Main loop
--------                         ---------
读取 bs->children
                                 删除一个 BdrvChild
继续解引用 child
→ 可能访问已释放对象

或者 reader 可能看到修改到一半的关系：

child 已从 parent->children 删除
但 node->parents 尚未同步完成
`

过去这些访问顺带由 AioContext 锁保护。但 QEMU 为了改善多队列和多 IOThread 的并发性能，
逐步移除了 AioContext 大锁，所以必须用一个范围更准确的锁单独保护 Block Graph。

这正是原始提交 aead9dc9d1ac 的目的：

> once [the AioContext lock] is removed, we need to make sure that reads do not happen while modifying the graph.

### 它如何解决

它实现了一个针对 QEMU 协程模型定制的读写锁：

- 唯一 writer：主循环，持有 BQL，负责修改图。
- 多个 reader：不同 IOThread/AioContext 中的协程，负责遍历图。
- reader 可以并发执行。
- writer 必须等已有 reader 全部退出。
- writer 修改期间，新 reader 进入 CoQueue 睡眠。
- writer 完成后唤醒所有 reader。

reader 在 block/graph-lock.c:207 中增加当前 AioContext 的计数；发现 has_writer 后进入协程等待队列。

writer 在 block/graph-lock.c:120 中：

1. 暂停新 I/O，避免持续进入的 reader 导致 writer 饥饿。
2. 等待全局 reader 数变成 0。
3. 设置 has_writer，阻止新 reader。
4. 修改图。
5. 清除 has_writer 并唤醒 reader：block/graph-lock.c:173。

### 为什么不用普通 QemuRWLock

因为 reader 是协程。如果一个协程在普通 pthread rwlock 上阻塞，会把整个 IOThread 一起阻塞；而正在该 IOThread 上运行的其他协程可能正
是 writer 等待退出的 reader，于是可能死锁。

因此这里需要：

- reader 等待时只挂起当前协程；
- writer 等待时继续轮询 AIO，让旧 reader 有机会运行并退出；
- 每个 AioContext 使用独立 reader counter，减少共享 cache line 抖动：block/graph-lock.c:72。

另外，graph_lock 本身只是供 Clang Thread Safety Analysis 使用的虚拟锁对象；真正的运行时同步由 has_writer、各 AioContext 的
reader_count、内存屏障和 CoQueue 完成。

一句话概括：

> graph-lock.c 是在移除 AioContext 大锁之后，用协程友好的专用读写锁，保证 IOThread 遍历 Block Graph 与主线程修改 Block Graph 不会并
> 发发生。

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
