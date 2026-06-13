## 一个 request 创建的时候就需要 tag ，那么意味着创建的时候就需要知道发送给哪一个盘吗？

是的
```txt
@[
    __blk_mq_get_tag+1
    blk_mq_get_tag+594
    __blk_mq_alloc_requests+444
    blk_mq_submit_bio+413 <--- 在这里只是提交的 bio ，接下在需要将 bio 装换为 reuqest
    submit_bio_noacct_nocheck+653
    ext4_mpage_readpages+1072
    read_pages+130
    page_cache_ra_unbounded+301
    filemap_get_pages+1231
    filemap_read+229
    vfs_read+510
    ksys_read+111
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 42145
```

- __blk_mq_alloc_requests
  - blk_mq_get_new_requests
    - __blk_mq_alloc_requests

在 __blk_mq_alloc_requests 中间，这里找到的 ctx 就是根据当时是在哪一个 cpu 上决定的:
```c
	data->ctx = blk_mq_get_ctx(q);
	data->hctx = blk_mq_map_queue(q, data->cmd_flags, data->ctx);
	tag = blk_mq_get_tag(data);
    tag = __blk_mq_get_tag(data, bt);
```

### 如果执行 taskset -ac 0 fio test.fio ，那么一定只会提交到 hctx0 上吗？

是的，即便是存在 cpu 45% 的 wait ，并且 hctx0 是完全 busy 的时候。


## 如何理解这个输出

```txt
[root@nixos:/sys/kernel/debug/block/sda/hctx0]# cat tags
nr_tags=32
nr_reserved_tags=0
active_queues=0

bitmap_tags:
depth=32
busy=0
cleared=18
bits_per_word=8
map_nr=4
alloc_hint={4, 5, 4, 0, 22, 1, 23, 0, 18, 15, 30, 0, 4, 10, 23, 6, 7, 4, 5, 0, 0, 1, 0, 23, 12, 0, 0, 0, 0, 3, 0, 5}
wake_batch=4
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
round_robin=1
min_shallow_depth=4294967295
```

```txt
🤒  cat /sys/devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/block/sdb/queue/nr_requests
64

vn on  master [$!+] 13900k
🧀  cat /sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/block/sda/queue/nr_requests
64
```
也就是说，nr_requests 比 tags 还要大

### 可以将 nr_requests 修改为 1000 ，然后测试 scsi_log 中 tag 会达到 1000 吗?

## nvme 的队列深度不是 6w 多吗，为什么最后反应出来的 nr_requests 总是 1024 ?

## 如果在一个系统中观察到 3000 的 tag ，那么是不是当时非常的 busy 才会分配出来这个数值，例如 inflight 非常大

## [ ] nr_requests 是如何初始化总是为 256

或者说，将 nvme 的队列数量修改为 256 之后，然后队列深度就修改为 256 了

## blk_mq_tags 和 rqs 和 static_rqs 是什么关系?
```c
/*
 * Tag address space map.
 */
struct blk_mq_tags {
	unsigned int nr_tags;
	unsigned int nr_reserved_tags;
	unsigned int active_queues;

	struct sbitmap_queue bitmap_tags;
	struct sbitmap_queue breserved_tags;

	struct request **rqs;
	struct request **static_rqs;
	struct list_head page_list;

	/*
	 * used to clear request reference in rqs[] before freeing one
	 * request pool
	 */
	spinlock_t lock;
};
```

他们都是在 blk_mq_alloc_rq_map 中分配空间

从 commit 217b613a53d3 ("blk-mq: update driver tags request table when start request") 中看:

- blk_mq_alloc_rqs 中初始化 static_rqs ，之后就不会再去动这个东西了
- blk_mq_start_request 中初始化 blk_mq_tags::rqs 数组中内容




## 影响 nr_tags 的因素: can_queue + nr_requests

跟踪 blk_mq_alloc_map_and_rqs 的调用位置，可以大概的知道 can_queue 对于 nr_tags 的影响

1. block/blk-mq.c 中的调用是受 set->queue_depth 影响

- __blk_mq_alloc_map_and_rqs : 这里持有参数 set->queue_depth
  - blk_mq_alloc_map_and_rqs
    - blk_mq_alloc_rq_map
      - blk_mq_init_tags : 这里最后赋值到 blk_mq_tags::nr_tags ，

2. block/blk-mq-sched.c 显示是受 nr_requests 影响

## blk_mq_hw_ctx::tags 和 blk_mq_hw_ctx::sched_tags 的关系
<!-- 6094021d-ef56-4f68-9df9-1944645f9a1b -->

```c
struct blk_mq_hw_ctx {
	/**
	 * @tags: Tags owned by the block driver. A tag at this set is only
	 * assigned when a request is dispatched from a hardware queue.
	 */
	struct blk_mq_tags	*tags;
	/**
	 * @sched_tags: Tags owned by I/O scheduler. If there is an I/O
	 * scheduler associated with a request queue, a tag is assigned when
	 * that request is allocated. Else, this member is not used.
	 */
	struct blk_mq_tags	*sched_tags;
```
有种感觉，那就是硬件队列将 scheduler 放到自己中的一部分。

```c
static inline struct blk_mq_tags *blk_mq_tags_from_data(struct blk_mq_alloc_data *data)
{
	if (data->rq_flags & RQF_SCHED_TAGS)
		return data->hctx->sched_tags;
	return data->hctx->tags;
}
```

如果使用了 scheduler ，那么就不去初始化 rq->tag
```c
static struct request *blk_mq_rq_ctx_init(struct blk_mq_alloc_data *data,
		struct blk_mq_tags *tags, unsigned int tag)
{

	if (data->rq_flags & RQF_SCHED_TAGS) {
		rq->tag = BLK_MQ_NO_TAG;
		rq->internal_tag = tag;
	} else {
		rq->tag = tag;
		rq->internal_tag = BLK_MQ_NO_TAG;
	}
```

```c
static inline bool blk_mq_get_driver_tag(struct request *rq)
{
	if (rq->tag == BLK_MQ_NO_TAG && !__blk_mq_alloc_driver_tag(rq))
		return false;

	return true;
}
```

- blk_mq_init_sched : 创建 scheduler 的时候
  - blk_mq_sched_alloc_map_and_rqs 中分配 sched_tags

```c
	hctx->sched_tags = blk_mq_alloc_map_and_rqs(q->tag_set, hctx_idx,
						    q->nr_requests); // 用 nr_requests 来初始化
```

总结 : 如果使用了 sched ，首先从 sched 中分配，然后从 driver 中分配

(真的吗? 还是说，有了 sched 之后，仅仅使用 sched 就可以了)

### 例子
这里可以看到 queue_depth 比 nr_requests 小
```txt
/sys/devices/pci0000:80/0000:80:0a.0/0000:84:00.0/nvme/nvme1/nvme1n1/queue/nr_requests 1023
/sys/devices/pci0000:80/0000:80:08.0/0000:83:00.0/nvme/nvme0/nvme0n1/queue/nr_requests 1023
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:18/0:0:18:0/block/sdf/queue/nr_requests 256
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:16/0:0:16:0/block/sde/queue/nr_requests 256
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:12/0:0:12:0/block/sdc/queue/nr_requests 256
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:17/0:0:17:0/block/sdd/queue/nr_requests 256
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:15/0:0:15:0/block/sdb/queue/nr_requests 256
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:13/0:0:13:0/block/sda/queue/nr_requests 256
/sys/devices/virtual/block/dm-1/queue/nr_requests 128
/sys/devices/virtual/block/dm-2/queue/nr_requests 128
/sys/devices/virtual/block/dm-0/queue/nr_requests 128

/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:18/0:0:18:0/queue_depth 32
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:16/0:0:16:0/queue_depth 32
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:12/0:0:12:0/queue_depth 32
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:65/0:0:65:0/queue_depth 1
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:17/0:0:17:0/queue_depth 32
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:15/0:0:15:0/queue_depth 32
/sys/devices/pci0000:80/0000:80:10.0/0000:87:00.0/host0/target0:0:13/0:0:13:0/queue_depth 32
```

那么，nr_requests ，queue_depth 和 tags 的数量都是什么时候起作用的?

## shared tags

只有在 blk_mq_is_shared_tags 中判断。

插入的位置 : scsi_mq_setup_tags
```c
	if (shost->host_tagset)
		tag_set->flags |= BLK_MQ_F_TAG_HCTX_SHARED;
```

virtio-scsi 没有这个 flags ，但是直接修改代码，让 virtio-scsi 设置为不是共享 tags 内核也是正常运行的

从 blk_mq_tag_set 的定义可以很好的说明了，如果是 shared_tags ，那么所有的 hw queue 共享一个队列，
否则都使用 shared_tags 来。
```c
/*
 * @shared_tags:
 *		   Shared set of tags. Has @nr_hw_queues elements. If set,
 *		   shared by all @tags.
 * @tag_list_lock: Serializes tag_list accesses.
 * @tag_list:	   List of the request queues that use this tag set. See also
 *		   request_queue.tag_set_list.
 * @srcu:	   Use as lock when type of the request queue is blocking
 *		   (BLK_MQ_F_BLOCKING).
 */
struct blk_mq_tag_set {
	struct blk_mq_tags	**tags;

	struct blk_mq_tags	*shared_tags;
};
```

是否 share tags 也会同样作用于 sched 上:
- blk_mq_init_sched_shared_tags : 这里初始化 request_queue::sched_shared_tags
-  blk_mq_sched_alloc_map_and_rqs : 这里，hctx->sched_tags 直接指向 request_queue::sched_shared_tags

## nr_reserved_tags

nvme 使用 fabrics 的模式的时候， nvme_alloc_io_tag_set 初始化 blk_mq_tag_set::reserved_tags

- blk_mq_alloc_map_and_rqs : 这里传递 blk_mq_tag_set::reserved_tags
  - blk_mq_alloc_rq_map
    - blk_mq_init_tags

唯一赋值的位置: blk_mq_init_tags

绝大多数时候，nr_requests_tags 都是 0 ，暂时不要管这个。

## request::internal_tag 的用途

注意看，当使用了 sched 之后，internal_tag 表示为 sched 的 tag ，
而 request::tag 描述是 driver tag ，当合并了 request 也就是在
blk_mq_free_request -> __blk_mq_free_request 的时候，
释放的 tag 就是从 request::internal_tag 中取出来，然后释放 scheduler tag

```c
struct request {

	int tag;
	int internal_tag;
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
