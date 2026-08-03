#include "internal.h"
#include <linux/mmu_notifier.h>
struct mmu_notifier_test {
	struct mmu_notifier mmu_notifier;
} kvm;

static int
test_mmu_notifier_invalidate_range_start(struct mmu_notifier *mn,
					 const struct mmu_notifier_range *range)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static void
test_mmu_notifier_invalidate_range_end(struct mmu_notifier *mn,
				       const struct mmu_notifier_range *range)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}

static int test_mmu_notifier_clear_flush_young(struct mmu_notifier *mn,
					       struct mm_struct *mm,
					       unsigned long start,
					       unsigned long end)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static int test_mmu_notifier_clear_young(struct mmu_notifier *mn,
					 struct mm_struct *mm,
					 unsigned long start, unsigned long end)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static int test_mmu_notifier_test_young(struct mmu_notifier *mn,
					struct mm_struct *mm,
					unsigned long address)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static void test_mmu_notifier_release(struct mmu_notifier *mn,
				      struct mm_struct *mm)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}

static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {
	.invalidate_range_start = test_mmu_notifier_invalidate_range_start,
	.invalidate_range_end = test_mmu_notifier_invalidate_range_end,
	.clear_flush_young = test_mmu_notifier_clear_flush_young,
	.clear_young = test_mmu_notifier_clear_young,
	.test_young = test_mmu_notifier_test_young,
	.release = test_mmu_notifier_release,
};

static int kvm_init_mmu_notifier(struct mmu_notifier_test *kvm)
{
	kvm->mmu_notifier.ops = &kvm_mmu_notifier_ops;
	return mmu_notifier_register(&kvm->mmu_notifier, current->mm);
}

int test_mmu_notifier_init(void)
{
	return 0;
}

int test_mmu_notifier_exit(void)
{
	return 0;
}

// 忽然感觉 mmu_notifier 也很简单，其实也没有太大的必要测试:
// TODO
// 1. 如何实现进程被杀，自动释放 mmu_notifier_unregister 的
//	- 通过释放文件描述符的来实现吗?
// 2. 如果一个进程只是 mmu_notifier_unregister ，但是不去 mmu_notifier_register ，最后如何?
int test_mmu_notifier(long action)
{
	switch (action) {
	case 1:
		return kvm_init_mmu_notifier(&kvm);
		break;
	case 2:
		mmu_notifier_unregister(&kvm.mmu_notifier, current->mm);
		break;
	}
	return 0;
}
