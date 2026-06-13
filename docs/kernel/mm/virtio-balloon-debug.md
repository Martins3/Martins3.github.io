# Virtio Balloon Debug
非常好，那么如何可以自动的利用这个东西呢?

```txt
static bool get_free_page_hints(VirtIOBalloon *dev)
{
    /* 等待 VM 恢复运行 */
    while (dev->block_iothread) {
        qemu_cond_wait(&dev->free_page_cond, &dev->free_page_lock);
    }

```

iothread 中的调用路线为:
- virtio_ballloon_get_free_page_hints
	- get_free_page_hints 的调用时机是什么?

这也是一个异步的事件了:
virtio_balloon_free_page_hint_notify : 这就是 callback

iothread 是一定必须使用的吗?

看上去的确是如此的? 是必须要 balloon 的才可以的
```txt
        s->free_page_bh = aio_bh_new_guarded(iothread_get_aio_context(s->iothread),
                                             virtio_ballloon_get_free_page_hints, s,
                                             &dev->mem_reentrancy_guard);
```


第三个问题，如果过程中，balloon 发生了变化，如何办?


还需要梳理一下在热迁移中的位置才可以:
migration_bitmap_sync_precopy

是划分为多个阶段的啊


那些 thread 可能会使用 bitmap_mutex 这个东西?

## 通知 guest 发送，需要等 guest 返回吗?

## 还是没懂，为什么需要在 sync 的时候不可以


## 理解一下这个东西

因为 memory_region_clear_dirty_bitmap() 清掉的是“当前已经记录的 dirty 状态”，不是永久禁止跟踪。
如果 guest 之后又写这个页，dirty logging 会重新把它记脏，下一轮 migration_bitmap_sync() 还能重新发现它。
所以 free-page hint 本质上是在说：

“截至现在，这页可视为 free，不值得迁移；如果以后又被用到，再重新记脏、再迁移。”

告诉你可以跳过，但是 clear_bmap 没有消失的。

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
