# bio-based、request-based 与 `request_queue` 的关系

本文重新整理 [bio-based-device.md](./bio-based-device.md) 中没有说清楚的几个问题：

1. bio-based 和 request-based 到底按什么区分；
2. bio-based 设备为什么仍然有 `request_queue`；
3. `request_queue` 对应磁盘、分区还是 HBA；
4. 同一 SCSI HBA 下的磁盘为什么拥有不同的 `hctx`；
5. loop 设备为什么是 blk-mq 设备，以及它能否使用 I/O scheduler。

分析基于本机 `/home/martins3/data/kernel/linux`：

```text
commit d41bd6abfe34380d9e5e1bee888232b80d2138f7
v7.1.2-1-gd41bd6abfe34
```

这里讨论的是这个版本已经注册成功的 block device。设备注册前可能还处于中间状态，
不能只看某个分配函数就判断最终模式。

## 先给结论

在当前内核中，对外注册的 block device 有两种提交入口：

| 模式 | `request_queue::mq_ops` | `gendisk::fops->submit_bio` | `__submit_bio()` 后续入口 | 本层是否创建 `request` |
|---|---:|---:|---|---:|
| bio-based | `NULL` | 必须存在 | `disk->fops->submit_bio(bio)` | 否 |
| request-based，也就是 blk-mq | 非 `NULL` | 必须为 `NULL` | `blk_mq_submit_bio(bio)` | 是 |

因此，在**已经通过 `add_disk()` 注册的整盘设备**上，可以把下面两个判断视为相反结果：

```c
queue_is_mq(disk->queue)
bdev_test_flag(disk->part0, BD_HAS_SUBMIT_BIO)
```

但它们不是同一个字段的两个名字，也不能脱离设备注册状态说它们永远等价：

- `queue_is_mq(q)` 只是检查 `q->mq_ops`；
- `BD_HAS_SUBMIT_BIO` 是 `block_device` 上的提交分流标志；
- `__add_disk()` 负责检查两套接口不能混用，并为 bio-based 设备设置该标志；
- 分区创建时，`bdev_alloc()` 把整盘 `part0` 上的标志复制给分区。

当前内核已经没有传统的 legacy single-queue request driver 模型。本文说的
request-based 是“本层把 bio 转成 request，再经 blk-mq 提交”，不能再理解成
“旧 single queue 与新 multi queue 二选一”。

## 两条提交路径

上层提交给块层的对象都是 `struct bio`。分叉发生在 `block/blk-core.c::__submit_bio()`：

```text
submit_bio()
  -> submit_bio_noacct()
     -> __submit_bio()
        |
        |  BD_HAS_SUBMIT_BIO
        +----------------------> disk->fops->submit_bio(bio)
        |                          bio-based 层处理、拆分、克隆或重映射 bio
        |                          再向下层 submit_bio_noacct()
        |
        `----------------------> blk_mq_submit_bio(bio)
                                   分配或合并 struct request
                                   经 scheduler/plug 或直接 dispatch
                                   最终调用 q->mq_ops->queue_rq()
```

### bio-based 不等于整条 I/O 路径没有 `request`

bio-based 只描述**当前这一层**的入口。以常见的 md 或 bio-based DM 为例：

```text
filesystem bio
  -> md/dm 的 ->submit_bio()       本层不创建 request
     -> clone/remap 后的下层 bio
        -> SCSI/NVMe/virtio-blk
           -> blk_mq_submit_bio()
              -> request            在下层 mq 设备创建
```

所以“raid1 根本不会制作 request”只适用于 md/raid1 这一层。只要它最终落到 blk-mq
下层设备，那里仍然会创建 request。

反过来，mq 也不表示设备一定是物理盘。loop、nbd、ublk、request-based DM 都可以
对外提供 blk-mq/request 接口。

### bio-based 层的递归提交

bio-based 驱动经常在 `->submit_bio()` 中再次提交下层 bio。当前代码使用
`current->bio_list` 收集这种递归提交，等当前回调返回后再处理，并优先处理发往更
低层 queue 的 bio。这样避免深层 stacked device 通过 C 调用栈无限递归。

## 注册阶段如何保证两种模式互斥

`block/genhd.c::__add_disk()` 的约束可以概括为：

```c
if (queue_is_mq(disk->queue)) {
	/* blk-mq 绕过 ->submit_bio 和 ->poll_bio */
	if (disk->fops->submit_bio || disk->fops->poll_bio)
		return -EINVAL;
} else {
	if (!disk->fops->submit_bio)
		return -EINVAL;
	bdev_set_flag(disk->part0, BD_HAS_SUBMIT_BIO);
}
```

这段检查才是当前代码中区分设备模式的最终依据。

### 不要仅凭分配 helper 判断最终模式

通常情况下：

- `blk_alloc_disk()` / `__blk_alloc_disk()` 只调用 `blk_alloc_queue()`，得到普通的
  非 mq queue；
- `blk_mq_alloc_disk()` / `__blk_mq_alloc_disk()` 在创建 disk 时已经用 tag set
  初始化 blk-mq queue。

但 Device Mapper 是一个重要例外。`alloc_dev()` 先用 `blk_alloc_disk()` 创建一个
尚未定型的设备；加载 request-based table 后，`dm_mq_init_request_queue()` 再对同一个
queue 调用 `blk_mq_init_allocated_queue()`。该函数首先设置：

```c
q->mq_ops = set->ops;
q->tag_set = set;
```

然后分配 `queue_ctx`、`blk_mq_hw_ctx` 并建立 CPU 到 hctx 的映射。因此“调用过
`blk_alloc_queue()`”不等于设备最终一定是 bio-based，最终仍应以 `__add_disk()` 时的
状态为准。

## bio-based 为什么仍然需要 `request_queue`

`request_queue` 这个名字带有历史痕迹。当前结构体既包含所有块设备共用的 queue
状态，也包含 blk-mq 专用状态。bio-based 设备不经过本层的 request 分配和调度，
但仍需前一类功能。

### 两种模式共用的部分

主要包括：

- `limits`：逻辑块大小、最大 sectors、segment、discard、zone 等限制；
- `q_usage_counter`、freeze 状态和等待队列：阻止新 I/O 并等待使用者退出；
- `refs`、`queue_flags` 和设备生命周期状态；
- 统计、blktrace、blk-cgroup、throttling 和部分 `rq_qos` 状态；
- sysfs 中两种 queue 都需要展示的 limits、read ahead、poll 等属性；
- `queuedata`、`disk`、PM 和 debugfs 等归属信息。

即使走 `disk->fops->submit_bio()`，`__submit_bio()` 也会先通过 `bio_queue_enter()`
取得 `q_usage_counter` 引用，回调结束后再 `blk_queue_exit()`。因此 bio-based queue
不是一个为了兼容而保留的空壳。

### blk-mq 专用的部分

主要包括：

- `mq_ops` 和 `tag_set`；
- per-CPU `queue_ctx`，即 `struct blk_mq_ctx`；
- `nr_hw_queues` 和 `queue_hw_ctx[]` 中的 `struct blk_mq_hw_ctx`；
- tag、request pool、requeue、timeout、flush 和 scheduler 相关状态；
- `elevator`、`nr_requests`、`async_depth` 等 request 调度参数。

在 bio-based queue 中，`mq_ops`、`tag_set`、`queue_ctx` 和 hctx 数组都不会被
`blk_mq_init_allocated_queue()` 初始化。

这也解释了为什么 bio-based md 设备的 `/sys/block/mdX/queue/` 中没有
`nr_requests`。`block/blk-sysfs.c` 明确把 sysfs 属性拆成两组：

- `queue_attrs[]`：bio-based 和 request-based 共用；
- `blk_mq_queue_attrs[]`：只对 `queue_is_mq(q)` 为真的设备可见，其中包含
  `scheduler`、`nr_requests`、`io_timeout` 和 `rq_affinity`。

旧文档中摘录的 `request_queue::hctx_table` 也已经过时；当前代码使用的是：

```c
unsigned int nr_hw_queues;
struct blk_mq_hw_ctx * __rcu *queue_hw_ctx;
```

## 一个 `request_queue` 对应什么

对当前常规 block device，可以按下面的关系理解：

```text
一个 gendisk（整盘）
  -> 一个 request_queue
     <- part0 和所有分区 block_device 的 bd_queue
```

`__alloc_disk_node()` 设置 `disk->queue = q` 和 `q->disk = disk`。随后
`bdev_alloc()` 无论创建 part0 还是分区，都执行：

```c
bdev->bd_queue = disk->queue;
```

因此：

- `sda` 与 `sda1` 共享同一个 queue；
- `sda` 与 `sdb` 通常拥有不同的 queue；
- 一个 queue 不是整个 HBA，也不是每个分区各一个；
- `/sys/block/sda/queue/` 是整盘 queue 的属性，不是 `sda1` 的独立调度队列。

`blk_mq_alloc_disk_for_queue()` 说明某些子系统会先创建 queue，再把 gendisk 挂到它
上面，例如 SCSI 的 `scsi_device` 先拥有 queue，sd 驱动随后创建 gendisk。这只是
分配顺序不同，没有改变“每个 LUN/整盘一套 queue”的模型。

## 同一 SCSI HBA 下为何每个盘的 `hctx` 都不同

SCSI 把“每盘状态”和“host 共享资源”放在不同对象中：

```text
Scsi_Host
  `-> blk_mq_tag_set                    host 级，多个 LUN 共用
      |-- queue map / mq_ops
      |-- tags[] 或 shared_tags         可共享的命令槽资源
      |
      |-- sdb 的 request_queue
      |    |-- per-CPU blk_mq_ctx
      |    `-- sdb 自己的 blk_mq_hw_ctx[]
      |
      `-- sdc 的 request_queue
           |-- per-CPU blk_mq_ctx
           `-- sdc 自己的 blk_mq_hw_ctx[]
```

代码路径是：

```text
scsi_add_host_with_dma()
  -> scsi_mq_setup_tags()
     -> blk_mq_alloc_tag_set(&shost->tag_set)

scsi_alloc_sdev()                         每发现一个 LUN 调用一次
  -> blk_mq_alloc_queue(&shost->tag_set, ..., sdev)

sd_probe()
  -> blk_mq_alloc_disk_for_queue(sdev->request_queue, ...)
```

每次 `blk_mq_alloc_queue()` 都会为这个 queue 调用 `blk_mq_alloc_ctxs()`，并在
`blk_mq_realloc_hw_ctxs()` 中创建自己的 hctx。hctx 不能直接由两个磁盘共用，因为它
包含明显的 per-queue 状态：

- `hctx->queue` 指回唯一的 `request_queue`；
- `dispatch` 是该 queue 因资源不足而暂存的 request 链表；
- `ctxs`、`ctx_map` 是这个 queue 的 CPU software queue 映射；
- `sched_data`、`sched_tags` 属于这个 queue 上安装的 I/O scheduler；
- stop、restart、run work、flush queue 等状态也按 queue 管理。

所以 drgn 看到 `sdb.queue_hw_ctx[0] != sdc.queue_hw_ctx[0]` 是预期行为，并不表示
它们没有共享同一 HBA 的硬件资源。

### 实际共享的是 tag set 和 tag

所有 LUN 的 queue 都指向 `shost->tag_set`。同一个 tag set 被第二个 queue 使用时，
blk-mq 会设置 `BLK_MQ_F_TAG_QUEUE_SHARED`，表示不同 queue 的对应 hctx 使用同一组
tag set 资源。

如果 SCSI low-level driver 还设置了 `shost->host_tagset`，
`scsi_mq_setup_tags()` 会增加 `BLK_MQ_F_TAG_HCTX_SHARED`。当前实现随后只分配一个
`set->shared_tags`，让该 host 下所有 queue、所有 hctx 竞争同一个全局 tag bitmap。
这适合“控制器只有一组全局命令槽”的硬件。

因此需要区分三件事：

| 对象 | sdb 与 sdc 是否相同 | 原因 |
|---|---:|---|
| `request_queue` | 否 | 每个 LUN/整盘有自己的 limits、冻结、统计和调度状态 |
| `blk_mq_ctx` / `blk_mq_hw_ctx` | 否 | 都是 per-queue 的提交和 dispatch 状态 |
| `Scsi_Host::tag_set` | 是 | 描述 host 的 mq ops、queue map 和命令资源 |
| `blk_mq_tags` | 通常由共同 tag set 提供 | 对应 hctx 间共享；启用 host tagset 时所有 hctx 共用一个池 |

`hctx` 是“这个 block queue 对某个硬件 dispatch queue 的软件视图”，不是硬件队列
本身。多个 hctx 可以映射到同一控制器资源，这正是这里最容易误解的地方。

## loop 为什么是 mq 设备

当前 `drivers/block/loop.c` 注册了：

```c
static const struct blk_mq_ops loop_mq_ops = {
	.queue_rq = loop_queue_rq,
	.complete = lo_complete_rq,
};
```

`loop_add()` 分配 tag set 后调用 `blk_mq_alloc_disk()`，所以 loop 对上层呈现的是
request-based/blk-mq 接口。它虽然是 stacked virtual device，但仍希望使用 blk-mq
提供的 request 生命周期、tag、并发控制和异步 worker 提交模型。

loop **具备切换 blk-mq I/O scheduler 的接口**，但当前代码给 tag set 设置了：

```c
BLK_MQ_F_STACKING | BLK_MQ_F_NO_SCHED_BY_DEFAULT
```

`BLK_MQ_F_NO_SCHED_BY_DEFAULT` 使 `elevator_set_default()` 保持 `none`，避免 stacked
设备默认再增加一层调度。它不是禁止用户切换 scheduler；可用调度器仍可通过
`/sys/block/loopX/queue/scheduler` 查看和选择。

## 如何在运行中的机器上判断

先检查 mq 专属 sysfs 节点：

```bash
dev=sda

test -d "/sys/block/$dev/mq" && echo blk-mq || echo bio-based
test -e "/sys/block/$dev/queue/nr_requests" && echo request-based
cat "/sys/block/$dev/queue/scheduler" 2>/dev/null
```

这些检查适用于已经注册的设备。不要用下面这些属性单独分类：

- 物理盘还是虚拟盘：loop 和 nbd 也是 mq；
- 是否 stacked：DM 既能 bio-based，也保留 request-based 模式；
- 是否有 `/sys/block/DEV/queue/`：两种模式都有公共 queue 属性；
- queue 的分配 helper：DM 可以把先前分配的普通 queue 再初始化成 mq。

需要观察对象地址时，应该同时打印：

```text
gendisk
request_queue
request_queue.tag_set
request_queue.queue_ctx
request_queue.queue_hw_ctx[i]
queue_hw_ctx[i].queue
queue_hw_ctx[i].tags
tag_set.tags[i] / tag_set.shared_tags
```

对于同一 SCSI host 下的两个盘，正确预期是：queue、ctx、hctx 地址不同，tag_set
地址相同，hctx 的 tags 指针则按 tag set 的共享模式相同。

## 对旧图的正确使用方式

[LWN 的 bio layer 文章](https://lwn.net/Articles/736534/) 对“stacked driver 在 bio 层
重映射、底层 driver 在 request 层接收命令”的概念仍然有帮助，但图中的函数名和
legacy request queue 分支不能直接套到当前代码。分析新内核时，应以这三个位置为
主线：

1. `block/genhd.c::__add_disk()`：决定注册接口是否合法；
2. `block/blk-core.c::__submit_bio()`：决定 bio 进入哪条提交路径；
3. `block/blk-mq.c::blk_mq_submit_bio()`：bio 转换、合并并提交 request。

## 代码索引

- `include/linux/blkdev.h`：`struct request_queue`、`queue_is_mq()`；
- `include/linux/blk-mq.h`：`struct blk_mq_hw_ctx`、`struct blk_mq_tag_set`；
- `block/blk-core.c`：`blk_alloc_queue()`、`__submit_bio()`；
- `block/genhd.c`：`__add_disk()`、`__alloc_disk_node()`、`__blk_alloc_disk()`；
- `block/bdev.c`：`bdev_alloc()`；
- `block/blk-mq.c`：`blk_mq_submit_bio()`、`blk_mq_init_allocated_queue()`；
- `block/blk-sysfs.c`：公共和 mq 专属 queue 属性；
- `drivers/scsi/hosts.c`、`scsi_lib.c`、`scsi_scan.c`、`sd.c`：SCSI host、LUN、queue
  和 gendisk 的创建关系；
- `drivers/md/dm.c`、`dm-rq.c`：DM 从未定型 queue 切换为 request-based 的例外；
- `drivers/block/loop.c`：虚拟设备使用 blk-mq 的实例。

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
