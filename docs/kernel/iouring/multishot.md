## iouring multishot
<!-- 4a9d4de4-f7b1-4d84-9173-69a8dd034bd6 -->


普通 io_uring 请求：
1. 一次 SQE → 一次 CQE → 请求结束
multishot 请求：
2. 一次 SQE → 多次 CQE → 直到内核主动结束或用户取消


内核在请求仍然“有效”时，会在每次事件发生时向 CQ ring 投递一个新的 CQE。

https://man7.org/linux/man-pages/man3/io_uring_prep_read_multishot.3.html

https://zhuanlan.zhihu.com/p/580417741

> 使用 io_uring 做 polling 与 epoll、poll 的默认模式有一个很大的区别就是 io_uring 的 polling 始终工作在 one-shot 模式下
（等同于 epoll 的 EPOLLONESHOT）， 即一旦某个 poll 操作完成，用户必须重新提交 poll 请求否则不会触发新的事件，
这样保证每个 poll 请求有且只有一个响应。然后既然是 one-shot 模式，也就没有类似 epoll 中的 LT、ET 模式之分


- IORING_RECV_MULTISHOT
- IORING_SEND_MULTISHOT
- IORING_ACCEPT_MULTISHOT

```c
void io_uring_prep_multishot_accept(struct io_uring_sqe *sqe, int sockfd, struct sockaddr *addr,
                                    socklen_t *addrlen, int flags);
void io_uring_prep_multishot_accept_direct(struct io_uring_sqe *sqe, int sockfd, struct sockaddr *addr,
                                           socklen_t *addrlen, int flags);
void io_uring_prep_poll_multishot(struct io_uring_sqe *sqe, int fd, unsigned poll_mask);
```

## io_uring_prep_poll_multishot
liburing 中的 test/poll.c

最符合我的心意的是:
code/src/c/iouring/multishot-poll.c

## 2025-10-02 没进入主线
https://www.phoronix.com/news/io-uring-multishot-provided-buf

哦，对于 IORING_URING_CMD_MULTISHOT 也可以使用


##  很好的
https://www.reddit.com/r/programming/comments/1mqfp7c/from_epoll_to_io_urings_multishot_receives_why/
https://codemia.io/blog/path/From-epoll-to-iourings-Multishot-Receives--Why-2025-Is-the-Year-We-Finally-Kill-the-Event-Loop

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
