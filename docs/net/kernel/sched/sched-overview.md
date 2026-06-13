# sched
关于调度器中的基本内容:
1. 进程的生命周期
  1. fork
  2. exit
  3. exec
2. scheduler
  1. context switch
  2. cfs
    1. load_avg
    2. task group
    3. bandwidth
    4. load balance
    5. numa balance
    6. autogroup
  3. preempt
3. signal

## https://www.kernel.org/doc/html/latest/scheduler/index.html

## [Classless Queuing Disciplines (qdiscs)](https://www.cnblogs.com/charlieroro/p/13993695.html) https://tldp.org/en/Traffic-Control-HOWTO/ar01s06.html 的翻译

里面介绍了各种队列的使用方法理论配置:

- [wiki](https://en.wikipedia.org/wiki/Network_scheduler)


# code
```c
struct Qdisc_ops pfifo_fast_ops __read_mostly = {
	.id		=	"pfifo_fast",
	.priv_size	=	sizeof(struct pfifo_fast_priv),
	.enqueue	=	pfifo_fast_enqueue,
	.dequeue	=	pfifo_fast_dequeue,
	.peek		=	pfifo_fast_peek,
	.init		=	pfifo_fast_init,
	.destroy	=	pfifo_fast_destroy,
	.reset		=	pfifo_fast_reset,
	.dump		=	pfifo_fast_dump,
	.change_tx_queue_len =  pfifo_fast_change_tx_queue_len,
	.owner		=	THIS_MODULE,
	.static_flags	=	TCQ_F_NOLOCK | TCQ_F_CPUSTATS,
};
```

对应的调用者在 /home/maritns3/core/linux/net/core/dev.c 中间，感觉应该就是在发送给 NIC 的时候如果数据太多了就丢掉一下.

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
