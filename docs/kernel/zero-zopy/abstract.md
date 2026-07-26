# 概述

## iouring

docs/kernel/iouring/net.md

## qemu

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_scsi_handle_cmd
                            - virtio_scsi_pop_req :
                            - virtio_scsi_handle_cmd_vq
                              - virtio_scsi_handle_cmd_req_prepare
                                - virtio_scsi_parse_req
                                  - iov_to_buf ( 这里有 memcpy ，不过拷贝的只是 VirtIOSCSIReq::req)
                              - virtio_scsi_handle_cmd_req_submit
                                - scsi_do_read
                                  - dma_blk_io
                                    - dma_blk_cb
                                      - blk_aio_preadv
                                        - blk_aio_prwv
                                - defer_call_end
                                  - ioq_submit
                                    - io_submit

很简单，就是直接看发起 aio 的地址是不是内核日志就可以了。

打断点的位置 : io_prep_preadv 或者 io_prep_pwritev2
然后 next 两次，进入到 laio_do_submit 中，执行 p *(qiov->iov)

用系统盘测试就是会有问题，fio randread ，然后发现 io 读入的地方不是虚拟机的地址，
我认为这里是 qemu 的优化，但是如果是普通的盘，fio 无论是 randread 还是 randwrite 。

## sva
https://lpc.events/event/11/contributions/1021/attachments/744/1700/lpc-2021-kernel-svm-jp.pdf
https://www.kernel.org/doc/html/next/x86/sva.html

很有想象力的东西了

## vduse / ublk / tcmu 的 zero copy

## RDMA

## dpdk 的 zero copy

## 共享内存
https://github.com/facebookexperimental/libunifex

## fuse
fuse pass throughout

FUSE_PASSTHROUGH

## 代价是什么？
copy fail bug 就是 zero copy 的问题

## 如果无法避免，那么是存在很多优化的

1. GPU 中的 async copy : pipeline + fma

## 参考文献
https://quant67.com/post/linux/zero-copy-dirty-truth.html

## 只能说，存储不难，但是网络不容易。
https://stackoverflow.com/questions/18343365/zero-copy-networking-vs-kernel-bypass

说网络的代价主要是 pin 和 notify ，我感觉
存储也是有的，但是存储的一个关键原因在可以 by pass
这是两个不同性质的事情

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
