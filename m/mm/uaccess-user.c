#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include "../user/sysfs.h"
#include "../user/lib.h"

static void test_access(int test)
{
	uint64_t val = 0x233456;
	int kernel_test = test / 10;
	int which = test % 10;
	printf("kernel : %d user : %d\n", kernel_test, which);

	void *ptr = mmap(NULL, get_pagesize(), PROT_READ | PROT_WRITE,
			 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	switch (which) {
	case 0:
		// 访问正确地址
		para_write(0, (long)&val);
		sysfs_write(kernel_test);
		break;
	case 1:
		// 0x233456 显然是错误地址，
		// 这才是地址空间的开始 564d8c61e000-564d8c620000
		para_write(0, val);
		sysfs_write(kernel_test);
		break;
	case 2:
		// 没有映射的空间，内核在访问的时候自动填充上的
		/* @[
		 *     __do_fault+1
		 *     do_pte_missing+357
		 *     handle_mm_fault+2121
		 *     do_user_addr_fault+791
		 *     exc_page_fault+137
		 *     asm_exc_page_fault+38
		 *     __get_user_8+17
		 *     test_uaccess+145
		 *     uaccess_store+133
		 *     kernfs_fop_write_iter+240
		 *     vfs_write+892
		 *     ksys_write+114
		 *     do_syscall_64+237
		 *     entry_SYSCALL_64_after_hwframe+119
		 * ]: 1
		 **/
		para_write(0, (long)ptr);
		sysfs_write(kernel_test);
		break;
	case 3: {
		int *m = (int *)ptr;
		*m = 0x1234;
		madvise(ptr, get_pagesize(), MADV_PAGEOUT);
		// 测不出来 swap 的效果，似乎我们对于理解有问题

		para_write(0, (long)ptr);
		sysfs_write(kernel_test);
		break;
	}
	}
}

int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	test_access(test);
	return 0;
}
