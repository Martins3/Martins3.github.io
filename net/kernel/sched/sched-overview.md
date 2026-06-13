# sched

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
