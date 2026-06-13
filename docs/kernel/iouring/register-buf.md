# iouring register buffers
<!-- df72ab12-3cb0-46ae-832d-6d1524058d66 -->

https://unixism.net/loti/ref-iouring/io_uring_register.html

主要关联的源码: rsrc.c

- https://man7.org/linux/man-pages/man3/io_uring_register_buffers.3.html
- https://man7.org/linux/man-pages/man3/io_uring_register_files.3.html
- https://man7.org/linux/man-pages/man2/io_uring_register.2.html

内核关联代码: io_uring/rsrc.c

核心数据结构:
```c
struct io_mapped_ubuf {
	u64		ubuf;
	u64		ubuf_end;
	unsigned int	nr_bvecs;
	unsigned long	acct_pages;
	// 注册的 buffer 都保存到这里
	struct bio_vec	bvec[] __counted_by(nr_bvecs);
};
```

从 IORING_OP_READ_FIXED 和 IORING_OP_READ 就是经典观察了:

> [!NOTE]
> 参考 Deepseeek ，有待验证

- IORING_OP_READ:
  - 用于从文件描述符（fd）读取数据到用户指定的缓冲区。
  - 每次提交操作时，需在 struct io_uring_sqe 中指定缓冲区地址（addr）和长度（len）。
  - 内核在每次操作时会对提供的缓冲区地址进行验证和映射。

- IORING_OP_READ_FIXED:
  - 专门用于从文件描述符读取数据到预先通过 io_uring_register_buffers 注册的固定缓冲区。
  - 不需要在每次操作中指定缓冲区地址，而是通过缓冲区索引（buf_index）引用已注册的缓冲区。
  - 依赖于预先注册的缓冲区，减少了内核对缓冲区的重复验证和映射。

- [ ] 所以，找到每次 verified 这些地址的地方就可以了。

- https://man7.org/linux/man-pages/man3/io_uring_prep_read_fixed.3.html
- https://man7.org/linux/man-pages/man3/io_uring_prep_read.3.html

```c
void io_uring_prep_read(struct io_uring_sqe *sqe,
                            int fd,
                            void *buf,
                            unsigned nbytes,
                            __u64 offset);

void io_uring_prep_read_fixed(struct io_uring_sqe *sqe,
                               int fd,
                               void *buf,
                               unsigned nbytes,
                               __u64 offset,
                               int buf_index);
```
相比就是多了一个参数 buf_index


> [!NOTE]
> 参考神奇海螺的意见，有待验证

为什么有了 `buf_index` 还需要 `buf`？

在使用 `io_uring` 的注册缓冲区（Registered Buffers）特性时，你首先会调用 `io_uring_register` 将一组内存区域（数组）告知内核。内核会提前锁定（pin）这些物理页，以避免在每次 I/O 时进行复杂的内存映射和取消映射操作。
* **`buf_index`**：告诉内核你使用的是哪一个注册过的“大内存块”。
* **`buf`**：告诉内核数据**具体要写到这个内存块的哪个位置**。

## 一点简单的测试
对应测试代码在: vn/code/src/c/iouring/ubuf.c

首先注册这些 buffer

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_io_uring_register
        - __se_sys_io_uring_register
          - __do_sys_io_uring_register
            - __io_uring_register
              - io_sqe_buffers_register
                - io_sqe_buffer_register
                  - io_buffer_account_pin
                    - io_account_mem

在之后的 io 中来
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_io_uring_enter
        - __se_sys_io_uring_enter
          - __do_sys_io_uring_enter
            - io_submit_sqes
              - io_submit_sqe
                - io_init_req
                  - io_prep_read_fixed

                    - io_init_rw_fixed
                      - io_import_fixed (使用 io_mapped_ubuf:bvec )

## 内核中是如何 verified 注册地址的

io_sqe_buffer_register 中:
1. io_pin_pages : 首先将注册的 pages 都 pin 下来
2. io_buffer_account_pin : 统计这些 page 是否超过了

headpage_already_acct 的理解，这里为什么不去统计普通页的重复问题，只是统计大页的重复问题。

不过结果就是这样的:

将内存 ping 下来之后，的确 grep VmPin /proc/pid/status 可以看到这个:
```txt
cat /proc/5460/status| grep Pin
  VmPin:        40 kB
```


## 细节

### 聚合
io_mapped_ubuf::bvec 数组中是，一个位置存储一个 page ，现在可以存储一个 vector 了:

https://lore.kernel.org/all/20240731090133.4106-1-cliang01.li@samsung.com/

```diff
commit a8edbb424b1391b077407c75d8f5d2ede77aa70d
Author: Chenliang Li <cliang01.li@samsung.com>
Date:   Wed Jul 31 17:01:33 2024 +0800

    io_uring/rsrc: enable multi-hugepage buffer coalescing

    Add support for checking and coalescing multi-hugepage-backed fixed
    buffers. The coalescing optimizes both time and space consumption caused
    by mapping and storing multi-hugepage fixed buffers.

    A coalescable multi-hugepage buffer should fully cover its folios
    (except potentially the first and last one), and these folios should
    have the same size. These requirements are for easier processing later,
    also we need same size'd chunks in io_import_fixed for fast iov_iter
    adjust.

    Signed-off-by: Chenliang Li <cliang01.li@samsung.com>
    Reviewed-by: Pavel Begunkov <asml.silence@gmail.com>
    Link: https://lore.kernel.org/r/20240731090133.4106-3-cliang01.li@samsung.com
    Signed-off-by: Jens Axboe <axboe@kernel.dk>
```

io_sqe_buffer_register 中:

```c
	/* If it's huge page(s), try to coalesce them into fewer bvec entries */
	if (nr_pages > 1 && io_check_coalesce_buffer(pages, nr_pages, &data)) {
		if (data.nr_pages_mid != 1)
			coalesced = io_coalesce_buffer(&pages, &nr_pages, &data);
	}
```

让问题变的奇怪的是 io_imu_folio_data 中要求 head folio 可以 partially included :
```c
struct io_imu_folio_data {
	/* Head folio can be partially included in the fixed buf */
	unsigned int	nr_pages_head;
	/* For non-head/tail folios, has to be fully included */
	unsigned int	nr_pages_mid;
	unsigned int	folio_shift;
	unsigned int	nr_folios;
};
```
不知道为什么对于 head folio 会网开一面的处理

## io_uring/kbuf.c
- IORING_OP_PROVIDE_BUFFERS : io_uring_enter 的参数

- IORING_REGISTER_PBUF_RING : io_uring_register 的参数，据说是升级版本

- 所以，其实非常奇怪，为什么需要给 iovec 和 iovec 的指向内容分别在两个模块中处理，是为了
  io_uring/rsrc.c 和

- https://lwn.net/Articles/813311/

aio 似乎是需要拷贝 iovec 的吧!

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
