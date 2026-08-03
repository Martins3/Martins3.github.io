#include <linux/cleanup.h>
#include <linux/mutex.h>
#include "internal.h"

/*
 * 实现原理参考:
 * https://www.marcusfolkesson.se/blog/mutex-guards-in-the-linux-kernel/
 * 利用了 gcc / clang 提供的  __attribute__((cleanup()))
 */

static void test_guard_preempt(void)
{
	guard(preempt)();
}

static void test_guard_migrate(void)
{
	guard(migrate)();
}

static void test_guard_rcu(void)
{
	guard(rcu)();
}

static void test_scope(void)
{
	scoped_guard(irqsave)
	{
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	}
}

static DEFINE_MUTEX(test_mutex_abc);
static void test_guard_mutex(void)
{
	guard(mutex)(&test_mutex_abc);
}

/* 
 * 1. 从 include/linux/cleanup.h 注释看，__free 是实现 guard mutex 的基础
 *
 * 2. 类似的还有的机制为:
 *   static int kvm_vfio_file_del(struct kvm_device *dev, unsigned int fd)
 *  {
 *	struct kvm_vfio *kv = dev->private;
 *	struct kvm_vfio_file *kvf;
 *	CLASS(fd, f)(fd);
 *	int ret;
 *
 *	if (fd_empty(f))
 *		return -EBADF;
 *
 */
static int test_cleanup(void)
{
	char *buf __free(kfree);
	buf = kmalloc(128, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	strcpy(buf, "Hello, __free!");
	pr_info("Normal exit, buf: %s\n", buf);
	return 0;
}

int test_guard(long action)
{
	switch (action) {
	case 0:
		test_guard_mutex();
		test_guard_rcu();
		test_scope();
		test_guard_preempt();
		test_guard_migrate();
		break;
	case 1:
		break;
	}
	return 0;
}
