# lock
## 仔细 review 其中的同步机制

refcount 之类的

1. 先思考一个简单问题，如果多个 thread 同时
执行 io_uring_submit ，哪里来保护 queue 的访问?

这个 enter 是把 sqe 拷贝过去么? 显然不是，是
io_uring_get_sqe 直接提供 queue 的空间。

所以，如果多个 io_uring_get_sqe 在多线程中被同时调用，会有问题吗?

这个人说，是不适合的:
https://nick-black.com/dankwiki/index.php/Io_uring

> Multithreaded use
> urings (and especially the struct io_uring object of liburing) are not intended for multithreaded use (quoth Axboe, "don't share a ring between threads"), though they can be used in several threaded paradigms. A single thread submitting and some different single thread reaping is definitely supported. Descriptors can be sent among rings with IORING_OP_MSG_RING. Multiple submitters definitely must be serialized in userspace.
>
> If an op will be completed via a kernel task, the thread that submitted that SQE must remain alive until the op's completion. It will otherwise error out with -ECANCELED. If you must submit the SQE from a thread which will die, consider creating it disabled (see IORING_SETUP_R_DISABLED), and enabling it from the thread which will reap the completion event using IORING_REGISTER_ENABLE_RINGS with io_uring_register(2).
>
> If you can restrict all submissions (and creation/enabling of the uring) to a single thread, use IORING_SETUP_SINGLE_ISSUER to enable kernel optimizations. Otherwise, consider using io_uring_register_ring_fd(3) (or io_uring_register(2) directly) to register the ring descriptor with the ring itself, and thus reduce the overhead of io_uring_enter(2).
>
> When IORING_SETUP_SQPOLL is used, the kernel poller thread is considered to have performed the submission, providing another possible way around this problem.
>
> I am aware of no unannoying way to share some elements between threads in a single uring, while also monitoring distinct urings for each thread. I suppose you could poll on both.

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
