## blk-mq-debugfs.c

一共三个内容

```txt
├── hctx9                   <--- 一个硬件队列
│   ├── active
│   ├── busy
│   ├── cpu9
│   │   ├── default_rq_list <--- 硬件队列关联的软件队列
│   │   ├── poll_rq_list
│   │   └── read_rq_list
│   ├── ctx_map
│   ├── dispatch
│   ├── dispatch_busy
│   ├── flags
│   ├── run
│   ├── sched_tags
│   ├── sched_tags_bitmap
│   ├── state
│   ├── tags
│   ├── tags_bitmap
│   └── type
├── pm_only                  <---- queue 的信息
├── poll_stat
├── requeue_list
├── state
└── zone_wlock
```


```c
static const struct blk_mq_debugfs_attr blk_mq_debugfs_queue_attrs[] = {
	{ "poll_stat", 0400, queue_poll_stat_show },
	{ "requeue_list", 0400, .seq_ops = &queue_requeue_list_seq_ops },
	{ "pm_only", 0600, queue_pm_only_show, NULL },
	{ "state", 0600, queue_state_show, queue_state_write },
	{ "zone_wlock", 0400, queue_zone_wlock_show, NULL },
	{ },
};

static const struct blk_mq_debugfs_attr blk_mq_debugfs_hctx_attrs[] = {
	{"state", 0400, hctx_state_show},
	{"flags", 0400, hctx_flags_show},
	{"dispatch", 0400, .seq_ops = &hctx_dispatch_seq_ops},
	{"busy", 0400, hctx_busy_show},
	{"ctx_map", 0400, hctx_ctx_map_show},
	{"tags", 0400, hctx_tags_show},
	{"tags_bitmap", 0400, hctx_tags_bitmap_show},
	{"sched_tags", 0400, hctx_sched_tags_show},
	{"sched_tags_bitmap", 0400, hctx_sched_tags_bitmap_show},
	{"run", 0600, hctx_run_show, hctx_run_write},
	{"active", 0400, hctx_active_show},
	{"dispatch_busy", 0400, hctx_dispatch_busy_show},
	{"type", 0400, hctx_type_show},
	{},
};

static const struct blk_mq_debugfs_attr blk_mq_debugfs_ctx_attrs[] = {
	{"default_rq_list", 0400, .seq_ops = &ctx_default_rq_list_seq_ops},
	{"read_rq_list", 0400, .seq_ops = &ctx_read_rq_list_seq_ops},
	{"poll_rq_list", 0400, .seq_ops = &ctx_poll_rq_list_seq_ops},
	{},
};
```

## 分析下硬件队列中的内容

/sys/kernel/debug/block/sdb 的内容分析


```txt
➜  hctx0 cat tags
nr_tags=256
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=256
busy=0
cleared=203
bits_per_word=64
map_nr=4
alloc_hint={128, 175, 46, 149, 198, 9, 46, 83, 31, 62, 75, 156, 70, 230, 98, 130, 245, 176, 220, 104, 139, 57, 7, 209, 113, 115, 198, 109, 205, 93, 115, 131}
wake_batch=8
wake_index=0
ws_active=0
ws={
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
        {.wait=inactive},
}
round_robin=0
min_shallow_depth=4294967295
```

从 bitmap_tags 一下的内容都是直接展示的 sbitmap 的内容:
- [x] busy : 当前多少个
- [ ] depths 含义
- [ ] map_nr
- [ ] min_shallow_depth

## scehd 目录分析

不同的 scheduler 中的内容不同，以 deadline 为例:
```c
static const struct blk_mq_debugfs_attr deadline_queue_debugfs_attrs[] = {
	DEADLINE_QUEUE_DDIR_ATTRS(read0),
	DEADLINE_QUEUE_DDIR_ATTRS(write0),
	DEADLINE_QUEUE_DDIR_ATTRS(read1),
	DEADLINE_QUEUE_DDIR_ATTRS(write1),
	DEADLINE_QUEUE_DDIR_ATTRS(read2),
	DEADLINE_QUEUE_DDIR_ATTRS(write2),
	DEADLINE_NEXT_RQ_ATTR(read0),
	DEADLINE_NEXT_RQ_ATTR(write0),
	DEADLINE_NEXT_RQ_ATTR(read1),
	DEADLINE_NEXT_RQ_ATTR(write1),
	DEADLINE_NEXT_RQ_ATTR(read2),
	DEADLINE_NEXT_RQ_ATTR(write2),
	{"batching", 0400, deadline_batching_show},
	{"starved", 0400, deadline_starved_show},
	{"async_depth", 0400, dd_async_depth_show},
	{"dispatch0", 0400, .seq_ops = &deadline_dispatch0_seq_ops},
	{"dispatch1", 0400, .seq_ops = &deadline_dispatch1_seq_ops},
	{"dispatch2", 0400, .seq_ops = &deadline_dispatch2_seq_ops},
	{"owned_by_driver", 0400, dd_owned_by_driver_show},
	{"queued", 0400, dd_queued_show},
	{},
};
```

## rq_qos
但是现在 rq_qos 没有打开，所以没看到

## busy

例如观察 debufs : /sys/kernel/debug/block/nvme0n1 每一个 hctx 下 nr_tags 都是 1023

使用两个 fio ，aio 的 depth 设置为 1000 :

```txt
[root@nixos:/sys/kernel/debug/block/nvme0n1]# for i in {1..31};do cat hctx$i/tags | grep busy ; done
busy=1000
busy=0
busy=0
busy=0
busy=0
busy=0
busy=0
busy=0
busy=0
busy=0
busy=0
busy=1000
```

两个队列都是存在 1000 个 busy 的 tag

## 修改 nr_requests ，那么 sched_tags
当一个盘的 scheduler 是 none 的时候，修改 nr_requests
那么可以观察到

这里 nr_tags 还是原来 32 ，而 bitmap_tsgs: 下的 depth 变为 16
```txt
root@localhost:/sys/kernel/debug/block/sda/hctx0# cat tags
nr_tags=32
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=16
```

这个时候修改 scheduler 为 mq-deadline
可以看到其他的没有变化:
```txt
root@localhost:/sys/kernel/debug/block/sda/hctx0# cat tags
nr_tags=32
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=16
```

sched_tags 内容为:
```txt
root@localhost:/sys/kernel/debug/block/sda/hctx0# cat sched_tags
nr_tags=64
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=64
```
同时 nr_requests 变为 64

再修改 nr_requests 为 32
```txt
root@localhost:/sys/kernel/debug/block/sda/hctx0# cat sched_tags
nr_tags=64
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=32
```

## virtio-scsi 和 scsi_debug 就是很好的测试对象

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
