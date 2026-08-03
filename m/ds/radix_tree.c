#include "internal.h"
#include <linux/radix-tree.h>
#include <linux/slab.h>

/*
 * Radix Tree 测试
 * 
 * Radix Tree 是 Linux 内核中用于将整数索引映射到指针的数据结构。
 * 它被广泛用于文件系统缓存、页缓存等场景。
 */

struct test_item {
	unsigned long index;
	int data;
};

static RADIX_TREE(test_tree, GFP_KERNEL);

/* 基本操作测试：插入、查找、删除 */
static int test_basic_operations(void)
{
	struct test_item *item;
	struct test_item *found;
	int ret;
	int i;

	pr_info("=== 测试基本操作 ===\n");

	/* 插入一些元素 */
	for (i = 0; i < 10; i++) {
		item = kmalloc(sizeof(*item), GFP_KERNEL);
		if (!item)
			return -ENOMEM;
		
		item->index = i * 100;
		item->data = i * 10;
		
		ret = radix_tree_insert(&test_tree, item->index, item);
		if (ret) {
			pr_err("插入失败: index=%lu, ret=%d\n", item->index, ret);
			kfree(item);
			return ret;
		}
		pr_info("插入: index=%lu, data=%d\n", item->index, item->data);
	}

	/* 查找元素 */
	for (i = 0; i < 10; i++) {
		unsigned long idx = i * 100;
		found = radix_tree_lookup(&test_tree, idx);
		if (!found) {
			pr_err("查找失败: index=%lu\n", idx);
			return -ENOENT;
		}
		if (found->data != i * 10) {
			pr_err("数据不匹配: index=%lu, expected=%d, got=%d\n",
			       idx, i * 10, found->data);
			return -EINVAL;
		}
		pr_info("查找成功: index=%lu, data=%d\n", idx, found->data);
	}

	/* 查找不存在的元素 */
	found = radix_tree_lookup(&test_tree, 999);
	if (found) {
		pr_err("应该返回 NULL 但却找到了元素\n");
		return -EINVAL;
	}
	pr_info("查找不存在的元素返回 NULL，正确\n");

	/* 删除元素 */
	for (i = 0; i < 10; i++) {
		unsigned long idx = i * 100;
		found = radix_tree_delete(&test_tree, idx);
		if (!found) {
			pr_err("删除失败: index=%lu\n", idx);
			return -ENOENT;
		}
		pr_info("删除: index=%lu, data=%d\n", idx, found->data);
		kfree(found);
	}

	/* 验证树为空 */
	if (!radix_tree_empty(&test_tree)) {
		pr_err("树应该为空但却不为空\n");
		return -EINVAL;
	}
	pr_info("树已为空，测试通过\n");

	return 0;
}

/* 测试重复插入 */
static int test_duplicate_insert(void)
{
	struct test_item *item1, *item2, *found;
	int ret;

	pr_info("=== 测试重复插入 ===\n");

	item1 = kmalloc(sizeof(*item1), GFP_KERNEL);
	if (!item1)
		return -ENOMEM;
	item1->index = 50;
	item1->data = 100;

	ret = radix_tree_insert(&test_tree, 50, item1);
	if (ret) {
		kfree(item1);
		return ret;
	}

	/* 尝试重复插入相同索引 */
	item2 = kmalloc(sizeof(*item2), GFP_KERNEL);
	if (!item2) {
		radix_tree_delete(&test_tree, 50);
		kfree(item1);
		return -ENOMEM;
	}
	item2->index = 50;
	item2->data = 200;

	ret = radix_tree_insert(&test_tree, 50, item2);
	if (ret != -EEXIST) {
		pr_err("重复插入应该返回 -EEXIST，但返回了 %d\n", ret);
		kfree(item2);
		radix_tree_delete(&test_tree, 50);
		kfree(item1);
		return -EINVAL;
	}
	pr_info("重复插入正确返回 -EEXIST\n");

	/* 清理 */
	found = radix_tree_delete(&test_tree, 50);
	if (found)
		kfree(found);
	kfree(item2);

	return 0;
}

/* 测试标签功能 */
static int test_tags(void)
{
	struct test_item *items[5];
	int i, ret;
	void *tagged;

	pr_info("=== 测试标签功能 ===\n");

	/* 插入元素 */
	for (i = 0; i < 5; i++) {
		items[i] = kmalloc(sizeof(*items[i]), GFP_KERNEL);
		if (!items[i]) {
			ret = -ENOMEM;
			goto cleanup;
		}
		items[i]->index = i;
		items[i]->data = i * 10;
		
		ret = radix_tree_insert(&test_tree, i, items[i]);
		if (ret) {
			kfree(items[i]);
			goto cleanup;
		}
	}

	/* 设置标签 */
	for (i = 0; i < 5; i += 2) {
		tagged = radix_tree_tag_set(&test_tree, i, 0);
		if (!tagged) {
			pr_err("设置标签失败: index=%d\n", i);
			ret = -EINVAL;
			goto cleanup;
		}
		pr_info("设置标签: index=%d\n", i);
	}

	/* 检查标签 */
	for (i = 0; i < 5; i++) {
		ret = radix_tree_tag_get(&test_tree, i, 0);
		if (i % 2 == 0) {
			if (!ret) {
				pr_err("标签应该存在: index=%d\n", i);
				ret = -EINVAL;
				goto cleanup;
			}
		} else {
			if (ret) {
				pr_err("标签不应该存在: index=%d\n", i);
				ret = -EINVAL;
				goto cleanup;
			}
		}
	}
	pr_info("标签检查通过\n");

	/* 清除标签 */
	radix_tree_tag_clear(&test_tree, 0, 0);
	ret = radix_tree_tag_get(&test_tree, 0, 0);
	if (ret) {
		pr_err("清除标签后应该不存在\n");
		ret = -EINVAL;
		goto cleanup;
	}
	pr_info("清除标签成功\n");

	ret = 0;

cleanup:
	/* 清理 */
	for (i = 0; i < 5; i++) {
		void *item = radix_tree_delete(&test_tree, i);
		if (item)
			kfree(item);
	}
	return ret;
}

/* 测试 gang lookup */
static int test_gang_lookup(void)
{
	struct test_item *items[10];
	struct test_item *results[5];
	unsigned int found;
	int i, ret;

	pr_info("=== 测试 Gang Lookup ===\n");

	/* 插入元素 */
	for (i = 0; i < 10; i++) {
		items[i] = kmalloc(sizeof(*items[i]), GFP_KERNEL);
		if (!items[i]) {
			ret = -ENOMEM;
			goto cleanup;
		}
		items[i]->index = i * 10;
		items[i]->data = i;
		
		ret = radix_tree_insert(&test_tree, items[i]->index, items[i]);
		if (ret) {
			kfree(items[i]);
			goto cleanup;
		}
	}

	/* gang lookup */
	found = radix_tree_gang_lookup(&test_tree, (void **)results, 0, 5);
	if (found != 5) {
		pr_err("期望找到 5 个，但找到 %u 个\n", found);
		ret = -EINVAL;
		goto cleanup;
	}
	pr_info("找到 %u 个元素\n", found);

	for (i = 0; i < found; i++) {
		pr_info("  results[%d]: index=%lu, data=%d\n",
			i, results[i]->index, results[i]->data);
	}

	/* 从中间开始查找 */
	found = radix_tree_gang_lookup(&test_tree, (void **)results, 50, 3);
	pr_info("从 index=50 开始找到 %u 个元素\n", found);

	ret = 0;

cleanup:
	for (i = 0; i < 10; i++) {
		void *item = radix_tree_delete(&test_tree, i * 10);
		if (item)
			kfree(item);
	}
	return ret;
}

/* 测试迭代器 */
static int test_iterator(void)
{
	struct test_item *items[10];
	struct radix_tree_iter iter;
	void __rcu **slot;
	int count = 0;
	int i, ret;

	pr_info("=== 测试迭代器 ===\n");

	/* 插入元素 */
	for (i = 0; i < 10; i++) {
		items[i] = kmalloc(sizeof(*items[i]), GFP_KERNEL);
		if (!items[i]) {
			ret = -ENOMEM;
			goto cleanup;
		}
		items[i]->index = i * 5;
		items[i]->data = i * 100;
		
		ret = radix_tree_insert(&test_tree, items[i]->index, items[i]);
		if (ret) {
			kfree(items[i]);
			goto cleanup;
		}
	}

	/* 遍历所有元素 */
	radix_tree_for_each_slot(slot, &test_tree, &iter, 0) {
		struct test_item *item = radix_tree_deref_slot_protected(
			slot, NULL);
		if (item) {
			pr_info("迭代: index=%lu, data=%d\n",
				iter.index, item->data);
			count++;
		}
	}

	if (count != 10) {
		pr_err("期望遍历到 10 个元素，但只遍历到 %d 个\n", count);
		ret = -EINVAL;
		goto cleanup;
	}
	pr_info("成功遍历所有 %d 个元素\n", count);

	ret = 0;

cleanup:
	for (i = 0; i < 10; i++) {
		void *item = radix_tree_delete(&test_tree, i * 5);
		if (item)
			kfree(item);
	}
	return ret;
}

/* 测试大索引 */
static int test_large_indices(void)
{
	struct test_item *item, *found;
	unsigned long indices[] = {0, 1000, 1000000, ULONG_MAX / 2};
	int i, ret;

	pr_info("=== 测试大索引 ===\n");

	for (i = 0; i < ARRAY_SIZE(indices); i++) {
		item = kmalloc(sizeof(*item), GFP_KERNEL);
		if (!item)
			return -ENOMEM;
		
		item->index = indices[i];
		item->data = i;
		
		ret = radix_tree_insert(&test_tree, item->index, item);
		if (ret) {
			pr_err("插入大索引失败: index=%lu\n", item->index);
			kfree(item);
			return ret;
		}
		pr_info("插入大索引: index=%lu\n", item->index);
	}

	/* 查找 */
	for (i = 0; i < ARRAY_SIZE(indices); i++) {
		found = radix_tree_lookup(&test_tree, indices[i]);
		if (!found) {
			pr_err("查找大索引失败: index=%lu\n", indices[i]);
			return -ENOENT;
		}
		if (found->data != i) {
			pr_err("数据不匹配\n");
			return -EINVAL;
		}
		pr_info("查找大索引成功: index=%lu, data=%d\n",
			indices[i], found->data);
	}

	/* 清理 */
	for (i = 0; i < ARRAY_SIZE(indices); i++) {
		found = radix_tree_delete(&test_tree, indices[i]);
		if (found)
			kfree(found);
	}

	return 0;
}

/* 测试标签批量查找 */
static int test_tagged_lookup(void)
{
	struct test_item *items[20];
	struct test_item *results[10];
	unsigned int found;
	int i, ret;

	pr_info("=== 测试带标签的 Gang Lookup ===\n");

	/* 插入元素 */
	for (i = 0; i < 20; i++) {
		items[i] = kmalloc(sizeof(*items[i]), GFP_KERNEL);
		if (!items[i]) {
			ret = -ENOMEM;
			goto cleanup;
		}
		items[i]->index = i;
		items[i]->data = i;
		
		ret = radix_tree_insert(&test_tree, i, items[i]);
		if (ret) {
			kfree(items[i]);
			goto cleanup;
		}

		/* 给偶数索引设置标签 */
		if (i % 2 == 0)
			radix_tree_tag_set(&test_tree, i, 0);
	}

	/* 查找带标签的元素 */
	found = radix_tree_gang_lookup_tag(&test_tree, (void **)results,
					   0, 10, 0);
	pr_info("找到 %u 个带标签的元素\n", found);

	for (i = 0; i < found; i++) {
		if (results[i]->index % 2 != 0) {
			pr_err("不应该找到奇数索引\n");
			ret = -EINVAL;
			goto cleanup;
		}
	}
	pr_info("带标签查找测试通过\n");

	ret = 0;

cleanup:
	for (i = 0; i < 20; i++) {
		void *item = radix_tree_delete(&test_tree, i);
		if (item)
			kfree(item);
	}
	return ret;
}

int test_radix_tree(long action)
{
	int ret;

	switch (action) {
	case 0:
		/* 运行所有测试 */
		ret = test_basic_operations();
		if (ret)
			return ret;
		ret = test_duplicate_insert();
		if (ret)
			return ret;
		ret = test_tags();
		if (ret)
			return ret;
		ret = test_gang_lookup();
		if (ret)
			return ret;
		ret = test_iterator();
		if (ret)
			return ret;
		ret = test_large_indices();
		if (ret)
			return ret;
		ret = test_tagged_lookup();
		if (ret)
			return ret;
		pr_info("所有测试通过!\n");
		break;
	case 1:
		return test_basic_operations();
	case 2:
		return test_duplicate_insert();
	case 3:
		return test_tags();
	case 4:
		return test_gang_lookup();
	case 5:
		return test_iterator();
	case 6:
		return test_large_indices();
	case 7:
		return test_tagged_lookup();
	default:
		pr_info("未知 action: %ld\n", action);
		pr_info("可用测试:\n");
		pr_info("  0 - 运行所有测试\n");
		pr_info("  1 - 基本操作测试\n");
		pr_info("  2 - 重复插入测试\n");
		pr_info("  3 - 标签功能测试\n");
		pr_info("  4 - Gang Lookup 测试\n");
		pr_info("  5 - 迭代器测试\n");
		pr_info("  6 - 大索引测试\n");
		pr_info("  7 - 带标签查找测试\n");
		return -EINVAL;
	}
	return 0;
}
