## 安全问题
- https://news.ycombinator.com/item?id=30605356


## TODO

### iouring 可以一统超越 sendfile 吗?

examples/send-zerocopy.c

### docs/kernel/iouring/iouring-misc.md

### docs/kernel/iouring/iouring-signal.md

### wip-epoll.md

### io_uring_buf_ring_advance 做什么用的?


io_tee io_splice 和理解下

io_uring/timeout.c 中的代码测试下

## Welcome to Lord of the io_uring
https://news.ycombinator.com/item?id=23132549

```c
struct io_kiocb {

	u8				opcode;
```

- 这个流程很平常啊!

- io_issue_sqe => io_issue_def::issue
  - io_iter_do_read
    - call_read_iter
      - file->f_op->read_iter(kio, iter)

## 三种基本模式?
https://blogs.oracle.com/linux/post/an-introduction-to-the-io-uring-asynchronous-io-framework

1. interrupt driven
2. Polled
3. Kernel polled

## 还是先会使用再说吧 liburing/examples/

io_uring-udp.c 不会用

send-zerocopy.c

ucontext-cp.c

### io_uring-close-test.c
- io_uring_queue_init
- io_uring_register_ring_fd
- io_uring_close_ring_fd : 为什么要立刻关闭这个 fd 啊

## io_uring/timeout.c 和 block layer 的 timeout 如何理解?

## 为什么 iouring 的 CPU 消耗比 aio 高那么多?
```txt
[global]
time_based
runtime=1000
ioengine=io_uring
# ioengine=libaio
# ioengine=sync
iodepth=10000
direct=1
bs=4k

[trash]
rw=randread
filename=/dev/nvme0n1
```
都是 440k 的速度，这是盘的极限，aio 45 的 wait ，
但是 uring 所有的 cpu 占满

当然，如果换成 /dev/nullb0 ，iouring 可以实现 1800k ，而 aio 只有 1400 k 的性能。


## 为什么 io_uring 在 page cache 上是表现正常的

遇到了一个非常奇怪的问题:

```sh
	fio --name=fj --ioengine=libaio --rw=randread --bs=4k --numjobs=2 --iodepth=64 \
		--filename=/dev/sdc
```

cat /sys/block/sdd/inflight 发现只有 read 是 1 或者 2 ，其 iops 大约为 100k

但是如果修改为 write ，那么

```sh
	fio --name=fj --ioengine=libaio --rw=randwrite --bs=4k --numjobs=2 --iodepth=64 \
		--filename=/dev/sdc
```

那么 write 还是可以到 128 的，其 iops 大约为 400k

因为这个是一个失误，导致 libaio 和 buffer io 放到一起了:

猜测过程是这样的:
libaio 的 buffer read 需要在 io submit 的时候就让结果返回，如果这个结果没有命中 cache ，
那么就是一个同步的。而 buffer write 只会被当时的 dirty rate 所限制。

到时候验证下。

## 为什么 cqe 应该比 sqe 多?
https://www.bluepuni.com/archives/io-uring-low-level-programming/

## 需要理解下 Level poll supported
io uring 支持么？

# 从如下的角度分析

## epoll

### TIF_NOTIFY_SIGNAL
从 schduler 的角度分析下

## 队列 (dpdk / virito / io uring ) 的设计

## kernel by pass
- io_uring 真的可以拯救内核吗?

## 资源
- https://man.archlinux.org/man/io_uring.7.en

## 如何理解这两个 def 的区别

- io_cold_defs
- io_issue_defs

## io uring + nvme 终极之路
- https://www.usenix.org/system/files/fast24-joshi.pdf


## 基本的流程

```txt
@[
        io_prep_readv+5
        io_submit_sqes+219
        __do_sys_io_uring_enter+530
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 32
```

```txt
@[
        io_read+5
        io_issue_sqe+84
        io_wq_submit_work+196
        io_worker_handle_work+204
        io_wq_worker+218
        ret_from_fork+49
        ret_from_fork_asm+26
]: 32
@[
        io_read+5
        io_issue_sqe+84
        io_submit_sqes+274
        __do_sys_io_uring_enter+530
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 32

```

观察 io_read ，可以看到:
```txt
#define	EAGAIN		11	/* Try again */
#define	EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
```


```txt
  4)               |  __io_read() {
  4)               |    arch_irq_work_raise() {
  4) + 23.885 us   |      hv_send_ipi_self();
  4) + 18.445 us   |      apic_mem_wait_icr_idle();
  4) + 43.472 us   |    }
  4)               |    io_rw_init_file() {
  4)   0.130 us    |      io_file_get_flags();
  4)   0.971 us    |    }
  4)   0.290 us    |    io_file_supports_nowait();
  4)               |    rw_verify_area() {
  4)   0.521 us    |      security_file_permission();
  4)   0.851 us    |    }
  4)               |    xfs_file_read_iter [xfs]() {
  4) + 31.500 us   |      xfs_file_dio_read [xfs]();
  4) + 39.775 us   |    }
  4) + 91.073 us   |  }
```

## 为什么有 register 这个功能?
- [ ] 想不到还有进化: IORING_FILE_INDEX_ALLOC

## 需要记住，cqe 和 sqe 是两个队列，那么 sqe

所以，sqe 提交之后，不需要完成，只是需要 sqe 被内核取走了，
那么这个 sqe 就是 available 的状态。所以，liburing 默认
设置的 sqe 比 sqe 的数量多。

## 水平和边沿触发
似乎这个没有理解清楚，那岂不是 io uring 放弃了 level 触发模式

那么 epoll 什么时候是需要使用 io uring 的 level 模式的

还是需要测下，如果 io uring 来监听 uffd ，发送多个消息，会产生多个 cqe 么?

## 看看 get_signal() 的调用者，是不是超出想象了

居然有 io uring 的东西


## 这是在唤醒谁啊?
```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_io_uring_enter
        - __se_sys_io_uring_enter
          - __do_sys_io_uring_enter
            - io_submit_sqes
              - io_submit_state_end
                - io_submit_flush_completions
                  - __io_submit_flush_completions
                    - __io_cq_unlock_post
                      - io_cqring_wake
```

## 有趣的问题
```txt
Is it possible to open a directory with io_uring? It didn't look like it from the lib.
Someone told me they used the getdents64 syscall to do it without glibc,
but I don't see anything like that available in io_uring
```


## 看看这个
https://mp.weixin.qq.com/s/Vjxvk7EGCZFTPckZdjzU6w

## 关于
1. io uring 也支持 eventfd

https://unixism.net/loti/tutorial/register_eventfd.html : io_uring 可以注册 eventfd ，
从而每次 io_uring 的操作完成之后，eventfd 都可以收到消息，
而另一个 thread 调用 eventfd_read 的线程可以进入到下一步

https://kernel.dk/io_uring-whatsnew.pdf

> eventfd. io_uring now also supports eventfd notifications on the ring itself, for applications that want to use eventfd for
> notification of completion events.

似乎没有什么特殊的原因，只是支持而已


https://mp.weixin.qq.com/s/Avffs3SZlKB-9vQBA6qjaw

## 高级话题了
https://mp.weixin.qq.com/s/49f3fsGxjF4HRoFYnbda9w

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
