# aio

## 基本使用
- https://oxnz.github.io/2016/10/13/linux-aio/
- https://github.com/littledan/linux-aio
  - 尤其是其中的 : Performance considerations
- https://gist.github.com/larytet/87f90b08643ac3de934df2cadff4989c : 这个的代码更加好

## 两个使用的接口
/proc/sys/fs/aio-max-nr
/proc/sys/fs/aio-nr

在 Documentation/admin-guide/sysctl/fs.rs 中:
```rst
aio-nr & aio-max-nr
-------------------

``aio-nr`` shows the current system-wide number of asynchronous io
requests.  ``aio-max-nr`` allows you to change the maximum value
``aio-nr`` can grow to.  If ``aio-nr`` reaches ``aio-nr-max`` then
``io_setup`` will fail with ``EAGAIN``.  Note that raising
``aio-max-nr`` does not result in the
pre-allocation or re-sizing of any kernel data structures.
```

我发现，libaio 当 direct=1 和 direct=0 的时候，
 `cat /proc/sys/fs/aio-nr`
的结果都是给 fio 配置的 iodepth 。

1. 用户态的 io_context_t 就是内核中 kioctx::user_id

## 问题

Lock contention when many CPUs or requests share an io_context_t

如果多个用户态的 therad 同时访问 aio 的共享队列，该会有问题吧

## 问题和解答
1. io submit 和 io io_getevents 可以在不同的 thread 吗?
  - 当然可以，有 demo 为证

iocb 和 kiocb 什么关系?
> 为什么 kiocb 中间没有 nbytes 之类内容

用户态持有 io_context_t 的作用是什么，只是一个 tag 吧?

## 关键结构体

- iocb : 提交每一个 io 需要构建一个

## 基本流程

sync io 的基本流程:

- do_syscall_64
  - ksys_write
    - vfs_write
      - new_sync_write
        - init_sync_kiocb
        - iov_iter_ubuf
        - ext4_file_write_iter

- io_getevents
  - read_events
    - aio_read_events
      - aio_read_events_ring

aio_read_events_ring 会访问 kioctx::ring_pages 来提供给用户就绪的 io，使用 aio_complete 向其中添加。
aio_complete 会调用 eventfd_signal，这是实现 epoll 机制的核心。

- io_submit_one
  - `__io_submit_one`
    - aio_read
      - aio_setup_rw
      - call_read_iter : 调用 file_operations::read_iter ：其实普通的 read write 也是走的这个路径，应该是参数设置的不同，导致是否需要等待。
      - aio_rw_done
    - aio_write
    - aio_fsync
    - aio_poll : 是我们常规意义上的 poll ，还是 epoll 机制 ?

当结束之后，接收到问题:
```txt
@[
    aio_complete+1
    aio_complete_rw+300
    blkdev_bio_end_io_async+52
    blk_mq_end_request_batch+266
    nvme_irq+114
    ...
]: 604453
```

- do_io_getevents
  - read_events
    - aio_read_events
      - aio_read_events_ring





## 为什么 O_DIRECT=0 的时候 需要在 io_submit 的时候就需要让 page cache 到位?
因为直接转化为 buffer read ，最后大多数时间都是在 folio_wait_bit_common 上的，
而 aio 正常的时候，只是向硬件提交，然后就返回了。

## 如果 aio 写操作，O_DIRECT 是提交给硬件之后返回，但是如果是硬件队列满了，怎么办?
如果硬件队列满了，那么是应该阻塞的。

## read/write 这种 sync syscall 在 O_DIRECT 的时候，大致流程是什么?

generic_file_read_iter 中
kiocb_write_and_wait 就是等待的地方了。

## 大致原理
写的还行: https://juejin.cn/post/6956566854500515870
https://github.com/liexusong/linux-source-code-analyze/blob/master/native-aio.md

## 结构体
- aio_ring
- aio_kiocb
- kioctx
  - ring_pages : 这个什么 page 的
  - reqs :
  - rq_wait
  - wait :

## 如何理解 kioctx 中的两个 wait
### rq_wait
- kill_ioctx 中赋值 : ctx->rq_wait = wait;

看上去，就是在等待所有的 ioctx 结束
- exit_aio
  - 初始化 wait 机制
  - kill_ioctx
    - ctx->rq_wait = wait;
  - wait_for_completion : 等待

当 free_ioctx_reqs 的时候，
会 dec ctx->rq_wait->count 计数，当计数修改为 0 的，统计结束。

### wait
通过这个来实现 read_events 的等待的结束。

两个通知的方法:
1. aio_complete
```c
  if (waitqueue_active(&ctx->wait))
    wake_up(&ctx->wait);
```
2. kill_ioctx 中 wake_up_all

当 read_events 中，使用 ctx->wait 来等待存储子系统的返回。

那么 wake_up 和 wake_up_all 的区别，在
kill_ioctx 中使用的是 wake_up_all ，而 aio_complete 中使用的是 wake_up

## 如何理解 kioctx 中的两个引用计数
```c
  if (percpu_ref_init(&ctx->users, free_ioctx_users, 0, GFP_KERNEL))
    goto err;

  if (percpu_ref_init(&ctx->reqs, free_ioctx_reqs, 0, GFP_KERNEL))
    goto err;
```

- kill_ioctx
  - percpu_ref_kill(&ctx->users);



## 具体代码分析
- io_destroy
  - init_completion : 初始化 struct ctx_rq_wait
  - kill_ioctx :  让所有的 ioctx 都是 : ctx->rq_wait = wait;

```c
struct kioctx {

  /*
   * signals when all in-flight requests are done
   */
  struct ctx_rq_wait  *rq_wait;

  struct {
    struct mutex  ring_lock;
    wait_queue_head_t wait;       // 这个 wait 的使用位置特别奇怪
  } ____cacheline_aligned_in_smp;
}
```

当 reqs 的引用计数为 0 的时候:

```c
  if (percpu_ref_init(&ctx->reqs, free_ioctx_reqs, 0, GFP_KERNEL))
    goto err;
```

当 free_ioctx_reqs 的时候，调用 complete

## 为什么需要一个文件 : 为了 ring 的 page migration

- io_setup
  - ioctx_alloc
    - `aio_setup_ring`
      - file = aio_private_file(ctx, nr_pages); : 页面大小为 aio_ring 和 io_event

最终存储在:
```c
struct kioctx {
  // ...
  unsigned long   mmap_base;
  unsigned long   mmap_size;
  // ...
  struct page   *internal_pages[AIO_RING_PAGES];
  struct file   *aio_ring_file;
```

```c
static const struct vm_operations_struct aio_ring_vm_ops = {
  .mremap   = aio_ring_mremap,
#if IS_ENABLED(CONFIG_MMU)
  .fault    = filemap_fault,
  .map_pages  = filemap_map_pages,
  .page_mkwrite = filemap_page_mkwrite,
#endif
};
```

居然是为了页面迁移，我的龟龟
```c
static const struct address_space_operations aio_ctx_aops = {
  .dirty_folio  = noop_dirty_folio,
  .migrate_folio  = aio_migrate_folio,
};
```
该文件用户不可见的。

- 还是感觉超级迷茫
  - 尝试将 aio_ring_mremap 删除掉试试，看看内核如何 crash 的?

io_uring 是怎么操作的？也需要调查下

## 分析 O_DIRECT 以及 O_SYNC 的含义
<!-- 7c0a271b-42ee-4acf-b6d0-4092435480f1 -->

这两个都是 open(2) 的 flags

man open(2) 中关于 O_SYNC 和 O_DIRECT 的讨论，才发现这个内容是看漏掉了:
https://stackoverflow.com/questions/5055859/how-are-the-o-sync-and-o-direct-flags-in-open2-different-alike

man open(2)

>       O_DIRECT (since Linux 2.4.10)
>              Try to minimize cache effects of the I/O to and  from
>              this file.  In general this will degrade performance,
>              but  it is useful in special situations, such as when
>              applications do their own caching.  File I/O is  done
>              directly  to/from  user-space  buffers.  The O_DIRECT
>              flag on its own makes an effort to transfer data syn‐
>              chronously, but does not give the guarantees  of  the
>              O_SYNC  flag  that  data  and  necessary metadata are
>              transferred.  To guarantee  synchronous  I/O,  O_SYNC
>              must  be used in addition to O_DIRECT.  See NOTES be‐
>              low for further discussion.
>
>              A semantically similar (but deprecated) interface for
>              block devices is described in raw(8).

>       O_DSYNC       Write  I/O  operations on the file descriptor shall
>                     complete as defined by synchronized  I/O  data  in‐
>                     tegrity completion.

>       O_RSYNC       Read I/O operations on the  file  descriptor  shall
>                     complete  at  the same level of integrity as speci‐
>                     fied by the  O_DSYNC  and  O_SYNC  flags.  If  both
>                     O_DSYNC and O_RSYNC are set in oflag, all I/O oper‐
>                     ations on the file descriptor shall complete as de‐
>                     fined  by  synchronized  I/O data integrity comple‐
>                     tion. If both O_SYNC and O_RSYNC are set in  flags,
>                     all  I/O  operations  on  the file descriptor shall
>                     complete as defined by synchronized  I/O  file  in‐
>                     tegrity completion.

>       O_SYNC        Write  I/O  operations on the file descriptor shall
>                     complete as defined by synchronized  I/O  file  in‐
>                     tegrity completion.
>
>                     The  O_SYNC  flag  shall  be  supported for regular
>                     files, even if the Synchronized  Input  and  Output
>                     option is not supported.

direct=1
```txt
[pid 177618] openat(AT_FDCWD, "/tmp/a", O_RDWR|O_CREAT|O_DIRECT, 0600) = 7
```

各个表格理解一下:

| aio | O_DIRECT | O_SYNC |
|-----|----------|--------|---------------------------------------------------------------------------------------------------------------------|
| 1   | 0        | 0      | io_submit 的时候退化为 buffer write ，例如在 ext4_file_write_iter                                                   |
| 1   | 0        | 1      |
| 1   | 1        | 0      | 标准情况                                                                                                            |
| 1   | 1        | 1      | 如果是文件，那么不到 1k 的 iops ，如果是 funclatency 显示 syscall 执行很快，是 O_SYNC 导致写 nvme 效果不同导致的? |
| 0   | 0        | 0      | 写到 buffer 中，等 os 来 flush                                                                                      |
| 0   | 0        | 1      | 写入 buffer 中，直到 sync 才返回
| 0   | 1        | 0      |
| 0   | 1        | 1      | 绕过 buffer ，需要等待元数据 sync 完成才可以返回

即便是 write() + O_DIRECT ，也不存在这种逆天操作，盘没有返回(end_bio() 没有执行)，write() 就返回了。
会卡到这里的，来等待 end_bio 的返回的。
```txt
[<0>] submit_bio_wait+0xa0/0xf0
[<0>] __blkdev_direct_IO_simple+0x11c/0x250
[<0>] blkdev_direct_IO+0x25c/0x700
[<0>] blkdev_write_iter+0x214/0x330
[<0>] vfs_write+0x224/0x3b0
[<0>] ksys_write+0x78/0x120
[<0>] __arm64_sys_write+0x24/0x40
[<0>] invoke_syscall.constprop.0+0x58/0xf0
[<0>] do_el0_svc+0x48/0xf0
[<0>] el0_svc+0x5c/0x240
[<0>] el0t_64_sync_handler+0x10c/0x138
[<0>] el0t_64_sync+0x198/0x1a0
```

1. O_SYNC 相对于 O_DSYNC ，还添加了刷新元数据的
2. O_DSYNC 会实现刷 disk cache 到盘下，也就是执行新的命令来保证，当 fio 加了 sync=dsync 的时候
可以观察到大量的 blk_insert_flush 操作，不过对于 nvme 而言，这个是一个车空操作。
3. 此外，O_ASYNC ，极为罕见的东西，不用看了。

https://transactional.blog/how-to-learn/disk-io 的解释的完全正确，就是 O_DSYNC 带磁盘落盘


## 永不返回的 io

如果 io_getevents 会卡到这里
```txt
[<0>] read_events+0x155/0x1d0
[<0>] do_io_getevents+0x71/0xe0
[<0>] __x64_sys_io_getevents+0x6d/0xe0
[<0>] do_syscall_64+0x43/0xf0
[<0>] entry_SYSCALL_64_after_hwframe+0x6f/0x77
```
如果想要强制 exit 的时候，那么就会卡在这里:

如果 kill 掉这个进程，那么就会最后卡成这个样子:

- do_syscall_64
  - syscall_exit_to_user_mode
    - __syscall_exit_to_user_mode_work
      - exit_to_user_mode_prepare
        - exit_to_user_mode_loop
          - arch_do_signal_or_restart
            - get_signal
              - do_group_exit
                - do_exit
                  - exit_mm
                    - mmput
                      - __mmput
                        - exit_aio

## IO_CMD_POLL
https://blog.cloudflare.com/io_submit-the-epoll-alternative-youve-never-heard-about/
https://lkml.iu.edu/1803.3/05292.html

```txt
static inline void io_prep_poll(struct iocb *iocb, int fd, int events)
{
        memset(iocb, 0, sizeof(*iocb));
        iocb->aio_fildes = fd;
        iocb->aio_lio_opcode = IO_CMD_POLL;
        iocb->aio_reqprio = 0;
        iocb->u.poll.events = events;
}
```

原来，这个不是用来发送的，而是用来监听的。

## 参考
- https://zhuanlan.zhihu.com/p/368913613 源码分析
- [How io-uring implementation is different from AIO?](https://stackoverflow.com/questions/65075339/how-io-uring-implementation-is-different-from-aio)
- [asynchronous IO io_submit latency in Ubuntu Linux](https://stackoverflow.com/questions/34572559/asynchronous-io-io-submit-latency-in-ubuntu-linux)

sudo funclatency do_io_getevents
sudo funclatency __x64_sys_io_submit

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
