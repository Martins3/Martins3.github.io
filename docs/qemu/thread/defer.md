## util/defer-call.c
- defer_call_begin
- defer_call
- defer_call_end


## QEMUBH

- aio_bh_call
- qemu_bh_schedule


- 提交任务 : `qemu_bh_schedule(ctx->co_schedule_bh)`
- 执行任务 : 当 aio_poll 的时候，会执行 `ctx->co_schedule_bh` 上的 hook, 也即是
  co_schedule_bh_cb, 在其中调用 qemu_aio_coroutine_enter 来执行。

将一个函数挂到队列上，之后从队列上取出函数(也许是另一个 thread) 来执行。
```c
struct QEMUBH {
    AioContext *ctx;
    const char *name;
    QEMUBHFunc *cb;
    void *opaque;
    QSLIST_ENTRY(QEMUBH) next;
    unsigned flags;
};
```
QEMU 默认使用 eventfd 来进行通知(参考 : event_notifier_init)，

而且 aio_set_event_notifier 之后会调用的 g_source_add_poll 的,
将 AioContext::notifier 作为一个普通的 fd 来监控。

aio_context_new 中:
```c
aio_set_event_notifier(ctx, &ctx->notifier, false,
                       aio_context_notifier_cb,
                       aio_context_notifier_poll);
```

- 提交任务 : `qemu_bh_schedule`
  - 通知认为已经提交了: `aio_notify` => `event_notifier_set(&ctx->notifier)` => 一个简单的 write 操作
- 轮询: `aio_poll` => `aio_bh_poll` => `aio_bh_call`


```c
void qemu_bh_schedule(QEMUBH *bh)
{
    aio_bh_enqueue(bh, BH_SCHEDULED);
}
```

## defer-call 经典案例



那么  linux-aio 中使用，和 virtio blk 中使用是不是有什么区别?
```txt
block/linux-aio.c
237:    defer_call_end();

block/io_uring.c
226:    defer_call_end();

include/qemu/defer-call.h
13:void defer_call_end(void);

```

调用 defer_call 的调用地方都是存储后端，
也就是 aio io_uring 和 nvme 直通，以及 virtio 的 irqfd

1. virtio_notify_irqfd 中容易理解，无需多次注入中断
2. 在 block/linux-aio.c

### scsi

```txt
- virtio_scsi_handle_cmd_req_submit
  - scsi_do_read
    - dma_blk_io
      - dma_blk_cb
        - blk_aio_preadv
          - blk_aio_prwv
  - defer_call_end
    - ioq_submit
      - io_submit
```


### virtio blk

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

简单分析 virtio_scsi_handle_cmd_req_submit 的流程

- 由于接受到 eventfd 的消息，所以会调用到 virtio_scsi_handle_cmd_vq 中

- virtio_scsi_pop_req : 取出来需要执行的任务
- virtio_scsi_handle_cmd_req_prepare
  - defer_call_begin
- defer_call_end

- virtio_scsi_handle_cmd_req_submit
  - scsi_req_enqueue : 这里可能根据解析的内容，需要提交很多次 io
  - defer_call_end : 在这里最后完成 aio 的提交

## qemu bh 的经典案例

### qmp
例如执行:
```json
{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }
```

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - do_qmp_dispatch_bh
                            - qmp_marshal_qom_get
                              - qmp_qom_get
                                - object_property_get_qobject
                                  - object_property_get
                                    - property_get_alias
                                      - object_property_get
                                        - balloon_stats_get_all

### nvme
在 nvme 上随意触发一个 io ，就可以得到这样的结果:

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - nvme_process_sq
                            - nvme_update_sq_tail
                              - ldl_le_pci_dma
                                - ldl_le_dma
                                  - dma_memory_read
                                    - dma_memory_rw
                                      - dma_memory_rw_relaxed
                                        - address_space_rw
                                          - address_space_read_full
                                            - flatview_read
                                              - flatview_translate
                                                - flatview_do_translate
                                                  - address_space_translate_iommu
                                                    - amdvi_translate

### [ ] timer
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - qemu_clock_run_all_timers
          - qemu_clock_run_timers
            - timerlist_run_timers
              - timerlist_run_timers
                - qemu_bh_schedule

### scsi

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
