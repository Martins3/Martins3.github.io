#include "internal.h"

/*
 * 看内部实现，最后是将 ctl_table_header 添加到 rb tree 中:
 *
 * struct ctl_node {
 *     struct rb_node node;
 *     struct ctl_table_header *header;
 * };
 *
 * 注册的大致流程为:
 * @[
 *    __register_sysctl_table+5
 *    test_sysctl_init+33
 *    sysctl_store+99
 *    kernfs_fop_write_iter+289
 *    vfs_write+665
 *    ksys_write+110
 *    do_syscall_64+95
 *    entry_SYSCALL_64_after_hwframe+118
 *  ]: 1
 */

/*
 * 参考 drop_caches_sysctl_handler 的实现，的确好用，很简单
 */
static int foo = 12;
static int bar = 34;
static int hugetlb_sysctl_handler(const struct ctl_table *table, int write,
				  void *buffer, size_t *length, loff_t *ppos)
{
	int ret = proc_dointvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		return ret;
	if (write) {
		pr_info("foo=%d bar=%d\n", foo, bar);
	}
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return ret;
}

static struct ctl_table hugetlb_table[] = {
	{
		.procname = "foo",
		.data = &foo,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = hugetlb_sysctl_handler,
	},
	{
		.procname = "bar",
		.data = &bar,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = hugetlb_sysctl_handler,
	},
};
static struct ctl_table_header *cdrom_sysctl_header;

int test_sysctl_init(void)
{
	/* 
	 * mod sysctl 0
	 * 那么会增加一个目录 /proc/sys/martins3/test
	 */

	cdrom_sysctl_header = register_sysctl("martins3/test", hugetlb_table);
	return 0;
}

int test_sysctl_exit(void)
{
	unregister_sysctl_table(cdrom_sysctl_header);
	return 0;
}

int test_sysctl(long action)
{
	switch (action) {
	}
	return 0;
}
