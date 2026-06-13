# coroutine

## qemu coroutine 基本的 API 理解
<!-- 76283f20-9da5-4d91-b900-1307ca7f15e6 -->

其实就两个函数:
1. qemu_coroutine_enter
2. qemu_coroutine_yield

tests/unit/test-coroutine.c
对应的二进制为: 在 build/tests/unit/test-coroutine

1. qemu_coroutine_create(yield_5_times, &done); 两个参数，一个 callback ，一个 callback 的参数
2. `qemu_coroutine_enter` 开始执行一个 coroutine ，开始执行的位置为 coroutine 上次执行 yield 的位置
3. 如果 coroutine 执行了 qemu_coroutine_yield 之后，那么 qemu_coroutine_enter 就可以返回了

那么 coroutine 中如果 yield 多次吗？ 当进入的时候，可以选择从哪里进入吗?

不可以，首先我们理解一下，为什么我们会使用 yield ，指的是现在放弃执行，但是内部的流程是
准确的，也就是 step1 -> step2 这样的

```c
void coroutine_fn my_co(void *opaque)
{
    step1();
    qemu_coroutine_yield();   // yield #1

    step2();
    qemu_coroutine_yield();   // yield #2

    step3();
    qemu_coroutine_yield();   // yield #3

    step4();
    qemu_coroutine_yield();   // yield #4

    step5();
    qemu_coroutine_yield();   // yield #5

    step6();
}
```
而且不要忘记了，只有当 qemu_coroutine_enter 执行完成之后，才可以继续执行下一个任务。

Coroutine 是可以动态创建的，例如很容易观察到:
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
                    - aio_dispatch_ready_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_scsi_handle_cmd
                            - virtio_scsi_handle_cmd_vq
                              - virtio_scsi_handle_cmd_req_submit
                                - scsi_req_enqueue
                                  - scsi_disk_dma_command
                                    - blk_is_available
                                      - qemu_coroutine_create


## qemu 使用 coroutine 的经典案例
<!-- 0ff96482-e084-47af-8851-36ced9d1b4c4 -->

1. 哪里使用 qemu_coroutine_yield ，哪里就是关键点，表示在 coroutine 中离开了
2. 观察那些被标记了 coroutine_fn 的函数，在 coroutine_fn 的函数不能直接被普通的函数调用
```c
/**
 * Mark a function that executes in coroutine context
 *
 * Functions that execute in coroutine context cannot be called directly from
 * normal functions.  In the future it would be nice to enable compiler or
 * static checker support for catching such errors.  This annotation might make
 * it possible and in the meantime it serves as documentation.
 *
 * For example:
 *
 *   static void coroutine_fn foo(void) {
 *       ....
 *   }
 */
#ifdef __clang__
#define coroutine_fn QEMU_ANNOTATE("coroutine_fn")
#else
#define coroutine_fn
#endif
```


### ssh backend

```c
/* A non-blocking call returned EAGAIN, so yield, ensuring the
 * handlers are set up so that we'll be rescheduled when there is an
 * interesting event on the socket.
 */
static coroutine_fn void co_yield(BDRVSSHState *s, BlockDriverState *bs)
{
    int r;
    IOHandler *rd_handler = NULL, *wr_handler = NULL;
    BDRVSSHRestart restart = {
        .bs = bs,
        .co = qemu_coroutine_self()
    };

    r = ssh_get_poll_flags(s->session);

    if (r & SSH_READ_PENDING) {
        rd_handler = restart_coroutine;
    }
    if (r & SSH_WRITE_PENDING) {
        wr_handler = restart_coroutine;
    }

    trace_ssh_co_yield(s->sock, rd_handler, wr_handler);

    aio_set_fd_handler(bdrv_get_aio_context(bs), s->sock,
                       rd_handler, wr_handler, NULL, NULL, &restart);
    qemu_coroutine_yield();
    trace_ssh_co_yield_back(s->sock);
}
```
这个效果和 io channel 很类似，也就是在qemu_coroutine_yield 的时候，设置
fd 的 handler 为新的 callback ，而这个 callback 的工作就是，立刻执行 aio_co_enter
让之后会立刻回到原来的位置中。


### io channel
- qio_channel_writev_full_all

```c
        if (len == QIO_CHANNEL_ERR_BLOCK) {
            if (qemu_in_coroutine()) {
                qio_channel_yield(ioc, G_IO_OUT);
            } else {
                qio_channel_wait(ioc, G_IO_OUT);
            }
            continue;
        }
```

还有的例子就是 process_incoming_migration_co

在如下 qemu_coroutine_yield 中：
block/nvme.c
block/nfs.c


### qemu 热迁移使用 coroutine
<!-- a6a68621-51e3-4049-895c-27db7e6b2e01 -->

热迁移中唯一的使用就是在 target 端
```c
void migration_incoming_process(void)
{
    Coroutine *co = qemu_coroutine_create(process_incoming_migration_co, NULL);
    qemu_coroutine_enter(co);
}
```

最开始的时候:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - qio_net_listener_channel_func
                  - migration_channel_process_incoming
                    - migration_ioc_process_incoming
                      - migration_incoming_process
                        - qemu_coroutine_enter
                          - qemu_aio_coroutine_enter
                            - qemu_coroutine_switch
                              - ??
                                - ??
                                  - ??
                                    - coroutine_trampoline
                                      - process_incoming_migration_co

忽然发现， process_incoming_migration_co 会去调用 qemu_loadvm_state ，
而所有的工作都是在 qemu_loadvm_state 中完成的。
qemu_loadvm_state 会阻塞很长事件，显然不能让 main loop 阻塞太长时间。

运行了一会儿之后，可以注意观察到这样的流程:

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
                    - aio_bh_poll
                      - co_schedule_bh_cb
                        - qemu_aio_coroutine_enter
                          - qemu_coroutine_switch
                            - ??
                              - ??
                                - ??
                                  - coroutine_trampoline
                                    - process_incoming_migration_co
                                      - qemu_loadvm_state
                                        - qemu_loadvm_state_main


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
                        - qemu_aio_coroutine_enter
                          - qemu_coroutine_switch
                            - ??
                              - ??
                                - ??
                                  - coroutine_trampoline
                                    - process_incoming_migration_co
                                      - qemu_loadvm_state
                                        - qemu_loadvm_state_main
                                          - qemu_loadvm_section_part_end
                                            - ram_load
                                              - ram_load_precopy
                                                - qemu_coroutine_yield


```c
static int ram_load_precopy(QEMUFile *f)
{
	// ... 就是自己主动的释放的
        /*
         * Yield periodically to let main loop run, but an iteration of
         * the main loop is expensive, so do it each some iterations
         */
        if ((i & 32767) == 0 && qemu_in_coroutine()) {
            aio_co_schedule(qemu_get_current_aio_context(),
                            qemu_coroutine_self());
            qemu_coroutine_yield();
        }
```

### snapshot

非常奇怪，为什么这个地方
```c
static int coroutine_fn snapshot_save_job_run(Job *job, Error **errp)
{
    SnapshotJob *s = container_of(job, SnapshotJob, common);
    s->errp = errp;
    s->co = qemu_coroutine_self();
    aio_bh_schedule_oneshot(qemu_get_aio_context(),
                            snapshot_save_job_bh, job);
    qemu_coroutine_yield();
    return s->ret ? 0 : -1;
}
```

### block 的例子
具体看 [为什么 qemu 需要使用 coroutine](#为什么-qemu-需要使用-coroutine)

## 为什么 qemu 需要使用 coroutine
<!-- 0d60dee2-ca79-49fa-b2ec-7e1a493485c8 -->

在 QEMU 中 coroutine 的实现原理和其他的 coroutine 没有区别，

Stefan Hajnoczi 说 QEMU 中需要 coroutine 是为了避免 callback hell
- http://blog.vmsplice.net/2014/01/coroutines-in-qemu-basics.html

下面是一个经典例子，首先接受到 Guest 的请求，所以需要开始读盘，读盘的时候，使用 coroutine 进入，
其中的工作就是通过 linux aio 来提交任务，提交之后立刻 yield 。后来任务完成，从 epoll 中接受到信号，
将直接跳转到 laio_co_submit 的位置，然后一路返回，那么从上层的调用者看，就像是调用了一个普通的同步函数。

所以，我们回到 virtio_blk_handle_vq 这个函数，他作为 kvm eventfd 的 callback ，在其他的循环中，需要不断的提交
任务给 linux aio ，但是看上去都是提交之后，然后可以立刻返回，或者会阻塞到返回，但是实际上，这是利用了 coroutine
才可以如此的。

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
                    - aio_dispatch_ready_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_blk_handle_vq
                            - virtio_blk_submit_multireq
                              - submit_requests
                                - blk_aio_preadv
                                  - blk_aio_prwv
                                    - qemu_aio_coroutine_enter
                                      - qemu_coroutine_switch
                                        - ??
                                          - ??
                                            - ??
                                              - coroutine_trampoline
                                                - blk_aio_read_entry
                                                  - blk_co_do_preadv_part
                                                    - bdrv_co_preadv_part
                                                      - bdrv_aligned_preadv
                                                        - bdrv_driver_preadv
                                                          - qcow2_co_preadv_part
                                                            - qcow2_add_task
                                                              - bdrv_co_preadv_part
                                                                - bdrv_aligned_preadv
                                                                  - bdrv_driver_preadv
                                                                    - raw_co_preadv
                                                                      - raw_co_prw
                                                                        - laio_co_submit
                                                                          - qemu_coroutine_yield

```c
int coroutine_fn laio_co_submit(int fd, uint64_t offset, QEMUIOVector *qiov,
                                int type, BdrvRequestFlags flags,
                                uint64_t dev_max_batch)
{
    // ...
      ret = laio_do_submit(fd, &laiocb, offset, type, flags, dev_max_batch);
    if (ret < 0) {
        return ret;
    }

    if (laiocb.ret == -EINPROGRESS) {
        qemu_coroutine_yield();
    }
```

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
                    - aio_dispatch_ready_handlers
                      - aio_dispatch_handler
                        - qemu_laio_completion_cb
                          - qemu_laio_process_completions_and_submit
                            - qemu_laio_process_completions
                              - qemu_aio_coroutine_enter


考虑一下，如果没有 coroutine 是如何解决的，内核的答案是:
- 调用 folio_wait_bit_common 之类的，然后被 schedule 走，当任务结束之后，被唤醒
- 类似 raid1 中，提交任务，立刻返回，中断结束之后，调用 raid_end_bio_io
    - 内核中大多数操作都是类似，提交之后(submit bio)，工作就结束了，不存在提交完成第一个任务，然后等待，然后提交第二个任务。
- aio 和 iouring 有点特殊，是把工作交给用户态了，没有参考性

从上面例子，我们也容易知道 coroutine yield ，如何知道唤醒哪一个:
首先确定是哪一个 fd 导致的，然后找到对应的 struct Coroutine ，然后进行执行，

### callback hell

暂时没有在 qemu 中找到例子，但是我相信一定存在类似的例子:
假设一个网页请求，需要加载三个资源，其逻辑为

```txt
load_pic_1
load_pic_2
load_pic_3
```
这三个 load 都是需要执行一些时间的，最简单就是等待。
但是 `load_pic_1` 可以提交之后，立刻离开，当 `load_pic_1`
完成之后，立刻开始执行 `load_pic_2` 。我们需要的可以立刻
可以跳回来。

类似，加入 qemu 面对一个 Guest 请求，需要对于物理盘发起多个 io ，他们互相有逻辑依赖，
必须完成第一个然后才决定如何进入第二个，那么就需要使用 coroutine 了。

## aio 的进一步封装

先看完这个再说吧:
docs/devel/multiple-iothreads.rst

https://www.qemu.org/docs/master/devel/multiple-iothreads.html

The main difference between legacy code and new code that can run in an IOThread
is dealing explicitly with the event loop object, AioContext (see include/block/aio.h).
Code that only works in the main loop implicitly uses the main loop’s AioContext.
Code that supports running in IOThreads must be aware of its AioContext.

```c
void aio_co_enter(AioContext *ctx, Coroutine *co)
{
    // 如果 aio_co_enter 加入的 AioContext 不是当前线程的，那么加入到该 AioContext
    if (ctx != qemu_get_current_aio_context()) {
        aio_co_schedule(ctx, co);
        return;
    }

    if (qemu_in_coroutine()) {
        // 如果是递归的 aio_co_enter，那么挂载到 list 上
        Coroutine *self = qemu_coroutine_self();
        assert(self != co);
        QSIMPLEQ_INSERT_TAIL(&self->co_queue_wakeup, co, co_queue_next);
    } else {
        // 否则可以直接执行了
        qemu_aio_coroutine_enter(ctx, co);
    }
}
```

其具体实现接口可以参考 https://www.cnblogs.com/VincentXu/p/3350389.html

## qemu_aio_coroutine_enter 和 qemu_coroutine_enter 有什么区别吗?

## 实现细节
https://royhunter.github.io/2016/06/24/qemu-coroutine/

```txt
🧀  l coro*
Permissions Size User     Date Modified Name
.rw-r--r--  9.0k martins3  5 Oct  2023   coroutine-sigaltstack.c
.rw-r--r--   11k martins3 26 Jan  2024   coroutine-ucontext.c
.rw-r--r--  3.7k martins3 28 May 13:46   coroutine-wasm.c
.rw-r--r--  3.3k martins3 22 Jun 21:53   coroutine-windows.c
```

https://qemu-project.gitlab.io/qemu/devel/block-coroutine-wrapper.html

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
