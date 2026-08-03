#include "internal.h"
#include <linux/idr.h>

/*
 * Small id to pointer translation service avoiding fixed sized tables.
 */
static DEFINE_IDR(nbd_index_idr);

struct A {
	int a;
};
static int basic_test(void)
{
	struct A a = { .a = 123 };
	struct A b = { .a = 456 };
	struct A c = { .a = 789 };

	idr_alloc(&nbd_index_idr, &a, 0, 100, GFP_KERNEL);
	idr_alloc(&nbd_index_idr, &b, 10, 100, GFP_KERNEL);
	idr_alloc(&nbd_index_idr, &c, 20, 30, GFP_KERNEL);

	struct A *nbd;
	int id;
	idr_for_each_entry(&nbd_index_idr, nbd, id)
		pr_info("%d %d\n", nbd->a, id);
	return 0;
}

static int conrrent_test(void)
{
	// TODO
	return 0;
}
int test_idr(long action)
{
	switch (action) {
	case 1:
		return basic_test();
	case 2:
		return conrrent_test();
	}
	return 0;
}
