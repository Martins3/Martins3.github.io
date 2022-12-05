## QEMU 侧如何接受数据

virtio 标准在 virtio 设备的配置空间中，增加了一个 Queue Notify 寄存器，驱动准备好了 virtqueue 之后, 向 Queue Notify 寄存器发起写操作，
切换到 Host 状态中间。

- virtio_queue_rq
  - virtblk_add_req : 消息加入到队列中间
  - virtqueue_kick
    - virtqueue_kick_prepare
    - virtqueue_notify
      - `vq->notify`
        - vring_virtqueue::notify : 在 vring_alloc_queue 中间注册的 vp_notify

- [ ] 如果存在 eventfd 机制，因为通知方式是 MMIO，所以，其实内核可以在另一侧就通知出来这个事情，不需要在下上通知退出原因是什么位置的 MMIO


```txt
#0  virtio_blk_handle_output (vdev=0x555557cae140, vq=0x7ffff40d8010) at ../hw/block/virtio-blk.c:810
#1  0x0000555555b2307f in virtio_queue_notify_vq (vq=0x7ffff40d8010) at ../hw/virtio/virtio.c:2365
#2  0x0000555555d5b628 in aio_dispatch_handler (ctx=ctx@entry=0x55555661d7c0, node=0x7ffcd0006340) at ../util/aio-posix.c:369
#3  0x0000555555d5bee2 in aio_dispatch_handlers (ctx=0x55555661d7c0) at ../util/aio-posix.c:412
#4  aio_dispatch (ctx=0x55555661d7c0) at ../util/aio-posix.c:422
#5  0x0000555555d6ed7e in aio_ctx_dispatch (source=<optimized out>, callback=<optimized out>, user_data=<optimized out>) at ../util/async.c:320
#6  0x00007ffff79e7dfb in g_main_context_dispatch () from /nix/store/s7yq6ngnxf4gsp4263q7xywfjihh5mpn-glib-2.72.2/lib/libglib-2.0.so.0
#7  0x0000555555d7b058 in glib_pollfds_poll () at ../util/main-loop.c:297
#8  os_host_main_loop_wait (timeout=91100000) at ../util/main-loop.c:320
#9  main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:596
#10 0x00005555559d28f7 in qemu_main_loop () at ../softmmu/runstate.c:734
#11 0x000055555582712c in qemu_main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:38
```

virtio_blk_handle_output 中，调用 virtio_blk_handle_request 来解析 virtio_blk_outhdr


当 IO 任务结束之后，virtio_blk_rw_complete 调用 virtio_notify 来通知 Guest

```txt
0  virtio_blk_rw_complete (opaque=0x555557bc4400, ret=0) at ../hw/block/virtio-blk.c:119
#1  0x0000555555c5bd38 in blk_aio_complete (acb=0x5555570c0060) at ../block/block-backend.c:1503
#2  blk_aio_complete (acb=0x5555570c0060) at ../block/block-backend.c:1500
#3  blk_aio_read_entry (opaque=0x5555570c0060) at ../block/block-backend.c:1558
#4  0x0000555555d70d8b in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:177
#5  0x00007ffff769ef60 in __correctly_grouped_prefixwc () from /nix/store/scd5n7xsn0hh0lvhhnycr9gx0h8xfzsl-glibc-2.34-210/lib/libc.so.6
#6  0x0000000000000000 in ?? ()
$ qemu coroutine
usage: qemu coroutine <coroutine-pointer>
$ qemu bt
#0  virtio_blk_rw_complete (opaque=0x555557bc4400, ret=0) at ../hw/block/virtio-blk.c:119
#1  0x0000555555c5bd38 in blk_aio_complete (acb=0x5555570c0060) at ../block/block-backend.c:1503
#2  blk_aio_complete (acb=0x5555570c0060) at ../block/block-backend.c:1500
#3  blk_aio_read_entry (opaque=0x5555570c0060) at ../block/block-backend.c:1558
#4  0x0000555555d70d8b in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:177
#5  0x00007ffff769ef60 in __correctly_grouped_prefixwc () from /nix/store/scd5n7xsn0hh0lvhhnycr9gx0h8xfzsl-glibc-2.34-210/lib/libc.so.6
#6  0x0000000000000000 in ?? ()
Coroutine at 0x7ffff7212320:
#0  qemu_coroutine_switch (from_=from_@entry=0x7ffff7212320, to_=to_@entry=0x555556a55d40, action=action@entry=COROUTINE_ENTER) at ../util/coroutine-ucontext.c:307
#1  0x0000555555d7b4b8 in qemu_aio_coroutine_enter (ctx=ctx@entry=0x55555661d7c0, co=co@entry=0x555556a55d40) at ../util/qemu-coroutine.c:162
#2  0x0000555555d6fbf3 in aio_co_enter (ctx=0x55555661d7c0, co=0x555556a55d40) at ../util/async.c:665
#3  0x0000555555cc2e17 in luring_process_completions (s=s@entry=0x55555686ae90) at ../block/io_uring.c:215
#4  0x0000555555cc31f8 in ioq_submit (s=0x55555686ae90) at ../block/io_uring.c:260
#5  0x0000555555c6c84c in bdrv_io_unplug (bs=0x555556863900) at ../block/io.c:3286
#6  0x0000555555c6c81d in bdrv_io_unplug (bs=<optimized out>) at ../block/io.c:3291
#7  0x0000555555c5ca16 in blk_io_unplug (blk=<optimized out>) at ../block/block-backend.c:2284
#8  0x0000555555ada9dd in virtio_blk_handle_vq (s=0x555557cae140, vq=0x7ffff40d8010) at ../hw/block/virtio-blk.c:802
#9  0x0000555555b2307f in virtio_queue_notify_vq (vq=0x7ffff40d8010) at ../hw/virtio/virtio.c:2365
#10 0x0000555555d5b628 in aio_dispatch_handler (ctx=ctx@entry=0x55555661d7c0, node=0x7ffcd0006340) at ../util/aio-posix.c:369
#11 0x0000555555d5bee2 in aio_dispatch_handlers (ctx=0x55555661d7c0) at ../util/aio-posix.c:412
#12 aio_dispatch (ctx=0x55555661d7c0) at ../util/aio-posix.c:422
#13 0x0000555555d6ed7e in aio_ctx_dispatch (source=<optimized out>, callback=<optimized out>, user_data=<optimized out>) at ../util/async.c:320
#14 0x00007ffff79e7dfb in g_main_context_dispatch () from /nix/store/s7yq6ngnxf4gsp4263q7xywfjihh5mpn-glib-2.72.2/lib/libglib-2.0.so.0
#15 0x0000555555d7b058 in glib_pollfds_poll () at ../util/main-loop.c:297
#16 os_host_main_loop_wait (timeout=91100000) at ../util/main-loop.c:320
#17 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:596
#18 0x00005555559d28f7 in qemu_main_loop () at ../softmmu/runstate.c:734
#19 0x000055555582712c in qemu_main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:38
#20 0x00007ffff7676237 in __libc_start_call_main () from /nix/store/scd5n7xsn0hh0lvhhnycr9gx0h8xfzsl-glibc-2.34-210/lib/libc.so.6
#21 0x00007ffff76762f5 in __libc_start_main_impl () from /nix/store/scd5n7xsn0hh0lvhhnycr9gx0h8xfzsl-glibc-2.34-210/lib/libc.so.6
#22 0x0000555555827051 in _start () at ../sysdeps/x86_64/start.S:116
```

## VirtQueueElement 是什么
- virtio_balloon_handle_report : 总是首先获取  while ((elem = virtqueue_pop(vq, sizeof(VirtQueueElement)))) {
  - 将 VirtQueueElement 获取到一个 sg ，然后对于 sg 中的元素遍历

- iov_to_buf : 将 sg 中的一个内容
