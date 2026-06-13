## qatomic_set 有意义吗？
set 本身就是 atomic 的

```c
/* Weak atomic operations prevent the compiler moving other
 * loads/stores past the atomic operation load/store. However there is
 * no explicit memory barrier for the processor.
 *
 * The C11 memory model says that variables that are accessed from
 * different threads should at least be done with __ATOMIC_RELAXED
 * primitives or the result is undefined. Generally this has little to
 * no effect on the generated code but not using the atomic primitives
 * will get flagged by sanitizers as a violation.
 */
#define qatomic_read__nocheck(ptr) \
    __atomic_load_n(ptr, __ATOMIC_RELAXED)
```

只是为了告诉 sanitizers 而已。




## 既然有  FDMonOps， 为什么还是有 glib 的事件监听

例如在 aio_set_fd_handler

```txt
g_source_add_poll(&ctx->source, &new_node->pfd);
```


https://www.qemu.org/docs/master/devel/multiple-iothreads.html


## 这几个都是需要看看的
[^11]: [io_uring in QEMU: high-performance disk IO for Linux](https://archive.fosdem.org/2020/schedule/event/vai_io_uring_in_qemu/attachments/slides/4145/export/events/attachments/vai_io_uring_in_qemu/slides/4145/io_uring_fosdem.pdf)
[^12]: [Improving the QEMU Event Loop](http://events17.linuxfoundation.org/sites/events/files/slides/Improving%20the%20QEMU%20Event%20Loop%20-%203.pdf)
[^13]: [Effective multi-threading in QEMU](https://www.linux-kvm.org/images/1/17/Kvm-forum-2013-Effective-multithreading-in-QEMU.pdf)

http://blog.vmsplice.net/2020/08/qemu-internals-event-loops.html

## 记录一个有趣的 qemu lock 的问题

启动 qds ，让 qds 在 mmap 的时候卡主，这个时候，似乎这个时候，由于 bql 的持有，
qemu 连 hmp 都不会去响应的，所以，我猜测，当 qemu 在处理 io 的时候，持有了 bql ，
其他的 thread 都是需要卡主的。

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - memory_region_dispatch_write
                              - access_with_adjusted_size
                                - memory_region_write_accessor
                                  - virtio_pci_common_write
                                    - virtio_set_status
                                      - vhost_user_blk_set_status
                                        - vhost_user_blk_start
                                          - vhost_dev_start
                                            - vhost_user_set_mem_table
                                              - vhost_user_add_remove_regions
                                                - send_add_regions
                                                  - process_message_reply
                                                    - vhost_user_read
                                                      - vhost_user_read_header
                                                        - qemu_chr_fe_read_all
                                                          - tcp_chr_sync_read
                                                            - tcp_chr_recv
                                                              - qio_channel_socket_readv
                                                                - recvmsg

## 只有在 coroutine 中，才可以调用 qemu_coroutine_yield 吗?


## 先操作一下各种 block 的特性 吧
例如 snapshot 和 mirror 之类的

## 理解一下 qemu_co_mutex_init 是做什么的

qemu_in_coroutine 测试一下这个东西

## aio_co_enter 和 qemu_coroutine_enter 的区别是什么?


## qemu_notify_event 这个函数是做什么的

```c
static void notify_event_cb(void *opaque)
{
    /* No need to do anything; this bottom half is only used to
     * kick the kernel out of ppoll/poll/WaitForMultipleObjects.
     */
}
```

## 那么 aio_co_wait 的作用是什么?


## 通过这个测试理解一下

```txt
static BlockDriver bdrv_test = {
    .format_name            = "test",
    .instance_size          = sizeof(BDRVTestState),
    .supports_backing       = true,

    .bdrv_close             = bdrv_test_close,
    .bdrv_co_preadv         = bdrv_test_co_preadv,

    .bdrv_drain_begin       = bdrv_test_drain_begin,
    .bdrv_drain_end         = bdrv_test_drain_end,

    .bdrv_child_perm        = bdrv_default_perms,

    .bdrv_co_change_backing_file = bdrv_test_co_change_backing_file,
};
```

```c
static int coroutine_fn bdrv_test_co_preadv(BlockDriverState *bs,
                                            int64_t offset, int64_t bytes,
                                            QEMUIOVector *qiov,
                                            BdrvRequestFlags flags)
{
    BDRVTestState *s = bs->opaque;

    /* We want this request to stay until the polling loop in drain waits for
     * it to complete. We need to sleep a while as bdrv_drain_invoke() comes
     * first and polls its result, too, but it shouldn't accidentally complete
     * this request yet. */
    qemu_co_sleep_ns(QEMU_CLOCK_REALTIME, 100000);

    if (s->bh_indirection_ctx) {
        // 在这里注册一个 bh
        aio_bh_schedule_oneshot(s->bh_indirection_ctx, co_reenter_bh,
                                qemu_coroutine_self());
        // yield 之后，那么会从当时 co enter 的地方离开
        qemu_coroutine_yield();

        // 之后，执行 bh callback 的地方，也就是 co_reenter_bh 中，
        // 来执行执行 co enter ，然后离开
    }

    return 0;
}
```

bdrv_test_co_preadv 似乎是一个经典案例，不过我不懂为什么要执行的这么复杂

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
