#include "internal.h"
#include <linux/mm.h>
#include <linux/vmalloc.h>
#define PAGE_NUM 10
/*
 * 如果 PAGE_NUM = 1 的时候
 * [  494.917147] [martins3:basic:22] ffffea0004cb1600 ffff888132c59000
 * [  494.918233] [martins3:basic:24] ffffea0004cb1600 ffff888132c5a000
 *
 * 如果 PAGE_NUM = 10 的时候
 * [  596.403409] [martins3:basic:28] ffffea00040d6c00 ffff8881035b0000
 * [  596.403957] [martins3:basic:30] ffffea000409fc00 ffff8881027f0000
 *
 * 对比 https://www.kernel.org/doc/html/v5.8/x86/x86_64/mm.html
 * 发现 的确还是在传统地址中。
 *
 *  检查 __kvmalloc_node 的注释
 *
 * attempt to allocate physically contiguous memory, but upon
 * failure, fall back to non-contiguous (vmalloc) allocation.
 *
 * TODO
 * 1. PAGE_NUM = 1 的时候，为什么指向了同一个 folio ? 也许是 slab 内部的机制
 * 2. 测试下 vmap 的效果，然后用 vmap 看看 virt_to_folio 得到的 folio ，做操作，应该会 crash 吧
 *	- 参考 alloc_thread_stack_node
 */
static void basic(void)
{
	void *kcalloc_addr = kcalloc(PAGE_SIZE, PAGE_NUM, GFP_KERNEL_ACCOUNT);
	if (!kcalloc_addr) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		return;
	}
	void *kvcalloc_addr = kvcalloc(
		PAGE_SIZE, PAGE_NUM, GFP_KERNEL_ACCOUNT | __GFP_RETRY_MAYFAIL);
	if (!kvcalloc_addr) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		goto err;
	}

	struct folio *f1 = virt_to_folio(kcalloc_addr);
	struct folio *f2 = virt_to_folio(kvcalloc_addr);

	pr_info("kcalloc %px %px\n", f1, kcalloc_addr);
	pr_info("kvcalloc %px %px\n", f2, kvcalloc_addr);

	kvfree(kvcalloc_addr);
err:
	kfree(kcalloc_addr);
}

static void layout(void)
{
	/*
	 * x86 中测试结果，和
	 * https://www.kernel.org/doc/html/v5.8/x86/x86_64/mm.html
	 * 中说的一模一样
	 *
	 * [46805.132186] vmalloc_base : ffffc90000000000
	 * [46805.132581] vmemmap_base : ffffea0000000000
	 * [ 1512.362620] TASK_SIZE: 7ffffffff000
	 * [ 1512.363086] STACK_TOP: 7ffffffff000
	 *
	 * aarch64 中测试:
	 * TODO
	 */
#ifdef CONFIG_X86_64
	pr_info("vmalloc_base : %lx\n", vmalloc_base);
	pr_info("vmemmap_base : %lx\n", vmemmap_base);
#endif
	/* 看看 arch/x86/include/asm/page_64_types.h 中的几个变量 */
	pr_info("TASK_SIZE: %lx\n", TASK_SIZE);
	pr_info("STACK_TOP: %lx\n", STACK_TOP);

	// TODO 有办法直接答应出来 kallsyms 中的 _text 吗?
	// x86_64
	// ffffffff81000000 T _text

	// aarch64
	// ffffce5a2ef60000 T _text
}

int test_vmalloc(long action)
{
	switch (action) {
	case 0:
		basic();
		break;
	case 1:
		layout();
		break;
	}
	return 0;
}
