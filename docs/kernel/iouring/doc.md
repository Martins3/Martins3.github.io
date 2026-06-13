# 文档
## API 使用
https://nick-black.com/dankwiki/index.php/Io_uring

这里里面的东西全部都需要背下来吧

### io uring 一共支持哪些系统调用
<!-- c2a9dcf1-71be-4c9d-9eb2-055215acca14 -->

一共就是三个:
```c
int io_uring_setup(u32 entries, struct io_uring_params *p);
int io_uring_register(unsigned fd, unsigned opcode, void *arg, unsigned int nr_args);
int io_uring_enter(unsigned fd, u32 to_submit, u32 min_complete, u32 flags, const void* argp, size_t argsz);
```
所以，当运行起来的时候，只会去调用 io_uring_enter 的

liburing 提供的两个接口都是回去调用 io_uring_enter :
- io_uring_submit
- io_uring_wait_cqe : 不需要复杂的设置就可以避免系统调用，如果他发现已经存在完成的任务，那么可以直接返回。

### io_uring_enter 参数 flags 的含义
<!-- ce0527d2-b905-4bdc-be00-6567a3c2e574 -->

https://manpages.debian.org/unstable/liburing-dev/io_uring_enter.2.en.html

```c
/*
 * io_uring_enter(2) flags
 */
#define IORING_ENTER_GETEVENTS		(1U << 0)
#define IORING_ENTER_SQ_WAKEUP		(1U << 1)
#define IORING_ENTER_SQ_WAIT		(1U << 2)
#define IORING_ENTER_EXT_ARG		(1U << 3)
#define IORING_ENTER_REGISTERED_RING	(1U << 4)
#define IORING_ENTER_ABS_TIMER		(1U << 5)
#define IORING_ENTER_EXT_ARG_REG	(1U << 6)
#define IORING_ENTER_NO_IOWAIT		(1U << 7)
```

- IORING_ENTER_GETEVENTS :	Wait until at least min_complete CQEs are ready before returning.
- IORING_ENTER_SQ_WAKEUP :	Wake up the kernel thread created when using IORING_SETUP_SQPOLL.
- IORING_ENTER_SQ_WAIT :	Wait until at least one entry is free in the submission ring before returning.
- IORING_ENTER_EXT_ARG :	(Since Linux 5.11) Interpret sig to be a io_uring_getevents_arg rather than a pointer to sigset_t. This structure can specify both a sigset_t and a timeout.
- IORING_ENTER_REGISTERED_RING :	ring_fd is an offset into the registered ring pool rather than a normal file descriptor.
- IORING_ENTER_ABS_TIMER :	(Since Linux 6.12) The timeout argument in the io_uring_getevents_arg is an absolute time, using the registered clock.



### io_uring_setup(2)
https://man7.org/linux/man-pages/man2/io_uring_setup.2.html

### io_uring_register(2) 支持的功能
<!-- c1a44df9-9e07-40cc-9d7d-e30074899ee0 -->

主要是这几类:
1. Ring : IORING_REGISTER_RING_FDS (不能理解!)
2. Buffers : 固定 buffer I/O（高性能）
3. File descriptors : 内核无需每次通过 fdtable 查找 file 结构，提升性能。
  - 但是通过 fd 查询找到文件的过程比较很低效的

https://man7.org/linux/man-pages/man2/io_uring_register.2.html
完整结果如下:

```c
/*
 * io_uring_register(2) opcodes and arguments
 */
enum io_uring_register_op {
	IORING_REGISTER_BUFFERS			= 0,
	IORING_UNREGISTER_BUFFERS		= 1,
	IORING_REGISTER_FILES			= 2,
	IORING_UNREGISTER_FILES			= 3,
	IORING_REGISTER_EVENTFD			= 4,
	IORING_UNREGISTER_EVENTFD		= 5,
	IORING_REGISTER_FILES_UPDATE		= 6,
	IORING_REGISTER_EVENTFD_ASYNC		= 7,
	IORING_REGISTER_PROBE			= 8,
	IORING_REGISTER_PERSONALITY		= 9,
	IORING_UNREGISTER_PERSONALITY		= 10,
	IORING_REGISTER_RESTRICTIONS		= 11,
	IORING_REGISTER_ENABLE_RINGS		= 12,

	/* extended with tagging */
	IORING_REGISTER_FILES2			= 13,
	IORING_REGISTER_FILES_UPDATE2		= 14,
	IORING_REGISTER_BUFFERS2		= 15,
	IORING_REGISTER_BUFFERS_UPDATE		= 16,

	/* set/clear io-wq thread affinities */
	IORING_REGISTER_IOWQ_AFF		= 17,
	IORING_UNREGISTER_IOWQ_AFF		= 18,

	/* set/get max number of io-wq workers */
	IORING_REGISTER_IOWQ_MAX_WORKERS	= 19,

	/* register/unregister io_uring fd with the ring */
	IORING_REGISTER_RING_FDS		= 20,
	IORING_UNREGISTER_RING_FDS		= 21,

	/* register ring based provide buffer group */
	IORING_REGISTER_PBUF_RING		= 22,
	IORING_UNREGISTER_PBUF_RING		= 23,

	/* sync cancelation API */
	IORING_REGISTER_SYNC_CANCEL		= 24,

	/* register a range of fixed file slots for automatic slot allocation */
	IORING_REGISTER_FILE_ALLOC_RANGE	= 25,

	/* return status information for a buffer group */
	IORING_REGISTER_PBUF_STATUS		= 26,

	/* set/clear busy poll settings */
	IORING_REGISTER_NAPI			= 27,
	IORING_UNREGISTER_NAPI			= 28,

	IORING_REGISTER_CLOCK			= 29,

	/* clone registered buffers from source ring to current ring */
	IORING_REGISTER_CLONE_BUFFERS		= 30,

	/* send MSG_RING without having a ring */
	IORING_REGISTER_SEND_MSG_RING		= 31,

	/* register a netdev hw rx queue for zerocopy */
	IORING_REGISTER_ZCRX_IFQ		= 32,

	/* resize CQ ring */
	IORING_REGISTER_RESIZE_RINGS		= 33,

	IORING_REGISTER_MEM_REGION		= 34,

	/* this goes last */
	IORING_REGISTER_LAST,

	/* flag added to the opcode to use a registered ring fd */
	IORING_REGISTER_USE_REGISTERED_RING	= 1U << 31
};
```

## sqe->flags 支持哪些
```c
/*
 * sqe->flags
 */
/* use fixed fileset */
#define IOSQE_FIXED_FILE	(1U << IOSQE_FIXED_FILE_BIT)
/* issue after inflight IO */
#define IOSQE_IO_DRAIN		(1U << IOSQE_IO_DRAIN_BIT)
/* links next sqe */
#define IOSQE_IO_LINK		(1U << IOSQE_IO_LINK_BIT)
/* like LINK, but stronger */
#define IOSQE_IO_HARDLINK	(1U << IOSQE_IO_HARDLINK_BIT)
/* always go async */
#define IOSQE_ASYNC		(1U << IOSQE_ASYNC_BIT)
/* select buffer from sqe->buf_group */
#define IOSQE_BUFFER_SELECT	(1U << IOSQE_BUFFER_SELECT_BIT)
/* don't post CQE if request succeeded */
#define IOSQE_CQE_SKIP_SUCCESS	(1U << IOSQE_CQE_SKIP_SUCCESS_BIT)
```

## https://kernel.dk/io_uring.pdf

### 8.1
When `O_DIRECT` is used, the kernel must map the application pages
into the kernel before it can do IO to them, and subsequently unmap those same pages when IO is done. This can be a
costly operation.

这是为什么?

难道不用 O_DIRECT 就不会有问题吗?
  - 如果是 buffer io ，内存是首先拷贝到内核的 page cache 中，
然后用 page cache 拷贝到用户态。所以，当提供页给硬件的时候，不用管用户态，
但是 O_DIRECT 就不一样，是直接用的用户态的页面，所以需要 pin 住。

难道 aio 不会来 map 这个区域吗?
这个的确很奇怪，因为如果 aio map 了这个区域，那么 aio 就不该有 -EFAULT

write / read 和 aio 也是

## [迟先生 : io_uring 的接口与实现](https://www.skyzh.dev/posts/articles/2021-06-14-deep-dive-io-uring/)

- [ ] https://github.com/frevib/io_uring-echo-server

## 更新
https://kernel.dk/io_uring-whatsnew.pdf

## [io_uring is not an event system](https://despairlabs.com/posts/2021-06-16-io-uring-is-not-an-event-system/)

## [zhihu : io_uring introduction](https://zhuanlan.zhihu.com/p/62682475?utm_source=wechat_timeline)
> io_uring 有如此出众的性能，主要来源于以下几个方面：
> 1. 用户态和内核态共享提交队列（submission queue）和完成队列（completion queue）
> 2. IO 提交和收割可以 offload 给 Kernel，且提交和完成不需要经过系统调用（system call）
> 3. 支持 Block 层的 Polling 模式
> 4. 通过提前注册用户态内存地址，减少地址映射的开销
> 5. 不仅如此，io_uring 还可以完美支持 buffered IO，而 libaio 对于 buffered IO 的支持则一直是被诟病的地方。

- [ ] polling mode
- [ ] 支持 buffer io 到底是什么含义?

## [Linux I/O 原理和 Zero-copy 技术全面揭秘](https://zhuanlan.zhihu.com/p/308054212)
- [ ] sendfile, etc

各种零拷贝的技术的总结

## [Efficient IO with io_uring](https://kernel.dk/io_uring.pdf)
read the doc :
aio's limitation :
1. only support async IO for O_DIRECT
> TODO how O_DIRECT works
> 1. checked
> 2. skip page cache
2. some constrans for sync submit
3. api lead noticeable copying

TODO please continue the documentation


- [ ] [liburing](https://github.com/axboe/liburing)

read write 会直到事情做完
epoll : 当有的工作做好之后，通知，其实是一个 thread 可以监控一大群的 socket，任何一个 ready 之后就开始工作

https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/ : 这个也可以参考参考
https://kernel-recipes.org/en/2019/talks/faster-io-through-io_uring/

[nice blog](https://github.com/shuveb/io_uring-by-example) provide example code about liburing and directly syscall


[Man page]
  - io_uring_setup :

- IORING_SETUP_IOPOLL
- IORING_SETUP_SQPOLL
- IORING_ENTER_GETEVENTS
- IORING_ENTER_SQ_WAKEUP

- https://lwn.net/Articles/879724/ ：终于，两年之后，开始处理网络的问题了

- https://lwn.net/Articles/903855/ : 据说可以优化 qcow2
  - https://lore.kernel.org/io-uring/20220509092312.254354-1-ming.lei@redhat.com/


- https://unixism.net/2020/04/io-uring-by-example-article-series/
- https://lwn.net/Articles/776703/

[io uring and eBPF](https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/)

## 细节
IORING_SETUP_NO_SQARRAY

IORING_FEAT_SINGLE_MMAP : 既然 io_uring 和 virtio 为什么都去将 queue 设置为 1 个

到底为什么要 register 来着?

## 分析下这个报告

- https://kernel-recipes.org/en/2022/wp-content/uploads/2022/06/axboe-kr2022-1.pdf

> AIO not widely used because it only supports one niche use case, even 20 years later.

啊，只有一个 use case ，是什么 use case ?

> Requests passing in data structs need to ensure
> validity only until submit is done, not until
> completion.

话虽如此，我们知道 request 已经被提交了

> Originally io-wq used kernel threads that assumed the identity of the original task when needed. This was risky.
>
> Available in 5.12, io-wq is based on io-threads. These are normal task threads, except they never leave the kernel and they don’t take signals.

什么意思，assumed the identity of original task ?

> Native io-threads eliminate security concerns with io-wq offload, for the requests that need that.
>
> It also makes offload a bit more efficient, as no
> identify switching is needed (files_struct, mm, creds,
> etc).
>
> It also fixes cases that didn’t previously work, like
> /proc/self, reading from signalfd, etc.
>
> Enables IORING_SETUP_SQPOLL to work with any file
> type, or any request in general, and without
> privilege requirements.

1. 啊，native worker 可以做到这么多功能!

> io-wq used to just block when offloaded.

> Normal flow of request is attempt to issue, arm
> poll if data / space not available.
>
> IORING_RECVSEND_POLL_FIRST
> Don’t attempt issue first, go straight to poll.
>
> IORING_CQE_F_SOCK_NONEMPTY
> Previous eg recv() returns if there was more
> data available.

1. 什么叫做 arm poll 的问题 ?

TO BE CONTINUE !

## 即便是这个也是需要读读的
https://man7.org/linux/man-pages/man7/io_uring.7.html

## https://lwn.net/Articles/863071/
## https://github.com/frevib/io_uring-echo-server

## 其他
https://news.ycombinator.com/item?id=35547316 : io uring is syscall batch

## io uring
[Descriptorless files for io_uring](https://lwn.net/Articles/863071/)

[Why you should use io_uring for network I/O](https://developers.redhat.com/articles/2023/04/12/why-you-should-use-iouring-network-io)


## I/O Passthru: Upstreaming a flexible and efficient I/O Path in Linux (fast 2024)
https://www.usenix.org/conference/fast24/presentation/joshi

## 继续看看这个吧
- https://news.ycombinator.com/item?id=41992975 : rust iouring
- https://news.ycombinator.com/item?id=42135412

## 记住这些东西
https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023

## 有趣的这个东西
https://vmsplice.net/~stefan/stefanha-fosdem-2021.pdf


## Redesigned workqueues for io_uring
https://lwn.net/Articles/803070/

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
