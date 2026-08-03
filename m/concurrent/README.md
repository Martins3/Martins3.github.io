## 按照 chapter 4 中
还是需要将 posix 的接口都先测试一次，当然也需要阅读一下
- 性能测试
- 和功能测试

## completion 和 wait event 有区别吗?

还有 swait 和 waitbit
https://kernelnewbies.kernelnewbies.narkive.com/lLxcBrgc/wait-event-interruptible-vs-wait-for-completion-interruptible

## 使用 RCU 测试 memory barrier 吧

## 应该测试下所谓的 lockless 算法的性能

https://lwn.net/Articles/844224/

## [ ] 给 qemu 增加代码来测试 qemu 用户态的 RCU 才可以

## 测试下这种工具的使用

```txt
__must_hold(mas->tree->ma_lock)
```

## 测试 call_rcu_hurry

## memory-barrier
- 将 memory-barrier 的每一种场景都列举出来

## 使用环形队列来测试 barrier 的效果

为什么环形队列需要使用 barrier 来着

## 两个文档
- Documentation/virt/kvm/locking.rst
- https://docs.kernel.org/filesystems/locking.html
  - https://mp.weixin.qq.com/s/xm6WIB69VlcZAiZRUdkljA
- Documentation/filesystems/directory-locking.rst

## destroy_rcu_head 做啥的

scsi_end_request

```txt
	/*
	 * Calling rcu_barrier() is not necessary here because the
	 * SCSI error handler guarantees that the function called by
	 * call_rcu() has been called before scsi_end_request() is
	 * called.
	 */
	destroy_rcu_head(&cmd->rcu);
```

## 是否存在一个 lock counter ，当 lock conflits 的增加一个计数器

参考 onload 文档，

类似下面这里的操作，是可以将

```sh
onload_stackdump lots | egrep "(lock_)|(sleep)"
```

## 需要阅读下两本书

在 ali 盘中

### 搭建一个 arm 的汇编的测试环境

## rcu 的 api 用起来
https://lwn.net/Articles/777036/

## rcu 应该测试下 list 的使用


## hlist 和 rcuref 需要简单看看
文件已经建立好了

尝试下 hashtable 的效果:

参考: https://liujunming.top/2018/03/12/linux-kernel%E4%B8%ADHList%E5%92%8CHashtable/

include/linux/hashtable.h hashtable 居然只有一个简单

## 可以尝试分析更多的数据结构，看看那些是多线程的

## 测试下 spin_lock_bh 的效果

对于其功能效果的期待是，spin_lock_bh 可以被中断打断，
但是中断执行之后，如果检测到当前的上下文是 bh disable 的，那么不会去执行
bh 。

问题 : 如果 spin_unlock_bh 执行之后，是不是会立刻开始执行 bh ，如果不是，什么时候执行 bh ?

我有点怀疑，这种 turrotue 工具，社区早就有人写过了吧？

## 如果进行了 CPU isolation ，软中断还可以发生在上面吗？

## 这里的几个 memory barrier 的测试可以简化一下吗?

mm_*.c 的那几个!

至少这个比 store load 要简单啊

https://stackoverflow.com/questions/41858540/whats-are-practical-example-where-acquire-release-memory-order-differs-from-seq


https://news.ycombinator.com/item?id=41085713

## wake_up_interruptible 也是需要 interruptable 的含义是什么?

wake_up_interruptible

## 应该看看 lock 的 toruter 测试

## 如何
如果分发出来的是二进制，如果保证可以
在支持的 RCpc 和不支持 RCpc 的机器上运行。

## 3 ifso appendix 的含金量还在上升

所以，一旦进入到 cache ，顺序就是保证的吗?

使用 load buffer 和 store buffer 不会导致出现一致性问题吗?

例如，两个 thread 都在写 buffer ，那么读到数据都是自己的 store buffer 的。

## 3

涉及到 DMA 的操作之类的，这个是如何定义的?

看看 dma 完成之后，有没有 flush cache 过 ?

## 这里就是入口
docs/concurrent/linearizability.md

## 测试下

include/linux/bit_spinlock.h
```txt
/*
 *  bit-based spin_lock()
 *
 * Don't use this unless you really need to: spin_lock() and spin_unlock()
 * are significantly faster.
 */
```

为什么 bit lock 没有 spin lock 快?
## 这个文档需要看看的

Documentation/core-api/this_cpu_ops.rst

## 需要测试一下 wake_up_interruptible_sync_poll
```c
void sock_def_readable(struct sock *sk)
{
	struct socket_wq *wq;

	trace_sk_data_ready(sk);

	rcu_read_lock();
	wq = rcu_dereference(sk->sk_wq);
	if (skwq_has_sleeper(wq))
		wake_up_interruptible_sync_poll(&wq->wait, EPOLLIN | EPOLLPRI |
						EPOLLRDNORM | EPOLLRDBAND);
	sk_wake_async_rcu(sk, SOCK_WAKE_WAITD, POLL_IN);
	rcu_read_unlock();
}
```

```txt
 - sock_def_readable
   - __wake_up_sync_key
     - __wake_up_common_lock
       - __wake_up_common
         - pollwake
           - __pollwake
             - default_wake_function
               - try_to_wake_up
                 - select_task_rq
                   - select_task_rq_fair
```

## 测试下这里的内容
Documentation/core-api/refcount-vs-atomic.rst

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
