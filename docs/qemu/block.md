# block

## 记录下我的一生之敌
- [ ] `aio_co_enter`

## 疑惑
- [ ] 什么是 filter，例如
    - `bdrv_drop_filter`
- [ ] gluster 的后端是什么？
- [ ] block jobs : Streaming, mirroring, commit,
    - ？？？
- [ ] SCSI 是可以 passthrough 的哇?
- [ ] quorum ? 这个是 colo 曾经使用的东西吗?

忽然意识到，block 文件夹下，其实基本是这些 driver 的：
- `BlockDriver`
    - vmdk
    - dmg
    - vdi: A VDI file is a virtual disk image used by Oracle VM VirtualBox

阅读其他模块同时检测到的内容，也是比较关键的内容:
- block/block-backend.c
- block.c : 这也是我一直没有理解的地方，为什么将 block.c 不是放到 block 目录下，而是放到顶层目录中
- block/dirty-bitmap.c
- block/io.c

## 有趣的
- BlockDriverState 和 BlockBackend 的关系是什么?
```c
/*
 * Return the BlockDriverState attached to @blk if any, else null.
 */
BlockDriverState *blk_bs(BlockBackend *blk)
{
    IO_CODE();
    return blk->root ? blk->root->bs : NULL;
}
```


## 代码量比较大的地方

- vhdx.c
- vvfat.c
- ssh.c
- rbd.c
- qed.c
- qcow2-refcount.c
- qcow2-cluster.c
- qapi.c
- nvme.c # 这个指的是，让 QEMU 直接访问 nvme ?，不是吧
- nfs.c
- nbd.c
- mirror.c
- iscsi.c
- io.c
- gluster.c
- file-posix.c
- block-backend.c

## blockdev.c
1. 前面的好几千行就是用于定义这个的
2. 后面的就是各种 qmp 操作的

```c
static const BlkActionOps actions[] = {
    [TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT] = {
        .instance_size = sizeof(ExternalSnapshotState),
        .prepare  = external_snapshot_prepare,
        .commit   = external_snapshot_commit,
        .abort = external_snapshot_abort,
        .clean = external_snapshot_clean,
    },
    // ... 还有很多类似的，就不拷贝了

    /* Where are transactions for MIRROR, COMMIT and STREAM?
     * Although these blockjobs use transaction callbacks like the backup job,
     * these jobs do not necessarily adhere to transaction semantics.
     * These jobs may not fully undo all of their actions on abort, nor do they
     * necessarily work in transactions with more than one job in them.
     */
};
```

在 `qmp_transaction` 中的，根据命令来调用这些内容:

## block.c

- 几乎所有的函数都是 `bdrv` 开头的
    - 应该就是 `BlockDriverState` 的缩写了


## 一些 backtrace

### `io_uring`

```c
typedef enum BlockdevAioOptions {
    BLOCKDEV_AIO_OPTIONS_THREADS,
    BLOCKDEV_AIO_OPTIONS_NATIVE,
#if defined(CONFIG_LINUX_IO_URING)
    BLOCKDEV_AIO_OPTIONS_IO_URING,
#endif /* defined(CONFIG_LINUX_IO_URING) */
    BLOCKDEV_AIO_OPTIONS__MAX,
} BlockdevAioOptions;
```
- [ ] 其中 native 模式指的是？preadv 和 pwritev 吗？

```txt
#0  luring_do_submit (type=1, offset=2686976, s=0x55555685b1c0, luringcb=0x7fff7f3f36a0, fd=12) at ../block/io_uring.c:344
#1  luring_co_submit (bs=0x555556853c30, s=0x55555685b1c0, fd=12, offset=2686976, qiov=<optimized out>, type=1) at ../block/io_uring.c:389
#2  0x0000555555c60321 in bdrv_driver_preadv (bs=0x555556853c30, offset=2686976, bytes=65536, qiov=0x7fff7f3f3aa0, qiov_offset=0, flags=0) at ../block/io.c:1160
#3  0x0000555555c62da1 in bdrv_aligned_preadv (child=child@entry=0x555556859870, req=req@entry=0x7fff7f3f3960, offset=offset@entry=2686976, bytes=bytes@entry=65536, a8
#4  0x0000555555c63526 in bdrv_co_preadv_part (child=child@entry=0x555556859870, offset=<optimized out>, offset@entry=2686976, bytes=<optimized out>, bytes@entry=65531
#5  0x0000555555c6366b in bdrv_co_preadv (child=child@entry=0x555556859870, offset=offset@entry=2686976, bytes=bytes@entry=65536, qiov=qiov@entry=0x7fff7f3f3aa0, flag0
#6  0x0000555555c2c8be in bdrv_co_pread (flags=0, buf=0x7ffff5dc8000, bytes=65536, offset=2686976, child=0x555556859870) at /home/martins3/core/qemu/include/block/blo4
#7  bdrv_pread (child=0x555556859870, offset=offset@entry=2686976, bytes=65536, buf=0x7ffff5dc8000, flags=flags@entry=0) at block/block-gen.c:96
#8  0x0000555555c7384d in qcow2_cache_do_get (bs=0x55555684c570, c=0x5555568595f0, offset=2686976, table=0x7fff7f3f3c20, read_from_disk=<optimized out>) at ../block/q2
#9  0x0000555555c764b6 in qcow2_get_host_offset (bs=bs@entry=0x55555684c570, offset=offset@entry=97710505472, bytes=bytes@entry=0x7fff7f3f3c98, host_offset=host_offse7
#10 0x0000555555c83e41 in qcow2_co_preadv_part (bs=0x55555684c570, offset=97710505472, bytes=512, qiov=0x5555574a8d58, qiov_offset=0, flags=<optimized out>) at ../blo7
#11 0x0000555555c62da1 in bdrv_aligned_preadv (child=child@entry=0x555556808140, req=req@entry=0x7fff7f3f3e40, offset=offset@entry=97710505472, bytes=bytes@entry=512,8
#12 0x0000555555c63526 in bdrv_co_preadv_part (child=0x555556808140, offset=<optimized out>, offset@entry=97710505472, bytes=<optimized out>, bytes@entry=512, qiov=<o1
#13 0x0000555555c5427d in blk_co_do_preadv_part (blk=0x55555684c220, offset=97710505472, bytes=512, qiov=0x5555574a8d58, qiov_offset=qiov_offset@entry=0, flags=0) at 1
#14 0x0000555555c54406 in blk_aio_read_entry (opaque=0x555556847bc0) at ../block/block-backend.c:1556
#15 0x0000555555d6852b in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:177
#16 0x00007ffff769ef60 in __correctly_grouped_prefixwc () from /nix/store/scd5n7xsn0hh0lvhhnycr9gx0h8xfzsl-glibc-2.34-210/lib/libc.so.6
#17 0x0000000000000000 in ?? ()
[Thread 0x7fff863ff640 (LWP 1652674) exited]
Coroutine at 0x7ffff7212320:
#0  qemu_coroutine_switch (from_=from_@entry=0x7ffff7212320, to_=to_@entry=0x555556593b10, action=action@entry=COROUTINE_ENTER) at ../util/coroutine-ucontext.c:307
#1  0x0000555555d72c58 in qemu_aio_coroutine_enter (ctx=ctx@entry=0x55555660f920, co=co@entry=0x555556593b10) at ../util/qemu-coroutine.c:162
#2  0x0000555555d67393 in aio_co_enter (ctx=0x55555660f920, co=0x555556593b10) at ../util/async.c:665
#3  0x0000555555c520d5 in blk_aio_prwv (blk=0x55555684c220, offset=97710505472, bytes=512, iobuf=0x5555574a8d58, co_entry=co_entry@entry=0x555555c543e0 <blk_aio_read_8
#4  0x0000555555c53666 in blk_aio_preadv (blk=<optimized out>, offset=<optimized out>, qiov=<optimized out>, flags=<optimized out>, cb=<optimized out>, opaque=<optimi8
#5  0x0000555555ad4805 in submit_requests (niov=-1, num_reqs=1, start=0, mrb=0x7fffffffcae0, blk=0x55555684c220) at ../hw/block/virtio-blk.c:427
#6  virtio_blk_submit_multireq (blk=0x55555684c220, mrb=mrb@entry=0x7fffffffcae0) at ../hw/block/virtio-blk.c:457
#7  0x0000555555ad5427 in virtio_blk_handle_vq (s=0x555557878ed0, vq=0x7ffff44881d8) at ../hw/block/virtio-blk.c:799
#8  0x0000555555b1dadf in virtio_queue_notify_vq (vq=0x7ffff44881d8) at ../hw/virtio/virtio.c:2365
#9  0x0000555555d52dc8 in aio_dispatch_handler (ctx=ctx@entry=0x55555660f920, node=0x7fffec60aae0) at ../util/aio-posix.c:369
#10 0x0000555555d53682 in aio_dispatch_handlers (ctx=0x55555660f920) at ../util/aio-posix.c:412
#11 aio_dispatch (ctx=0x55555660f920) at ../util/aio-posix.c:422
#12 0x0000555555d6651e in aio_ctx_dispatch (source=<optimized out>, callback=<optimized out>, user_data=<optimized out>) at ../util/async.c:320
#13 0x00007ffff79e7dfb in g_main_context_dispatch () from /nix/store/s7yq6ngnxf4gsp4263q7xywfjihh5mpn-glib-2.72.2/lib/libglib-2.0.so.0
#14 0x0000555555d727f8 in glib_pollfds_poll () at ../util/main-loop.c:297
#15 os_host_main_loop_wait (timeout=1000000000) at ../util/main-loop.c:320
#16 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:596
#17 0x00005555559cf3a3 in qemu_main_loop () at ../softmmu/runstate.c:726
#18 0x0000555555820800 in qemu_main (envp=0x0, argv=<optimized out>, argc=<optimized out>) at ../softmmu/main.c:36
#19 main (argc=<optimized out>, argv=<optimized out>) at ../softmmu/main.c:45
```

- [ ] 跟踪一下，这是两层的 block driver 的，首先是 qcow2，然后是 file 的

## block job
- `block_job_add_bdrv` 的调用者
  - block/commit.c
  - block/mirror.c
  - block/stream.c

## block dirty bitmap

## qmp ：没办法，不搞的话，dirty bitmap 是没有办法维持生活的
- [ ] grep 一下目前对于 qmp 的所有问题，尝试将 qmp 和 qemu option 融合一下
- [ ] https://gist.github.com/rgl/dc38c6875a53469fdebb2e9c0a220c6c
- [ ] https://wiki.qemu.org/Documentation/QMP
