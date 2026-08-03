#include "internal.h"
#include <linux/uaccess.h>

// TODO mm/maccess.c 中的 api 都看看吧
// 1. 测试下 copy_to_user_nofault ，如果 fault 了，到时候怎么办?
// 2. unsafe_get_user
//
// - 这么说，gup 的用户，使用这些内存的时候，其实都是需要小心的
//		- 可能刚刚 gup 了，然后这些内存被释放了( ref count)
//
// copy_from_user 的一般考虑 : https://lwn.net/Articles/736348/
//
// 一共两个技术:
// 1. 使用 SMAP 禁止随便访问 user space 空间
// 2. 使用 section ，对于指定区域的代码才可以
//
// TODO 有时间测试下这个:
// fault_in_readable 和
// fault_in_safe_writeable

int test_uaccess(long action)
{
	u64 val = 0x1234;
#ifdef CONFIG_X86_64
	u8 st_preempted = 12;
	struct kvm_steal_time *st;
	int err = -EFAULT;
#endif

	u64 __user *uaddr = (u64 *)get_parameter(0);
	struct kvm_steal_time {
		u8 preempted;
	};
	switch (action) {
	case 0:
		// 如果访问的是非法地址，get_user 得到的是 0
		get_user(val, uaddr);
		pr_info("%px --> [0x%llx]\n", uaddr, val);
		break;
	case 1:
		if (copy_from_user(&val, uaddr, sizeof(val))) {
			// 如果访问的是非法地址，会走到这里
			pr_info("fault on : %px \n", uaddr);
			return -EFAULT;
		}
		pr_info("%px --> [0x%llx]\n", uaddr, val);
		break;
	case 2:
		/* 即使是正确的地址也会 crash
		 *
		 * 这是由于 SMAP 的机制，让 kernel 直接 panic
		 *
		 * #PF: supervisor read access in kernel mode
		 * #PF: error_code(0x0001) - permissions violation
		 * PGD 116beb067 P4D 116beb067 PUD 10711f067 PMD 115c6a067 PTE 800000011c637067
		 * Oops: Oops: 0001 [#1] PREEMPT SMP NOPTI
		 *
		 */
		val = *uaddr;
		pr_info("%px --> [0x%llx]\n", uaddr, val);
		break;
	case 3:
		if (!user_access_begin(uaddr, sizeof(*uaddr)))
			return -EFAULT;
		// 这个不可以访问非法地址的，不然直接挂掉
		val = xchg(uaddr, 0llu);
		// XXX 理解一下，这里为什么不可以 pr_info
		// case 4 中的两个 pr_debug 也控制的，继续理解下 user_access_begin 吧
		// martins3.o: warning: objtool: test_uaccess+0x14a: call to _printk() with UACCESS enabled
		pr_debug("%px --> [0x%llx]\n", uaddr, val);
		user_access_end();
		break;
#ifdef CONFIG_X86_64
	case 4:
		// 没有 user_access_begin 保护，只有是访问用户态地址
		// 直接 crash ，无论是不是用户态的非法地址
		st = (struct kvm_steal_time *)uaddr;
		asm volatile("1: xchgb %0, %2\n"
			     "xor %1, %1\n"
			     "2:\n" _ASM_EXTABLE_UA(1b, 2b)
			     : "+q"(st_preempted), "+&r"(err),
			       "+m"(st->preempted));
		break;
	case 5:
		st = (struct kvm_steal_time *)uaddr;
		if (!user_access_begin(st, sizeof(*st)))
			return -EACCES;

		// 这里的 _ASM_EXTABLE_UA 只是对于 copy_user_generic 的模仿
		//
		// 为什么可以知道 xor 的结果，是因为触发了 page fault 的时候，这个是
		// page fault handler 最后传递过来的
		asm volatile("1: xchgb %0, %2\n"
			     "xor %1, %1\n"
			     "2:\n" _ASM_EXTABLE_UA(1b, 2b)
			     : "+q"(st_preempted), "+&r"(err),
			       "+m"(st->preempted));
		if (err)
			// 命中了非法地址可以被发现，其中的 _ASM_EXTABLE_UA 是必须的
			pr_debug("EFAULT found\n");
		else
			pr_debug("no page fault, st->preempted=%d st_preempted=%d\n",
				st->preempted, st_preempted);

		user_access_end();
		break;
#endif
	default:
		break;
	}
	return 0;
}
