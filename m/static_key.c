#include "internal.h"
#include <linux/static_call.h>
// 配套文档 : docs/kernel/compiler.md

// 参考 arch/x86/kernel/process.c 中的 DEFINE_STATIC_CALL_NULL(x86_idle, default_idle);
// 来实现的
//
// TODO 用户态实现 https://github.com/dtcxzyw/static-key
// [ ] 这里的动态修改函数指针应该是配合 icache flush 吧?

static void foo(void)
{
	pr_info("call foo\n");
}

static void bar(void)
{
	pr_info("call bar\n");
}
DEFINE_STATIC_CALL_NULL(x86_idle, foo);

// 参考 arch/x86/events/core.c 中 perf_is_hybrid
DEFINE_STATIC_KEY_FALSE(perf_is_hybrid);
#define is_hybrid() static_branch_unlikely(&perf_is_hybrid)

/*
 * 如果没有注册，那么 static_call 不会调用任何一个函数。
 * 如果注册了，那么就是调用对应的函数。 只能说，使用起来真的简单
 */
static void basic_api(void)
{
	for (size_t i = 0; i < 1000; i++) {
		static_call(x86_idle)();
		pr_info("%d\n", !!static_call_query(x86_idle));
		pr_info("key : %ld\n", is_hybrid());
		if (schedule_timeout_interruptible(HZ))
			break;
	}
}

int test_static_key(long action)
{
	switch (action) {
		// 基本 API 测试
	case 0:
		basic_api();
		break;
	case 1:
		static_branch_enable(&perf_is_hybrid);
		static_call_update(x86_idle, bar);
		break;
	case 2:
		static_branch_disable(&perf_is_hybrid);
		static_call_update(x86_idle, foo);
		break;
	case 3:
		// 性能测试
		// 看看使用函数指针的方法差别有多大
		//
		// TODO 这里有我的一个一直未解之谜，既然 static key 是性能很好的，为什么 fs 的用户不去用这个来更新?
		break;
	}
	return 0;
}
