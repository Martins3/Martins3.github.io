#include "internal.h"
#include <linux/maple_tree.h>

// 参考资料：
// https://lwn.net/Articles/901714/ - Maple Tree 介绍
// https://docs.kernel.org/core-api/maple_tree.html - 官方文档
// https://lwn.net/Articles/845507/ - 替换基数树
// https://blogs.oracle.com/linux/post/the-maple-tree-a-modern-data-structure-for-a-complex-problem
//
// 目前的用户只有 mmap.c
//
// - [ ] rcu
// - [ ] cache 命中率如何体现
//
// 似乎这个替换在不断的进行:
// commit 721255b9826b ("genirq: Use a maple tree for interrupt descriptor management")

// Maple Tree 是用于存储范围（range）的 RCU 安全自适应树结构
// 主要用于 VMA (Virtual Memory Area) 管理，替代了之前的 radix tree 和 interval tree

// 测试数据指针，使用特殊值避免与内核指针冲突
#define TEST_PTR_1 ((void *)0x12345678UL)
#define TEST_PTR_2 ((void *)0xABCDEF00UL)
#define TEST_PTR_3 ((void *)0xDEADBEEFUL)

/*
 * 测试1: 基本插入、查找、删除操作
 * mtree_insert - 在指定索引处插入条目
 * mtree_load - 加载指定索引处的条目
 * mtree_erase - 删除指定索引处的条目
 */
static int test_maple_tree_basic(void)
{
	struct maple_tree mt;
	void *entry;
	int ret;
	unsigned long i;

	pr_info("=== 测试1: 基本插入/查找/删除 ===\n");

	mt_init(&mt);

	// 插入一些条目
	for (i = 0; i < 10; i++) {
		ret = mtree_insert(&mt, i, (void *)(i + 1), GFP_KERNEL);
		if (ret) {
			pr_err("插入失败 at %lu, ret=%d\n", i, ret);
			return ret;
		}
	}

	// 验证查找
	for (i = 0; i < 10; i++) {
		entry = mtree_load(&mt, i);
		if (entry != (void *)(i + 1)) {
			pr_err("查找失败 at %lu, got %p, expected %p\n",
			       i, entry, (void *)(i + 1));
			return -EINVAL;
		}
	}

	// 删除奇数索引
	for (i = 1; i < 10; i += 2) {
		entry = mtree_erase(&mt, i);
		if (entry != (void *)(i + 1)) {
			pr_err("删除失败 at %lu\n", i);
			return -EINVAL;
		}
	}

	// 验证删除后的状态
	for (i = 0; i < 10; i++) {
		entry = mtree_load(&mt, i);
		if (i % 2 == 0) {
			if (entry != (void *)(i + 1)) {
				pr_err("偶数索引应该存在 at %lu\n", i);
				return -EINVAL;
			}
		} else {
			if (entry != NULL) {
				pr_err("奇数索引应该已被删除 at %lu\n", i);
				return -EINVAL;
			}
		}
	}

	// 检查 mtree_empty
	if (mtree_empty(&mt)) {
		pr_err("树不应该为空\n");
		return -EINVAL;
	}

	mtree_destroy(&mt);

	// 重新初始化并检查为空
	mt_init(&mt);
	if (!mtree_empty(&mt)) {
		pr_err("新初始化的树应该为空\n");
		return -EINVAL;
	}
	mtree_destroy(&mt);

	pr_info("基本插入/查找/删除测试通过\n");
	return 0;
}

/*
 * 测试2: 范围操作
 * mtree_insert_range - 在范围 [first, last] 内插入相同条目
 * mtree_store_range - 在范围 [first, last] 内存储条目（覆盖已有）
 */
static int test_maple_tree_range(void)
{
	struct maple_tree mt;
	void *entry;
	int ret;
	unsigned long i;

	pr_info("=== 测试2: 范围操作 ===\n");

	mt_init(&mt);

	// 测试范围插入
	ret = mtree_insert_range(&mt, 100, 199, TEST_PTR_1, GFP_KERNEL);
	if (ret) {
		pr_err("范围插入失败, ret=%d\n", ret);
		return ret;
	}

	// 验证范围内所有条目
	for (i = 100; i <= 199; i++) {
		entry = mtree_load(&mt, i);
		if (entry != TEST_PTR_1) {
			pr_err("范围查找失败 at %lu, got %p\n", i, entry);
			return -EINVAL;
		}
	}

	// 范围外的应该为 NULL
	entry = mtree_load(&mt, 99);
	if (entry != NULL) {
		pr_err("范围外应该为 NULL at 99\n");
		return -EINVAL;
	}
	entry = mtree_load(&mt, 200);
	if (entry != NULL) {
		pr_err("范围外应该为 NULL at 200\n");
		return -EINVAL;
	}

	// 测试重叠插入应该失败
	ret = mtree_insert_range(&mt, 150, 250, TEST_PTR_2, GFP_KERNEL);
	if (ret != -EEXIST) {
		pr_err("重叠插入应该返回 -EEXIST, got %d\n", ret);
		return -EINVAL;
	}

	// 测试 store_range（可以覆盖）
	ret = mtree_store_range(&mt, 150, 250, TEST_PTR_2, GFP_KERNEL);
	if (ret) {
		pr_err("store_range 失败, ret=%d\n", ret);
		return ret;
	}

	// 验证覆盖后的值
	for (i = 150; i <= 250; i++) {
		entry = mtree_load(&mt, i);
		if (entry != TEST_PTR_2) {
			pr_err("store_range 后查找失败 at %lu\n", i);
			return -EINVAL;
		}
	}

	// 100-149 应该保持原值
	for (i = 100; i < 150; i++) {
		entry = mtree_load(&mt, i);
		if (entry != TEST_PTR_1) {
			pr_err("未覆盖区域应该保持原值 at %lu\n", i);
			return -EINVAL;
		}
	}

	mtree_destroy(&mt);
	pr_info("范围操作测试通过\n");
	return 0;
}

/*
 * 测试3: 遍历操作
 * mt_find - 从指定索引开始查找下一个非空条目
 * mt_find_after - 从指定索引之后开始查找
 * mt_next - 获取下一个条目
 * mt_prev - 获取上一个条目
 * mas_for_each - 使用 maple state 遍历
 */
static int test_maple_tree_iterate(void)
{
	struct maple_tree mt;
	unsigned long index;
	void *entry;
	int count = 0;
	MA_STATE(mas, &mt, 0, 0);

	pr_info("=== 测试3: 遍历操作 ===\n");

	mt_init(&mt);

	// 插入稀疏的条目
	mtree_insert(&mt, 10, (void *)10UL, GFP_KERNEL);
	mtree_insert(&mt, 20, (void *)20UL, GFP_KERNEL);
	mtree_insert(&mt, 30, (void *)30UL, GFP_KERNEL);
	mtree_insert(&mt, 100, (void *)100UL, GFP_KERNEL);
	mtree_insert(&mt, 200, (void *)200UL, GFP_KERNEL);

	// 测试 mt_find
	index = 0;
	entry = mt_find(&mt, &index, 500);
	if (entry != (void *)10UL || index != 10) {
		pr_err("mt_find 失败, expected (10, 10), got (%lu, %p)\n",
		       index, entry);
		return -EINVAL;
	}

	// 继续查找下一个
	entry = mt_find_after(&mt, &index, 500);
	if (entry != (void *)20UL || index != 20) {
		pr_err("mt_find_after 失败, expected (20, 20), got (%lu, %p)\n",
		       index, entry);
		return -EINVAL;
	}

	// 测试 mas_for_each 遍历
	pr_info("mas_for_each 遍历结果: ");
	mas_lock(&mas);
	mas_for_each(&mas, entry, 500) {
		pr_cont("[%lu-%lu]=%p ", mas.index, mas.last, entry);
		count++;
	}
	mas_unlock(&mas);
	pr_cont("\n");

	if (count != 5) {
		pr_err("遍历应该找到 5 个条目, got %d\n", count);
		return -EINVAL;
	}

	// 测试反向查找
	mas_set(&mas, 250);
	rcu_read_lock();
	entry = mas_find_rev(&mas, 0);
	if (entry != (void *)200UL) {
		pr_err("反向查找失败, expected 200, got %p\n", entry);
		rcu_read_unlock();
		return -EINVAL;
	}
	rcu_read_unlock();

	mtree_destroy(&mt);
	pr_info("遍历操作测试通过\n");
	return 0;
}

/*
 * 测试4: 分配范围 (ALLOC_RANGE)
 * 用于查找并分配空闲区域，常用于 mmap 等场景
 * MT_FLAGS_ALLOC_RANGE - 启用 gap 跟踪
 * mas_empty_area - 查找指定大小的空闲区域
 * mtree_alloc_range - 分配指定大小的范围
 */
static int test_maple_tree_alloc(void)
{
	struct maple_tree mt;
	unsigned long start;
	int ret;

	pr_info("=== 测试4: 分配范围 ===\n");

	// 使用 ALLOC_RANGE 标志初始化
	mt_init_flags(&mt, MT_FLAGS_ALLOC_RANGE);

	// 先占据一些区域
	mtree_insert_range(&mt, 0, 99, TEST_PTR_1, GFP_KERNEL);
	mtree_insert_range(&mt, 200, 299, TEST_PTR_2, GFP_KERNEL);
	mtree_insert_range(&mt, 500, ULONG_MAX, TEST_PTR_3, GFP_KERNEL);

	// 查找 50 大小的空闲区域
	{
		MA_STATE(mas, &mt, 0, 0);
		mas_lock(&mas);
		ret = mas_empty_area(&mas, 0, 1000, 50);
		mas_unlock(&mas);
		if (ret) {
			pr_err("mas_empty_area 失败, ret=%d\n", ret);
			return ret;
		}
		pr_info("找到空闲区域 [%lu-%lu] 大小 %lu\n",
			mas.index, mas.last, mas.last - mas.index + 1);
		if (mas.index != 100 || mas.last != 149) {
			pr_err("期望找到 [100-149]\n");
			return -EINVAL;
		}
	}

	// 测试 mtree_alloc_range
	start = 0;
	ret = mtree_alloc_range(&mt, &start, TEST_PTR_1, 50, 300, 499, GFP_KERNEL);
	if (ret) {
		pr_err("mtree_alloc_range 失败, ret=%d\n", ret);
		return ret;
	}
	pr_info("分配范围从 %lu 开始\n", start);
	if (start != 300) {
		pr_err("期望分配到 300\n");
		return -EINVAL;
	}

	mtree_destroy(&mt);
	pr_info("分配范围测试通过\n");
	return 0;
}

/*
 * 测试5: 使用 maple state 的高级操作
 * mas_walk - 遍历到指定索引
 * mas_store - 存储条目
 * mas_erase - 删除条目
 * mas_find - 查找条目
 */
static int test_maple_tree_state(void)
{
	struct maple_tree mt;
	void *entry;
	MA_STATE(mas, &mt, 50, 50);

	pr_info("=== 测试5: Maple State 操作 ===\n");

	mt_init(&mt);

	// 使用 mas 插入
	mas_lock(&mas);
	mas_store_gfp(&mas, TEST_PTR_1, GFP_KERNEL);
	mas_unlock(&mas);

	// 使用 mas_walk 查找
	mas_set(&mas, 50);
	mas_lock(&mas);
	entry = mas_walk(&mas);
	mas_unlock(&mas);
	if (entry != TEST_PTR_1) {
		pr_err("mas_walk 失败\n");
		return -EINVAL;
	}

	// 存储范围
	mas_set_range(&mas, 60, 70);
	mas_lock(&mas);
	mas_store_gfp(&mas, TEST_PTR_2, GFP_KERNEL);
	mas_unlock(&mas);

	// 验证范围
	entry = mtree_load(&mt, 65);
	if (entry != TEST_PTR_2) {
		pr_err("mas_store range 失败\n");
		return -EINVAL;
	}

	// 使用 mas_erase 删除
	mas_set(&mas, 50);
	mas_lock(&mas);
	mas_erase(&mas);
	mas_unlock(&mas);

	entry = mtree_load(&mt, 50);
	if (entry != NULL) {
		pr_err("mas_erase 失败\n");
		return -EINVAL;
	}

	mtree_destroy(&mt);
	pr_info("Maple State 操作测试通过\n");
	return 0;
}

/*
 * 测试6: RCU 模式
 * MT_FLAGS_USE_RCU - 启用 RCU 模式
 * mt_set_in_rcu - 切换到 RCU 模式
 * mt_clear_in_rcu - 退出 RCU 模式
 */
static int test_maple_tree_rcu(void)
{
	struct maple_tree mt;
	void *entry;

	pr_info("=== 测试6: RCU 模式 ===\n");

	mt_init(&mt);

	// 插入一些数据
	mtree_insert(&mt, 1, TEST_PTR_1, GFP_KERNEL);
	mtree_insert(&mt, 2, TEST_PTR_2, GFP_KERNEL);

	// 切换到 RCU 模式
	mt_set_in_rcu(&mt);
	pr_info("树已切换到 RCU 模式\n");

	// RCU 读锁下访问
	rcu_read_lock();
	entry = mtree_load(&mt, 1);
	if (entry != TEST_PTR_1) {
		pr_err("RCU 模式下查找失败\n");
		rcu_read_unlock();
		return -EINVAL;
	}
	rcu_read_unlock();

	// 退出 RCU 模式
	mt_clear_in_rcu(&mt);
	pr_info("树已退出 RCU 模式\n");

	mtree_destroy(&mt);
	pr_info("RCU 模式测试通过\n");
	return 0;
}

/*
 * 测试7: 大规模数据测试
 * 测试树的平衡和性能
 */
static int test_maple_tree_large_scale(void)
{
	struct maple_tree mt;
	unsigned long i;
	void *entry;
	unsigned long start_time, end_time;

	pr_info("=== 测试7: 大规模数据 ===\n");

	mt_init(&mt);

	// 插入 10000 个条目
	start_time = jiffies;
	for (i = 0; i < 10000; i++) {
		mtree_insert(&mt, i * 10, (void *)(i + 1), GFP_KERNEL);
	}
	end_time = jiffies;
	pr_info("插入 10000 个条目耗时 %u ms\n",
		jiffies_to_msecs(end_time - start_time));

	// 验证所有条目
	start_time = jiffies;
	for (i = 0; i < 10000; i++) {
		entry = mtree_load(&mt, i * 10);
		if (entry != (void *)(i + 1)) {
			pr_err("大规模查找失败 at %lu\n", i * 10);
			return -EINVAL;
		}
	}
	end_time = jiffies;
	pr_info("查找 10000 个条目耗时 %u ms\n",
		jiffies_to_msecs(end_time - start_time));

	pr_info("树高度: %u\n", mt_height(&mt));

	mtree_destroy(&mt);
	pr_info("大规模数据测试通过\n");
	return 0;
}

/*
 * 测试8: 复制树
 * mtree_dup - 复制 maple tree
 */
static int test_maple_tree_dup(void)
{
	struct maple_tree mt_src, mt_dst;
	void *entry;
	int ret;

	pr_info("=== 测试8: 复制树 ===\n");

	mt_init(&mt_src);
	mt_init(&mt_dst);

	// 在源树中插入数据
	mtree_insert(&mt_src, 1, TEST_PTR_1, GFP_KERNEL);
	mtree_insert(&mt_src, 2, TEST_PTR_2, GFP_KERNEL);
	mtree_insert(&mt_src, 3, TEST_PTR_3, GFP_KERNEL);

	// 复制
	ret = mtree_dup(&mt_src, &mt_dst, GFP_KERNEL);
	if (ret) {
		pr_err("mtree_dup 失败, ret=%d\n", ret);
		return ret;
	}

	// 验证复制后的数据
	entry = mtree_load(&mt_dst, 1);
	if (entry != TEST_PTR_1) {
		pr_err("复制后数据不匹配 at 1\n");
		return -EINVAL;
	}
	entry = mtree_load(&mt_dst, 2);
	if (entry != TEST_PTR_2) {
		pr_err("复制后数据不匹配 at 2\n");
		return -EINVAL;
	}

	// 修改源树不影响目标树
	mtree_insert(&mt_src, 4, (void *)0x44444444UL, GFP_KERNEL);
	entry = mtree_load(&mt_dst, 4);
	if (entry != NULL) {
		pr_err("复制应该是独立的\n");
		return -EINVAL;
	}

	mtree_destroy(&mt_src);
	mtree_destroy(&mt_dst);
	pr_info("复制树测试通过\n");
	return 0;
}

int test_maple_tree(long action)
{
	int ret;

	switch (action) {
	case 0:
		ret = test_maple_tree_basic();
		break;
	case 1:
		ret = test_maple_tree_range();
		break;
	case 2:
		ret = test_maple_tree_iterate();
		break;
	case 3:
		ret = test_maple_tree_alloc();
		break;
	case 4:
		ret = test_maple_tree_state();
		break;
	case 5:
		ret = test_maple_tree_rcu();
		break;
	case 6:
		ret = test_maple_tree_large_scale();
		break;
	case 7:
		ret = test_maple_tree_dup();
		break;
	case 100:
		// 运行所有测试
		pr_info("========== 运行所有 Maple Tree 测试 ==========\n");
		ret = test_maple_tree_basic();
		if (ret) return ret;
		ret = test_maple_tree_range();
		if (ret) return ret;
		ret = test_maple_tree_iterate();
		if (ret) return ret;
		ret = test_maple_tree_alloc();
		if (ret) return ret;
		ret = test_maple_tree_state();
		if (ret) return ret;
		ret = test_maple_tree_rcu();
		if (ret) return ret;
		ret = test_maple_tree_large_scale();
		if (ret) return ret;
		ret = test_maple_tree_dup();
		if (ret) return ret;
		pr_info("========== 所有 Maple Tree 测试通过 ==========\n");
		break;
	default:
		pr_err("未知 action: %ld\n", action);
		pr_info("可用 action:\n");
		pr_info("  0 - 基本插入/查找/删除\n");
		pr_info("  1 - 范围操作\n");
		pr_info("  2 - 遍历操作\n");
		pr_info("  3 - 分配范围 (ALLOC_RANGE)\n");
		pr_info("  4 - Maple State 高级操作\n");
		pr_info("  5 - RCU 模式\n");
		pr_info("  6 - 大规模数据\n");
		pr_info("  7 - 复制树\n");
		pr_info("  100 - 运行所有测试\n");
		return -EINVAL;
	}

	return ret;
}
