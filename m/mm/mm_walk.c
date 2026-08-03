#include "internal.h"

#include <linux/pgtable.h>
#include <linux/pagewalk.h>
#include <linux/swap.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_X86_64
static void test_cache_flush(void)
{
	// TODO 放到其他的地方测试吧
	long start = get_parameter(1);
	clflush_cache_range((void *)start, PAGE_SIZE);
}

/*
 * 其实参考 mm/ptdump.c /sys/kernel/debug/check_wx_pages
 *
 * 更好的是参考： static const struct mm_walk_ops clear_refs_walk_ops
 */

// TODO 这个 return 的数值是什么含义来着 ?
static int test_pte_entry(pte_t *pte, unsigned long addr, unsigned long next,
			  struct mm_walk *walk)
{
	/*
	 * [ 2594.682223]  test_pte_entry+0xe/0x20 [martins3]
	 * [ 2594.682540]  walk_pgd_range+0x6d1/0x930
	 * [ 2594.682832]  __walk_page_range+0x5b/0x1b0
	 * [ 2594.683115]  walk_page_range+0xb8/0x250
	 */

	if (!pte_present(*pte)) {
		pr_info("pte hole\n");
		return 0;
	}

	if (!test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *)&pte->pte))
		pr_info("idle\n");
	else
		pr_info("accessed\n");

	return 1;
}

static void iter_pte(void)
{
	struct mm_walk_ops *mm_walk_ops = NULL;
	struct mm_walk mm_walk = {};
	long start = get_parameter(1);
	pr_info("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, start);
	mm_walk_ops = kzalloc(sizeof(struct mm_walk_ops), GFP_KERNEL);
	mm_walk_ops->pte_entry = test_pte_entry;

	mm_walk.mm = current->mm;
	mm_walk.ops = mm_walk_ops;
	mm_walk.private = NULL;
	mm_walk.pgd = NULL;
	mm_walk.no_vma = false;

	// TODO
	// 这里为什么需要 lock ?
	down_read(&current->mm->mmap_lock);
	local_irq_disable();
	// 用 /proc/kallsyms 来获取符号吧
	/* walk_page_range(current->mm, start, start + PAGE_SIZE, mm_walk.ops, */
	/* 		mm_walk.private); */
	local_irq_enable();
	up_read(&current->mm->mmap_lock);
}
#endif

static void iter_vma(void)
{
	// 参考 acct_collect
	// 一个简单的模拟 : fs/proc/task_mmu.c
	struct mm_struct *mm = current->mm;
	VMA_ITERATOR(vmi, mm, 0);
	struct vm_area_struct *vma;

	mmap_read_lock(mm);
	for_each_vma(vmi, vma) {
		pr_info("%lx-%lx", vma->vm_start, vma->vm_end);
		if (vma->vm_file) {
			const struct path *path = file_user_path(vma->vm_file);
			char name_buf[128];
			const char *name = d_path(path, name_buf, 128);
			pr_info("%s\n", name);
		/* } else if (vma->anon_name) { */
			/* pr_info("%s\n", vma->anon_name->name); */
		} else
			pr_info("anon\n");
	}
	mmap_read_unlock(mm);
}

int test_mm_walk(long action)
{
	switch (action) {
	case 1:
		iter_vma();
		break;
#ifdef CONFIG_X86_64
	case 2:
		iter_pte();
		break;
	case 3:
		test_cache_flush();
		break;
#endif
	}
	return 0;
}
