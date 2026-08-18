# qemu defer 机制

这个机制和 Linux kernel 的 block plug 机制非常类似

util/defer-call.c 开头的位置就说的非常清楚了:
```txt
defer_call_begin();

defer_call(fn, obj);
defer_call(fn, obj);
defer_call(fn, obj);

defer_call_end();   /* 在这里、当前线程中调用一次 fn(obj) */
```

## virtio-blk 为什么使用 defer-call

  你记录的 virtio-blk/Linux AIO 例子选得很好，只是应该这样理解：

  virtio_blk_handle_vq()
      defer_call_begin()

      处理 request 1
          加入 Linux AIO pending queue
          defer_call(laio_deferred_fn, s)

      处理 request 2
          加入同一个 pending queue
          defer_call(laio_deferred_fn, s)

      处理 request 3
          加入同一个 pending queue
          defer_call(laio_deferred_fn, s)

      defer_call_end()
          laio_deferred_fn(s) 只执行一次
              ioq_submit()
                  一次 io_submit() 提交多个 iocb

  外层位置在 hw/block/virtio-blk.c:1017，Linux AIO 调用在 block/linux-aio.c:479。

  这不是把 I/O 交给另一个线程，而是避免：

  每取出一个 virtqueue request
      就调用一次昂贵的 io_submit()

  换成：

  一次取出一批 request
      最后统一 io_submit()

  virtio irqfd 里的 defer_call 同理：一批请求可能完成很多次，但相同 notifier 最后只需要通知一次。

## 例子

例如 virtio blk 中，

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
                            - virtio_scsi_handle_cmd_vq
                              - virtio_scsi_handle_cmd_req_submit
                                - defer_call_end
                                  - ioq_submit (这个在 linux 中的 block/linux-aio.c)
                                    - qemu_laio_process_completions
                                      - qemu_bh_schedule

# TODO
## 为什么会出现 aio 的嵌套?

```c
/**
 * qemu_laio_process_completions:
 * @s: AIO state
 *
 * Fetches completed I/O requests and invokes their callbacks.
 *
 * The function is somewhat tricky because it supports nested event loops, for
 * example when a request callback invokes aio_poll().  In order to do this,
 * indices are kept in LinuxAioState.  Function schedules BH completion so it
 * can be called again in a nested event loop.  When there are no events left
 * to complete the BH is being canceled.
 */
static void qemu_laio_process_completions(LinuxAioState *s)
```
为什么会出现在 callback 中调用 aio_poll 的?


```diff
History:        #0
Commit:         2cdff7f620ebd3b5246cf0c0d1f6fa0eededa4ca
Author:         Stefan Hajnoczi <stefanha@redhat.com>
Author Date:    Mon 04 Aug 2014 11:56:33 PM CST
Committer Date: Fri 29 Aug 2014 10:59:17 PM CST

linux-aio: avoid deadlock in nested aio_poll() calls

If two Linux AIO request completions are fetched in the same
io_getevents() call, QEMU will deadlock if request A's callback waits
for request B to complete using an aio_poll() loop.  This was reported
to happen with the mirror blockjob.

This patch moves completion processing into a BH and makes it resumable.
Nested event loops can resume completion processing so that request B
will complete and the deadlock will not occur.

Cc: Kevin Wolf <kwolf@redhat.com>
Cc: Paolo Bonzini <pbonzini@redhat.com>
Cc: Ming Lei <ming.lei@canonical.com>
Cc: Marcin Gibuła <m.gibula@beyond.pl>
Reported-by: Marcin Gibuła <m.gibula@beyond.pl>
Signed-off-by: Stefan Hajnoczi <stefanha@redhat.com>
Tested-by: Marcin Gibuła <m.gibula@beyond.pl>
```

哦，原来的确有那么多位置都是在调用 aio_poll 的

例如，
```c
static int nvme_admin_cmd_sync(BlockDriverState *bs, NvmeCmd *cmd)
{
    BDRVNVMeState *s = bs->opaque;
    NVMeQueuePair *q = s->queues[INDEX_ADMIN];
    AioContext *aio_context = bdrv_get_aio_context(bs);
    NVMeRequest *req;
    int ret = -EINPROGRESS;
    req = nvme_get_free_req_nowait(q);
    if (!req) {
        return -EBUSY;
    }
    nvme_submit_command(q, req, cmd, nvme_admin_cmd_sync_cb, &ret);

    AIO_WAIT_WHILE(aio_context, ret == -EINPROGRESS);
    return ret;
}
```
这个例子有点难以触发，需要直通 nvme 才可以

使用 hmp 执行 screendump a.ppm 可以非常容易的触发

我们观察到一个这样的调用路径，以前就是 handler 中，会继续去调用 aio_poll

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - aio_poll




在 handle_hmp_command 中，当走这个分支的时候:


有的 hmp 命令希望在 coroutine 中执行:
```c
        HandleHmpCommandCo data = {
            .mon = &mon->common,
            .cmd = cmd,
            .qdict = qdict,
            .done = false,
        };
        Coroutine *co = qemu_coroutine_create(handle_hmp_command_co, &data);
        monitor_set_cur(co, &mon->common);
        aio_co_enter(qemu_get_aio_context(), co);
        AIO_WAIT_WHILE_UNLOCKED(NULL, !data.done);
        // TODO 但是我不理解，为什么这里需要在这里执行的
```

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - qemu_aio_coroutine_enter
                            - qemu_coroutine_switch
                              - ??
                                - ??
                                  - ??
                                    - coroutine_trampoline
                                      - handle_hmp_command_co


通过一个个的找，然后就可以知道真正的工作在:

- hmp_screendump
  - qmp_screendump
    - qemu_console_co_wait_update
      - 注册并且运行 graphic_hw_update_bh

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - aio_poll
                            - aio_bh_poll
                              - graphic_hw_update_bh
                                - graphic_hw_update

在看看这个例子吧，这个例子真的好:

- main
  - qemu_init
    - qemu_create_early_backends
      - configure_blockdev
        - qemu_opts_foreach
          - drive_init_func
            - drive_new
              - blockdev_init
                - blk_new_open
                  - bdrv_open
                    - bdrv_open_inherit
                      - bdrv_open_common
                        - bdrv_open_driver
                          - qcow2_open
                            - aio_poll
                              - aio_bh_poll
                                - aio_bh_call



- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_drive_add
                                - drive_new
                                  - blockdev_init
                                    - blk_new_open
                                      - bdrv_open
                                        - bdrv_open_inherit
                                          - bdrv_open_common
                                            - bdrv_open_driver
                                              - qcow2_open

- qcow2_open 中的 AIO_WAIT_WHILE_UNLOCKED 如何始终无法等待到
qoc.ret == -EINPROGRESS 会导致 hmp 卡住吗?

答案是，就是会卡住

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_drive_add
                                - drive_new
                                  - blockdev_init
                                    - blk_new_open
                                      - bdrv_open
                                        - bdrv_open_inherit
                                          - bdrv_open_common
                                            - bdrv_open_driver
                                              - qcow2_open
                                                - aio_poll
                                                  - fdmon_poll_wait
                                                    - qemu_poll_ns
                                                      - ppoll
                                                        - ppoll


1. 第一个奇怪的事情，aio_co_enter 和 qemu_coroutine_enter 不同，qcow2_open_entry 实际上是作为
不会立刻去执行的

```txt
    aio_co_enter(bdrv_get_aio_context(bs),
                 qemu_coroutine_create(qcow2_open_entry, &qoc));
    AIO_WAIT_WHILE_UNLOCKED(NULL, qoc.ret == -EINPROGRESS);
```
但是这个时候监听的也是所有的吧

2. 此时，虚拟机将无法继续操作，也就是此时的 ppoll 的
  - hmp 失效
  - qmp 可以正常工作
  - 无法 ssh
  - 可以介绍到 ctrl c ，但是无法正确的处理

认为这里的是由于 aio_poll 监听的 fd 不能包含

## 为什么内部的 aio_poll 不能监听外部的事件的情况

通过 blk_drain 继续调试一下，按道理，这里的 while 是不影响整个 callback 的工作的
```txt
    AIO_WAIT_WHILE(blk_get_aio_context(blk),
                   qatomic_read(&blk->in_flight) > 0);
```

## aio_wait_kick 的作用是什么?

## 执行 aio_poll 就是为了死等

## 我 tm 的受不了了，居然还有 record replay 的问题

```c
static void virtio_net_handle_tx_bh(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIONet *n = VIRTIO_NET(vdev);
    VirtIONetQueue *q = &n->vqs[vq2q(virtio_get_queue_index(vq))];

    if (unlikely(n->vhost_started)) {
        return;
    }

    if (unlikely((n->status & VIRTIO_NET_S_LINK_UP) == 0)) {
        virtio_net_drop_tx_queue_data(vdev, vq);
        return;
    }

    if (unlikely(q->tx_waiting)) {
        return;
    }
    q->tx_waiting = 1;
    /* This happens when device was stopped but VCPU wasn't. */
    if (!vdev->vm_running) {
        return;
    }
    virtio_queue_set_notification(vq, 0);
    replay_bh_schedule_event(q->tx_bh);
}
```

- main,
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - aio_bh_call
                        - virtio_net_tx_bh

## 全村最后的希望

```c
static inline BlockAIOCB *null_aio_common(BlockDriverState *bs,
                                          BlockCompletionFunc *cb,
                                          void *opaque)
{
    NullAIOCB *acb;
    BDRVNullState *s = bs->opaque;

    acb = qemu_aio_get(&null_aiocb_info, bs, cb, opaque);
    /* Only emulate latency after vcpu is running. */
    if (s->latency_ns) {
        aio_timer_init(bdrv_get_aio_context(bs), &acb->timer,
                       QEMU_CLOCK_REALTIME, SCALE_NS,
                       null_timer_cb, acb);
        timer_mod_ns(&acb->timer,
                     qemu_clock_get_ns(QEMU_CLOCK_REALTIME) + s->latency_ns);
    } else {
        replay_bh_schedule_oneshot_event(bdrv_get_aio_context(bs),
                                         null_bh_cb, acb);
    }
    return &acb->common;
}
```
忽然想到，这个 callback 的执行时机是非常特殊的，是 eventfd 的 callback 执行
完成之后，

- null_bh_cb 中执行的 cb 居然是 bdrv_co_io_em_complete

```c
static void bdrv_co_io_em_complete(void *opaque, int ret)
{
    CoroutineIOCompletion *co = opaque;

    co->ret = ret;
    aio_co_wake(co->coroutine);
}
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
