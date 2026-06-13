## 在 blk-mq 里，shared tags 的意思
<!-- 4a314c6d-37f4-44a2-8bef-79ebd8e57478 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(不过， 我感觉就是这个意思)

多个硬件队列 hctx，不再各自拥有一套独立 tag bitmap，而是共用同一套 tags。

对应的开关是 include/linux/blk-mq.h:695。打开后：

- tag_set 只分配一份 set->shared_tags
  见 block/blk-mq.c:4681
- 每个 hctx->tags 都指向这同一个 shared_tags
  见 block/blk-mq.c:4130

所以它不是“每个硬件队列 128 个 tag”，而是“所有硬件队列一起竞争这 128 个 tag”。

可以把它理解成两种模式：

1. 非 shared tags
   每个 hctx 有自己独立的 tag 池。
   hctx0 用完自己的 tag，不影响 hctx1。
2. shared tags
   所有 hctx 共用一个全局 tag 池。
   hctx0 多占一些，就会挤压 hctx1 能拿到的 tag。

这通常出现在“底层硬件/host 真正能并发的命令总数是全局限制，而不是每队列独立限制”的设备上。SCSI host tagset 就会设置它，
见 drivers/scsi/scsi_lib.c:2130。

为什么需要它：

- 有些控制器的队列很多，但真正可用的命令槽位是全局共享的
- 如果还按“每个 hctx 一套 tag”建模，会高估并发能力
- shared tags 让 block layer 的 tag 分配更接近硬件真实资源模型

但共享后会带来一个问题：某个活跃队列可能把 tag 抢光。
所以内核又加了一层“公平分配”控制：

- tags->active_queues 记录当前有多少活跃队列
  见 block/blk-mq-tag.c:45
- hctx_may_queue() 会按活跃队列数给每个队列一个大致公平的深度份额
  见 block/blk-mq.h:389

也就是大概这个思路：

每个活跃 hctx 最多先用到 shared depth / active_users，至少保底 4 个。

再补一个容易混淆的点：

shared tags 和 tag set 被多个 request_queue 共享 不是一回事。

- BLK_MQ_F_TAG_HCTX_SHARED：多个 hctx 共用一套 tags
- BLK_MQ_F_TAG_QUEUE_SHARED：同一个 tag_set 被多个 request_queue 共用时的活跃/公平控制

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
