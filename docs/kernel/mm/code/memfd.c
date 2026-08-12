#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lib.h"

/*
 * 这种没有配置大小 memfd 的大小，如果去 mmap 可以成功，但是访问该区域，
 * 最后是会触发 SIGBUS 的（不是 SIGSEGV）: 访问的 offset 超过 inode 的 size，
 * mm/shmem.c 的 shmem_fault() 返回 VM_FAULT_SIGBUS
 */
static void test_memfd_size()
{
	int fd = memfd_create("test_memfd", MFD_ALLOW_SEALING);
	if (fd == -1)
		error("memfd_create");
	long page_size = get_page_size();
	char *m = mmap_region_internal(page_size * 10, fd, true, false);
	if (m == MAP_FAILED) {
		perror("mmap_region_internal");
		exit(1);
	}

	/*
	 * 映射大小就是 10 个 page
	 * 7f0409dd1000-7f0409ddb000 rw-s 00000000 00:01 17837                      /memfd:test_memfd (deleted)
	 */
	char cmdline[128];
	sprintf(cmdline, "cat /proc/%d/maps | grep test_memfd", getpid());
	int ret = system(cmdline);
	if(ret)
		error("system");
	*m = 12;
}

/*
 * 验证确认一个小问题，就是 close memfd + unmap 才可以释放掉所有的内存
 * 仅仅 close memfd 是没有办法关闭地址空间的，还需要 unmap
 */
static void test_memfd_release()
{
	long page_size = get_page_size();
	long size = get_size(20, 'G');
	int fd;
	char *region = mmap_region_memfd(size, &fd);

	touch(region, page_size, size, true);
	close(fd);
	getchar();
	munmap(region, size);
	sleep(1000);
}

/*
 * 如果 memfd 被关闭，访问映射 memfd 的区域会触发 segfault 吗?
 */
static void test_memfd_fault_if_close()
{
	long page_size = get_page_size();
	long size = get_size(1, 'M');
	int fd;
	char *region = mmap_region_memfd(size, &fd);
	close(fd);

	touch(region, page_size, size, true);
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		test_memfd_size();
		return EXIT_SUCCESS;
	}

	int opt = atoi(argv[1]);
	if (opt == 0)
		return -1;
	switch (opt) {
	case 0:
		test_memfd_size();
		break;
	case 1:
		test_memfd_release();
		break;
	case 2:
		test_memfd_fault_if_close();
		break;
	}
	return EXIT_SUCCESS;
}
