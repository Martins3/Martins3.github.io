#include "internal.h"
// https://stackoverflow.com/questions/10459688/what-is-the-asmlinkage-modifier-meant-for

#ifdef CONFIG_X86_64
static asmlinkage void my_asmlinkage_func(int a, int b, int c)
{
	pr_info("asmlinkage: a=%d, b=%d, c=%d\n", a, b, c);
}

static void my_normal_func(int a, int b, int c)
{
	printk(KERN_INFO "normal: a=%d, b=%d, c=%d\n", a, b, c);
}

static int test(void)
{
	printk(KERN_INFO "Testing asmlinkage...\n");

	// 使用内联汇编调用 my_asmlinkage_func
	// 注意：我们手动压栈，然后调用
	asm volatile("push $3\n\t" // 第三个参数 c
		     "push $2\n\t" // 第二个参数 b
		     "push $1\n\t" // 第一个参数 a
		     "call %P0\n\t" // 调用函数
		     "add $12, %%esp\n\t" // 平衡栈（清除 3 个 int）
		     :
		     : "i"(my_asmlinkage_func)
		     : "memory");

	// 调用普通函数（由编译器处理）
	my_normal_func(4, 5, 6);

	return 0;
}
#endif

#ifdef CONFIG_ARM64
static asmlinkage void my_arm64_syscall_entry(long arg1, long arg2, long arg3)
{
	printk(KERN_INFO "ARM64 syscall entry: arg1=%ld, arg2=%ld, arg3=%ld\n",
	       arg1, arg2, arg3);
}

static void normal_c_function(long a, long b, long c)
{
	printk(KERN_INFO "Normal C func: a=%ld, b=%ld, c=%ld\n", a, b, c);
}

static int test_asmlinkage(void)
{
	printk(KERN_INFO "Testing asmlinkage on ARM64...\n");

	// 使用内联汇编调用 my_arm64_syscall_entry
	// 按照 AAPCS64：x0, x1, x2 传参

	asm volatile("mov x9, %x0\n\t" // 将函数地址加载到临时寄存器 x9
		     "mov x0, %x1\n\t" // arg1 = 100
		     "mov x1, %x2\n\t" // arg2 = 200
		     "mov x2, %x3\n\t" // arg3 = 300
		     "blr x9\n\t" // 跳转到 x9 指向的函数
		     :
		     : "r"(my_arm64_syscall_entry), "r"(100), "r"(200), "r"(300)
		     : "x9", "x0", "x1", "x2", "x30", "memory");

	// 调用普通函数（编译器自动处理）
	normal_c_function(11, 22, 33);

	return 0;
}
#endif

int test_modifier(long action)
{
	switch (action) {
	case 0:
		test_asmlinkage();
		break;
	}
	return 0;
}
