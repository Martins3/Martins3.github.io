# Userfaultfd

涉及 userfaultfd 处理的主要有以下几个文件:
- fs/userfaultfd.c：主要逻辑都在该文件中
- mm/userfaultfd.c：一些跟页表相关的底层函数
- mm/memory.c：通用的处理 page fault 的代码
- mm/mremap.c / mm/mmap.c：处理 mremap 和 mmap 的代码，本文暂时不研究

## DOC
- https://lwn.net/Articles/718198/
- https://lwn.net/Articles/897260/
- http://blog.jcix.top/2018-10-01/userfaultfd_intro/
- [ ] https://noahdesu.github.io/2016/10/10/userfaultfd-hello-world.html#:~:text=The%20userfaultfd%20feature%20in%20the,pages%20and%20handling%20write%20events.

- https://lwn.net/Articles/615086/
- https://github.com/LLNL/umap
- https://ctf-wiki.org/pwn/linux/kernel-mode/exploitation/race/userfaultfd/
- https://www.cons.org/cracauer/cracauer-userfaultfd.html

- https://github.com/torvalds/linux/blob/master/Documentation/vm/userfaultfd.txt
- https://github.com/torvalds/linux/blob/master/include/uapi/linux/userfaultfd.h

- https://docs.huihoo.com/kvm/kvm-forum/2016/Userland-Page-Faults-and-Byond-Why-How-and-Whats-Next.pdf
- https://www.cons.org/cracauer/cracauer-userfaultfd.html : 这里还配置了踩坑说明
- http://brieflyx.me/2020/linux-tools/userfaultfd-internals/

## 杂七杂八

kvm 测试中居然有这个:
tools/testing/selftests/kvm/lib/userfaultfd_util.c
官方的单元测试:
tools/testing/selftests/mm/uffd-unit-tests.c

## 问题
- fs/userfaultfd.c 和 mm/userfaultfd.c 的关系

当 thread 出发 page fault 的时候:
```txt
➜  ~ cat /proc/2239/stack
[<0>] handle_userfault+0x5c9/0x710
[<0>] handle_mm_fault+0x804/0x1160
[<0>] do_user_addr_fault+0x1f8/0x780
[<0>] exc_page_fault+0x89/0x1f0
[<0>] asm_exc_page_fault+0x26/0x30
```

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

## 安全性说明
从 /dev/userfaultfd 说起:

- https://mp.weixin.qq.com/s/2kChzwgU-k-DLJdPQROijQ
	- https://lwn.net/Articles/897260/

https://duasynt.com/blog/linux-kernel-heap-spray

```txt
echo 1 | sudo tee /proc/sys/vm/unprivileged_userfaultfd
```

那么如果希望 userfaultfd() 调用成功，就一定需要满足如下两个条件之一：
1. 要么调用进程具有 CAP_SYS_PTRACE 这个 capability，要么就需要在调用系统调用的时候提供 UFFD_USER_MODE_ONLY flag。
2. 在后者情况下，在内核中运行时遇到的 page fault 将不会通过 userfaultfd() 机制来处理，哪怕它们发生在注册进来的内存区域里。

### O_DIRECT 中的 ubuf 问题
ubuf 相关问题全部都在 lib/iov_iter.c 中

真的可以 write protect 吗?

解释 docs/kernel/iouring/doc.md 中的问题，为什么
io uring 在使用 O_DIRECT 的时候，就需要 map 到内核中?

ubuf 问题配合 userfault ，太方便了:

如果是 buffer write ，也就是 open 的时候没有 O_DIRECT
```txt
@[
        handle_userfault+5
        shmem_get_folio_gfp+924
        shmem_fault+128
        __do_fault+51
        do_read_fault+297
        do_fault+295
        __handle_mm_fault+800
        handle_mm_fault+324
        do_user_addr_fault+517
        exc_page_fault+106
        asm_exc_page_fault+38
        fault_in_readable+106
        fault_in_iov_iter_readable+74
        generic_perform_write+638
        ext4_buffered_write_iter+104
        aio_write+278
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

使用 O_DIRECT 的时候:
```txt
@[
        handle_userfault+5
        shmem_get_folio_gfp+924
        shmem_fault+128
        __do_fault+51
        do_read_fault+297
        do_fault+295
        __handle_mm_fault+800
        handle_mm_fault+324
        __get_user_pages+389
        __gup_longterm_locked+195
        gup_fast_fallback+272
        iov_iter_extract_pages+230
        __bio_iov_iter_get_pages+130
        bio_iov_iter_get_pages+60
        iomap_dio_bio_iter+572
        __iomap_dio_rw+827
        iomap_dio_rw+18
        ext4_dio_write_iter+412
        aio_write+278
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

uffd 的考虑:
```txt
History:        #0
Commit:         37cd0575b8510159992d279c530c05f872990b02
Author:         Lokesh Gidra <lokeshgidra@google.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Tue 15 Dec 2020 11:13:49 AM CST
Committer Date: Wed 16 Dec 2020 04:13:46 AM CST

userfaultfd: add UFFD_USER_MODE_ONLY

Patch series "Control over userfaultfd kernel-fault handling", v6.

This patch series is split from [1].  The other series enables SELinux
support for userfaultfd file descriptors so that its creation and movement
can be controlled.

It has been demonstrated on various occasions that suspending kernel code
execution for an arbitrary amount of time at any access to userspace
memory (copy_from_user()/copy_to_user()/...) can be exploited to change
the intended behavior of the kernel.  For instance, handling page faults
in kernel-mode using userfaultfd has been exploited in [2, 3].  Likewise,
FUSE, which is similar to userfaultfd in this respect, has been exploited
in [4, 5] for similar outcome.

This small patch series adds a new flag to userfaultfd(2) that allows
callers to give up the ability to handle kernel-mode faults with the
resulting UFFD file object.  It then adds a 'user-mode only' option to the
unprivileged_userfaultfd sysctl knob to require unprivileged callers to
use this new flag.

The purpose of this new interface is to decrease the chance of an
unprivileged userfaultfd user taking advantage of userfaultfd to enhance
security vulnerabilities by lengthening the race window in kernel code.

[1] https://lore.kernel.org/lkml/20200211225547.235083-1-dancol@google.com/
[2] https://duasynt.com/blog/linux-kernel-heap-spray
[3] https://duasynt.com/blog/cve-2016-6187-heap-off-by-one-exploit
[4] https://googleprojectzero.blogspot.com/2016/06/exploiting-recursion-in-linux-kernel_20.html
[5] https://bugs.chromium.org/p/project-zero/issues/detail?id=808

This patch (of 2):

userfaultfd handles page faults from both user and kernel code.  Add a new
UFFD_USER_MODE_ONLY flag for userfaultfd(2) that makes the resulting
userfaultfd object refuse to handle faults from kernel mode, treating
these faults as if SIGBUS were always raised, causing the kernel code to
fail with EFAULT.

A future patch adds a knob allowing administrators to give some processes
the ability to create userfaultfd file objects only if they pass
UFFD_USER_MODE_ONLY, reducing the likelihood that these processes will
exploit userfaultfd's ability to delay kernel page faults to open timing
windows for future exploits.

Link: https://lkml.kernel.org/r/20201120030411.2690816-1-lokeshgidra@google.com
Link: https://lkml.kernel.org/r/20201120030411.2690816-2-lokeshgidra@google.com
Signed-off-by: Daniel Colascione <dancol@google.com>
Signed-off-by: Lokesh Gidra <lokeshgidra@google.com>
Reviewed-by: Andrea Arcangeli <aarcange@redhat.com>
Cc: Alexander Viro <viro@zeniv.linux.org.uk>
Cc: <calin@google.com>
Cc: Daniel Colascione <dancol@dancol.org>
Cc: Eric Biggers <ebiggers@kernel.org>
Cc: Iurii Zaikin <yzaikin@google.com>
Cc: Jeff Vander Stoep <jeffv@google.com>
Cc: Jerome Glisse <jglisse@redhat.com>
Cc: "Joel Fernandes (Google)" <joel@joelfernandes.org>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: Jonathan Corbet <corbet@lwn.net>
Cc: Kalesh Singh <kaleshsingh@google.com>
Cc: Kees Cook <keescook@chromium.org>
Cc: Luis Chamberlain <mcgrof@kernel.org>
Cc: Mauro Carvalho Chehab <mchehab+huawei@kernel.org>
Cc: Mel Gorman <mgorman@techsingularity.net>
Cc: Mike Rapoport <rppt@linux.vnet.ibm.com>
Cc: Nitin Gupta <nigupta@nvidia.com>
Cc: Peter Xu <peterx@redhat.com>
Cc: Sebastian Andrzej Siewior <bigeasy@linutronix.de>
Cc: Shaohua Li <shli@fb.com>
Cc: Stephen Smalley <stephen.smalley.work@gmail.com>
Cc: Suren Baghdasaryan <surenb@google.com>
Cc: Vlastimil Babka <vbabka@suse.cz>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

当 qemu 通过 aio 发起请求，其需要写 page ，一样会 userfaultfd 检测到:
```txt
@[
        handle_userfault+5
        __handle_mm_fault+800
        handle_mm_fault+324
        __get_user_pages+389
        __gup_longterm_locked+195
        gup_fast_fallback+272
        iov_iter_extract_pages+230
        __bio_iov_iter_get_pages+130
        bio_iov_iter_get_pages+60
        iomap_dio_bio_iter+572
        __iomap_dio_rw+827
        iomap_dio_rw+18
        ext4_file_read_iter+326
        aio_read+239
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
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

## 为什么 android 的 art 虚拟机需要用 userfaultfd
看来理解这个问题还比较远，恐怕需要理解一下 art 大致处理内存的方法才可以

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

## FAULT_FLAG_USER 似乎是大多数的情况

源码里能确认两个点：handle_userfault() 在 FAULT_FLAG_USER 缺失且 fd 是 UFFD_USER_MODE_ONLY 时直接走 SIGBUS；pin_user_pages*() 会设置
FOLL_PIN，并打 MMF_HAS_PINNED。所以如果目标是“找真正的 pin”，trace pin_user_pages* 比只看 handle_userfault 更直接。


```txt
	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;
```

### 我们有可能 kill 掉一个 kthread 吗?
- 似乎可以啊

### 如果是这个范围的话，是不是太大了

• user_mode(regs) 为假，表示 page fault 发生时 CPU 正在执行内核态代码，而不是用户态代码。它判断的是 fault 发生瞬间的执行特权级，不是 fault
  地址是不是用户地址。

  典型场景：

  1. 系统调用里 copy_from_user() / get_user() / put_user() 触发 fault

     例如用户进程调用 read/write/ioctl，内核在 syscall 中执行：

     copy_from_user(kbuf, user_buf, len);

     如果 user_buf 对应的页还没 present，就会 page fault。此时 fault 地址是用户地址，但 faulting instruction 在内核里，所以：

     user_mode(regs) == false

     这种 fault 通常可以被处理：内核补页后继续执行；如果地址非法，则通过 exception table fixup，让 copy_from_user() 返回 -EFAULT。

  2. 内核访问自己的地址时触发 fault

     比如访问 vmalloc/module/ioremap 区域、某些懒同步的页表映射、或者内核栈/数据页异常。这时也是内核态 fault。能否恢复取决于架构和上下文。

  3. 真正的内核 bug

     例如：

     *(int *)0 = 1;

     或 use-after-free、野指针、非法执行地址。此时 user_mode(regs) 也是 false。x86 里如果 fixup_exception() 接不住，就会走
     page_fault_oops()，通常就是 Oops/panic 路径。

  4. 中断、NMI、atomic/disabled fault handler 上下文中的 fault

     这种上下文一般不能正常睡眠补页。如果 faulthandler_disabled()、current->mm == NULL，或者处在不能处理 page fault 的上下文，就会走 no-
     context / oops / fixup 路径。

  所以可以这样记：

  if (user_mode(regs))
          flags |= FAULT_FLAG_USER;

  含义是：这个 fault 是用户态指令触发的。

  而不是：

  fault address is user address

  最容易混淆的是 copy_from_user()：它访问的是用户地址，但执行位置在内核态，所以 user_mode(regs) 为假。内核会靠 uaccess 的 exception table 和
  fault handler 来区分“可恢复的用户内存访问失败”和“真正的内核访问错误”。

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
