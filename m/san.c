#include "internal.h"
#include <linux/slab.h>

static void ubsan(void)
{
	int m[2];
	pr_info("%x", m[2]);
}

static void kcsan(void)
{
	pr_info("TODO kcsan 打开之后，系统误报太频繁了\n");
}

static void kasan(void)
{
}

// https://docs.kernel.org/dev-tools/kfence.html
static void kfence(void)
{
	pr_info("kfence\n");

	// 这个 use-after-free 测试不出来
	int *m = kmalloc(sizeof(int), GFP_KERNEL);
	kfree(m);
	*m = 12;
}

int test_san(long action)
{
	switch (action) {
	case 0:
		kasan();
		break;
	case 1:
		kfence();
		break;
	case 2:
		ubsan();
		break;
	case 7:
		kcsan();
		break;
	}
	return 0;
}
