## refcount

测试在: /home/martins3/data/vn/code/src/m/percpu_ref.c

### percpu refcount
<!-- 16f6ea52-e777-4979-9410-fa31f58093c9 -->

[Per-CPU reference counts](https://lwn.net/Articles/557478/)

首先使用 percpu_ref_init 来初始化，用 percpu_ref_get 和 percpu_ref_put 来控制

This problem is avoided with a simple observation:
as long as the initial reference is held, the count cannot be zero, so percpu_ref_put() does not bother to check.

The implication is that the thread which calls percpu_ref_init() must indicate when it is dropping its reference;
that is done with a call to:
```c
void percpu_ref_kill(struct percpu_ref *ref);
```
After this call, the reference count degrades to the usual model with a single shared atomic_t counter;
that counter will be decremented and checked whenever a reference is released.

> [!NOTE]
> 参考 Deepseeek ，有待验证

request_queue 是一个完美的例子，因为系统中的任何进程在任何 CPU 上都可能提交 I/O，导致对 request_queue 的引用获取和释放操作非常频繁。

q_usage_counter 就是一个 Per-CPU 引用计数器。我们来看看它的工作流程：
1. 获取队列引用 (blk_queue_enter)
  当一个线程准备向这个队列提交 bio 时，它会调用 percpu_ref_get() 来增加引用计数。
  这个操作实际上只是在当前 CPU 的本地计数器上执行了一个 ++ 操作。这非常快，没有锁，也没有缓存行弹跳。
2. 释放队列引用 (blk_queue_exit)
  当 I/O 完成或线程不再需要访问队列时，它会调用 percpu_ref_put() 来减少引用计数。
  这个操作也只是在当前 CPU 的本地计数器上执行了一个 -- 操作。
3. 最后的释放（销毁队列）
  这是最关键的部分。当一个线程调用 percpu_ref_put() 并且发现它自己这个 CPU 的本地计数器变成了 0 时，事情就变得特殊了。
  这个线程不能立即销毁 request_queue，因为它不知道其他 CPU 上的本地计数器是不是也都是 0。
  此时，它会进入一个“慢速路径”(slow path)。在这个路径中，它会去汇总所有 CPU 的本地计数器。
  如果所有 Per-CPU 计数器的总和确实为 0，那么这个线程就知道自己是最后一个使用者，可以安全地触发 request_queue 的销毁流程了。


### include/linux/percpu-refcount.h 最上面也有一段详细的分析

继续分析一下其中的 rcu 讲解吧

## https://docs.kernel.org/RCU/rcuref.html

### include/linux/refcount.h 最上面一段分析

## 实际上，kernel 中的 refcount 可以作为一个单独的分析话题

如何保证 refcount 不重不漏

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
