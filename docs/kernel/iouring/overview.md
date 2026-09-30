# iouring 基础

## 整体印象

[zhihu : io_uring introduction](https://zhuanlan.zhihu.com/p/62682475)
> io_uring 有如此出众的性能，主要来源于以下几个方面：
> 1. 用户态和内核态共享提交队列（submission queue）和完成队列（completion queue）
> 2. IO 提交和收割可以 offload 给 Kernel，且提交和完成不需要经过系统调用（system call）
> 3. 支持 Block 层的 Polling 模式
> 4. 通过提前注册用户态内存地址，减少地址映射的开销
> 5. 不仅如此，io_uring 还可以完美支持 buffered IO，而 libaio 对于 buffered IO 的支持则一直是被诟病的地方。

io uring 可以使用的位置
- 替代 aio
- network
  - https://developer.aliyun.com/article/834974
- epoll
- async submit syscall
- 替代 dpdk / spdk :



## 基本 API 使用
推荐 : https://nick-black.com/dankwiki/index.php/Io_uring

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
<!-- 3a707a3f-cc70-46ab-8dd1-51a10e5ee525 -->
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

## ioring 函数 `__cold`

例如:
```c
static __cold void io_tctx_exit_cb(struct callback_head *cb)
```

```c
/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-cold-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html#index-cold-label-attribute
 *
 * When -falign-functions=N is in use, we must avoid the cold attribute as
 * contemporary versions of GCC drop the alignment for cold functions. Worse,
 * GCC can implicitly mark callees of cold functions as cold themselves, so
 * it's not sufficient to add __function_aligned here as that will not ensure
 * that callees are correctly aligned.
 *
 * See:
 *
 *   https://lore.kernel.org/lkml/Y77%2FqVgvaJidFpYt@FVFF77S0Q05N
 *   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88345#c9
 */
#if !defined(CONFIG_CC_IS_GCC) || (CONFIG_FUNCTION_ALIGNMENT == 0)
#define __cold				__attribute__((__cold__))
#else
#define __cold
#endif
```

文档： https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html

表示该函数执行概率很低。

- io_wq_submit_work
  - io_arm_poll_handler
     - `__io_arm_poll_handler`
- io_poll_check_events

## show_fdinfo

```txt
[root@nixos:/proc/118496/fdinfo]# cat 4
pos:    0
flags:  02000002
mnt_id: 14
ino:    240059
SqMask: 0x7f
SqHead: 3777186
SqTail: 3777186
CachedSqHead:   3777186
CqMask: 0x7f
CqHead: 3777060
CqTail: 3777091
CachedCqTail:   3777091
SQEs:   0
CQEs:   31
   36: user_data:0, res:4096, flag:0
   37: user_data:0, res:4096, flag:0
   38: user_data:0, res:4096, flag:0
   39: user_data:0, res:4096, flag:0
   40: user_data:0, res:4096, flag:0
   41: user_data:0, res:4096, flag:0
   42: user_data:0, res:4096, flag:0
   43: user_data:0, res:4096, flag:0
   44: user_data:0, res:4096, flag:0
   45: user_data:0, res:4096, flag:0
   46: user_data:0, res:4096, flag:0
   47: user_data:0, res:4096, flag:0
   48: user_data:0, res:4096, flag:0
   49: user_data:0, res:4096, flag:0
   50: user_data:0, res:4096, flag:0
   51: user_data:0, res:4096, flag:0
   52: user_data:0, res:4096, flag:0
   53: user_data:0, res:4096, flag:0
   54: user_data:0, res:4096, flag:0
   55: user_data:0, res:4096, flag:0
   56: user_data:0, res:4096, flag:0
   57: user_data:0, res:4096, flag:0
   58: user_data:0, res:4096, flag:0
   59: user_data:0, res:4096, flag:0
   60: user_data:0, res:4096, flag:0
   61: user_data:0, res:4096, flag:0
   62: user_data:0, res:4096, flag:0
   63: user_data:0, res:4096, flag:0
   64: user_data:0, res:4096, flag:0
   65: user_data:0, res:4096, flag:0
   66: user_data:0, res:4096, flag:0
SqThread:       -1
SqThreadCpu:    -1
UserFiles:      1
UserBufs:       128
PollList:
CqOverflowList:
```

## Links
- https://man7.org/linux/man-pages/man7/io_uring.7.html
- https://lwn.net/Articles/863071/
- https://github.com/frevib/io_uring-echo-server
- https://news.ycombinator.com/item?id=35547316 : io uring is syscall batch
- https://www.usenix.org/conference/fast24/presentation/joshi
	- I/O Passthru: Upstreaming a flexible and efficient I/O Path in Linux (fast 2024)
- https://news.ycombinator.com/item?id=41992975 : rust iouring
- https://news.ycombinator.com/item?id=42135412
- [迟先生 : io_uring 的接口与实现](https://www.skyzh.dev/posts/articles/2021-06-14-deep-dive-io-uring/)
- [io_uring is not an event system](https://despairlabs.com/posts/2021-06-16-io-uring-is-not-an-event-system/)
- https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/ : 这个也可以参考参考
- https://kernel-recipes.org/en/2019/talks/faster-io-through-io_uring/
- https://github.com/shuveb/io_uring-by-example
- https://lwn.net/Articles/903855/ : 据说可以优化 qcow2
  - https://lore.kernel.org/io-uring/20220509092312.254354-1-ming.lei@redhat.com/
- https://unixism.net/2020/04/io-uring-by-example-article-series/
- https://lwn.net/Articles/776703/
- https://kernel.dk/io_uring.pdf

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
