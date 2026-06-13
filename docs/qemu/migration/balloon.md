• 严格说，热迁移里“跳过 balloon 页”主要不是靠传统 balloon inflate 本身完成的，而是靠 virtio-balloon 的 free-page-hint 迁移优化。

  传统 balloon inflate 这条路里，guest 把 PFN 交给 balloon 后，QEMU 在源端只是把对应宿主页做 discard，回收宿主内存
  balloon_inflate_page() 里直接调用 ram_block_discard_range()；底层通常走 madvise(DONTNEED) 或 fallocate(PUNCH_HOLE)，
  是“把 host backing 扔掉”，不是直接改迁移位图。

  真正让 precopy “别发这些页”的，是 free-page-hint。QEMU 在每轮 precopy 的 dirty bitmap sync 前后插了 notifier
  ：sync 前先停 hint，sync 后再让 guest 开始上报当前 free pages，具体看 virtio_balloon_free_page_hint_notify()。

  guest 通过 free-page virtqueue 把空闲页范围发给 QEMU，QEMU 收到后调用
  get_free_page_hints -> qemu_guest_free_page_hint(addr, len)。

qemu_guest_free_page_hint() 做的事就是核心：
	- 把这些页从迁移 dirty bitmap rb->bmap 里清掉；
	- 同时把对应底层 dirty log 也清掉，避免下一轮 sync 又把它们重新加回来；
	- 于是这些页后续就不会进入发送队列了。 具体看 qemu_guest_free_page_hint()

另外还有一条独立路径是 RamDiscardManager：对“逻辑上已 unplug/discard”的内存范围，迁移会直接把这些范围从 dirty bitmap 中排除，
具体看 ramblock_dirty_bitmap_clear_discarded_pages() 像 virtio-mem/sparse RAM 的语义，不是普通 balloon free-page-hint 本身。

问题:
首先，这里为什么存在一个单独的机制? 还搞的这么复杂
2. virtio_balloon_free_page_hint_notify 的作用到底是什么?
3. 哦，我意识到，现在的热迁移机制本来就可以使用这个来搞了。


---

  这里的 sync 指的是 migration_bitmap_sync_precopy() 里的 dirty log 同步，也就是把底层脏页记录拉进 QEMU 的迁移位图 rb->bmap
 也就是 migration/ram.c:migration_bitmap_sync_precopy()
  调用顺序是：

  1. PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC
  2. migration_bitmap_sync(...)
  3. PRECOPY_NOTIFY_AFTER_BITMAP_SYNC

  virtio_balloon_free_page_hint_notify() 就挂在这两个点上。hw/virtio/virtio-balloon.c:649

  更具体地说：

  - BEFORE_BITMAP_SYNC 时，它调用 virtio_balloon_free_page_stop()，把状态设成 S_STOP。
  - 然后迁移线程去做这一轮 dirty bitmap 同步。migration/ram.c:1193
  - AFTER_BITMAP_SYNC 时，如果 VM 还在运行，就调用 virtio_balloon_free_page_start()，把状态设成 S_REQUESTED，通过 config notify 告诉 guest 开启一轮新的 free-page
    hint。

  所以它本质上是在划分一个 epoch：

  - 先冻结 hinting
  - 做一次“官方的” dirty bitmap 快照
  - 再重新开始收这一轮新的 free-page hints

  这样做的目的，是避免 free-page hint 和 dirty bitmap sync 交错，导致页被重复加入、漏清，或者跨轮次混在一起。


因为 get_free_page_hints() 即使被调用，也只有在状态是 S_START 时，才会对收到的 in_sg 调 qemu_guest_free_page_hint()。
而 qemu_guest_free_page_hint() 才是真正把这些页从 rb->bmap 里清掉的地方。

也就是 get_free_page_hints 中的结果:
```c
    if (elem->in_num && dev->free_page_hint_status == FREE_PAGE_HINT_S_START) {
        for (i = 0; i < elem->in_num; i++) {
            qemu_guest_free_page_hint(elem->in_sg[i].iov_base,
                                      elem->in_sg[i].iov_len);
        }
    }
```

## 问题
- 那些没有 touch 的页面，都是如何跳过的?



- [ ] 迁移的时候，guest 没有使用的页不用发送的?
  - 似乎比到 proc/pid/map 下去检查更加好的
    - 怀疑，qemu 中是否实现过这个功能

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
