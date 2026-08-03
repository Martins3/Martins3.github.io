#define pr_fmt(fmt) "lru : " fmt

#include "internal.h"
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/page-flags.h>

/* 测试用的 folio - 注意：不能命名为 test_folio，因为 DECLARE_TESTER(folio) 定义了 test_folio() 函数 */
static struct folio *g_lru_test_folio;
static struct proc_dir_entry *lru_proc_entry;

/*
 * folio_add_lru - 将 folio 添加到 LRU 链表
 *
 * 效果:
 * 1. 设置 folio 的 LRU bit (PageLRU)
 * 2. 将 folio 添加到对应 lruvec 的 inactive 链表中
 * 3. 更新 lruvec 的计数
 *
 * 观察方法:
 * - /proc/meminfo 中的 Active/Inactive 计数
 * - /proc/zoneinfo 中的 LRU 链表详情
 * - /proc/lru_folio_test (本模块创建) 查看测试 folio 状态
 */

/* proc 文件 show 函数 - 显示测试 folio 的 LRU 状态 */
static int lru_folio_show(struct seq_file *m, void *v)
{
	if (!g_lru_test_folio) {
		seq_printf(m, "测试 folio 未分配\n");
		return 0;
	}

	seq_printf(m, "===== 测试 Folio LRU 状态 =====\n");
	seq_printf(m, "folio 地址: %p\n", g_lru_test_folio);
	seq_printf(m, "folio 阶数: %u (nr_pages=%ld)\n",
		   folio_order(g_lru_test_folio), folio_nr_pages(g_lru_test_folio));
	seq_printf(m, "folio 物理地址: 0x%lx\n", folio_pfn(g_lru_test_folio) << PAGE_SHIFT);
	seq_printf(m, "\n");

	/* LRU 相关状态 */
	seq_printf(m, "--- LRU 状态 ---\n");
	seq_printf(m, "PageLRU bit: %d (是否在 LRU 链表中)\n",
		   folio_test_lru(g_lru_test_folio));
	seq_printf(m, "PageActive bit: %d (是否在 active 链表)\n",
		   folio_test_active(g_lru_test_folio));
	seq_printf(m, "PageReferenced bit: %d (是否被访问过)\n",
		   folio_test_referenced(g_lru_test_folio));
	seq_printf(m, "PageWorkingset bit: %d (是否在工作集中)\n",
		   folio_test_workingset(g_lru_test_folio));
	seq_printf(m, "\n");

	/* 其他相关状态 */
	seq_printf(m, "--- 其他状态 ---\n");
	seq_printf(m, "PageLocked bit: %d\n", folio_test_locked(g_lru_test_folio));
	seq_printf(m, "PagePrivate bit: %d\n", folio_test_private(g_lru_test_folio));
	seq_printf(m, "PageSwapCache bit: %d\n", folio_test_swapcache(g_lru_test_folio));
	seq_printf(m, "引用计数: %d\n", folio_ref_count(g_lru_test_folio));
	seq_printf(m, "映射计数: %d\n", folio_mapcount(g_lru_test_folio));

	return 0;
}

static int lru_folio_open(struct inode *inode, struct file *file)
{
	return single_open(file, lru_folio_show, NULL);
}

static const struct proc_ops lru_folio_proc_ops = {
	.proc_open = lru_folio_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static void test_folio_add_lru(void)
{
	if (!g_lru_test_folio) {
		pr_err("测试 folio 未分配\n");
		return;
	}

	pr_info("=== 测试 folio_add_lru ===\n");
	pr_info("添加前 - PageLRU: %d\n", folio_test_lru(g_lru_test_folio));

	/* 将 folio 添加到 LRU */
	folio_add_lru(g_lru_test_folio);

	pr_info("添加后 - PageLRU: %d\n", folio_test_lru(g_lru_test_folio));
	pr_info("folio_add_lru 完成，可通过 /proc/lru_folio_test 查看状态\n");
}

static void test_folio_lru_state(void)
{
	if (!g_lru_test_folio) {
		pr_err("测试 folio 未分配\n");
		return;
	}

	pr_info("=== 当前 Folio 状态 ===\n");
	pr_info("PageLRU: %d, PageActive: %d, PageReferenced: %d\n",
		folio_test_lru(g_lru_test_folio),
		folio_test_active(g_lru_test_folio),
		folio_test_referenced(g_lru_test_folio));
}

int test_lru_init(void)
{
	/* 分配一个 folio 用于测试 */
	g_lru_test_folio = folio_alloc(GFP_USER, 0); /* 分配单页 folio */
	if (!g_lru_test_folio) {
		pr_err("分配 folio 失败\n");
		return -ENOMEM;
	}

	pr_info("测试 folio 已分配: %p (pfn=0x%lx)\n",
		g_lru_test_folio, folio_pfn(g_lru_test_folio));

	/* 创建 proc 文件 */
	lru_proc_entry = proc_create("lru_folio_test", 0444, NULL,
				     &lru_folio_proc_ops);
	if (!lru_proc_entry) {
		pr_err("创建 proc 文件失败\n");
		folio_put(g_lru_test_folio);
		g_lru_test_folio = NULL;
		return -ENOMEM;
	}

	pr_info("proc 文件已创建: /proc/lru_folio_test\n");
	return 0;
}

int test_lru_exit(void)
{
	/* 如果 folio 还在 LRU 中，需要先移除 */
	if (g_lru_test_folio && folio_test_lru(g_lru_test_folio)) {
		/* folio 会被 lru 处理自动移除，或者我们需要手动处理 */
		pr_info("清理: folio 仍在 LRU 中\n");
	}

	if (g_lru_test_folio) {
		folio_put(g_lru_test_folio);
		g_lru_test_folio = NULL;
	}

	if (lru_proc_entry) {
		remove_proc_entry("lru_folio_test", NULL);
		lru_proc_entry = NULL;
	}

	pr_info("LRU 测试模块已清理\n");
	return 0;
}

int test_lru(long action)
{
	switch (action) {
	case 0:
		/* 显示当前状态 */
		test_folio_lru_state();
		break;
	case 1:
		/* 执行 folio_add_lru */
		test_folio_add_lru();
		break;
	case 2:
		/* 重新分配 folio (释放旧的，分配新的) */
		if (g_lru_test_folio) {
			folio_put(g_lru_test_folio);
		}
		g_lru_test_folio = folio_alloc(GFP_USER, 0);
		if (g_lru_test_folio) {
			pr_info("重新分配 folio: %p\n", g_lru_test_folio);
		} else {
			pr_err("重新分配 folio 失败\n");
		}
		break;
	default:
		pr_info("未知 action: %ld (可用: 0=状态 1=add_lru 2=重新分配)\n",
			action);
		break;
	}
	return 0;
}
