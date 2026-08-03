#include "internal.h"

/*
 * 参考 : https://github.com/mykhani/kernel_notifications/blob/master/notified.c
 *
 * 这个例子使用了两个 module
 */

static int file_op_event_handler(struct notifier_block *unused,
				 unsigned long event, void *ptr)
{
	/*
	 * blocking 的技术没有什么高级的
	 *
	 * [  232.795251]  dump_stack_lvl+0x86/0xc0
	 * [  232.795857]  file_op_event_handler+0x3c/0x50 [martins3]
	 * [  232.796668]  blocking_notifier_call_chain+0x5f/0xf0
	 * [  232.797375]  test_notifier+0x76/0xa0 [martins3]
	 * [  232.797985]  notifier_store+0xa0/0xd0 [martins3]
	 * [  232.798569]  kernfs_fop_write_iter+0xf0/0x170
	 * [  232.799089]  vfs_write+0x38d/0x450
	 * [  232.799468]  ksys_write+0x72/0xe0
	 * [  232.799809]  do_syscall_64+0xef/0x210
	 *
	 */
	pr_info("[%s:%d] [%ld] [%px] [%s]\n", __FUNCTION__, __LINE__, event, ptr, current->comm);
	dump_stack();
	return 0;
}

struct notifier_block file_op_event = {
	.notifier_call = file_op_event_handler,
};

struct blocking_notifier_head file_op_event_chain;

static bool status_ready = false;
static int status_init(void)
{
	int ret;
	BLOCKING_INIT_NOTIFIER_HEAD(&file_op_event_chain);

	ret = blocking_notifier_chain_register(&file_op_event_chain,
					       &file_op_event);
	/* blocking_notifier_chain_unregister(&file_op_event_chain, &file_op_event); */

	if (ret){
		pr_err("register notified chain failed\n");
		return ret;
	}

	status_ready = true;
	return 0;
}
int test_notifier(long action)
{
	int ret;
	if (!status_ready) {
		ret = status_init();
		if (ret < 0)
			return ret;
	}
	switch (action) {
	case 10 ... LONG_MAX:
		blocking_notifier_call_chain(&file_op_event_chain, action,
					     (void *)0xDEADBEAF);

		break;
	}
	return 0;
}
