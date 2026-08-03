// 这就是最小的模块了
// 其实 module_init 不是必需的，
// 有的模块是库，所以也没有必要一定需要用 module_init
#include <linux/module.h>
#include <linux/slab.h>
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple kernel module test kernel api");

static int __init mini_init(void)
{
	return 0;
}

static void __exit mini_exit(void)
{
}

module_init(mini_init);
module_exit(mini_exit);
