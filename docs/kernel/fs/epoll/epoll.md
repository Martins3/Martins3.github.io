## epoll
- [ ] https://zhou-yuxin.github.io/articles/2017/%E7%AC%AC%E4%B8%80%E4%B8%AALinux%E9%A9%B1%E5%8A%A8%E7%A8%8B%E5%BA%8F%EF%BC%88%E4%B8%89%EF%BC%89%E2%80%94%E2%80%94aMsg%E7%9A%84%E9%9D%9E%E9%98%BB%E5%A1%9E%E5%BC%8FIO%E4%B9%8Bselect-poll/index.html

在内核中间实现一个支持的 poll 的模块

fs/eventfd.md

epoll_create
```c
       int epoll_create(int size); // 过时了，内核不需要动态分配，不需要 size 变量
       int epoll_create1(int flags);
```

```c
/*
 * Open an eventpoll file descriptor.
 */
static int do_epoll_create(int flags)

   * Creates all the items needed to setup an eventpoll file. That is,
   * a file structure and a free file descriptor.


/* File callbacks that implement the eventpoll file behaviour */
static const struct file_operations eventpoll_fops = {
#ifdef CONFIG_PROC_FS
  .show_fdinfo  = ep_show_fdinfo,
#endif
  .release  = ep_eventpoll_release,
  .poll   = ep_eventpoll_poll,
  .llseek   = noop_llseek,
};
```

- do_epoll_create
  - get_unused_fd_flags
  - anon_inode_getfile
  - fd_install

- do_epoll_ctl
  - ep_insert
    - ep_rbtree_insert : 插入到 eventpoll:rbr 这个结构体上
    - init_poll_funcptr(&epq.pt, ep_ptable_queue_proc);
  - ep_remove :
  - ep_modify


- do_epoll_wait
  - ep_poll
    - ep_events_available
      - `ep->rdllist`

> Because different file systems have different implementations, it is impossible to get the waiting queue directly through the struct file object, so we use the poll operation of struct file to return the waiting queue of the object in the way of callback.
The callback function set here is `ep_ptable_queue_proc`

1. 感觉 ep_ptable_queue_proc 是用于加入队列的时候初始化
2. ep_item_poll
  - `__ep_eventpoll_poll`
    - poll_wait
      - poll_table_struct:poll_queue_proc : 调用 callback 也就是 ep_ptable_queue_proc
3. ep_poll_callback : 是 wakeup 的时候，调用的 callback 函数


> 总结，从描述上看，似乎并没有什么神奇的不得了的事情，只是 epoll 和 eventfd , aio , io_uring 在异步机制上的区别是什么

还需要阅读 poll, ppoll, select, pselect 的代码吗 ? 没必要, 可以看看 [Evans 的 blog](https://jvns.ca/blog/2017/06/03/async-io-on-linux--select--poll--and-epoll/)

## 可以看看
- [epoll 原理](https://zhuanlan.zhihu.com/p/63179839)
- [Async IO on Linux: select, poll, and epoll](https://jvns.ca/blog/2017/06/03/async-io-on-linux--select--poll--and-epoll/)

## echosystem
https://github.com/libuv/libuv

## 几个系统调用的总结

epoll 相关：
- epoll_create1：创建 epoll 实例，返回一个 epoll 文件描述符，支持 EPOLL_CLOEXEC 标志。
- epoll_ctl：控制 epoll 实例，添加、修改或删除文件描述符的监控事件。
- epoll_wait：等待 epoll 实例上注册的事件发生，返回就绪的文件描述符。
- epoll_pwait：类似 epoll_wait，但允许指定信号掩码以处理信号中断（类似于 ppoll）。

poll 相关：
- poll：监控多个文件描述符的 I/O 事件，支持超时机制。
- ppoll：poll 的增强版，支持更高精度的时间（纳秒级）以及信号掩码。

select：
- select：传统的 I/O 多路复用系统调用，监控多个文件描述符的读、写、异常事件，限制较多（如文件描述符数量上限）

似乎 epoll 是最好的，为什么 qemu 不去使用 epoll ，似乎都是历史因素了。

## 问题
1. 观察下各个 file_operations::poll 的实现方法?
2. poll_wait

```c
static inline void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
{
	if (p && p->_qproc && wait_address)
		p->_qproc(filp, wait_address, p); // 相对于 file_operations::poll 注册数量来说，似乎大多数都是注册的 __pollwait 的
}

void poll_initwait(struct poll_wqueues *pwq)
{
	init_poll_funcptr(&pwq->pt, __pollwait);
	pwq->polling_task = current;
	pwq->triggered = 0;
	pwq->error = 0;
	pwq->table = NULL;
	pwq->inline_index = 0;
}
```

## 思考一个问题
网络和磁盘似乎有一个很大的不同的:
1. 网络的 epoll ready 表示，你现在可以去调用 recv 了，但是 recv 本身可能卡主
2. 存储的 ready 表示，提交的 io 已经落盘了。

4. 一个 epoll 可以同时监听很多 fd ，fd 的 poll 函数和 _qproc 都不同 ，poll 的时候加入到 wait queue 中。write / read fd 的时候，
将会调用 wait queue 的，然后把 ... 唤醒，看看 __pollwait ，这里又是对于 waitqueue 深入的分析。

5. epoll 和 select 的区别?
内核中，他们会复用一个代码吧

## 似乎需要更加深入的理解
https://github.com/libevent/libevent
https://www.ulduzsoft.com/2014/01/select-poll-epoll-practical-difference-for-system-architects/

## windows 的 epoll
https://github.com/piscisaureus/wepoll

## 看看

https://despairlabs.com/blog/posts/2021-06-16-io-uring-is-not-an-event-system/
https://news.ycombinator.com/item?id=27540248

## [io_uring is not an event system](https://news.ycombinator.com/item?id=27540248)

看了半天，一会搞出什么?  都是类似这种:
> epoll is based on a "readiness" model (i.e. it tells when when you can start I/O).
io_uring is based on a "completion" model (i.e. it tells you when I/O is finished).

相比 aio，似乎只是少了一次 getevent 那个系统调用而已。

## lwn 的文章
https://lwn.net/Kernel/Index/#Epoll

## epoll 的基本使用
<!-- a5b5a17e-95bc-4c0b-96ac-5bfe519c92fb -->

调试代码在 vn/code/src/c/fs/epoll.c 中

就是 epoll_ctl 的使用搞清楚就可以了:
https://man7.org/linux/man-pages/man2/epoll_ctl.2.html

edge 和 level 的差别也很简单:
https://stackoverflow.com/questions/9162712/what-is-the-purpose-of-epolls-edge-triggered-option

## 这里有一个小故事
nvim: epoll_ctl support in io_uring is deprecated and will be removed in a future Linux kernel version.

```txt
commit 4ea0bf4b98d66a7a790abb285539f395596bae92
Author: Ben Noordhuis <info@bnoordhuis.nl>
Date:   Sat May 6 11:55:02 2023 +0200

    io_uring: undeprecate epoll_ctl support

    Libuv recently started using it so there is at least one consumer now.

    Cc: stable@vger.kernel.org
    Fixes: 61a2732af4b0 ("io_uring: deprecate epoll_ctl support")
    Link: https://github.com/libuv/libuv/pull/3979
    Signed-off-by: Ben Noordhuis <info@bnoordhuis.nl>
    Link: https://lore.kernel.org/r/20230506095502.13401-1-info@bnoordhuis.nl
    Signed-off-by: Jens Axboe <axboe@kernel.dk>

commit 61a2732af4b0337f7e36093612c846e9f5962965
Author: Jens Axboe <axboe@kernel.dk>
Date:   Wed Jun 1 12:36:42 2022 -0600

    io_uring: deprecate epoll_ctl support

    As far as we know, nobody ever adopted the epoll_ctl management via
    io_uring. Deprecate it now with a warning, and plan on removing it in
    a later kernel version. When we do remove it, we can revert the following
    commits as well:
```

## 最后看看 sysfs_notify 的效果
kernfs_notify 似乎是更加底层的接口

## set nonblocking

epoll 需要配置为 non blocking ，不然就会让主循环卡死:
```c
uffd_fd = uffd_create_fd(UFFD_FEATURE_EVENT_REMOVE, true);
```

```c
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```
O_NONBLOCK 和 O_ASYNC 什么关系，参考 7c0a271b-42ee-4acf-b6d0-4092435480f1
简而言之，O_ASYNC 不需要管。

O_NONBLOCK 和 block 的关系? 无影响，除了特殊的文件，例如 fifo
https://man7.org/linux/man-pages/man2/open.2.html 中

> Note that the setting of this flag has no effect on the
> operation of poll(2), select(2), epoll(7), and similar,
> since those interfaces merely inform the caller about
> whether a file descriptor is "ready", meaning that an I/O
> operation performed on the file descriptor with the
> O_NONBLOCK flag clear would not block.
>
> Note that this flag has no effect for regular files and
> block devices; that is, I/O operations will (briefly) block
> when device activity is required, regardless of whether
> O_NONBLOCK is set.  Since O_NONBLOCK semantics might
> eventually be implemented, applications should not depend
> upon blocking behavior when specifying this flag for
> regular files and block devices.
>
> For the handling of FIFOs (named pipes), see also fifo(7).
> For a discussion of the effect of O_NONBLOCK in conjunction
> with mandatory file locks and with file leases, see
> fcntl(2).

## 再仔细的读读这个东西吧
- https://man7.org/linux/man-pages/man7/epoll.7.html

## 思考一个问题
如果使用的 level 的模式，如果 read 了所有的内容，
也触发了 EAGAIN ，但是在再次 epoll 之前，数据到了，
如何保证一定触发事件？

## EPOLLHUP EPOLLRDHUP EPOLLERR 有什么区别

https://man7.org/linux/man-pages/man2/epoll_ctl.2.html

果然，文档是看不懂的

```txt
       EPOLLRDHUP (since Linux 2.6.17)
              Stream socket peer closed connection, or shut down writing
              half of connection.  (This flag is especially useful for
              writing simple code to detect peer shutdown when using
              edge-triggered monitoring.)

       EPOLLPRI
              There is an exceptional condition on the file descriptor.
              See the discussion of POLLPRI in poll(2).

       EPOLLERR
              Error condition happened on the associated file descriptor.
              This event is also reported for the write end of a pipe
              when the read end has been closed.

              epoll_wait(2) will always report for this event; it is not
              necessary to set it in events when calling epoll_ctl().

       EPOLLHUP
              Hang up happened on the associated file descriptor.

              epoll_wait(2) will always wait for this event; it is not
              necessary to set it in events when calling epoll_ctl().

              Note that when reading from a channel such as a pipe or a
              stream socket, this event merely indicates that the peer
              closed its end of the channel.  Subsequent reads from the
              channel will return 0 (end of file) only after all
              outstanding data in the channel has been consumed.
```

有时间看看内核源码吧，看看这些 flags 的区别是什么?

## epoll 中的 edge 和 level
<!-- 8cbf6403-7926-4401-9b5c-e1fdc9152168 -->

1. 关键点是：“读到 EAGAIN” 和 “新数据到达” 之间如果发生竞争，内核会保证不会丢 edge。典型循环是：
```c
  for (;;) {
      n = read(fd, buf, sizeof(buf));
      if (n > 0) {
          handle(buf, n);
          continue;
      }

      if (n < 0 && errno == EAGAIN) {
          break;
	  // 回 epoll_wait ，如果这个时候到来数据了，内核如何保证，消息可以继续触发的
	  // 没有看具体的实现，但是猜测如下，用网络作为例子，假如当前是一个 queue buffer
	  // read 在访问的时候 lock ，然后标记最后放到了那里，后续看有无新的
      }

      // EOF or real error
      close(fd);
      break;
  }
```

## EPOLLOUT 和 EPOLLIN 分别描述的是什么
<!-- 6c54c148-a276-42a9-bd38-489442160a20 -->

哦，linnbd 真的让我感觉到奇怪哦:

- EPOLLIN	: 可读，也就是发送请求，然后请求返回，并非要求一定是读操作的。
- EPOLLOUT	: 可写

对于一个网络

为什么在 close fd 的时候，可以接受到 EPOLLOUT ，也就是说，似乎就是需要 libnbd 的方向自动转换的模式才可以

## 为什么 epoll 必须配合 nonblocking 来使用
<!-- 9fd7f1b8-bd42-4165-9172-e4c88cbb9d56 -->

```c
int setnonblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) ==
	    -1) {
		return -1;
	}
	return 0;
}
```

epoll 配合 O_NONBLOCK 不是语法强制，而是工程上的必需。核心原因在于：epoll 的职责是"通知就绪"，
但真正的 I/O 操作（read/write/accept）仍由你自己执行。如果 socket 是阻塞的，epoll 返回后你仍可能在 I/O 调用上卡住。

具体分两种情况：
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
1. 水平触发 (LT, Level-Triggered) — 默认模式
LT 模式下，只要 fd 处于可读/可写状态，epoll 就会持续通知。
假设 socket 是阻塞的：
- epoll_wait 返回，通知 fd 可读
- 你调用 read(fd, buf, 1024)
- 内核缓冲区恰好只有 100 字节
- read 阻塞，等待剩余 924 字节

结果：整个线程挂在这个 fd 上，epoll 同时监控的其他成百上千个 fd 全部得不到处理，多路复用的意义被完全抵消。

2. 边缘触发 (ET, Edge-Triggered) — 高性能首选
ET 模式下，epoll 只在状态变化时通知一次。你必须在收到通知后一次性把数据全部处理完（循环 read 直到返回 -1 且 errno == EAGAIN）。
如果 socket 是阻塞的：
- 循环 read
- 数据读完后，read 阻塞而不是返回 EAGAIN
- 线程永久死锁在这个 fd 上

所以 ET 模式强制要求 non-blocking。
总结

 模式   是否必须 non-blocking            原因
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 LT     强烈建议，否则失去多路复用意义   避免在 read/write 上阻塞，导致其他 fd 饥饿
 ET     必须                             需要循环读到 EAGAIN，blocking 会死锁

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
