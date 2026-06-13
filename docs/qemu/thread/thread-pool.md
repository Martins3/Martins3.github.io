## qemu 的 thread pool 的作用
<!-- d5b72a47-2bd0-4ebf-8078-f1e96a762e8d -->

显然，是由于 preadv 之类的同步 io

主要使用者为 : block/file-posix.c

### 如果磁盘的后端不是 aio
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
                                - scsi_write_data
                                  - dma_blk_io
                                    - dma_blk_cb
                                      - blk_aio_pwritev
                                        - blk_aio_prwv
                                          - qemu_aio_coroutine_enter
                                            - qemu_coroutine_switch
                                              - ??
                                                - ??
                                                  - ??
                                                    - coroutine_trampoline
                                                      - blk_aio_write_entry
                                                        - blk_co_do_pwritev_part
                                                          - bdrv_co_pwritev_part
                                                            - bdrv_aligned_pwritev
                                                              - bdrv_driver_pwritev
                                                                - qcow2_co_pwritev_part
                                                                  - qcow2_add_task
                                                                    - qcow2_co_pwritev_task_entry
                                                                      - qcow2_co_pwritev_task
                                                                        - bdrv_co_pwritev_part
                                                                          - bdrv_aligned_pwritev
                                                                            - bdrv_driver_pwritev
                                                                              - raw_co_pwritev
                                                                                - raw_co_prw
                                                                                  - raw_thread_pool_submit
                                                                                    - thread_pool_submit_co
                                                                                      - thread_pool_submit_aio
                                                                                        - spawn_thread

创建是在 bh 中进行的:
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
                      - spawn_thread_bh_fn
                        - do_spawn_thread
                          - qemu_thread_create

最后的结果在这里:
- __clone3
  - start_thread
    - qemu_thread_start
      - worker_thread
        - handle_aiocb_rw

### 如果磁盘的后端为 aio

从 ioeventfd 到 linux aio 的结果:
- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - virtio_queue_notify_vq
                - virtio_blk_handle_vq
                  - defer_call_end

### 并不是所有的后端都是使用 thread-pool 的

例如 ssh 作为后端，就是同步的:

- ??
  - coroutine_trampoline
    - blk_aio_write_entry
      - blk_co_do_pwritev_part
        - bdrv_co_pwritev_part
          - bdrv_aligned_pwritev
            - bdrv_driver_pwritev
              - qcow2_co_pwritev_part
                - qcow2_add_task
                  - qcow2_co_pwritev_task_entry
                    - qcow2_co_pwritev_task
                      - bdrv_co_pwritev_part
                        - bdrv_aligned_pwritev
                          - bdrv_driver_pwritev
                            - ssh_co_writev
                              - ssh_write
                                - sftp_write (libssh 提供的库函数)


## 实现原理
总体来说，worker pool 的设计比较简单的，整个 thread-pool.c 也就是只有 300 行左右, 这个主要关联的两个结构体:

```c
struct ThreadPool {
    QemuSemaphore sem; // 工作线程idle时休眠的信号量

    /* The following variables are protected by lock.  */
    QTAILQ_HEAD(, ThreadPoolElement) request_list;
};

struct ThreadPoolElement {
    ThreadPool *pool;      // 所属线程池
    ThreadPoolFunc *func;  // 要在线程池中完成的工作
    void *arg;             // 线程池中完成的工作的参数

    /* Access to this list is protected by lock.  */
    QTAILQ_ENTRY(ThreadPoolElement) reqs; // 通过这个将自己放到 ThreadPool::request_list 上
};
```

- thread_pool_submit_aio : 将任务提交给 thread pool，如果 pool 中没有 idle thread，会调用 spawn_thread 来创建
- worker_thread 和核心执行流程，在 thread_pool_submit_aio 中 qemu_sem_post(ThreadPool::sem) 会让 worker_thread 从这个 lock 上醒过来
然后会从 ThreadPool::request_list 中获取需要执行的函数，最后使用 `qemu_bh_schedule(pool->completion_bh)` 通知这个任务结束了

- 在 worker_thread 中，qemu_sem_timedwait(ThreadPool::sem) 最多只会等待 10s 如果没有任务过来，那么这个 thread 结束。


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
