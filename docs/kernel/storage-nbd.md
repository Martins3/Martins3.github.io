## nbd
将 `blk_mq_ops` 替换成为网络的接口就可以了

```c
static const struct blk_mq_ops nbd_mq_ops = {
    .queue_rq   = nbd_queue_rq,
    .complete   = nbd_complete_rq,
    .init_request   = nbd_init_request,
    .timeout    = nbd_xmit_timeout,
};
```

为什么 nfs 那么复杂，大约 5 万多行，而 nbd 只要 2500 行就结束了。
