#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAPPING_PROT PROT_READ | PROT_WRITE
#define PAGE_SIZE (4 * 1024)

static void panic(const char * msg)
{
	printf("[martins3:%s:%d] %s\n", __FUNCTION__, __LINE__, msg);
	perror(strerror(errno));
	exit(1);
}

static void test_mmap()
{
	void *ptr = mmap(NULL, PAGE_SIZE, MAPPING_PROT, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (ptr == MAP_FAILED)
		panic("mmap");

	int *s = (int *)ptr;
	*s = 12;
	// TODO 不过这里意外的发现，即便是程序退出之后，使用的 swap 的空间也不会释放
	// 那么这些空间到底什么释放呢 ?
	//
	// pageout 之后，的确也不会调用到这些函数，看来也是不可以用的
	if (madvise(ptr, PAGE_SIZE, MADV_PAGEOUT) == -1)
		panic("madvise");
}

int fd = -1;

static void init()
{
	fd = open("/sys/kernel/hacking/mmu_notifier", O_RDWR, 0644);
	if (fd == -1)
		panic("open");
}

static int write_num(int num)
{
	char m[16];
	snprintf(m, 16, "%d", num);
	/* return write(fd, m, 16); */
	return 0;
}

int main(int argc, char *argv[])
{
	init();
	if (write_num(1) == -1)
		panic("write 1");

	test_mmap();
	// 有点糟糕，还需要手动回收
	// 如果不回收，是什么结果?

	if (write_num(2) == -1)
		panic("write 2");

	sleep(3);
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}
