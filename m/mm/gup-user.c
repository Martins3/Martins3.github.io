#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stddef.h>
#include "../user/sysfs.h"
#include "../user/lib.h"

static const unsigned long pages = 200l * 1024 + 1;

static void pin_in_kernel(void *ptr)
{
	para_write(0, (long)(ptr));
	para_write(1, pages);
	sysfs_write(0);
	printf("pin pages in kernel finished\n");
	for (unsigned long i = 0; i < 10000; i++) {
		sleep(1);
		printf(".\n");
	}
}

static void pin_in_us(void *ptr)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	printf("pin pages in userspace\n");
	for (unsigned long i = 0; i < pages; i++) {
		char *m = (char *)ptr + i * pagesize;
		*m = i;
	}
	for (int i = 0; i < 5; i++) {
		sleep(1);
		printf(".\n");
	}
}

static void test0(void)
{
	// TODO 测试一下没有权限的 gup 吧
	void *ptr = map_region(pages);

	// gup 的第一个基本功能，增加 user space page 的 refcount:
	// 1. get_user_pages_unlocked 会导致 page refcount 变为 2
	// 2. 无论是让 kernel pin 还是让 us 先来 pin ，page refcount 都是 2
	// 3. kernel pin 之后，然后 put_page ，内存最后可以释放，如果不去 put_page ，最后无法释放

	pin_in_kernel(ptr);
	pin_in_us(ptr);
}

// 其实 gup 就是像是帮助 user space 来进行 page fault
// 例如这里映射了一个文件，那么最后就会映射到一个文件上
// @[
//  filemap_fault+5
//  __do_fault+48
//  do_fault+407
//  __handle_mm_fault+1490
//  handle_mm_fault+226
//  __get_user_pages+1794
//  get_user_pages_unlocked+230
//  test_gup.cold+104
//  gup_store.cold+52
//  kernfs_fop_write_iter+293
//  vfs_write+672
//  ksys_write+109
//  do_syscall_64+188
//  entry_SYSCALL_64_after_hwframe+119
// ]: 204801

static void test1(void)
{
	void *ptr = map_file(pages);
	pin_in_kernel(ptr);
}

/*
 * 测试一下 pin_user_pages_fast 
 */
static void test2(void)
{
	void *ptr = map_region(pages);
	para_write(0, (long)(ptr));
	para_write(1, pages);
	sysfs_write(2);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <test_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int test = atoi(argv[1]);
	switch (test) {
	case 0:
		test0();
		break;
	case 1:
		test1();
		break;
	case 2:
		test2();
		break;
	default:
		break;
	}

	return 0;
}
