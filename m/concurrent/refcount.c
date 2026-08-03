#include "internal.h"
#include <linux/refcount.h>
// TODO refcount.h 中很大一段注释
// TODO Documentation/core-api/refcount-vs-atomic.rst

// refcount_t 中间就是一个 atomic 变量
static refcount_t vm_refcnt;

// 使用真的很简单，如果 refcount_dec_and_test 是 0 ，那么认为
// 当前的该释放了
//
// 参考 blkcg_unpin_online

static inline void inc(void)
{
	refcount_inc(&vm_refcnt);
}

static inline void dec(void)
{
	if (refcount_dec_and_test(&vm_refcnt))
		pr_info("should release\n");
	else
		pr_info("not yet \n");
}
static void basic_test(void)
{
	// 不可以初始化为 0 ，当 refcount 为 0 的时候
	//
	refcount_set(&vm_refcnt, 1);

	inc();
	dec(); // not yet
	pr_info("counter=%x\n", vm_refcnt.refs.counter);

	dec(); // should release
	pr_info("counter=%x\n", vm_refcnt.refs.counter);

	dec(); // not yet
	pr_info("counter=%x\n", vm_refcnt.refs.counter);

	dec(); // not yet
	pr_info("counter=%x\n", vm_refcnt.refs.counter);

	// not yet
	// counter=1
	// should release
	// counter=0
	// not yet
	// counter=c0000000
	// not yet
	// counter=c0000000
}
int test_refcount(long action)
{
	switch (action) {
	case 0:
		basic_test();
		break;
	}
	return 0;
}
