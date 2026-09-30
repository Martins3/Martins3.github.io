# qemu thread io

## qemu 中的 aio 的工作机制
<!-- e1784cdd-846a-43ee-bfbf-d42b4926ba42 -->

我们理解 aio 机制的时候，总是会用 io_submit 和 io_getevents 组合，但是实际上，
qemu 使用 aio 的方法和想象的不一样，他是使用的

具体来说，在 block/linux-aio.c:laio_do_submit 中的
io_set_eventfd(&laiocb->iocb, event_notifier_get_fd(&s->e));

在 aio_complete 中如果检查到了，然后 epoll 就会收到消息

可以发现，qemu 只会调用 只有 io_submit ，没有 io_getevents
```txt
sudo syscount -p 43821
[sudo] password for martins3:
Tracing syscalls, printing top 10... Ctrl+C to quit.
^C[13:40:44]
SYSCALL                   COUNT
ppoll                     14687
io_submit                 12265
write                     10230
ioctl                      9074
read                       7990
sendto                        5
```

才意识到，

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
                    - ioq_submit
                      - qemu_laio_process_completions
                        - io_getevents_advance_and_peek
                          - io_getevents_peek

qemu iothread 的 perf 结果:
```txt
   - qemu_thread_start
      - 46.36% iothread_run
         - 42.84% aio_poll
            - 14.41% clock_gettime@@GLIBC_2.17
                 __vdso_clock_gettime
            - 13.74% aio_dispatch_handler
               - 12.21% virtio_queue_notify_vq.part.0
                  - 12.13% virtio_blk_handle_vq
                     - 5.03% virtqueue_split_pop
                        - 1.52% virtqueue_map_desc
                             1.11% address_space_map
                     - 2.52% virtio_blk_submit_multireq
                        - 2.20% blk_aio_pwritev
                             2.10% blk_aio_prwv
                     - 1.77% virtio_blk_handle_request
                        - 1.09% virtio_blk_submit_multireq
                             blk_aio_pwritev
                             blk_aio_prwv
                       1.45% __memset_avx2_unaligned_erms
                     - 0.98% defer_call_end
                        - 0.93% ioq_submit
                             0.61% qemu_laio_process_completions
                 0.73% qemu_laio_poll_ready
                 0.66% qemu_laio_completion_cb
            - 5.81% virtio_queue_host_notifier_aio_poll
                 4.76% virtio_queue_empty
                 0.80% get_ptr_rcu_reader
              1.16% virtio_queue_empty
              0.98% qemu_clock_get_ns
              0.66% __vdso_clock_gettime
            - 0.64% fdmon_poll_wait
               - ppoll
                  - 0.60% entry_SYSCALL_64_after_hwframe
                       do_syscall_64
                     - __x64_sys_ppoll
                          do_sys_poll
           0.85% virtio_queue_host_notifier_aio_poll
           0.71% qemu_laio_poll_cb
           0.70% aio_context_notifier_poll
           0.66% qemu_clock_get_ns
```

也就是 qemu 会去使用 epoll 来监听 event fd ，然后
使用来直接访问 aio 的共享队列，而不是使用 io_getevents

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - qemu_laio_poll_ready


### aio 本来就是有环形队列的
qemu 中的 io_getevents_peek 让我意识到，aio 也是环形队列啊，为什么不去直接使用 aio
而且重新发明 iouring 出来，我猜测，是因为 aio 的确可以，但是是破缺的状态，而且这个不是 iouring 的全部。

### 所有的文件都是公用一个 eventfd 的

所有的文件后端都是公用一个 AioContext 的，这自然是废话，
如果不用 io thread ，那么一共就两个 AioContext :
```txt
#0  laio_init (errp=0x7ffff2bcf768) at ../block/linux-aio.c:468
#1  0x0000555555f33ccd in aio_setup_linux_aio (ctx=0x5555572665a0, errp=errp@entry=0x7ffff2bcf768) at ../util/async.c:450
#2  0x0000555555e422e2 in raw_check_linux_aio (s=0x555557532380) at ../block/file-posix.c:2546
#3  raw_co_prw (bs=0x55555752b880, offset_ptr=offset_ptr@entry=0x7ffff2bcf7b0, bytes=4096, qiov=0x7ffff2bcfaa8, type=type@entry=1, flags=0) at ../block/file-posix.c:2593
#4  0x0000555555e3f52b in raw_co_preadv (bs=0x7ffff2bcf768, offset=46983020544, bytes=0, qiov=0x200, flags=BDRV_REQ_FUA) at ../block/file-posix.c:2656
#5  0x0000555555de4f9f in bdrv_driver_preadv (bs=bs@entry=0x55555752b880, offset=offset@entry=46983020544, bytes=bytes@entry=4096, qiov=qiov@entry=0x7ffff2bcfaa8, qiov_offset=0, flags=0) at ../block/io.c:1003
```

此外，可以发现原来所有的 laio_init ，一个 AioContext 对应一个 LinuxAioState ，对应一个 eventfd
而这个 eventfd 来收听所有的文件的:
```c
LinuxAioState *laio_init(Error **errp)
{
    int rc;
    LinuxAioState *s;

    s = g_malloc0(sizeof(*s));
    rc = event_notifier_init(&s->e, false);
    if (rc < 0) {
        error_setg_errno(errp, -rc, "failed to initialize event notifier");
        goto out_free_state;
    }

    rc = io_setup(MAX_EVENTS, &s->ctx);
    if (rc < 0) {
        error_setg_errno(errp, -rc, "failed to create linux AIO context");
        goto out_close_efd;
    }

    ioq_init(&s->io_q);

    return s;

out_close_efd:
    event_notifier_cleanup(&s->e);
out_free_state:
    g_free(s);
    return NULL;
}
```

## qemu 中 iouring 的角色
<!-- 1ef43b9c-9948-4df2-b2f9-41dc4e49416c -->

1. block/io_uring.c : 作为 io engine ，可以和 epoll 协同工作，让 epoll 来监听 uring_fd
2. util/fdmon-io_uring.c : 作为 fd monitor ，其定位和 epoll 相同，是当前默认选项 (2026-04-10)

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch,
                  - aio_dispatch
                    - fdmon_io_uring_dispatch



## qemu 模拟 io 的使用两个 fd
<!-- 29773215-b5bf-4c8f-9ad0-1414a2d6e178 -->
1. 一个本地文件，一个来自 eventfd
2. blk_aio_read_entry 是完成 callback hell 的工作的


首先，是 eventfd 触发，然后就开始利用 aio 提交中断，
然后提交 io  ，在中间切换为 coroutine ，一路调用到
laio_co_submit 提交只有立刻 yield 。

之后，本地文件 io 事件到达，然后很快的就切入到 coroutine 中，
那么就是 laio_co_submit 工作的继续了。

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
                                                                                  - laio_co_submit
                                                                                    - qemu_coroutine_yield

没有 iothread 的:
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch,
                  - aio_dispatch
                    - aio_dispatch_handlers
                      - aio_dispatch_handler
                        - qemu_laio_completion_cb
                          - qemu_laio_process_completions_and_submit
                            - qemu_laio_process_completions
                              - qemu_aio_coroutine_enter
                                - qemu_coroutine_switch
                                  - __start_context
                                    - coroutine_trampoline
                                      - blk_aio_read_entry
                                        - blk_aio_complete
                                          - blk_aio_complete
                                            - dma_blk_cb
                                              - dma_complete
                                                - scsi_dma_complete
                                                  - scsi_dma_complete_noio
                                                    - scsi_req_complete
                                                      - virtio_scsi_command_complete
                                                        - virtio_scsi_complete_cmd_req
                                                          - virtio_scsi_complete_req
                                                            - virtio_notify_irqfd

使用了 iothread 的:
- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_dispatch_ready_handlers
            - aio_dispatch_handler
              - qemu_laio_completion_cb
                - qemu_laio_process_completions_and_submit
                  - qemu_laio_process_completions
                    - qemu_aio_coroutine_enter
                      - qemu_coroutine_switch
                        - ??
                          - ??
                            - ??
                              - coroutine_trampoline
                                - blk_aio_read_entry
                                  - blk_aio_complete
                                    - blk_aio_complete
                                      - virtio_blk_rw_complete


所以，所谓 callback hell 就是折叠在这里的:
```c
static void coroutine_fn blk_aio_read_entry(void *opaque)
{
    BlkAioEmAIOCB *acb = opaque;
    BlkRwCo *rwco = &acb->rwco;
    QEMUIOVector *qiov = rwco->iobuf;

    assert(qiov->size == acb->bytes);
    // 这个函数中会 yield ，直到接受到该任务已经结束了
    rwco->ret = blk_co_do_preadv_part(rwco->blk, rwco->offset, acb->bytes, qiov,
                                      0, rwco->flags);
    // 现在来完成工作，所以，这里看上去就像是一个串行的一样
    blk_aio_complete(acb);
}
```

## QEMU_LOCK_GUARD
<!-- ccab10af-7cd5-471b-8656-5df4c4a8291f -->

利用 g_autoptr ，应该是利用 gcc 的 cleanup attribute 机制了

## 文档
http://blog.vmsplice.net/2011/03/qemu-internals-overall-architecture-and.html

https://www.qemu.org/docs/master/devel/multiple-iothreads.html

> Side note: the best way to schedule a function call across threads is to call aio_bh_schedule_oneshot().

[Towards Multi-threaded Device Emulation in QEMU](https://www.linux-kvm.org/images/a/a7/02x04-MultithreadedDevices.pdf)

# qemu 到底有那些 thread
<!-- 841889be-f42d-453d-8160-a499075e2305 -->

```txt
gdb) info threads
  Id   Target Id                                            Frame
* 1    Thread 0xffff7f98e500 (LWP 771090) "qemu-system-aar" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  2    Thread 0xfffd557cea60 (LWP 771112) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  3    Thread 0xfffd55fdea60 (LWP 771111) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  4    Thread 0xfffd567eea60 (LWP 771109) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  5    Thread 0xffff7abeca60 (LWP 771108) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  6    Thread 0xfffd56ffea60 (LWP 771099) "vnc_worker"      0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  7    Thread 0xfffd66fd9a60 (LWP 771098) "CPU 1/KVM"       0x0000ffff7dc98fd0 in ioctl () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  8    Thread 0xfffd677e9a60 (LWP 771097) "CPU 0/KVM"       0x0000ffff7dc98fd0 in ioctl () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  9    Thread 0xfffd67ff9a60 (LWP 771096) "IO mon_iothread" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  10   Thread 0xffff7f98e500 (LWP 771095) "vhost-771090"    0x0000000000000000 in ?? ()
  11   Thread 0xffff7f98e500 (LWP 771094) "vhost-771090"    0x0000000000000000 in ?? ()
  12   Thread 0xffff7b4fda60 (LWP 771092) "qemu-system-aar" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  13   Thread 0xffff7be0ea60 (LWP 771091) "qemu-system-aar" 0x0000ffff7dc9bee4 in syscall () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
```
- 1 是 main loop
- 12 virtio_blk_io0
- 13 rcu

在 `qemu_thread_create` 中添加调试语句:
```txt
call_rcu
IO virtio_blk_io0
worker
IO mon_iothread
CPU 0/KVM
CPU 1/KVM
```

作为热迁移 target 端启动，但是还没有发起热迁移的时候:
```txt
info threads
  Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 236075) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 236078) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 236079) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  4    Thread 0x7fffef7fe6c0 (LWP 236081) "worker"          0x00007ffff649450e in __futex_abstimed_wait_common ()
  5    Thread 0x7fffecffb6c0 (LWP 236086) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 236087) "CPU 0/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  7    Thread 0x7fffe77fe6c0 (LWP 236088) "CPU 1/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  8    Thread 0x7ffddebff6c0 (LWP 236091) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
```

当发起热迁移的时候:

在 target 端:
```txt
  Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 240613) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 240616) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 240617) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  5    Thread 0x7fffecffb6c0 (LWP 240623) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 240624) "CPU 0/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  7    Thread 0x7fffe77fe6c0 (LWP 240625) "CPU 1/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  8    Thread 0x7ffddebff6c0 (LWP 240628) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
  9    Thread 0x7ffdde3fe6c0 (LWP 241156) "mig/dst/recv_0"  0x00007ffff651e117 in recvmsg ()
  10   Thread 0x7ffdddbfd6c0 (LWP 241157) "mig/dst/recv_1"  0x00007ffff651e117 in recvmsg ()
  11   Thread 0x7ffddd3fc6c0 (LWP 241158) "mig/dst/recv_2"  0x00007ffff651e117 in recvmsg ()
  12   Thread 0x7ffddcbfb6c0 (LWP 241160) "mig/dst/recv_3"  0x00007ffff651e117 in recvmsg ()
```

在 source 端:
```txt
 Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 238966) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 238969) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 238970) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  5    Thread 0x7fffecffb6c0 (LWP 238979) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 238980) "CPU 0/KVM"       0x00007ffff6516f0f in ioctl ()
  7    Thread 0x7fffe77fe6c0 (LWP 238981) "CPU 1/KVM"       0x00007ffff6516f0f in ioctl ()
  8    Thread 0x7ffddebff6c0 (LWP 238984) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
  17   Thread 0x7ffddd3fc6c0 (LWP 241142) "mig/src/main"    0x00007ffff649450e in __futex_abstimed_wait_common ()
  20   Thread 0x7ffdddbfd6c0 (LWP 241147) "mig/src/send_0"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  22   Thread 0x7ffdde3fe6c0 (LWP 241151) "mig/src/send_1"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  24   Thread 0x7fffee7fc6c0 (LWP 241154) "mig/src/send_2"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  25   Thread 0x7fffedffb6c0 (LWP 241155) "mig/src/send_3"  0x00007ffff649450e in __futex_abstimed_wait_common ()
```

热迁移的参数为:
```c
			local cmds=(
				"migrate_set_capability multifd on"
				"migrate_set_parameter max-bandwidth 1 "
				"migrate_set_parameter multifd-channels 4"
				"migrate_set_parameter multifd-compression zstd"
				"migrate_incoming unix:$vm_dir/migrate.sock"
			)
```

所以，正好会构建出来 4 个额外的 thread 来发送和接受，这是预期的。

发送端的结果为:
- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_send_thread
        - qemu_sem_wait
          - qemu_cond_wait_impl
            - pthread_cond_wait@@GLIBC_2.3.2
              - __futex_abstimed_wait_common
                - __futex_abstimed_wait_common

接收端的结果为:
- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_recv_thread
        - qio_channel_readv_full_all_eof
          - qio_channel_socket_readv
            - recvmsg

## qemu 中有三个 thread 在调用 ppoll
<!-- f6cd118b-8c5e-4cea-a1af-97461adb9917 -->

### virtio blk iothread
- thread_start
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_poll_wait
            - qemu_poll_ns
              - ppoll
                - ppoll

这种的 iothread 是通过 qom 构建的:
- main
  - qemu_init
    - qemu_create_early_backends
      - object_option_foreach_add
        - user_creatable_add_qapi
          - user_creatable_add_type
            - object_new_with_type
              - object_initialize_with_type
                - object_init_with_type
                  - iothread_instance_init

### main loop
基本上监听任何东西:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - qemu_poll_ns
            - ppoll
              - ppoll

### 发现

iothread_run 总是有两个不同的调用方向:

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_poll_wait
            - qemu_poll_ns
              - ppoll
                - ppoll

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - g_main_loop_run
          - g_main_context_iterate_unlocked.isra
            - ppoll

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
