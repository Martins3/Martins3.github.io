#include "internal.h"
#include <linux/module.h>

#define CREATE_TRACE_POINTS
#include "tracepoint.h"

/* https://lwn.net/Articles/379903/
 * 
 * 注意，header 可以多次 include ，但是 CREATE_TRACE_POINTS 只能出现一次
 * 
 * 所以 参考 io_uring/io_uring.h 中的写法
 * #ifndef CREATE_TRACE_POINTS
 * #include <trace/events/io_uring.h>
 * #endif
 * 
 * 大多数模块的 trace 都是放到单独的模块中，例如 include/trace/events/erofs.h
 * 外部模块可以参考这个 drivers/net/thunderbolt/ 来写，基本操作为:
 * 1. 写一个 header
 * 2. 定义 CREATE_TRACE_POINTS ，并且 #include "tracepoint.h"
 * 3. Makefile 中添加
 */

/*
 * 在 TRACE_EVENT 是自动定义了 register_trace_mini 这种函数
 */
static void mini_hook(void *ignore, int m)
{
	pr_info("mini hook %d\n", m);
	// dump_stack();
}

int test_tracepoint_init(void)
{
	int ret;
	ret = register_trace_mini(mini_hook, NULL);
	WARN_ON(ret);
	return 0;
}

int test_tracepoint_exit(void)
{
	unregister_trace_mini(mini_hook, NULL);
	return 0;
}

int test_tracepoint(long action)
{
	/* 
	 * 不要在生产环境中使用 trace_printk
	 */
	// trace_printk("hello\n");

	// tracepoint 的函数可以多次调用
	// 使用 perf top -e 来观察
	trace_hack_eventname(action);
	trace_hack_eventname(action + 1);

	trace_mini(0);

	/*
	 * 由于 register_trace_mini ，所以导致 mini 被打开
	 */
	if (trace_mini_enabled())
		pr_info("tracepoint mini is enabled\n");
	else
		pr_info("tracepoint mini is not enabled\n");

	if (trace_hack_eventname_enabled())
		pr_info("tracepoint hack_eventname is enabled\n");
	else
		pr_info("tracepoint hack_eventname is not enabled\n");

	return 0;
}
