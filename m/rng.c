#include "internal.h"
#include <linux/random.h>

static void very_basic_test(void)
{
	u32 arr[100];
	for (size_t i = 0; i < 100; i++) {
		u32 m = get_random_u32_below(10);
		arr[i] = m;
	}
	pr_info("Random : \n");
	for (size_t i = 0; i < 100; i++) {
		printk(KERN_CONT "%u ", arr[i]);
	}
	pr_info("Done \n");
}

int test_rng(long action)
{
	switch (action) {
	case 0:
		very_basic_test();
		break;
	}
	return 0;
}
