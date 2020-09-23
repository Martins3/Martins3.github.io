# bfq-iosched.c


## interface
1. 找到这些函数的调用位置是什么 ?
2. 大致了解使用方法是什么 ?
```c
static struct elevator_type iosched_bfq_mq = {
	.ops = {
		.limit_depth		= bfq_limit_depth,
		.prepare_request	= bfq_prepare_request,
		.requeue_request        = bfq_finish_requeue_request,
		.finish_request		= bfq_finish_requeue_request,
		.exit_icq		= bfq_exit_icq,
		.insert_requests	= bfq_insert_requests,
		.dispatch_request	= bfq_dispatch_request,
		.next_request		= elv_rb_latter_request,
		.former_request		= elv_rb_former_request,
		.allow_merge		= bfq_allow_bio_merge,
		.bio_merge		= bfq_bio_merge,
		.request_merge		= bfq_request_merge,
		.requests_merged	= bfq_requests_merged,
		.request_merged		= bfq_request_merged,
		.has_work		= bfq_has_work,
		.depth_updated		= bfq_depth_updated,
		.init_hctx		= bfq_init_hctx,
		.init_sched		= bfq_init_queue,
		.exit_sched		= bfq_exit_queue,
	},

	.icq_size =		sizeof(struct bfq_io_cq),
	.icq_align =		__alignof__(struct bfq_io_cq),
	.elevator_attrs =	bfq_attrs,
	.elevator_name =	"bfq",
	.elevator_owner =	THIS_MODULE,
};
```
