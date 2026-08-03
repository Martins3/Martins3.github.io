#include "internal.h"
#include <linux/slab.h>

// 验证这个问题:
//
// 需要模块程序运行在 numa node >= 2 的环境中:
//
// commit 9198ffbd2b49 ("mm/slub: Reduce memory consumption in extreme scenarios")
//
// 如果分配的 folio 不属于目标的 numa node ，那么将会自动 deactivated ，并且
// 添加把这个 slab 放到其所在的 numa node 的 partial 中。
//
// 也就是，很多时候从 node2 中分配，然后放到了 node2 的 partial 中
//
// [root@big2 15:11:38 kmalloc-512]$ cat partial
// 67048 N0=2 N2=19729 N3=27677 N4=3 N5=4 N6=4 N7=3 N8=3 N9=4 N10=19603 N11=4 N12=3 N13=3 N14=3 N15=3
//
// 这个内存不足甚至是被 page cache 占用就可以了，
// 所以，就有一个问题，如果 slab 不可以在目标 numa node 上分配，
// 那么为什么不是首先 kick 掉 page cache 。
//
// 把 vm.watermark_scale_factor ，让一个节点的 free 为 2G 就需要 cache 就需要从其他的节点上分配，
// 结果发现还是如此。
//
// 通过这个问题研究分析一下，GFP flags 的影响吧

static void test_deactivate_slab(void)
{
	int i;
	struct foo *f;
	for (i = 0; i < 100000; i++) {
		f = kzalloc_node(512, GFP_KERNEL, 0);
		if (!f) {
			pr_err("failed to alloc\n");
			return;
		}
		if (i == 0)
			pr_info("%px\n", f);
	}
}

int test_slub(long action)
{
	switch (action) {
	case 1:
		test_deactivate_slab();
		break;
	}
	return 0;
}
