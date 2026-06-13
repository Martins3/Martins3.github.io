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

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_dispatch_handlers
                        - aio_dispatch_handler
                          - virtio_queue_notify_vq
                            - virtio_blk_handle_vq
                              - blk_io_unplug
                                - bdrv_io_unplug
                                  - bdrv_io_unplug
                                    - ioq_submit
                                      - luring_process_completions
                                        - aio_co_enter
                                          - qemu_aio_coroutine_enter
                                            - qemu_coroutine_switch
                                              - ??
                                                - __correctly_grouped_prefixwc
                                                  - coroutine_trampoline
                                                    - blk_aio_read_entry
                                                      - blk_aio_complete
                                                        - blk_aio_complete
                                                          - virtio_blk_rw_complete

## VirtQueueElement 是什么
- virtio_balloon_handle_report : 总是首先获取  while ((elem = virtqueue_pop(vq, sizeof(VirtQueueElement)))) {
  - 将 VirtQueueElement 获取到一个 sg ，然后对于 sg 中的元素遍历

- iov_to_buf : 将 sg 中的一个内容

## 似乎我终于理解了这两个设备的区别了

就是 virtio-blk 可以基于 pci ，也可以基于 mmio 的:
```txt
    # microvm 使用 virtio-device，其他使用 virtio-pci
    if is_microvm_mode; then
        arg_share_dir+=" -device vhost-user-fs-device,chardev=virtme_root,tag=ROOTFS"
    else
        arg_share_dir+=" -device vhost-user-fs-pci,chardev=virtme_root,tag=ROOTFS"
    fi
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
