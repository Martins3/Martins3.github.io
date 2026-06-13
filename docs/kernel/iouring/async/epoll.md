# epoll
## iouring 的 poll 是如何工作的
<!-- 9821ed93-4686-4942-a632-c9f105269380 -->

才意识到 iouring 是如何深度介入到 epoll 机制的:

1. 第一种，也就是下面提到的，类似 read / recv 这种网络的 io 提交之后，这个相当于是 iouring 来重新实现 epoll 系统调用
2. epoll 可以来监听 iouring 的事件是否到来
3. iouring 提交的 action 可以是 epoll 的动作

简而言之，注册，唤醒，执行总结如下:

```txt
  关键回调函数总结
   io_async_queue_proc   poll 注册时      进程上下文    将请求加入文件的 waitqueue
   io_poll_wake          事件到来时       中断/软中断   快速检查，调度 task_work
   io_poll_task_func     task_work 执行   进程上下文    真正处理事件，完成请求
```

提交，告诉具体的子系统，如果 ready 了，到时候告诉我:
```txt
@[
        io_async_queue_proc+5
        datagram_poll_queue+51
        udp_poll+24
        sock_poll+84
        __io_arm_poll_handler+194
        io_arm_apoll+510
        io_queue_async+106
        io_handle_tw_list+41
        tctx_task_work_run+86
        tctx_task_work+58
        task_work_run+93
        io_run_task_work+74
        io_cqring_wait+910
        __do_sys_io_uring_enter+306
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 997
```

中断到来，提交任务:
```txt
@[
        io_poll_wake+5
        __wake_up_common+114
        __wake_up_sync_key+67
        sock_def_readable+66
        __udp_enqueue_schedule_skb+615
        udp_queue_rcv_one_skb+749
        udp_unicast_rcv_skb+116
        __udp4_lib_rcv+1651
        ip_protocol_deliver_rcu+205
        ip_local_deliver_finish+133
        __netif_receive_skb_one_core+133
        process_backlog+135
        __napi_poll+49
        net_rx_action+752
        handle_softirqs+240
        do_softirq.part.0+59
        __local_bh_enable_ip+96
        __dev_queue_xmit+1041
        ip_finish_output2+587
        ip_output+99
        ip_send_skb+152
        udp_send_skb+423
        udp_sendmsg+2646
        sock_write_iter+385
        vfs_write+964
        ksys_write+191
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 999
```

执行 callback ，任务完成:
```txt
@[
        io_poll_task_func+5
        io_handle_tw_list+41
        tctx_task_work_run+86
        tctx_task_work+58
        task_work_run+93
        io_run_task_work+74
        io_cqring_wait+910
        __do_sys_io_uring_enter+306
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 4095
```

```c
static void __io_poll_execute(struct io_kiocb *req, int mask)
{
	unsigned flags = 0;

	io_req_set_res(req, mask, 0);
	req->io_task_work.func = io_poll_task_func;

	trace_io_uring_task_add(req, mask);

	if (!(req->flags & REQ_F_POLL_NO_LAZY))
		flags = IOU_F_TWQ_LAZY_WAKE;
	__io_req_task_work_add(req, flags);
}
```

## io_arm_poll_handler
<!-- 93bd81ab-1ec4-4a77-97cb-87a0f23e32c6 -->

io_submit_sqes 如何进入到 io_queue_async 中的
```c
static void io_queue_async(struct io_kiocb *req, unsigned int issue_flags, int ret)
	__must_hold(&req->ctx->uring_lock)
{
	if (ret != -EAGAIN || (req->flags & REQ_F_NOWAIT)) {
fail:
		io_req_defer_failed(req, ret);
		return;
	}

	ret = io_req_sqe_copy(req, issue_flags);
	if (unlikely(ret))
		goto fail;

	switch (io_arm_poll_handler(req, 0)) {
	case IO_APOLL_READY:
		io_req_task_queue(req);
		break;
	case IO_APOLL_ABORTED:
		io_queue_iowq(req);
		break;
	case IO_APOLL_OK:
		break;
	}
}
```
简单清晰的三个事情:
1. 最优路径：IO_APOLL_OK - 事件驱动，无需线程，资源就绪时自动唤醒
2. 快速路径：IO_APOLL_READY - 发现资源已就绪，立即调度任务执行
3. 降级路径：IO_APOLL_ABORTED - 退化成传统线程池模型（io-wq）阻塞等待

```c
static inline void io_queue_sqe(struct io_kiocb *req, unsigned int extra_flags)
	__must_hold(&req->ctx->uring_lock)
{
	unsigned int issue_flags = IO_URING_F_NONBLOCK |
				   IO_URING_F_COMPLETE_DEFER | extra_flags;
	int ret;

	ret = io_issue_sqe(req, issue_flags);

	/*
	 * We async punt it if the file wasn't marked NOWAIT, or if the file
	 * doesn't support non-blocking read/write attempts
	 */
	if (unlikely(ret))
		io_queue_async(req, issue_flags, ret);
}
```

原来还是相同的函数来重试啊，这不就是 50% 的原因吗，好像不是的，
原来是看命中率的，还是有很多发 write 来自于 worker 的，
还是是一个工作启动一个 worker 吗?
```txt
@[
        io_write+5
        __io_issue_sqe+59
        io_issue_sqe+55
        io_wq_submit_work+184
        io_worker_handle_work+327
        io_wq_worker+214
        ret_from_fork+244
        ret_from_fork_asm+26
]: 393 2676
@[
        io_write+5
        __io_issue_sqe+59
        io_issue_sqe+55
        io_submit_sqes+274
        __do_sys_io_uring_enter+516
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 3253 4350
```

## Missing Manuals - io_uring worker pool
<!-- 851aadb7-2385-4523-b0e4-e4a08782884c -->

https://blog.cloudflare.com/missing-manuals-io_uring-worker-pool/

其实讲的事情很简单，而且关于 io wq 过时了:

> io-wq divides work into two categories:
>
> 1) Work that completes in a bounded time, like reading from a regular file
>    or a block device. This type of work is limited based on the size of
>    the SQ ring.
>
> 2) Work that may never complete, we call this unbounded work. The amount
>    of workers here is just limited by RLIMIT_NPROC.
>
> https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=2e480058ddc21ec53a10e8b41623e245e908bdbc

IOSQE_ASYNC
```txt
IOSQE_ASYNC

Normal operation for io_uring is to try and issue an sqe as non-blocking first, and if that fails, execute it in an async manner.

To support more efficient overlapped operation of requests that the application knows/assumes will always (or most of the time) block, the application can ask for an sqe to be issued async from the start. Available since 5.6.
```

```txt
sudo perf stat -e io_uring:io_uring_submit_req -- timeout 1 ./udp-read

 Performance counter stats for 'timeout 1 ./udp-read':
             4,096      io_uring:io_uring_submit_req
```

```txt
./udp-read & p=$!; sleep 1; ps -o thcount $p; kill $p; wait $p
[1] 599794
THCNT
    1
[1]  + 599794 terminated  ./udp-read
```

也就是每一次都会调用到这个:
```txt
sudo bpftrace -e 't:io_uring:io_uring_poll_arm { @[probe, args->opcode] = count(); } i:s:1 { exit(); }' -c ./udp-read

@[tracepoint:io_uring:io_uring_poll_arm, 22]: 4096
```

6.18.9-100.fc42.x86_64 中测试，现在就只有两个 thread 了
```txt
./udp-read --async & pid=$!; sleep 1; ps -o pid,thcount $pid; kill $pid; wait $pid
    PID THCNT
 602994     2
```

这个结果也是不同的:
```txt
🤒  sudo perf stat -a -e io_uring:io_uring_poll_arm,io_uring:io_uring_queue_async_work -- ./udp-read --async
^C./udp-read: Interrupt

 Performance counter stats for 'system wide':

             4,096      io_uring:io_uring_poll_arm
             4,096      io_uring:io_uring_queue_async_work
```

```txt
 sudo bpftrace -e 'tracepoint:io_uring:io_uring_poll_arm { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attached 2 probes
^C

@[
        io_arm_apoll+604
        io_wq_submit_work+594
        io_worker_handle_work+327
        io_wq_worker+214
        ret_from_fork+244
        ret_from_fork_asm+26
]: 4096
```

## vfs_poll
<!-- b2b82925-7d1a-40b0-b8ae-d23b68a69c05 -->

1. 第一步:

使用 vfs_poll 的场景
```txt
   子系统                          使用方式
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   select()/poll() 系统调用        轮询多个文件描述符
   epoll                           边沿触发和水平触发事件通知
   aio (IOCB_CMD_POLL)             异步 I/O 的 poll 支持
   io_uring (IORING_OP_POLL_ADD)   io_uring 的 poll 操作
   KVM irqfd/eventfd               中断注入机制
   vhost                           virtio 设备轮询
```

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_epoll_pwait
        - __se_sys_epoll_pwait
          - __do_sys_epoll_pwait
            - do_epoll_pwait
              - do_epoll_pwait
                - do_epoll_wait
                  - ep_poll
                    - ep_try_send_events
                      - ep_send_events
                        - ep_item_poll
                          - vfs_poll

2. 第二步，到来 vfs_poll 中

是内核中统一的事件通知接口：
- 查询：立即返回文件当前的事件状态
- 订阅：通过 poll_table 注册回调，实现异步事件通知

vfs_poll 是一个很简单的函数，其实各个子系统都会
```c
static inline __poll_t vfs_poll(struct file *file, struct poll_table_struct *pt)
{
	if (unlikely(!file->f_op->poll))
		return DEFAULT_POLLMASK;
	return file->f_op->poll(file, pt);
}
```
这里的 poll 注册者就很多了，例如 io_uring_poll


3. 到具体的案例中，例如我们写的 code/src/m/epoll.c 中

一般会调用 poll_wait ，其中调用各个 epoll 机制注册 callback，例如:
- aio_poll_queue_proc
- io_poll_add / io_poll_queue_proc：为用户态显式的 POLL_ADD 操作服务
- io_arm_apoll / io_async_queue_proc：为内核自动优化的 异步 poll 机制服务（让 read/write 等操作可以先尝试 poll，避免阻塞）
- ep_ptable_queue_proc
- kvm_irqfd_register

例如在 ep_ptable_queue_proc 中就注册了，当 amsg 就绪，来唤醒的时候该如何处理:
```txt
	init_waitqueue_func_entry(&pwq->wait, ep_poll_callback);
```

```txt
@[
        ep_poll_callback+5
        __wake_up_common+114
        __wake_up_sync_key+67
        sock_def_readable+66
        __udp_enqueue_schedule_skb+615
        udp_queue_rcv_one_skb+749
        udp_unicast_rcv_skb+116
        __udp4_lib_rcv+1651
        ip_protocol_deliver_rcu+205
        ip_local_deliver_finish+133
        ip_list_rcv_finish+701
        ip_list_rcv+293
        __netif_receive_skb_list_core+655
        netif_receive_skb_list_internal+462
        napi_complete_done+130
        iwl_pcie_napi_poll_msix+195
        __napi_poll+49
        net_rx_action+752
        handle_softirqs+240
        do_softirq.part.0+59
        __local_bh_enable_ip+96
        iwl_pcie_irq_rx_msix_handler+209
        irq_thread_fn+37
        irq_thread+246
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 11
```

类似的 iouring 中:
```c
static void io_init_poll_iocb(struct io_poll *poll, __poll_t events)
{
	poll->head = NULL;
#define IO_POLL_UNMASK	(EPOLLERR|EPOLLHUP|EPOLLNVAL|EPOLLRDHUP)
	/* mask in events that we always want/need */
	poll->events = events | IO_POLL_UNMASK;
	INIT_LIST_HEAD(&poll->wait.entry);
	init_waitqueue_func_entry(&poll->wait, io_poll_wake);
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
