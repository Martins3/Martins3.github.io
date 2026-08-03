#include "internal.h"

// 应该测试的 api 是:
//
// sg_page : 到 page 的转换
//
// sg_dma_address
//
// for_each_sg
//
// sg_page_iter : page
// for_each_sg_page
// sg_page_iter_dma_address
//
// sg_chain
//
// 就参考这个把
//
// http://www.wowotech.net/memory_management/scatterlist.html
// sg table 的实现，检查 SG_CHAIN 的位置即可
// 1. 如果有 SG_CHAIN bit ，那么说明 page_link 中指向的是下一个 sg
// 2. 如果有 SG_END ，page_link 指向的还是 page ，但是遍历的时候会停止

#include <linux/scatterlist.h>

static void print(struct scatterlist *sgl)
{
	int i;
	struct scatterlist *sg;
	for_each_sg(sgl, sg, INT_MAX, i) {
		// 需要立刻跳出来，所以，最好是将 INT_MAX 替换掉
		if (!sg)
			break;
		pr_info("length %d offset %d\n", sg->length, sg->offset);
	}
}
static void basic_test(void)
{
	struct scatterlist single, two[2], multiple[10];
	struct scatterlist *sg;
	int i;
	struct folio *folio = folio_alloc(GFP_USER, 1);
	char m[100];

	sg_init_one(&single, m, 100);
	pr_info("-> length %d offset %d\n", single.length, single.offset);

	sg_init_table(two, ARRAY_SIZE(two));
	sg_set_buf(two, (void *)100, 100);
	sg_set_buf(two + 1, (void *)200, 200);

	// 一次性初始化多个 scatterlist
	sg_init_table(multiple, ARRAY_SIZE(multiple));
	for (size_t i = 0; i < ARRAY_SIZE(multiple); i++) {
		// 通过 sg_set_buf 或者 sg_set_folio 来关联一个 scatterlist 的内容
		sg_set_buf(multiple + i, (void *)i, 1);
	}
	sg_set_folio(multiple, folio, PAGE_SIZE, 0);

	pr_info("-> length %d offset %d\n", single.length, single.offset);
	pr_info("single\n");
	for_each_sg(&single, sg, 1, i) {
		pr_info("single : length %d offset %d\n", sg->length,
			sg->offset);
	}
	pr_info("single : length %d offset %d\n", single.length, single.offset);

	pr_info("multiple\n");
	for_each_sg(multiple, sg, ARRAY_SIZE(multiple), i) {
		pr_info("length %d offset %d\n", sg->length, sg->offset);
	}

	// 将 scatterlist 连接起来
	pr_info("before chain\n");
	print(two);
	sg_chain(two, 2, multiple);
	pr_info("after chain\n");
	print(two);

	/*
	 * 似乎这是 sg_chain 的 bug ，没有办法 chain 只有一个元素的 sg table
	 * chain 完之后，遍历 single 的结果是错误的。
	 *
	 * sg_chain(&single, 1, two);
	 * print(&single);
	 *
	 * 仔细看，因为 sg_next 的实现总是找的下一个，没有处理。
	 */
}

int test_sg(long action)
{
	switch (action) {
	case 0:
		basic_test();
		break;
	}
	return 0;
}
