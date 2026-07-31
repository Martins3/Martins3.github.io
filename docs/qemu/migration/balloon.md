## migration 的通知机制就是为了
- 注册：启用 VIRTIO_BALLOON_F_FREE_PAGE_HINT 时调用 precopy_add_notifier()
  hw/virtio/virtio-balloon.c:894

- 回调处理：
  hw/virtio/virtio-balloon.c:655

- 设备销毁时注销：
  hw/virtio/virtio-balloon.c:925

各通知目前的用途：

```txt
 Reason                发送位置                              virtio-balloon 行为
━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 SETUP                 migration setup                       无操作
────────────────────  ────────────────────────────────────  ──────────────────────────────────
 BEFORE_BITMAP_SYNC    同步脏页 bitmap 前                    停止 guest 上报 free page
────────────────────  ────────────────────────────────────  ──────────────────────────────────
 AFTER_BITMAP_SYNC     同步完成后                            重新启动 free-page hint
────────────────────  ────────────────────────────────────  ──────────────────────────────────
 COMPLETE              precopy 切换完成                      无操作
────────────────────  ────────────────────────────────────  ──────────────────────────────────
 CLEANUP               migration 成功、失败或取消后的清理    通知 guest free-page hint 已结束
────────────────────  ────────────────────────────────────  ──────────────────────────────────
 MAX                   不发送                                仅作为枚举上界
```


核心目的，是防止 guest 异步上报空闲页和 migration dirty bitmap 同步产生竞态：

```txt
停止 free-page hint
        ↓
同步 dirty bitmap
        ↓
重新开启 free-page hint
```

另外，如果允许 postcopy，virtio-balloon 回调会直接跳过这个优化，
因为清掉 dirty bitmap 中的空闲页可能导致目标端 page fault 永久等待。
也就是说，这套 notifier 名字虽然很通用，但当前基本就是为 virtio-balloon free-page-hint 服务的；
SETUP 和 COMPLETE 目前甚至没有 实际消费者行为。

### 基本调用流程
这里的 sync 指的是 migration_bitmap_sync_precopy() 里的 dirty log 同步，也就是把底层脏页记录拉进 QEMU 的迁移位图 rb->bmap
 也就是 migration/ram.c:migration_bitmap_sync_precopy()

  1. PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC
  2. migration_bitmap_sync(...)
  3. PRECOPY_NOTIFY_AFTER_BITMAP_SYNC


## 遇到的问题和解决办法

可以。假设有一个 guest 物理页 P，migration bitmap 中：

- P = 1：需要迁移
- P = 0：可以跳过

考虑没有 notifier 协调时的简化时序：

```txt
Guest/balloon线程          KVM dirty log          Migration线程
      │                         │                       │
1. P 当前空闲
2. 异步上报“P 是空闲页”
      │
3. P 被重新分配并写入 ────────► P = dirty
      │
      │                                      4. 同步 dirty bitmap
      │                                         migration bitmap[P] = 1
      │
5. 迟到的 free-page hint 被处理
   migration bitmap[P] = 0
      │
      │                                      6. 看到 P=0，不发送 P
```

问题在第 5 步：这个 hint 描述的是“此前观察到 P 空闲”，但处理时 dirty bitmap 已经同步了更新的数据。
旧 hint 把刚同步出来的 dirty bit 又清掉，目标虚拟机就可能拿不到 P 的最新内容。

反方向也可能产生性能问题：

```txt
free-page hint：P 已空闲，清成 0
                  ↓
dirty bitmap 同步又合入旧的 dirty 记录，把 P 设成 1
                  ↓
本来可以跳过的空闲页仍然被发送
```

所以 QEMU 把 free-page hint 严格放在两次 bitmap sync 之间：

```txt
同步 dirty bitmap
        ↓
AFTER_BITMAP_SYNC (注意，这里是 after ，开启了 dirty 跟踪了)

启动 free-page hint
        ↓
guest 上报空闲页，QEMU 将会跳过这些页面 *
        ↓
BEFORE_BITMAP_SYNC
停止上报，并等 hint 处理线程退出
        ↓
下一次同步 dirty bitmap
```

对 * 标记的位置的分析:
1. AFTER_BITMAP_SYNC 后是开机了 dirty 跟踪的
2. 如果 free-page hint 报告了页面可以跳过，但是依旧在页面中 dirty write 了，那么
dirty 会记录到 KVM dirty bit 中，会在下一轮还是会记录下来


这样下一次同步开始以后，就不会有“上一轮迟到的 hint”再次清除同步结果。对应代码是：
- sync 前后发送通知：migration/ram.c:migration_bitmap_sync_precopy
- hint 清除 migration bitmap：migration/ram.c:qemu_guest_free_page_hint
- sync 前停止、sync 后启动：hw/virtio/virtio-balloon.c:virtio_balloon_free_page_hint_notify

这里不仅仅是 C 语言层面的数据竞争——bitmap_mutex 已经避免了同时修改数据结构；
更关键的是跨 guest、KVM dirty log 和 migration 线程的“事件先后顺序”竞争。
notifier 建立的是语义上的同步边界。

## 其他算法的考虑

1.对于 swap out 到共享存储页面也可以使用此方法

也就是如果检测到页面在 swap 中，那么就跳过

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
