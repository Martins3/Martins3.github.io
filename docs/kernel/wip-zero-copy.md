## 基本判断方法

在 perf 中观察 copy_user_enhanced_fast_string 的占比，很容易的:

- ext4_file_write_iter
  - ext4_dio_write_iter
    - iomap_dio_rw
      - `__iomap_dio_rw`
        - blk_finish_plug
  - ext4_buffered_write_iter
    - generic_perform_write ：提交
    - generic_write_sync ： 同步

```txt
✗ flamegraph -c "taskset -c 5 fio 4k-read.fio" -b 5
```
![](./fio-direct.svg)
![](./fio-no-direct.svg)

验证其实是很容易的，只是需要 perf 看看就可以了

## 只能说，存储不难，但是网络不容易。

https://stackoverflow.com/questions/18343365/zero-copy-networking-vs-kernel-bypass

一些老的思考

## iouring

docs/kernel/iouring/net.md

## RDMA

## qemu 中有 zero copy 吗?

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

## udmabuf 也是 zero copy 的一部分

## 网络的 zero copy

## rdma 的 zero copy
从这个角度再看看 rdma 的工作

## userfault 如何实现 zero copy

## sva
https://lpc.events/event/11/contributions/1021/attachments/744/1700/lpc-2021-kernel-svm-jp.pdf
https://www.kernel.org/doc/html/next/x86/sva.html

很有想象力的东西了

## vhost-net 的 zero copy
例如存在这个函数: vhost_zerocopy_callback

## vduse / ublk / tcmu 的 zero copy

## dpdk 的 zero copy

## 关联这里的东西
net/msg_zerocopy.md

## 重新仔细的想想
如果 open O_DIRECT

然后 write ，就可以不会出现 copy 吗？

如果 write 的 buffer 是一个 64 byte 的，其实需要先读入，修改，然后再写下去。

用 fio 测试了一下，内核是可以自动适应这种情况的。

还是分析一下代码看看，如果 write 一个区间，不是 page 对齐的，那么 copy 发生在什么位置

## 从这个角度分析也是极好的
https://mazzo.li/posts/fast-pipes.html

## 共享内存
https://github.com/facebookexperimental/libunifex

## fuse 我想也是会有拷贝的问题吧

所以，fuse 使用 io uring 的意义是什么?

## 很强的
https://quant67.com/post/linux/zero-copy-dirty-truth.html

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
