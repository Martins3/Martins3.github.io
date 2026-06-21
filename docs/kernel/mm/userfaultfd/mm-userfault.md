# Userfaultfd

1. 可以一个 process 给另外一个 process 注册
2. 如果触发 WP，但是 handler 接受到通知之后，没有 unprotect 掉，
，结果 qemu 就会一直卡在哪里，所以这个 unprotect syscall 是用来唤醒 qemu 的。
3. 为什么 userfaultfd 可以让 page fault 的消息不漏不重
  - 每一个消息都是放到 queue 中的，自然不会漏掉

## DOC
- https://lwn.net/Articles/718198/
- https://lwn.net/Articles/897260/
- http://blog.jcix.top/2018-10-01/userfaultfd_intro/
- [ ] https://noahdesu.github.io/2016/10/10/userfaultfd-hello-world.html#:~:text=The%20userfaultfd%20feature%20in%20the,pages%20and%20handling%20write%20events.

- https://lwn.net/Articles/615086/
- https://github.com/LLNL/umap

- https://www.cons.org/cracauer/cracauer-userfaultfd.html
- https://github.com/torvalds/linux/blob/master/Documentation/vm/userfaultfd.txt
- https://github.com/torvalds/linux/blob/master/include/uapi/linux/userfaultfd.h

- https://docs.huihoo.com/kvm/kvm-forum/2016/Userland-Page-Faults-and-Byond-Why-How-and-Whats-Next.pdf

- https://www.cons.org/cracauer/cracauer-userfaultfd.html : 这里还配置了踩坑说明

## http://brieflyx.me/2020/linux-tools/userfaultfd-internals/
涉及 userfaultfd 处理的主要有以下几个文件:
- fs/userfaultfd.c：主要逻辑都在该文件中
- mm/userfaultfd.c：一些跟页表相关的底层函数
- mm/memory.c：通用的处理 page fault 的代码
- mm/mremap.c / mm/mmap.c：处理 mremap 和 mmap 的代码，本文暂时不研究

## 问题
- fs/userfaultfd.c 和 mm/userfaultfd.c 的关系
- UFFDIO_COPY_MODE_DONTWAKE 效果测试一下

- 逐个尝试下:
```c
static long userfaultfd_ioctl(struct file *file, unsigned cmd,
			      unsigned long arg)
{
	int ret = -EINVAL;
	struct userfaultfd_ctx *ctx = file->private_data;

	if (cmd != UFFDIO_API && !userfaultfd_is_initialized(ctx))
		return -EINVAL;

	switch(cmd) {
	case UFFDIO_API:
		ret = userfaultfd_api(ctx, arg);
		break;
	case UFFDIO_REGISTER:
		ret = userfaultfd_register(ctx, arg);
		break;
	case UFFDIO_UNREGISTER:
		ret = userfaultfd_unregister(ctx, arg);
		break;
	case UFFDIO_WAKE:
		ret = userfaultfd_wake(ctx, arg);
		break;
	case UFFDIO_COPY:
		ret = userfaultfd_copy(ctx, arg);
		break;
	case UFFDIO_ZEROPAGE:
		ret = userfaultfd_zeropage(ctx, arg);
		break;
	case UFFDIO_MOVE:
		ret = userfaultfd_move(ctx, arg);
		break;
	case UFFDIO_WRITEPROTECT:
		ret = userfaultfd_writeprotect(ctx, arg);
		break;
	case UFFDIO_CONTINUE:
		ret = userfaultfd_continue(ctx, arg);
		break;
	case UFFDIO_POISON:
		ret = userfaultfd_poison(ctx, arg);
		break;
	}
	return ret;
}
```

当 thread 出发 page fault 的时候:
```txt
➜  ~ cat /proc/2239/stack
[<0>] handle_userfault+0x5c9/0x710
[<0>] handle_mm_fault+0x804/0x1160
[<0>] do_user_addr_fault+0x1f8/0x780
[<0>] exc_page_fault+0x89/0x1f0
[<0>] asm_exc_page_fault+0x26/0x30
```

## /dev/userfaultfd
https://mp.weixin.qq.com/s/2kChzwgU-k-DLJdPQROijQ

## 问题
每次 fault 都是单独的吗?

```txt
       If multiple events are available and the supplied buffer is large enough, read(2) returns as many events as will  fit
       in  the  supplied  buffer.  If the buffer supplied to read(2) is smaller than the size of the uffd_msg structure, the
       read(2) fails with the error EINVAL.
```
不是的，可以多个读出来，但是无法避免那些问题。

```c
static void init_once_userfaultfd_ctx(void *mem)
{
	struct userfaultfd_ctx *ctx = (struct userfaultfd_ctx *) mem;

	init_waitqueue_head(&ctx->fault_pending_wqh);
	init_waitqueue_head(&ctx->fault_wqh);
	init_waitqueue_head(&ctx->event_wqh);
	init_waitqueue_head(&ctx->fd_wqh);
	seqcount_spinlock_init(&ctx->refile_seq, &ctx->fault_pending_wqh.lock);
}
```

## 关键问题 : 可以构建多个 thread 来实现多线程的 swap in 吗?

1. 可以调用多个 userfaultfd 的系统调用吗?
2. 如果不可以， 那么 if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1) 的调用是否可以同时调用

Documentation/admin-guide/mm/userfaultfd.rst

这里说的 Non-cooperative userfaultfd 是什么意思:

> The current asynchronous model of the event delivery is optimal for
> single threaded non-cooperative ``userfaultfd`` manager implementations. A
> synchronous event delivery model can be added later as a new
> ``userfaultfd`` feature to facilitate multithreading enhancements of the
> non cooperative manager, for example to allow ``UFFDIO_COPY`` ioctls to
> run in parallel to the event reception. Single threaded
> implementations should continue to use the current async event
> delivery model instead.

从内核中入口是: handle_userfault

userfaultfd_poll 用户态的

### 似乎 userfault 只能是单线程的 ?

## 解决一个 bug ，很棒
https://mp.weixin.qq.com/s/Eyeq3UBBr1pZY_qgxLPdkA

## 为什么 tools/testing/selftests/kvm/
有 userfaultfd 的测试，那个和 kvm 有什么关系吗?

## 为什么 android 的 art 虚拟机需要用 userfaultfd
看来理解这个问题还比较远，恐怕需要理解一下 art 大致处理内存的方法才可以

实在是有趣的机制

## UFFDIO_MOVE 机制

很遗憾，这个功能在 5.10 内核中还没有。

https://lwn.net/Articles/947123/ commit messages 说，之前一直都没有找到应用场景，
这很奇怪，这个东西为什么会没有使用场景：

猜测是，如果知道了这个空间需要内存，就算从网卡或者存储系统获取到数据填充到内存中。
这些内存要在内核中拷贝一次。所以，正常来说，应该是直接填充到页面中，然后 move 过去.

似乎主要的问题在于，move 过去之后，还需要从内核中分配内存，这个分配过程不会比拷贝少花时间。
看来内存拷贝没有想象的花费时间。

pr 引用这里数据: https://lore.kernel.org/all/1425575884-2574-1-git-send-email-aarcange@redhat.com/

> The UFFDIO_REMAP method is still present in the patchset but it's
> provided primarily to remove (add not) memory from the userfault
> range. The addition of the UFFDIO_REMAP method is intentionally kept
> at the end of the patchset. The postcopy live migration qemu code will
> only use UFFDIO_COPY and UFFDIO_ZEROPAGE. UFFDIO_REMAP isn't intended
> to be merged upstream in the short term, and it can be dropped later
> if there's an agreement it's a bad idea to keep it around in the
> patchset.
>
> David run some KVM postcopy live migration benchmarks on a 8-way CPU
> system and he measured that using UFFDIO_COPY instead of UFFDIO_REMAP
> resulted in a roughly a -20% reduction in latency which is good. The
> standard deviation error on the latency measurement decreased
> significantly as well (because the number of CPUs that required IPI
> delivery was variable, while the copy always takes roughly the same
> time). A bigger improvement is expectable if measured on a larger host
> with more CPUs.


## 可以有 multithreading userfaultfd 吗?

## 可以使用 iouring 并行的提交 userfaultfd 的请求吗?

## https://ctf-wiki.org/pwn/linux/kernel-mode/exploitation/race/userfaultfd/


## 什么问题?
ram_block_discard_range 中

原来在 share memory 体系下，需要使用的系统调用是不一样的:
```txt
              ret = madvise(host_startaddr, length, QEMU_MADV_REMOVE);
          } else {
              ret = madvise(host_startaddr, length, QEMU_MADV_DONTNEED);
```
会一路调用到 shmem_fallocate 上，调用完之后，

需要对于该区域移除掉 userfaultfd_remove ，不太理解这个导致的结果是什么
似乎会有一些大问题。

## 为什么内核中的
> When using ``UFFDIO_REGISTER_MODE_WP`` in combination with either
> ``UFFDIO_REGISTER_MODE_MISSING`` or ``UFFDIO_REGISTER_MODE_MINOR``, when
> resolving missing / minor faults with ``UFFDIO_COPY`` or ``UFFDIO_CONTINUE``
> respectively, it may be desirable for the new page / mapping to be
> write-protected (so future writes will also result in a WP fault). These ioctls
> support a mode flag (``UFFDIO_COPY_MODE_WP`` or ``UFFDIO_CONTINUE_MODE_WP``
> respectively) to configure the mapping this way.

为什么如果想要可以 UFFDIO_COPY_MODE_WP ，那么就必须在 register 的时候提供
UFFDIO_REGISTER_MODE_WP 才可以?

## 这些 flag 的作用是什么?

```txt
#define UFFD_FEATURE_PAGEFAULT_FLAG_WP		(1<<0)
#define UFFD_FEATURE_EVENT_FORK			(1<<1)
#define UFFD_FEATURE_EVENT_REMAP		(1<<2)
#define UFFD_FEATURE_EVENT_REMOVE		(1<<3)
#define UFFD_FEATURE_MISSING_HUGETLBFS		(1<<4)
#define UFFD_FEATURE_MISSING_SHMEM		(1<<5)
#define UFFD_FEATURE_EVENT_UNMAP		(1<<6)
#define UFFD_FEATURE_SIGBUS			(1<<7)
#define UFFD_FEATURE_THREAD_ID			(1<<8)
#define UFFD_FEATURE_MINOR_HUGETLBFS		(1<<9)
#define UFFD_FEATURE_MINOR_SHMEM		(1<<10)
#define UFFD_FEATURE_EXACT_ADDRESS		(1<<11)
#define UFFD_FEATURE_WP_HUGETLBFS_SHMEM		(1<<12)
#define UFFD_FEATURE_WP_UNPOPULATED		(1<<13)
#define UFFD_FEATURE_POISON			(1<<14)
#define UFFD_FEATURE_WP_ASYNC			(1<<15)
#define UFFD_FEATURE_MOVE			(1<<16)
```

## 分析 aio 和 uffd 并发的 write protect 问题

发起 aio read ，同时对于页面 write protect ，在注册页面的时候，就会被阻塞:
显然，事情不能推迟到 DMA 写的时候。
```txt
sudo cat /proc/76396/stack
[<0>] handle_userfault+0x387/0x560
[<0>] __handle_mm_fault+0x30d/0x740
[<0>] handle_mm_fault+0xaa/0x2b0
[<0>] __get_user_pages+0x181/0x5d0
[<0>] __gup_longterm_locked+0xc0/0x990
[<0>] gup_fast_fallback+0xf0/0x160
[<0>] iov_iter_extract_pages+0xe8/0x6c0
[<0>] __bio_iov_iter_get_pages+0x80/0x3d0
[<0>] bio_iov_iter_get_pages+0x3c/0xb0
[<0>] iomap_dio_bio_iter+0x230/0x510
[<0>] __iomap_dio_rw+0x376/0x630
[<0>] iomap_dio_rw+0x12/0x40
[<0>] ext4_file_read_iter+0x14a/0x1c0 [ext4]
[<0>] aio_read+0xec/0x1c0
[<0>] io_submit_one+0xe1/0x360
[<0>] __x64_sys_io_submit+0x85/0x1d0
[<0>] do_syscall_64+0x5f/0x170
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

问题  1 :
但是，这里的 D 状态之后，程序可以被 kill 的
```txt
sudo cat /proc/76396/status
Name:   client.out
Umask:  0022
State:  D (disk sleep)
```

这个应该是内核的 bug 吧，既然已经这样设置了，岂不是会让
内核中有的资源无法释放?

普通的 io 的情况是这个:
```txt
 sudo cat /proc/124638/stack
[<0>] handle_userfault+0x387/0x560
[<0>] shmem_get_folio_gfp+0x597/0x5c0
[<0>] shmem_fault+0x7c/0x120
[<0>] __do_fault+0x33/0x180
[<0>] do_shared_fault+0x2d/0x190
[<0>] do_fault+0x3d/0x3a0
[<0>] __handle_mm_fault+0x30d/0x740
[<0>] handle_mm_fault+0xaa/0x2b0
[<0>] do_user_addr_fault+0x208/0x670

🧀  cat /proc/124638/status
Name:   client.out
Umask:  0022
State:  S (sleeping)
```

本质原因，这里是调用的 schedule() ，收到信号之后，就会从这里返回:
```txt
	if (likely(must_wait && !READ_ONCE(ctx->released))) {
		wake_up_poll(&ctx->fd_wqh, EPOLLIN);
    // pr_info
		schedule();
	}
```

从日志看，被 kill 之后，wake_up_poll 又执行了一次
也就是说，存在两次唤醒。

通过 /home/martins3/data/vn/code/src/m/sched/process_state.c 的测试，
我认为这个极大的概率是内核的 bug 。

问题 2 ：
如果一个 page 正在被 DMA 使用，这个时候，如果尝试使用 madvise
去 drop 掉这个页面，会发生什么
madvise 会被阻塞到这个 page 可以被释放？ 还是直接返回失败?

(应该是 不会阻塞，也不会失败，如果可以释放就释放，不能释放就算了)
(所以，绝对不能保证 madvise 之后，这个 page 就消失了)


问题 3 : 考虑这种操作顺序，对于 page table 的构建

```txt
aio read 发起 DMA 操作，
由于 write protect 被保护
当 handle_userfault 返回
的时候 认为 page 依旧准备好
                                ms 将页面写回，
                                通过 madvise drop 掉 page
                                和 page table

                                接受到 write protect event，
                                通过 ioctl uffd 告知

继续? aio read 导致这个
页面重新分配一个?
还需要重建 page table ?
```

观察 handle_userfault 的返回值，总是返回的是
handle_userfault 0x400 ，也就是 VM_FAULT_RETRY

没有进一步深入了，但是认为是 VM_FAULT_RETRY 会重试。

第一个读操作，就是想该区域写入一个 A
第二个操作是 aio 被 write protect 挡住。

```txt
@[
        handle_userfault+5
        shmem_get_folio_gfp+1431
        shmem_fault+124
        __do_fault+51
        do_shared_fault+45
        do_fault+61
        __handle_mm_fault+781
        handle_mm_fault+170
        do_user_addr_fault+520
        exc_page_fault+106
        asm_exc_page_fault+38
]: 1
@[
        handle_userfault+5
        __handle_mm_fault+781
        handle_mm_fault+170
        __get_user_pages+385
        __gup_longterm_locked+192
        gup_fast_fallback+240
        iov_iter_extract_pages+232
        __bio_iov_iter_get_pages+128
        bio_iov_iter_get_pages+60
        iomap_dio_bio_iter+560
        __iomap_dio_rw+886
        iomap_dio_rw+18
        ext4_file_read_iter+330
        aio_read+236
        io_submit_one+225
        __x64_sys_io_submit+133
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

## 自己写的测试都是远远不如这个的
tools/testing/selftests/mm/uffd-unit-tests.c

## 这个 commit 有意思的

commit 7f1101a0a181 ("userfaultfd: fix a crash in UFFDIO_MOVE when PMD is a migration entry")

## 和 poll 的关系
userfaultfd_fops 使用 poll 来检查队列中有没有，
如果有，那么立刻返回，而 read_iter 从队列中取东西过来，
所以，可以一次取多个东西，并且一次性有多个。
```c
static const struct file_operations userfaultfd_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= userfaultfd_show_fdinfo,
#endif
	.release	= userfaultfd_release,
	.poll		= userfaultfd_poll,
	.read_iter	= userfaultfd_read_iter,
	.unlocked_ioctl = userfaultfd_ioctl,
	.compat_ioctl	= compat_ptr_ioctl,
	.llseek		= noop_llseek,
};
```

## userfaultfd
- https://lore.kernel.org/all/8405A413-239A-47E8-9D46-B6060EF86A68@gmail.com/

## 关键功能的版本
- UFFDIO_COPY_MODE_WP : v5.7-rc1 (但是实际上测试发现，openeuler 6.6 不能正常使用，具体原因需要分析下)
- HAVE_ARCH_USERFAULTFD_WP : arm 需要到 6.9 才支持

## 对于这个东西还是有疑惑，为什么我一定需要来着?
```txt
   echo 1 | sudo tee /proc/sys/vm/unprivileged_userfaultfd
[sudo] password for martins3:
1
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
