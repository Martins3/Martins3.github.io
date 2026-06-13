## iouring 的 queue 同步
<!-- 982ea542-54ce-442a-a778-e28f91ab83db -->

在 tools/include/io_uring/mini_liburing.h 中

```c
#if defined(__x86_64) || defined(__i386__)
#define read_barrier()	__asm__ __volatile__("":::"memory")
#define write_barrier()	__asm__ __volatile__("":::"memory")
#else
#define read_barrier()	__sync_synchronize()
#define write_barrier()	__sync_synchronize()
#endif
```

为什么需要 barrier 来着?

### 类似的问题

perf 的队列

include/uapi/linux/perf_event.h 中
```c
/*
 * Structure of the page that can be mapped via mmap
 */
struct perf_event_mmap_page {
	__u32	version;		/* version number of this structure */
	__u32	compat_version;		/* lowest version this is compat with */

	/*
	 * Bits needed to read the HW events in user-space.
	 *
	 *   u32 seq, time_mult, time_shift, index, width;
	 *   u64 count, enabled, running;
	 *   u64 cyc, time_offset;
	 *   s64 pmc = 0;
	 *
	 *   do {
	 *     seq = pc->lock;
	 *     barrier()
	 * ...
	 */
```
进一步可以看看 Documentation/userspace-api/perf_ring_buffer.rst 的实现是如何考虑

## 队列问题是一个专题
(这个算不算经典的，想要搞一个完美的方案，最后完全无法推进的例子？)

1. stefan 的 blog 和 fenli 的 blog 都写一个了，有必要
2. 先看看一个 cqe 和 sqe 到底有多大
- [Comparing VIRTIO, NVMe, and io_uring queue designs](https://blog.vmsplice.net/2022/06/comparing-virtio-nvme-and-iouring-queue.html)

应该使用一个 virtio queue 的驱动自己写一写才可以

# 为什么使用队列
整理一下各种队列

## 基本设计

1. 共享内存
2. 对于预取非常友好
4. 无需上锁
  - 但是需要考虑 memory model

## 热迁移中 dirty ring
PML

## nvme / virtio / dpdk / vhost / io uring / vDPA

https://zhuanlan.zhihu.com/p/673995830

## io uring

## 各种常规的队列

1. aio :
2. network stack 中各级队列
  - backlog 是吗?
3. storage 的中的队列，scheduler 等等

## 调查一下，一个 hdd 的 queue 如何被 hba 的 queue 影响的

## 观察一下，各种 queue 里面，到底有多少乱序的部分的

## 仔细参考
http://blog.vmsplice.net/2022/06/comparing-virtio-nvme-and-iouring-queue.html

## 先搞搞基本的实现

应该参考一下 virtio 的 queue 写一个 demo ，
看看基本的 queue 的实现。

## 一个基本的实现
太垃圾了，应该看看内核中的 lib 或者 selftests 之类的吧

```c
#include <pthread.h>
#include <linux/userfaultfd.h>
#include <stdbool.h>

typedef struct {
	struct uffd_msg msg;
	struct vm_info *vi;
} ring_buffer_entry;

#define RING_BUFFER_SIZE 1024
typedef struct {
	ring_buffer_entry buffer[RING_BUFFER_SIZE];
	int head;
	int tail;
	int count;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} ring_buffer;

void uffd_ring_buffer_init();
void ring_buffer_init(ring_buffer *rb);
void ring_buffer_push(ring_buffer *rb, ring_buffer_entry entry);
ring_buffer_entry ring_buffer_pop(ring_buffer *rb);
static inline bool ring_buffer_empty(ring_buffer *rb)
{
	return rb->count == 0;
}
```

```c
#include "ringbuffer.h"

void ring_buffer_init(ring_buffer *rb)
{
	rb->head = 0;
	rb->tail = 0;
	rb->count = 0;
	pthread_mutex_init(&rb->mutex, NULL);
	pthread_cond_init(&rb->cond_full, NULL);
	pthread_cond_init(&rb->cond_empty, NULL);
}

// TODO did we do a copy at function parameter ?
void ring_buffer_push(ring_buffer *rb, ring_buffer_entry entry)
{
	pthread_mutex_lock(&rb->mutex);
	while (rb->count == RING_BUFFER_SIZE) {
		pthread_cond_wait(&rb->cond_full, &rb->mutex);
	}
	rb->buffer[rb->head] = entry;
	rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
	rb->count++;
	pthread_cond_signal(&rb->cond_empty);
	pthread_mutex_unlock(&rb->mutex);
}

ring_buffer_entry ring_buffer_pop(ring_buffer *rb)
{
	pthread_mutex_lock(&rb->mutex);
	while (rb->count == 0) {
		pthread_cond_wait(&rb->cond_empty, &rb->mutex);
	}
	ring_buffer_entry entry = rb->buffer[rb->tail];
	rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
	rb->count--;
	pthread_cond_signal(&rb->cond_full);
	pthread_mutex_unlock(&rb->mutex);
	return entry;
}
```

## 才是意识到，原来 rdma 也会是队列

那么 nvme 的而是定义了队列吧


## 的确经典
https://zhuanlan.zhihu.com/p/673995830

这里分析了四个 nvme iouring rdma 和 virtio

那么，请问 GPU 中存在类似的队列系统吗?

我们发现 rdma 是可以通过 mmio ，让用户态直接轮询的，
那么 nvme 不可以也是这样的，直接暴露用户态的 nvme 直接
用户态轮询。(似乎也可以，但是 nvme 是通过 vfio + 用户态驱动的)
也许是由于盘天然的不支持多个用户态，但是，rdma 网卡可以自动通知到不同的用户，所以，自动的就可以感知到了。

## tun 和 tap 设备也有自己的环形缓冲区?

## 还有一些 queue

热迁移的中的 dirty ring

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
