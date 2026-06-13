# rwlock
## rw spin lock 是 spin 的吗?

## 需要考虑 interrupt 吗?

## read 和 write 的时候的 interrupt 不同吧

## rw spin lock
Documentation/locking/spinlocks.rst 虽然说最后还是别用，
但是实际上，内核有使用 rw spin lokc 的

read_lock_irqsave

不对，rw_lock 没有进行 spin 吧

```txt
History:        #0
Commit:         a218cc4914209ac14476cb32769b31a556355b22
Author:         Roman Penyaev <rpenyaev@suse.de>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    2019年03月08日 星期五 08时28分53秒
Committer Date: 2019年03月08日 星期五 10时32分01秒

epoll: use rwlock in order to reduce ep_poll_callback() contention

The goal of this patch is to reduce contention of ep_poll_callback()
which can be called concurrently from different CPUs in case of high
events rates and many fds per epoll.  Problem can be very well
reproduced by generating events (write to pipe or eventfd) from many
threads, while consumer thread does polling.  In other words this patch
increases the bandwidth of events which can be delivered from sources to
the poller by adding poll items in a lockless way to the list.

The main change is in replacement of the spinlock with a rwlock, which
is taken on read in ep_poll_callback(), and then by adding poll items to
the tail of the list using xchg atomic instruction.  Write lock is taken
everywhere else in order to stop list modifications and guarantee that
list updates are fully completed (I assume that write side of a rwlock
does not starve, it seems qrwlock implementation has these guarantees).

The following are some microbenchmark results based on the test [1]
which starts threads which generate N events each.  The test ends when
all events are successfully fetched by the poller thread:

 spinlock
 ========

 threads  events/ms  run-time ms
       8       6402        12495
      16       7045        22709
      32       7395        43268

 rwlock + xchg
 =============

 threads  events/ms  run-time ms
       8      10038         7969
      16      12178        13138
      32      13223        24199

According to the results bandwidth of delivered events is significantly
increased, thus execution time is reduced.

This patch was tested with different sort of microbenchmarks and
artificial delays (e.g.  "udelay(get_random_int() & 0xff)") introduced
in kernel on paths where items are added to lists.

[1] https://github.com/rouming/test-tools/blob/master/stress-epoll.c
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
