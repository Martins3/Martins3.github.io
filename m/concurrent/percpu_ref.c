#include "internal.h"
#include <linux/percpu-refcount.h>
// docs/concurrent/kernel/api/refcount.md

static void blk_queue_usage_counter_release(struct percpu_ref *ref)
{
	pr_info("release hook\n");
	// dump_stack();
	//  blk_queue_usage_counter_release+0x24/0x38 [martins3]
	//  percpu_ref_put_many.constprop.0+0x170/0x228 [martins3]
	//  test_percpu_ref+0xf0/0x260 [martins3]
	//  percpu_ref_store+0xd0/0x120 [martins3]
	//  kobj_attr_store+0x18/0x30
}

static struct percpu_ref q_usage_counter;

// 参考 struct percpu_ref request_queue::q_usage_counter
static int basic_test(void)
{
	int error;
	error = percpu_ref_init(&q_usage_counter,
				blk_queue_usage_counter_release,
				PERCPU_REF_INIT_ATOMIC, GFP_KERNEL);
	percpu_ref_get(&q_usage_counter);
	for (size_t i = 0; i < 4; i++) {
		pr_info("%ld\n", i);
		percpu_ref_put(&q_usage_counter);
	}
	// 0
	// 1
	// release hook
	// 2
	// 3

	return 0;
}

int test_percpu_ref(long action)
{
	switch (action) {
	case 0:
		basic_test();
		break;
	}
	return 0;
}
