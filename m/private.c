#include "internal.h"
#include <linux/mm.h>
#include <linux/mmap_lock.h>

/**
 * 测试 ACCESS_PRIVATE 宏
 * 
 * ACCESS_PRIVATE 是内核中用于访问结构体私有字段的机制，
 * 定义在 <linux/compiler_types.h> 中:
 *   #define ACCESS_PRIVATE(p, member) ((p)->member)
 * 
 * 它允许直接访问结构体的任意字段，即使该字段在概念上是私有的。
 * 
 * 内核中的典型应用示例（mm/vm_flags.c）:
 *   static inline void vm_flags_set(struct vm_area_struct *vma,
 *                                   vm_flags_t flags)
 *   {
 *       vma_start_write(vma);
 *       ACCESS_PRIVATE(vma, __vm_flags) |= flags;
 *   }
 * 
 * 通过 ACCESS_PRIVATE，可以在不直接暴露 __vm_flags 字段的情况下，
 * 提供受控的访问接口。
 *
 * 这个需要配合 Documentation/dev-tools/sparse.rst 才可以体现效果
 * 原理容易，也就是只有通过这个宏才可以访问被 __private 标记的
 */

/* 测试用的结构体 - 模拟内核中的 vm_area_struct */
struct test_struct {
	int public_field;
	int __private __private_field;
};

/* 
 * 使用 ACCESS_PRIVATE 提供的受控访问接口
 * 类似于内核中的 vm_flags_set()/vm_flags_clear()
 */
static inline void private_field_set(struct test_struct *ts, int val)
{
	ACCESS_PRIVATE(ts, __private_field) = val;
}

static inline void private_field_or(struct test_struct *ts, int val)
{
	ACCESS_PRIVATE(ts, __private_field) |= val;
}

static inline int private_field_get(struct test_struct *ts)
{
	return ACCESS_PRIVATE(ts, __private_field);
}

/**
 * 测试 ACCESS_PRIVATE 基本功能
 * 验证可以通过 ACCESS_PRIVATE 访问和修改私有字段
 */
static void test_access_private_basic(void)
{
	struct test_struct ts;

	pr_info("=== 测试 ACCESS_PRIVATE 基本功能 ===\n");

	ts.public_field = 100;
	ts.__private_field = 200;

	pr_info("初始值: public=%d, private=%d\n", ts.public_field,
		ts.__private_field);

	/* 使用 ACCESS_PRIVATE 访问私有字段 */
	ACCESS_PRIVATE(&ts, __private_field) = 300;

	pr_info("通过 ACCESS_PRIVATE 修改后: public=%d, private=%d\n",
		ts.public_field, ts.__private_field);

	if (ts.__private_field == 300) {
		pr_info("成功: ACCESS_PRIVATE 可以修改私有字段\n");
	} else {
		pr_err("失败: ACCESS_PRIVATE 修改未生效\n");
	}

	/* 验证读取 */
	if (ACCESS_PRIVATE(&ts, __private_field) == 300) {
		pr_info("成功: ACCESS_PRIVATE 可以读取私有字段\n");
	}

	/* 演示通过受控接口访问 */
	private_field_set(&ts, 400);
	pr_info("通过受控接口修改后: private=%d\n", private_field_get(&ts));
}

int test_private(long action)
{
	switch (action) {
	case 0:
		pr_info("测试 ACCESS_PRIVATE 基本功能\n");
		test_access_private_basic();
		break;
	default:
		pr_err("未知的 action: %ld\n", action);
		return -EINVAL;
	}
	return 0;
}
