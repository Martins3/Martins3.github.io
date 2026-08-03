#include "internal.h"
#include <linux/kvm_host.h>
#include <linux/vfio.h>
#include <linux/pagewalk.h>

// https://stackoverflow.com/questions/77726002/what-is-the-difference-between-symbol-request-and-symbol-get-used-for-fetch
static struct kvm kvm;
static int test_symbol_get(void)
{
	bool (*fn)(struct kvm * kvm);
	int ret;

	// 这个会返回是成功
	fn = symbol_get(kvm_get_kvm_safe);
	if (WARN_ON(!fn)) {
		return -EINVAL;
	}
	ret = fn(&kvm);
	symbol_put(kvm_get_kvm_safe);

	// 这个会返回失败，因为这个不是 GPL exported 的
	int (*no)(struct mm_struct * mm, unsigned long start, unsigned long end,
		  const struct mm_walk_ops *ops, void *private);
	no = symbol_get(walk_page_range);
	if (WARN_ON(!no)) {
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_X86_64
static int test_symbol_get2(void)
{
	return kvm_get_kvm_safe(&kvm);
}
#endif

// 到时候分析下 kernel 中 kallsymls 之类都是如何构建的
int test_module(long action)
{
	switch (action) {
	case 0:
		return test_symbol_get();
		break;
#ifdef CONFIG_X86_64
	case 1:
		return test_symbol_get2();
		break;
#endif
	}
	return 0;
}
