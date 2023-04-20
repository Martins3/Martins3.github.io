## bio layer
- [A block layer introduction part 1: the bio layer](https://lwn.net/Articles/736534/)
- [Block layer introduction part 2: the request layer](https://lwn.net/Articles/738449/)

1. bio 给上下两个层次提供的接口是什么 ?
2. https://zhuanlan.zhihu.com/p/39199521
    1. bio 机制核心 : 合并请求

## request

```c
	/*
	 * The hash is used inside the scheduler, and killed once the
	 * request reaches the dispatch list. The ipi_list is only used
	 * to queue the request for softirq completion, which is long
	 * after the request has been unhashed (and even removed from
	 * the dispatch list).
	 */
	union {
		struct hlist_node hash;	/* merge hash */
		struct llist_node ipi_list;
	};
```
- [ ] `ipi_list` 的使用大致知道了，但是 `hash` 如何操作来着?

### 在硬中断中加入 request / bio

```c
static void blk_mq_raise_softirq(struct request *rq)
{
	struct llist_head *list;

	preempt_disable();
	list = this_cpu_ptr(&blk_cpu_done);
	if (llist_add(&rq->ipi_list, list))
		raise_softirq(BLOCK_SOFTIRQ);
	preempt_enable();
}
```

```txt
#0  blk_mq_raise_softirq (rq=0xffff8881067d7280) at block/blk-mq.c:1194
#1  blk_mq_complete_request_remote (rq=0xffff8881067d7280) at block/blk-mq.c:1220
#2  blk_mq_complete_request_remote (rq=0xffff8881067d7280) at block/blk-mq.c:1201
#3  0xffffffff8174a882 in blk_mq_complete_request (rq=0xffff8881067d7280) at block/blk-mq.c:1236
#4  0xffffffff81c29279 in ata_sff_hsm_move (ap=ap@entry=0xffff88810b93c000, qc=qc@entry=0xffff88810b93c130, status=<optimized out>, status@entry=80 'P', in_wq=in_wq@entry=0) at drivers/ata/libata-sff.c:1159
#5  0xffffffff81c29b41 in __ata_sff_port_intr (ap=ap@entry=0xffff88810b93c000, qc=qc@entry=0xffff88810b93c130, hsmv_on_idle=<optimized out>) at drivers/ata/libata-sff.c:1446
#6  0xffffffff81c29c68 in ata_bmdma_port_intr (ap=0xffff88810b93c000, qc=0xffff88810b93c130) at drivers/ata/libat a-sff.c:2747
#7  0xffffffff81c29eb5 in __ata_sff_interrupt (irq=<optimized out>, port_intr=0xffffffff81c29c30 <ata_bmdma_port_intr>, dev_instance=0xffff88810ae89380) at drivers/ata/libata-sff.c:1491
#8  ata_bmdma_interrupt (irq=<optimized out>, dev_instance=0xffff88810ae89380) at drivers/ata/libata-sff.c:2772
#9  0xffffffff811c912d in __handle_irq_event_percpu (desc=desc@entry=0xffff888100336400) at kernel/irq/handle.c:1 58
#10 0xffffffff811c9338 in handle_irq_event_percpu (desc=0xffff888100336400) at kernel/irq/handle.c:193
#11 handle_irq_event (desc=desc@entry=0xffff888100336400) at kernel/irq/handle.c:210
#12 0xffffffff811ce25b in handle_edge_irq (desc=0xffff888100336400) at kernel/irq/chip.c:819
#13 0xffffffff810d9d9f in generic_handle_irq_desc (desc=0xffff888100336400) at ./include/linux/irqdesc.h:158
#14 handle_irq (regs=<optimized out>, desc=0xffff888100336400) at arch/x86/kernel/irq.c:231
#15 __common_interrupt (regs=<optimized out>, vector=20) at arch/x86/kernel/irq.c:250
#16 0xffffffff82290dcf in common_interrupt (regs=0xffffc900000e7e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000349010
```

### 在软中断中处理 request / bio
```txt
#0  reschedule_retry (r1_bio=0xffff888172f0d080) at drivers/md/raid1.c:279
#1  0xffffffff8174bb11 in req_bio_endio (error=0 '\000', nbytes=65536, bio=0xffff888172d78600, rq=0xffff8881067d7280) at block/blk-mq.c:795
#2  blk_update_request (req=req@entry=0xffff8881067d7280, error=error@entry=0 '\000', nr_bytes=196608, nr_bytes@entry=262144) at block/blk-mq.c:927
#3  0xffffffff81ba1d47 in scsi_end_request (req=req@entry=0xffff8881067d7280, error=error@entry=0 '\000', bytes=bytes@entry=262144) at drivers/scsi/scsi_lib.c:538
#4  0xffffffff81ba288e in scsi_io_completion (cmd=0xffff8881067d7388, good_bytes=262144) at drivers/scsi/scsi_lib.c:976
#5  0xffffffff817482ed in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1132
#6  0xffffffff822a98d4 in __do_softirq () at kernel/softirq.c:571
#7  0xffffffff811496e6 in invoke_softirq () at kernel/softirq.c:445
#8  __irq_exit_rcu () at kernel/softirq.c:650
#9  0xffffffff81149eee in irq_exit_rcu () at kernel/softirq.c:662
#10 0xffffffff82290dd4 in common_interrupt (regs=0xffffc900000e7e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

```c
static DEFINE_PER_CPU(struct llist_head, blk_cpu_done);

static __latent_entropy void blk_done_softirq(struct softirq_action *h)
{
	blk_complete_reqs(this_cpu_ptr(&blk_cpu_done));
}

static void blk_complete_reqs(struct llist_head *list)
{
	struct llist_node *entry = llist_reverse_order(llist_del_all(list));
	struct request *rq, *next;

	llist_for_each_entry_safe(rq, next, entry, ipi_list)
		rq->q->mq_ops->complete(rq);
}
```
