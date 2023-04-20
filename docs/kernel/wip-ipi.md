# 收集一下 ipi 相关的内容，到时候一并分析一下

## blk
```c
static void blk_mq_complete_send_ipi(struct request *rq)
{
	struct llist_head *list;
	unsigned int cpu;

	cpu = rq->mq_ctx->cpu;
	list = &per_cpu(blk_cpu_done, cpu);
	if (llist_add(&rq->ipi_list, list)) {
		INIT_CSD(&rq->csd, __blk_mq_complete_request_remote, rq);
		smp_call_function_single_async(cpu, &rq->csd);
	}
}
```
## kvm 中似乎很多
