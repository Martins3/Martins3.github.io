#include "dummy.h"
#include <linux/delay.h>

// 想要验证，一个 request 这包含了那些 bio ，
// 这些 bio 持有的那些 buffer 地址，找到这些
// page 的地址，refcount ，map count
void debug_dump_request(struct request *req)
{
}

// TODO 希望通过这个了解 gendisk 和 block_device 的关系，应该是
// gendisk 指向整个盘，而每一个分区有一个 block_device ，
// 此外 gendisk::part0 为整个盘的 block_device 
void debug_dump_gendisk(struct gendisk *disk)
{
	struct block_device *part;
	unsigned long idx;
	pr_info("disk_name	%s\n", disk->disk_name);
	pr_info("part0		%pg\n", disk->part0);
	pr_info("major		%d\n", disk->major);
	pr_info("first_minor	%d\n", disk->first_minor);
	pr_info("minors		%d\n", disk->minors);
	// 输出结果为:
	// [  858.899487] disk_name        dummy
	// [  858.899523] part0            dummy
	// [  858.899571] major            250
	// [  858.899586] first_minor      1
	// [  858.899598] minors           1

	// TODO 不清楚为什么读 part_tbl 需要 rcu
	// 而且写需要 mutex_lock(&disk->open_mutex);
	rcu_read_lock();
	xa_for_each(&disk->part_tbl, idx, part)
		pr_info("%pg", part);
	rcu_read_unlock();

	// 如何理解 part0
	// 
	// #define disk_to_dev(disk) (&((disk)->part0->bd_device))
	// #define bdev_whole(_bdev) ((_bdev)->bd_disk->part0)

}

// 这里只有 : arch/x86/include/asm/current.h
// 用这个测试 virtio 的堆栈
// 1. hardirq_stack_inuse 是如何赋值的，如何使用的?
// 2. 更加简单的方法获取 rsp rfp 来做测试
static inline void show_stack(void)
{
#ifdef CONFIG_X86_64
	pr_info("stack context");
	pr_info("	current_task : %s", current->comm);
	pr_info("	current stack : %px", current->stack);
	pr_info("	in_hardirq : %lu", (unsigned long)in_hardirq());
	pr_info("	in_softirq : %lu", (unsigned long)in_softirq());
#endif
}

// 测试软中断
void debug_softirq(void)
{
	/*
         * dummy_complete_rq
         * blk_complete_reqs
         * __do_softirq
         * __irq_exit_rcu
         * irq_exit_rcu
         * common_interrupt
         * asm_common_interrupt
	 */
	// TODO 补充一下如何使用
	// if (testcase == BLK_TRACE)
	// 	blk_add_trace_msg(req->q, "--> %s", current->comm);

	if (testcase == INTERRUPT_STACK) {
		pr_info("stack in softirq %s : %px\n", current->comm,
			current->stack);
		show_stack();
	}

	if (testcase == SLEEP_IN_SOFTIQR) {
		pr_info("in_softirq=%ld\n", in_softirq());
		msleep(1);
	}
}

void debug_hardirq(void)
{
	if (testcase == INTERRUPT_STACK) {
		pr_info("stack in hardirq %s : %px\n", current->comm,
			current->stack);

		/* 
		 * 如果是 process 上下文执行，那么结果，例如 rmmod 中
		 *
		 * 用户态的触发的确 hardirq_stack_inuse 为 0
		 *
		 * [ 1178.036445]  hardirq_stack_ptr : ffffc9000073cff8
		 * [ 1178.036713]  current_task : rmmod
	 	 * [ 1178.037438]  hardirq_stack_inuse : 0
	 	 */

		show_stack();
		/*
		 * 当时的日志:
		 * [ 2695.329046] stack in hardirq swapper/18 : ffffc90000148000
		 * [ 2695.329861] cpuhot
		 * [ 2695.329861]  hardirq_stack_ptr : ffffc900005dcff8
		 * [ 2695.330038]  current_task : swapper/18
		 *
		 * 用 panic("now"); 触发
		 * [ 2695.339866] RSP: 0018:ffffc9000014bee8 EFLAGS: 00000212
		 * [ 2695.340334] RAX: 0000000000000012 RBX: 0000000000000012 RCX: 0000000000138fec
		 * [ 2695.340950] RDX: 0000000000000000 RSI: 0000000000000001 RDI: 0000000000138fec
		 * [ 2695.341900] RBP: ffffc9000014bef8 R08: ffffc9000014be50 R09: 0000000000000000
		 * [ 2695.343156] R10: 0000000000000000 R11: ffffffff8113a880 R12: 0000000000000000
		 * [ 2695.344447] R13: ffff8881009da400 R14: 0000000000000000 R15: 0000000000000000
		 *
		 * 看来用的就是 process 的
		 *
		 * [ 1178.173517] cpuhot
		 * [ 1178.173518]  hardirq_stack_ptr : ffffc90000608ff8
		 * [ 1178.174010]  current_task : swapper/19
		 * [ 1178.175091]  hardirq_stack_inuse : 1
		 *
		 *
		 */
	}

	/*
	 * 无论是硬中断还是软中断中，都是可以访问 current 的，但是 current 指的是被打断的
	 * 进程。
	 */
	if (testcase == IRQ_CURRENT)
		pr_info("[%s:%d] %s\n", __FUNCTION__, __LINE__, current->comm);

	// TODO 似乎两个 panic 的严重程度不同
	if (testcase == SLEEP_IN_HARDIQR) {
		msleep(1);
	}

	if (testcase == MDELAY_IN_HARDIQR) {
		// mdelay 没问题的
		mdelay(1);
	}
}
