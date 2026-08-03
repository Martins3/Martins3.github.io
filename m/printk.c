#include "internal.h"
#include <linux/delay.h>

/*
 * 1. 尝试测试下 printk 的并发是否会触发问题！
 * 2. 把代码反汇编一下，看看和 ftrace 的关系是什么?
 * 3. 如何使用这个:
 *
 *   mlx5_core_dbg(dev, "func 0x%x, npages %d, outlen %d\n",
 *             func_id, npages, outlen);
 *
 * 测试 dev_printk.h 一下
 *
## pr_debug

这里有趣的 nopl ，可以看看 pr_debug 的反汇编是什么。

思考下，pr_debug 和 tracepoint 有什么本质的区别吗?

TODO 如何动态打开和 开机的时候就给他打开 dev_dbg
 */

static void test_level(void)
{
	printk(KERN_DEFAULT "printk default\n");
	printk("printk\n");
	pr_info("info\n");
	pr_notice("notice\n");
	pr_debug("[[pr_debug]]\n");
	printk(KERN_DEBUG "[[printk debug]]\n");
	pr_warn("warn\n");
}
static void test_limit(void)
{
	// TODO 理解这两个参数的含义是:
	// cat /proc/sys/kernel/printk_ratelimit : 在多长的时间里面
	// cat /proc/sys/kernel/printk_ratelimit_burst : 最多打印多少的信息出来
	// 但是很奇怪的是，调整这两个接口，内容并没有什么变化
	// 是 kernel config 没有打开吗?

	for (size_t i = 0; i < 10; i++) {
		for (size_t i = 0; i < 15; i++) {
			pr_info("iter : %ld\n", i);
			pr_info_once("hi\n");
			printk_ratelimited("ratelimit prink\n");
			pr_info_ratelimited("ratelimit info\n");
			pr_warn_ratelimited("ratelimit warn\n");
			if (printk_ratelimit())
				pr_info("if ratelimit\n");
		}
		msleep(6000);
	}
}

// 这个日志可以自动穿透到所有的 ssh 吗?
// 并不会，可能需要特殊的配置吧
static void test_emergency(void)
{
	for (size_t i = 0; i < 1000; i++) {
		pr_emerg_ratelimited("%s\n", __func__);
		if (schedule_timeout_interruptible(HZ))
			break;
	}
}

/*
 *
 * 参考 Documentation/core-api/printk-formats.rst
 *
 * Time and date
 * -------------
 * 
 * ::
 * 
 * 	%pt[RT]			YYYY-mm-ddTHH:MM:SS
 * 	%pt[RT]s		YYYY-mm-dd HH:MM:SS
 * 	%pt[RT]d		YYYY-mm-dd
 * 	%pt[RT]t		HH:MM:SS
 * 	%pt[RT][dt][r][s]
 * 
 * For printing date and time as represented by::
 * 
 * 	R  struct rtc_time structure
 * 	T  time64_t type
 * 
 * in human readable format.
 * 
 * By default year will be incremented by 1900 and month by 1.
 * Use %pt[RT]r (raw) to suppress this behaviour.
 * 
 * The %pt[RT]s (space) will override ISO 8601 separator by using ' ' (space)
 * instead of 'T' (Capital T) between date and time. It won't have any effect
 * when date or time is omitted.
 * 
 * Passed by reference.
 * 
 * struct clk
 * ----------
 * 
 * ::
 * 
 * 	%pC	pll1
 * 	%pCn	pll1
 * 
 * For printing struct clk structures. %pC and %pCn print the name of the clock
 * (Common Clock Framework) or a unique 32-bit ID (legacy clock framework).
 * 
 * Passed by reference.
 *
 * Flags bitfields such as page flags and gfp_flags
 * --------------------------------------------------------
 * 
 * ::
 * 
 * 	%pGp	0x17ffffc0002036(referenced|uptodate|lru|active|private|node=0|zone=2|lastcpupid=0x1fffff)
 * 	%pGg	GFP_USER|GFP_DMA32|GFP_NOWARN
 * 	%pGv	read|exec|mayread|maywrite|mayexec|denywrite
 * 
 * For printing flags bitfields as a collection of symbolic constants that
 * would construct the value. The type of flags is given by the third
 * character. Currently supported are:
 * 
 *         - p - [p]age flags, expects value of type (``unsigned long *``)
 *         - v - [v]ma_flags, expects value of type (``unsigned long *``)
 *         - g - [g]fp_flags, expects value of type (``gfp_t *``)
 * 
 * The flag names and print order depends on the particular type.
 * 
 * Note that this format should not be used directly in the
 * :c:func:`TP_printk()` part of a tracepoint. Instead, use the show_*_flags()
 * functions from <trace/events/mmflags.h>.
 * 
 * Passed by reference.
 */
static void test_format(void)
{
	/*
	 * 但是如何获取到带有 timezone 的时间，这是一个问题
	 */
	ktime_t b = ktime_get_real() / NSEC_PER_SEC;
	pr_info("%ptT\n", &b);
}

int test_printk(long action)
{
	switch (action) {
	case 0:
		test_level();
		break;
	case 1:
		test_limit();
		break;
	case 2:
		test_emergency();
		break;
	case 3:
		test_format();
		break;
	}
	return 0;
}
