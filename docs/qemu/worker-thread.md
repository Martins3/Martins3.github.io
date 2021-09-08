## QEMU 的 worker thread

- [ ] 更加窒息的地方在于，aio_poll 也是，之后就再也没有人调用了

KVM forum : [Towards Multi-threaded Device Emulation in QEMU](https://www.linux-kvm.org/images/a/a7/02x04-MultithreadedDevices.pdf)
似懂非懂的样子。

```c
/*
>>> thread 5
[Switching to thread 5 (Thread 0x7fffe92f3700 (LWP 1186990))]
#0  0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7fffe92ef220, clockid=0, expected=0, futex_word=0x555556701788) at ../sysdeps/nptl/futex-
internal.h:320
320     ../sysdeps/nptl/futex-internal.h: No such file or directory.
>>> bt
#0  0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7fffe92ef220, clockid=0, expected=0, futex_word=0x555556701788) at ../sysdeps/nptl/futex-
internal.h:320
#1  do_futex_wait (sem=sem@entry=0x555556701788, abstime=abstime@entry=0x7fffe92ef220, clockid=0) at sem_waitcommon.c:112
#2  0x00007ffff6296743 in __new_sem_wait_slow (sem=sem@entry=0x555556701788, abstime=abstime@entry=0x7fffe92ef220, clockid=0) at sem_waitcommon.c:184
#3  0x00007ffff62967ea in sem_timedwait (sem=sem@entry=0x555556701788, abstime=abstime@entry=0x7fffe92ef220) at sem_timedwait.c:40
#4  0x0000555555e7f36f in qemu_sem_timedwait (sem=sem@entry=0x555556701788, ms=ms@entry=10000) at ../util/qemu-thread-posix.c:327
#5  0x0000555555e7da75 in worker_thread (opaque=opaque@entry=0x555556701710) at ../util/thread-pool.c:91
#6  0x0000555555e7e5d3 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:541
#7  0x00007ffff628c609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#8  0x00007ffff61b3293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

```c
/*
#0  huxueshi () at ../util/qemu-thread-posix.c:547
#1  0x0000555555e7f765 in qemu_thread_create (thread=thread@entry=0x7fffffffcdd0, name=<optimized out>, name@entry=0x555555e992d0 "worker", start_routine=start_routine@
entry=0x555555e7d980 <worker_thread>, arg=arg@entry=0x555556701710, mode=mode@entry=1) at ../util/qemu-thread-posix.c:560
#2  0x0000555555e7d90d in do_spawn_thread (pool=pool@entry=0x555556701710) at ../util/thread-pool.c:134
#3  0x0000555555e7d965 in spawn_thread_bh_fn (opaque=0x555556701710) at ../util/thread-pool.c:142
#4  0x0000555555e63938 in aio_bh_poll (ctx=ctx@entry=0x55555670ab70) at ../util/async.c:169
#5  0x0000555555e7ad56 in aio_poll (ctx=ctx@entry=0x55555670ab70, blocking=blocking@entry=true) at ../util/aio-posix.c:659
#6  0x0000555555d9b715 in qcow2_open (bs=<optimized out>, options=<optimized out>, flags=<optimized out>, errp=<optimized out>) at ../block/qcow2.c:1909
#7  0x0000555555d7f455 in bdrv_open_driver (bs=bs@entry=0x555556af6400, drv=drv@entry=0x5555565c8560 <bdrv_qcow2>, node_name=<optimized out>, options=options@entry=0x555556b11aa0, open_flags=139266, errp=errp@entry=0x7fffffffd030) at ../block.c:1552
#8  0x0000555555d82524 in bdrv_open_common (errp=0x7fffffffd030, options=0x555556b11aa0, file=0x555556a20ad0, bs=0x555556af6400) at ../block.c:1827
#9  bdrv_open_inherit (filename=<optimized out>, filename@entry=0x555556954a70 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0,options=0x555556b11aa0, options@entry=0x555556ac3a90, flags=<optimized out>, flags@entry=0, parent=parent@entry=0x0, child_class=child_class@entry=0x0, child_role=0, errp=0x555556617180 <error_fatal>) at ../block.c:3747
#10 0x0000555555d83607 in bdrv_open (filename=filename@entry=0x555556954a70 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0, options=options@entry=0x555556ac3a90, flags=flags@entry=0, errp=errp@entry=0x555556617180 <error_fatal>) at ../block.c:3840
#11 0x0000555555dc80bf in blk_new_open (filename=filename@entry=0x555556954a70 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0, options=options@entry=0x555556ac3a90, flags=0, errp=errp@entry=0x555556617180 <error_fatal>) at ../block/block-backend.c:435
#12 0x0000555555d37178 in blockdev_init (file=file@entry=0x555556954a70 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", bs_opts=bs_opts@entry=0x555556ac3a90, errp=errp@entry=0x555556617180 <error_fatal>) at ../blockdev.c:608
#13 0x0000555555d3811d in drive_new (all_opts=<optimized out>, block_default_type=<optimized out>, errp=0x555556617180 <error_fatal>) at ../blockdev.c:992
#14 0x0000555555cf8ee6 in drive_init_func (opaque=<optimized out>, opts=<optimized out>, errp=<optimized out>) at ../softmmu/vl.c:617
#15 0x0000555555e6c6e2 in qemu_opts_foreach (list=<optimized out>, func=func@entry=0x555555cf8ed0 <drive_init_func>, opaque=opaque@entry=0x55555681d1b0, errp=errp@entry=0x555556617180 <error_fatal>) at ../util/qemu-option.c:1135
#16 0x0000555555cfd8da in configure_blockdev (bdo_queue=0x5555565641d0 <bdo_queue>, snapshot=0, machine_class=0x55555681d100) at ../softmmu/vl.c:676
#17 qemu_create_early_backends () at ../softmmu/vl.c:1939
#18 qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3645
#19 0x0000555555940c8d in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```
这个执行流程实际上很有意思的:
- aio_poll 会导致 coroutine 被执行，所以这个 backtrace 是断掉的感觉
- 两次 spwan 的操作分析在下面的位置，由于
```c
/*
#0  spawn_thread (pool=0x555556739710) at ../util/thread-pool.c:261
#1  thread_pool_submit_aio (pool=0x555556739710, func=<optimized out>, arg=0x7fffe93f6a70, cb=<optimized out>, opaque=<optimized out>) at ../util/thread-pool.c:261
#2  0x0000555555e66a88 in thread_pool_submit_co (pool=0x555556739710, func=func@entry=0x555555dad780 <handle_aiocb_rw>, arg=arg@entry=0x7fffe93f6a70) at ../util/thread-
pool.c:287
#3  0x0000555555dacc4f in raw_thread_pool_submit (bs=bs@entry=0x555556c79df0, func=func@entry=0x555555dad780 <handle_aiocb_rw>, arg=arg@entry=0x7fffe93f6a70) at ../bloc
k/file-posix.c:2030
#4  0x0000555555dad633 in raw_co_prw (bs=0x555556c79df0, offset=0, bytes=112, qiov=0x7fffe93f6e30, type=1) at ../block/file-posix.c:2077

_offset@entry=0, flags=flags@entry=0) at ../block/io.c:1190
#6  0x0000555555d422ce in bdrv_aligned_preadv (child=child@entry=0x555556bf2270, req=req@entry=0x7fffe93f6cb0, offset=0, bytes=112, align=<optimized out>, qiov=0x7fffe93f6e30, qiov_offset=0, flags=0) at ../block/io.c:1577
#7  0x0000555555d42a04 in bdrv_co_preadv_part (child=child@entry=0x555556bf2270, offset=<optimized out>, offset@entry=0, bytes=<optimized out>, bytes@entry=112, qiov=<optimized out>, qiov@entry=0x7fffe93f6e30, qiov_offset=<optimized out>, qiov_offset@entry=0, flags=flags@entry=0) at ../block/io.c:1848
#8  0x0000555555d42b1f in bdrv_co_preadv (child=child@entry=0x555556bf2270, offset=offset@entry=0, bytes=bytes@entry=112, qiov=qiov@entry=0x7fffe93f6e30, flags=flags@entry=0) at ../block/io.c:1798
#9  0x0000555555d288aa in bdrv_preadv (child=0x555556bf2270, offset=offset@entry=0, bytes=bytes@entry=112, qiov=qiov@entry=0x7fffe93f6e30, flags=flags@entry=0) at block/block-gen.c:347
#10 0x0000555555d3ffd1 in bdrv_pread (child=<optimized out>, offset=offset@entry=0, buf=buf@entry=0x7fffe93f6ee0, bytes=bytes@entry=112) at ../block/io.c:1097
#11 0x0000555555da6335 in qcow2_do_open (bs=0x555556c75a00, options=0x555556b10a90, flags=139266, errp=0x7fffffffceb0) at ../block/qcow2.c:1309
#12 0x0000555555da7546 in qcow2_open_entry (opaque=0x7fffffffce50) at ../block/qcow2.c:1878
#13 0x0000555555e653a3 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:173
#14 0x00007ffff60ef660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91


#0  spawn_thread (pool=0x555556739710) at ../util/thread-pool.c:261
#1  thread_pool_submit_aio (pool=0x555556739710, func=<optimized out>, arg=0x7ffe2e9e9bc0, cb=<optimized out>, opaque=<optimized out>) at ../util/thread-pool.c:261
#2  0x0000555555e66a88 in thread_pool_submit_co (pool=0x555556739710, func=func@entry=0x555555dad780 <handle_aiocb_rw>, arg=arg@entry=0x7ffe2e9e9bc0) at ../util/thread-
pool.c:287
#3  0x0000555555dacc4f in raw_thread_pool_submit (bs=bs@entry=0x555556c79df0, func=func@entry=0x555555dad780 <handle_aiocb_rw>, arg=arg@entry=0x7ffe2e9e9bc0) at ../bloc
k/file-posix.c:2030
#4  0x0000555555dad633 in raw_co_prw (bs=0x555556c79df0, offset=1301217280, bytes=12288, qiov=0x7ffe2e9e9c60, type=1) at ../block/file-posix.c:2077
#5  0x0000555555d3d7f4 in bdrv_driver_preadv (bs=bs@entry=0x555556c79df0, offset=offset@entry=1301217280, bytes=bytes@entry=12288, qiov=0x7ffe2e9e9c60, qiov@entry=0x7ff
e4c321820, qiov_offset=qiov_offset@entry=20480, flags=flags@entry=0) at ../block/io.c:1190
#6  0x0000555555d422ce in bdrv_aligned_preadv (child=child@entry=0x555556bf2270, req=req@entry=0x7ffe2e9e9e00, offset=1301217280, bytes=12288, align=<optimized out>, qiov=0x7ffe4c321820, qiov_offset=20480, flags=0) at ../block/io.c:1577
#7  0x0000555555d42a04 in bdrv_co_preadv_part (child=0x555556bf2270, offset=<optimized out>, offset@entry=1301217280, bytes=<optimized out>, bytes@entry=12288, qiov=<optimized out>, qiov@entry=0x7ffe4c321820, qiov_offset=<optimized out>, qiov_offset@entry=20480, flags=flags@entry=0) at ../block/io.c:1848
#8  0x0000555555da264b in qcow2_co_preadv_task (qiov_offset=20480, qiov=0x7ffe4c321820, bytes=12288, offset=4313513984, host_offset=1301217280, subc_type=<optimized out>, bs=0x555556c75a00) at ../block/qcow2.c:2291
#9  qcow2_co_preadv_task_entry (task=<optimized out>) at ../block/qcow2.c:2307
#10 0x0000555555d79fb1 in aio_task_co (opaque=0x7ffe4c103da0) at ../block/aio_task.c:45
#11 0x0000555555e653a3 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:173
#12 0x00007ffff60ef660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

- bdrv_driver_pwritev : block/io.c 中
  * blk_aio_pwritev : 在 block-backend.c 中，这就是给 block driver 注册使用的, 例如注册到 NvmeRequest 中间去的。

```c
typedef struct NvmeRequest {
    struct NvmeSQueue       *sq;
    struct NvmeNamespace    *ns;
    BlockAIOCB              *aiocb;
    uint16_t                status;
    void                    *opaque;
    NvmeCqe                 cqe;
    NvmeCmd                 cmd;
    BlockAcctCookie         acct;
    NvmeSg                  sg;
    QTAILQ_ENTRY(NvmeRequest)entry;
} NvmeRequest;
```

才意识到为了处理 /home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2, 不仅仅需要
block/qcow2.c 而且需要 block/file-posix.c 来进行文件的 IO

- [ ] AIO_WAIT_WHILE : 一个有趣的位置，这个会去调用 aio_poll 的
    - [ ] 但是我的龟龟啊，你知不知道，这意味着这个调用 poll 的会一直等待到这个位置上。
      - 但是调用 AIO_WAIT_WHILE 的位置不要太多啊

如果想要提交任务 : 在 thread_pool_submit_aio 中 qemu_sem_post  ThreadPool::sem 这会让 worker_thread 从这个 lock 上醒过来
然后会从 ThreadPool::request_list 中获取需要执行的函数，最后使用 `qemu_bh_schedule(pool->completion_bh)` 通知这个任务结束了

其实整个 thread-pool.c 也就是只有 300 行

- worker 如何结束的，在 worker_thread 中，qemu_sem_timedwait 最多等待 10s 如果没有任务过来，那么这个 thread 结束。


https://blog.csdn.net/woai110120130/article/details/100049614 : 这个分析中规中矩

