## iouring FEAT_SINGLE_MMAP
<!-- 422dfcf1-4294-4e7d-9d2d-1276edf1fb4e -->

uapi 定义(注意这是 **features** 标志,不是 setup flag,
由内核通过 `io_uring_params.features` 返回给用户):

```c
/*
 * io_uring_params->features flags
 */
#define IORING_FEAT_SINGLE_MMAP		(1U << 0)
```

## 背景:以前需要两次 mmap

早期实现中 SQ ring 和 CQ ring 是**两个独立的内存分配**:
- `IORING_OFF_SQ_RING`(0)mmap 拿到 sq ring
- `IORING_OFF_CQ_RING`(0x8000000)mmap 再拿一份 cq ring

liburing/用户程序一直得做两次 mmap,即使内核已经支持合并。

## 两个关键 commit

### 1. 75b28affdd6a ("io_uring: allocate the two rings together") — Hristo Venev, 2019-08

把 sq ring 和 cq ring 合并进同一个 `struct io_rings` 分配:

> Both the sq and the cq rings have sizes just over a power of two, and
> the sq ring is significantly smaller. By bundling them in a single
> alllocation, we get the sq ring for free.

动机:ring 大小都"略大于 2 的幂",而 sq ring 明显比 cq ring 小,合并后页数不变,
sq ring 等于白送。`struct io_uring`(sq/cq 各自的 head/tail/mask/flags)都放进同一个结构:

```c
struct io_rings {
	struct io_uring	sq, cq;
	u32		sq_ring_mask, cq_ring_mask;
	u32		sq_ring_entries, cq_ring_entries;
	u32		sq_dropped, sq_flags;
	...
};
```

### 2. ac90f249e15c ("io_uring: expose single mmap capability") — Jens Axboe, 2019-09

合并是实现了,但**用户态不知道**。这个 commit 给 `io_uring_params` 增加 `features` 字段,
把该能力暴露出来:

> After commit 75b28affdd6a we can get by with just a single mmap to
> map both the sq and cq ring. However, userspace doesn't know that.
> Add a features variable to io_uring_params, and notify userspace
> that the kernel has this ability.

liburing 据此可以省掉第二次 mmap。这是 `io_uring_params.features` 的第一个 feature 位。

## 现在的用法

`IORING_OFF_SQ_RING` 和 `IORING_OFF_CQ_RING` 映射的是**同一份内存**;
`sq_off` 和 `cq_off` 里的偏移都相对同一基址。用户态只需:

```c
size = max(sq_ring_sz, cq_ring_sz);
sq_ptr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, fd, IORING_OFF_SQ_RING);
cq_ptr = sq_ptr;   // 不再需要第二次 mmap(IORING_OFF_CQ_RING)
sq->head  = sq_ptr + p.sq_off.head;
cq->tail  = sq_ptr + p.cq_off.tail;
```

注意:
- **sqes 不在此列**:SQE 数组仍然要单独 map(`IORING_OFF_SQES`, 0x10000000),不受此 feature 影响
- 内核侧 memmap.c 中 `IORING_OFF_SQ_RING` / `IORING_OFF_CQ_RING` 走同一个 region:
  ```c
  switch (offset & IORING_OFF_MMAP_MASK) {
  case IORING_OFF_SQ_RING:
  case IORING_OFF_CQ_RING:
  	page_limit = (sz + PAGE_SIZE - 1) >> PAGE_SHIFT;
  	break;
  }
  region = io_mmap_get_region(ctx, vma->vm_pgoff);
  ```
  现代内核里 rings 通过 io_uring regions(memfd)分配(`io_allocate_scq_urings` 里
  `io_create_region(ctx, &ctx->ring_region, &rd, IORING_OFF_CQ_RING)`),两个 offset 都映射到
  同一个 `ring_region`
- 当前内核的 `IORING_FEAT_FLAGS` 里 SINGLE_MMAP 恒置位(它没有对应的运行时开关)

## 收益

- 少一次 `mmap` 系统调用和一份页表/TLB 开销
- 逻辑上 sq/cq 的 head、tail 相邻,缓存局部性更好

## TODO

- 另一个"少一次 mmap"的 flag:`IORING_SETUP_NO_MMAP`(bit 14)是反过来——应用自己提供
  rings 内存,内核不再 mmap
- 后续 feature 位逐个追加:`IORING_FEAT_NODROP`、`IORING_FEAT_SUBMIT_STABLE`、...
  详见 `include/uapi/linux/io_uring.h` 的 features 段

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
