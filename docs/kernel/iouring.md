# io uring

## [Comparing VIRTIO, NVMe, and io_uring queue designs](https://blog.vmsplice.net/2022/06/comparing-virtio-nvme-and-iouring-queue.html)

## 基于 io-uring 的 ubd： https://lwn.net/Articles/900690/

## 系统调用
- io_uring_setup : 初始化
- io_uring_enter : 提交系统调用
- io_uring_register : 构建将要进行何种 io

## [迟先生 : io_uring 的接口与实现](https://www.skyzh.dev/posts/articles/2021-06-14-deep-dive-io-uring/)
> SQ_RING 中只存储 SQE 在 SQEs 区域中的序号，CQ_RING 存储完整的任务完成数据


- [ ] https://github.com/frevib/io_uring-echo-server : 别 手高眼低 了，先学会如何使用吧。

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

- https://github.com/tokio-rs/io-uring
- https://lwn.net/Articles/879724/ ：终于，两年之后，开始处理网络的问题了

- https://lwn.net/Articles/903855/ : 据说可以优化 qcow2
  - https://lore.kernel.org/io-uring/20220509092312.254354-1-ming.lei@redhat.com/


- https://unixism.net/2020/04/io-uring-by-example-article-series/
- https://lwn.net/Articles/776703/

[io uring and eBPF](https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/)



## 问题
- 为什么 def_blk_fops iopoll 完全没有办法用上啊
  - 似乎 iopoll 的用户似乎只有 iouring 的

- 难道是我的 fio 有问题，也许 fio 使用的 liburing 的动态库有问题?
```c
const struct file_operations def_blk_fops = {
	.open		= blkdev_open,
	.release	= blkdev_close,
	.llseek		= blkdev_llseek,
	.read_iter	= blkdev_read_iter,
	.write_iter	= blkdev_write_iter,
	.iopoll		= iocb_bio_iopoll,
	.mmap		= generic_file_mmap,
	.fsync		= blkdev_fsync,
	.unlocked_ioctl	= blkdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= compat_blkdev_ioctl,
#endif
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.fallocate	= blkdev_fallocate,
};
```

## Welcome to Lord of the io_uring
https://news.ycombinator.com/item?id=23132549

## [io_uring is not an event system](https://news.ycombinator.com/item?id=27540248)

看了半天，一会搞出什么?  都是类似这种:
> epoll is based on a "readiness" model (i.e. it tells when when you can start I/O). io_uring is based on a "completion" model (i.e. it tells you when I/O is finished).

相比 aio，似乎只是少了一次 getevent 那个系统调用而已。

## 也不知道相对于 epoll 存在什么优势
虽然存在测试
https://github.com/frevib/io_uring-echo-server

我感觉更多的是因为 aio 搞不好就不是真的 aio 吧

## 代码细节分析

| Files         | Lines | Code | Comments | Blanks | details |
|---------------|-------|------|----------|--------|--|
| ./io_uring.c  | 4557  | 3341 | 583      | 633    |
| ./net.c       | 1498  | 1236 | 49       | 213    |
| ./rsrc.c      | 1305  | 1015 | 109      | 181    |
| ./io-wq.c     | 1349  | 1005 | 131      | 213    |
| ./rw.c        | 1083  | 782  | 154      | 147    | 似乎是进行 rw 的主要位置
| ./poll.c      | 1045  | 737  | 165      | 143    | 感觉就像是普通的 poll 机制
| ./opdef.c     | 673   | 657  | 5        | 11     |
| ./timeout.c   | 692   | 538  | 51       | 103    |
| ./kbuf.c      | 636   | 482  | 56       | 98     |
| ./sqpoll.c    | 419   | 328  | 22       | 69     | 对于 submission 进行 poll
| ./tctx.c      | 334   | 264  | 17       | 53     |
| ./cancel.c    | 314   | 254  | 13       | 47     |
| ./msg_ring.c  | 300   | 236  | 19       | 45     |
| ./fs.c        | 293   | 228  | 1        | 64     |
| ./openclose.c | 262   | 205  | 14       | 43     |
| ./xattr.c     | 254   | 200  | 1        | 53     |
| ./fdinfo.c    | 216   | 177  | 13       | 26     |
| ./filetable.c | 181   | 145  | 5        | 31     |
| ./uring_cmd.c | 158   | 126  | 6        | 26     |
| ./splice.c    | 121   | 101  | 1        | 19     |
| ./advise.c    | 104   | 86   | 1        | 17     |
| ./sync.c      | 112   | 85   | 4        | 23     |
| ./notif.c     | 86    | 73   | 0        | 13     |
| ./statx.c     | 73    | 56   | 1        | 16     |
| ./epoll.c     | 65    | 51   | 1        | 13     |
| ./nop.c       | 25    | 17   | 4        | 4      |

如何理解?
```txt
enum io_uring_op {
	IORING_OP_NOP,
	IORING_OP_READV,
  // ...
}
```

```c
struct io_kiocb {

	u8				opcode;
```

- 这个流程很平常啊!

- io_issue_sqe => io_issue_def::issue
  - io_iter_do_read
    - call_read_iter
      - file->f_op->read_iter(kio, iter)

> 如果没有在创建 io_uring 时指定 IORING_SETUP_IOPOLL 选项，io_uring 的操作就会放进 io-wq 中执行。

## 分析
- io_uring_register
  - `__io_uring_register` : 可以注册 buffer file eventfd 之类的


### sqe->flags

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

### 区分 IORING_SETUP_IOPOLL 和 IOSQE_ASYNC

### 难道所谓的 async io on buffered io 是通过 io wq 实现的吗?


### 分析 IOSQE_ASYNC 的效果

### 为什么将 workqueue 替代为 wq-io

https://lore.kernel.org/linux-block/20191024134439.28498-1-axboe@kernel.dk/T/

> This adds support for io-wq, a smaller and specialized thread pool
> implementation. This is meant to replace workqueues for io_uring. Among
> the reasons for this addition are:
>
> - We can assign memory context smarter and more persistently if we
>   manage the life time of threads.
>
> - We can drop various work-arounds we have in io_uring, like the
>   async_list.
>
> - We can implement hashed work insertion, to manage concurrency of
>   buffered writes without needing a) an extra workqueue, or b)
>   needlessly making the concurrency of said workqueue very low
>   which hurts performance of multiple buffered file writers.
>
> - We can implement cancel through signals, for cancelling
>   interruptible work like read/write (or send/recv) to/from sockets.
>
> - We need the above cancel for being able to assign and use file tables
>   from a process.
>
> - We can implement a more thorough cancel operation in general.
>
> - We need it to move towards a syslet/threadlet model for even faster
>   async execution. For that we need to take ownership of the used
>   threads.
>
> This list is just off the top of my head. Performance should be the
> same, or better, at least that's what I've seen in my testing. io-wq
> supports basic NUMA functionality, setting up a pool per node.
>
> io-wq hooks up to the scheduler schedule in/out just like workqueue
> and uses that to drive the need for more/less workers.


## 三种基本模式?
https://blogs.oracle.com/linux/post/an-introduction-to-the-io-uring-asynchronous-io-framework
1. interrupt driven
2. Polled
3. Kernel polled

## 还是先会使用再说吧 liburing/examples/

io_uring-udp.c 不会用

send-zerocopy.c

ucontext-cp.c

### io_uring-close-test.c
- io_uring_queue_init
- io_uring_register_ring_fd
- io_uring_close_ring_fd : 为什么要立刻关闭这个 fd 啊

```txt
io_uring_setup(4, {flags=0, sq_thread_cpu=0, sq_thread_idle=0, sq_entries=4, cq_entries=8, features=IORING_FEAT_SINGLE_MMAP|IORING_FEAT_NODROP|IORING_FEAT_SUBMIT_STABLE|IORING_FEAT_RW_CUR_POS|IORING_FEAT_CUR_PERSONALITY|IORING_FEAT_FAST_POLL|IORING_FEAT_POLL_32BITS|IORING_FEAT_SQPOLL_NONFIXED|IORING_FEAT_EXT_ARG|IORING_FEAT_NATIVE_WORKERS|IORING_FEAT_RSRC_TAGS|IORING_FEAT_CQE_SKIP|IORING_FEAT_LINKED_FILE|IORING_FEAT_REG_REG_RING, sq_off={head=0, tail=64, ring_mask=256, ring_entries=264, flags=276, dropped=272, array=448}, cq_off={head=128, tail=192, ring_mask=260, ring_entries=268, overflow=284, cqes=320, flags=280}}) = 3
mmap(NULL, 464, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, 3, 0) = 0x7fcf6bfc3000
mmap(NULL, 256, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, 3, 0x10000000) = 0x7fcf6bfc2000
io_uring_register(3, IORING_REGISTER_RING_FDS, [{offset=-1, data=3}], 1) = 1
close(3)                                = 0
openat(AT_FDCWD, "io_uring-test.c", O_RDONLY) = 3
newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=2256, ...}, AT_EMPTY_PATH) = 0
getrandom("\xa3\x66\xe0\xfb\xb2\x13\x7d\xb1", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x1a0c000
brk(0x1a2d000)                          = 0x1a2d000
io_uring_enter(0, 1, 0, IORING_ENTER_REGISTERED_RING, NULL, 8) = 1
newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x4), ...}, AT_EMPTY_PATH) = 0
write(1, "Submitted=1, completed=1, bytes="..., 37Submitted=1, completed=1, bytes=2256)  = 37
close(3)                                = 0
munmap(0x7fcf6bfc2000, 256)             = 0
munmap(0x7fcf6bfc3000, 464)             = 0
io_uring_register(0, 0x80000015 /* IORING_REGISTER_??? */, 0x7ffda4ab94f0, 1) = 1
```

## [x] 很多函数上都有 `__cold`

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

- io_queue_async

- io_wq_submit_work
  - io_arm_poll_handler
     - `__io_arm_poll_handler`
- io_poll_check_events

## 分析下 https://unixism.net/loti/tutorial/sq_poll.html

- IORING_SETUP_SQPOLL : 存在一个 kernel thread 来 poll sq_thread_idle


```c
/*
 * Passed in for io_uring_setup(2). Copied back with updated info on success
 */
struct io_uring_params {
	__u32 sq_entries;
	__u32 cq_entries;
	__u32 flags;
	__u32 sq_thread_cpu;
	__u32 sq_thread_idle;
	__u32 features;
	__u32 wq_fd;
	__u32 resv[3];
	struct io_sqring_offsets sq_off;
	struct io_cqring_offsets cq_off;
};
```

- io_uring_setup
   - io_sq_offload_create : io_uring_params::sq_thread_idle 传递到 io_ring_ctx::sq_thread_idle
   - create_io_thread(io_sq_thread, sqd, NUMA_NO_NODE)

- io_sq_thread


## poll.c 是做什么的

https://vmsplice.net/~stefan/stefanha-fosdem-2021.pdf

```c
const struct io_issue_def io_issue_defs[] = {
  // ...
	[IORING_OP_POLL_ADD] = {
		.needs_file		= 1,
		.unbound_nonreg_file	= 1,
		.audit_skip		= 1,
		.prep			= io_poll_add_prep,
		.issue			= io_poll_add,
	},
	[IORING_OP_POLL_REMOVE] = {
		.audit_skip		= 1,
		.prep			= io_poll_remove_prep,
		.issue			= io_poll_remove,
	},
```

## io wq
- io_wq_submit_work
  - while loop { io_issue_sqe; io_arm_poll_handler; }
  - io_issue_sqe
  - io_arm_poll_handler
    - `__io_arm_poll_handler`
      - vfs_poll

## rsrc.c 主要做啥的?

资源管理的:

- io_sqe_buffers_register
  - io_buffers_map_alloc

看一个典型的结构:
```txt
@[
    io_sqe_buffers_register+5
    __do_sys_io_uring_register+2556
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1
```

至于能够注册什么资源，参考一下 : `__io_uring_register`

参考下这个深入理解下，到底在注册什么鸡儿东西:
https://unixism.net/loti/ref-iouring/io_uring_register.html

应该是，先 register 之后，才能提交 io_uring_prep_readv  io_uring_enter
