#include <linux/rbtree.h>
#include "internal.h"

static struct rb_root discovery_tables = RB_ROOT;

static inline int __type_cmp(const void *key, const struct rb_node *b)
{
	return 0;
}

static void basic(void)
{
	int m;
	struct rb_node *node = rb_find(&m, &discovery_tables, __type_cmp);
	for (node = rb_first(&discovery_tables); node; node = rb_next(node)) {
		// TODO 算是基本的使用了
	}
}

int test_rbtree(long action)
{
	switch (action) {
	case 1:
		basic();
		break;
	}
	return 0;
}
