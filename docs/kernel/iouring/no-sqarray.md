## iouring NO_SQARRAY
<!-- 3dfb6f3b-0301-4fe0-ae1a-c07811d5a13c -->

uapi 定义:

```c
/*
 * Removes indirection through the SQ index array.
 */
#define IORING_SETUP_NO_SQARRAY		(1U << 16)
```

## 背景:io_uring 的 SQ 是两级结构

提交队列实际上是两层:

1. **sq_array**:一个 `u32` 数组,每个元素存的是指向 sqes 的下标
2. **sqes**:真正的 `struct io_uring_sqe` 数组

提交时 `sq.head/tail` 先索引到 sq_array,再通过 `sq_array[i]` 的值去取真正的 SQE。
这是历史遗留设计,理论上允许用户乱序填充 sq_array 来重排提交顺序,但实际上
"没人真正用过,liburing 也没暴露它"(Pavel Begunkov 原话)。

## 这个 flag 做什么

设置后内核不再创建、也不再使用 sq_array,`sq.head/tail` 直接指向 sqes 数组,
`cached_sq_head & mask` 就是 SQE 下标。对应代码 `io_get_sqe()`:

```c
static bool io_get_sqe(struct io_ring_ctx *ctx, const struct io_uring_sqe **sqe)
{
	unsigned mask = ctx->sq_entries - 1;
	unsigned head = ctx->cached_sq_head++ & mask;

	if (static_branch_unlikely(&io_key_has_sqarray.key) &&
	    (!(ctx->flags & IORING_SETUP_NO_SQARRAY))) {
		head = READ_ONCE(ctx->sq_array[head]);   // 这层间接被跳过
		...
	}
	...
}
```

引入 commit:2af89abda7d9 ("io_uring: add option to remove SQ indirection"),2023-08, 6.6。

> To my knowledge, no one has ever seriously used it, nor liburing exposes it to users.
> Add IORING_SETUP_NO_SQARRAY, when set we don't bother creating and using the
> sq_array and SQ heads/tails will be pointing directly into the SQ. Improves memory
> footprint, in term of both allocations as well as cache usage, and also should make
> io_get_sqe() less branchy in the end.

## 收益

- **内存更小**:ring 布局计算时 `sq_array_offset` 直接设为 `SIZE_MAX`,省掉 `sq_entries * sizeof(u32)` 这块数组
  ```c
  rl->sq_array_offset = SIZE_MAX;   // io_uring.c io_rings_calc_size
  if (!(flags & IORING_SETUP_NO_SQARRAY)) {
  	size_t sq_array_size;
  	rl->sq_array_offset = off;
  	sq_array_size = array_size(sizeof(u32), sq_entries);
  	off = size_add(off, sq_array_size);
  }
  ```
- **缓存友好 / 更少分支**:提交路径少一次 `READ_ONCE` 间接访问;内核还用静态分支
  `io_key_has_sqarray` 做全局 fast path——只有当系统里有 ring 用了 sq_array 时才走那条路径
- 配套简化:
  - `fdinfo.c` 打印 pending SQE 时直接 `sq_idx = entry & sq_mask`
  - `IORING_REGISTER_RESIZE_RINGS` 调整 ring 大小时不用搬 sq_array(`register.c`)
  - 释放时 `if (!(ctx->flags & IORING_SETUP_NO_SQARRAY)) static_branch_slow_dec_deferred(...)`

## 注意点

- `p->sq_off.array` 不会设置(为 0),用户侧应通过 `sq_off.sqes/head/tail` 访问
- 与 `IORING_SETUP_SQ_REWIND`(bit 20)搭配:后者**要求** `IORING_SETUP_NO_SQARRAY`,
  且与 `IORING_SETUP_SQPOLL` 不兼容
  ```c
  /*
   * When set, io_uring ignores SQ head and tail and fetches SQEs to submit
   * starting from index 0 instead from the index stored in the head pointer.
   * ...
   * It requires IORING_SETUP_NO_SQARRAY and is incompatible with
   * IORING_SETUP_SQPOLL. ...
   */
  #define IORING_SETUP_SQ_REWIND		(1U << 20)
  ```
- liburing 里对应用法:`io_uring_queue_init_params` + 该 flag,提交时 SQE 依次放在 sqes 头部即可

## 相关

后续基于它的 `IORING_SETUP_SQ_REWIND` 非环形提交扩展(commit 5247c034a67f "io_uring: introduce non-circular SQ")。

## TODO
这让我想起来了 vittio 中的 split queue 和 packed queue ，不过，为什么不是一开始就设计为 NO_SQARRAY

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
