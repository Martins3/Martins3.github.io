## drivers/md/dm-rq.c
<!-- 702c229f-fbb9-490c-a797-307fe6906d20 -->

为什么有的注册到这里，有的没有:
```c
static const struct blk_mq_ops dm_mq_ops = {
	.queue_rq = dm_mq_queue_rq,
	.complete = dm_softirq_done,
	.init_request = dm_mq_init_request,
};
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

request-based vs. bio-based 目标的现状
在Device Mapper的世界里，目标驱动（target drivers）决定了DM设备的具体行为。它们主要分为两类：
- bio-based 目标 (现代主流)
  - 工作方式：直接在 bio 层面进行处理。当一个 bio（逻辑I/O单元）到达DM设备时，bio-based 目标会直接克隆或重映射这个 bio，然后将其提交到后端的物理设备。
  - 优势：与现代的 blk-mq 框架完美集成，路径短，延迟低，性能高。
  - 哪些是 bio-based？ 几乎所有您日常使用的目标：linear, striped, crypt (LUKS加密), thin-provisioning, cache, raid, verity, zero 等等。这些目标都已经迁移到了 bio-based 模型。
- request-based 目标 (遗留)
  - 工作方式：在 request 层面进行处理。这意味着 bio 必须先经过I/O调度器，被打包成一个 request（物理I/O单元），然后DM设备才能对这个 request 进行映射。
  - 优势：在过去，这允许DM目标利用I/O调度器提供的合并和排序功能。
  - 哪些是 request-based？ 历史上最著名的例子是 dm-multipath。多路径（Multipath）需要复杂的路径选择、故障转移和I/O重试逻辑，这些逻辑在 request 层面处理更为方便，因为一个 request 可能包含多个 bio，可以作为一个整体进行重试。

现状：即使是 dm-multipath，在现代内核中也已经深度集成了 blk-mq。它现在采用一种混合或自适应的模式。虽然它仍然保留了处理 request 的能力，但其队列管理和提交路径已经很大程度上 mq 化了。
如何尝试创建一个 dm-multipath 设备


**在现代内核中，您几乎无法强制创建一个纯粹的、对外呈现为非 mq 队列的 request-based DM设备。**


Device Mapper 是一个非常灵活的框架，它本身是 blk-mq 原生的。但是，它需要支持一些历史悠久的“目标驱动”（target drivers）。
1. 当一个DM目标驱动是基于旧的 request-based 模型编写时，DM核心会为其创建一个适配层。在这个适配层中，虽然外层的DM设备是 mq 的，但传递给这个遗留目标驱动的I/O请求（struct request）所经过的路径，在逻辑上是模拟单队列行为的。

居然是 dm_setup_md_queue 中是可以选择的到底是 bio based 还是 request based ，简单的测试了下，默认是
bio based ，怎么设置为 md 的方式，暂时还没有思考清楚:

```txt
#0  dm_setup_md_queue (md=md@entry=0xffff888105f34c00, t=0xffff888108969400) at drivers/md/dm.c:2347
#1  0xffffffff81f3f11a in table_load (filp=<optimized out>, param=0xffff88810e1da000, param_size=<optimized out>) at drivers/md/dm-ioctl.c:1532
#2  0xffffffff81f3d56a in ctl_ioctl (file=0xffff888115da2900, command=<optimized out>, user=0x55db27994310) at drivers/md/dm-ioctl.c:2082
#3  0xffffffff81f3d86e in dm_ctl_ioctl (file=<optimized out>, command=<optimized out>, u=<optimized out>) at drivers/md/dm-ioctl.c:2104
#4  0xffffffff81474d34 in vfs_ioctl (arg=94399750554384, cmd=<optimized out>, filp=0xffff888115da2900) at fs/ioctl.c:51
```

所以 dm 中存在不少位置是进行 queue_is_mq 判断的，

```c
void dm_start_queue(struct request_queue *q)
{
	blk_mq_unquiesce_queue(q);
	blk_mq_kick_requeue_list(q);
}
```

其实:

这个东西就是提供给 DM_TYPE_REQUEST_BASED 类型用的，目前
只有 dm-table 和 dm-mpath 在使用。所以，这里提到了
bio-based 和 request-based 的区别:

应该主要是 request 会做更多的工作，例如
上层的多个 bio 被 I/O 调度器（如 mq-deadline、bfq 等）合并、排序，形成 request。
内核将 request 放入设备的请求队列（request queue）。

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
