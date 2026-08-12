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
#include <time.h>

/*
 * 操作系统到底是如何才可以检测到 dirty 的
 * 
 * mmap 一个 file ，全部 read 一遍，产生 page cache 的过程中，
 * 这个时候时候都是需要标记为 clean 了。 如果这个时候写一个 page ，
 * 那么显然不可能将所有的 page 写下去:
 *
 * 所以，这就是 do_wp_page 的作用:
 * @[
 *     do_wp_page+1
 *     handle_mm_fault+2921
 *     do_user_addr_fault+503
 *     exc_page_fault+137
 *     asm_exc_page_fault+38
 * ]: 5267188
 *
 * 总结一下:
 * 1. fork 之后，page table 默认全部都拷贝，这个时候，如果 child 对该区域触发 page fault ，那么不会走到 do_pte_missing
 * 2. do_fault 是 do_pte_missing 下处理文件映射的
 * 3. cow :
 *	- file : fork 对于文件，无需特殊考虑 parent 的影响，考虑的还是如何处理文件
 *	- anon 
 *		- 没有 pte ，do_anonymous_page ，parent 在虚拟地址上也没有内容
 *		- 有 pte ，do_wp_page
 *
 * 思考题: 如果不去拷贝 page table ，如何 ?
 *   1. 如果 parent 在 cow 之前修改了 page 中内容，似乎不符合语义了？
*/

unsigned long MAP_SIZE = 10L * 1024 * 1024 * 1024;

#define MAPPING_PROT PROT_READ | PROT_WRITE

char m;
int get_file(void)
{
	int fd;
	fd = open("/home/martins3/qemu.ram", O_RDWR | O_CREAT, 0644);
	if (fd == -1)
		goto err;

	if (ftruncate(fd, MAP_SIZE) < 0)
		goto err;
	return fd;
err:
	printf("%s\n", strerror(errno));
	exit(1);
}

int test_guest(void *ptr, unsigned long PAGE_SIZE)
{
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		return 1;
	} else if (pid == 0) {
		// Child process
		printf("Child process (PID: %d) exiting\n", getpid());
		for (unsigned long i = 0; i < MAP_SIZE; i += PAGE_SIZE)
			*(char *)(ptr + i) = i;

		/* for (unsigned long i = 0; i < MAP_SIZE; i += PAGE_SIZE) */
		/* 	m += *((char *)(ptr + i)); */
	}
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	// child and parent sleep
	sleep(1000);
	return 0;
}

int main(int argc, char *argv[])
{
	// TODO 如果映射的文件是 shmem ，还会 wp 吗，应该不会吧
	unsigned long PAGE_SIZE = sysconf(_SC_PAGESIZE);
	int fd = get_file();
	int map_status = MAP_SHARED;
	/* map_status = MAP_PRIVATE; */
	void *ptr =
		mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, map_status, fd, 0);
	if (ptr == MAP_FAILED)
		goto err;

	if (madvise(ptr, MAP_SIZE, MADV_RANDOM) == -1)
		goto err;

	// do_read_fault
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	for (unsigned long i = 0; i < MAP_SIZE; i += PAGE_SIZE)
		m += *((char *)(ptr + i));

	fprintf(stderr, " (after m += *((char *)(ptr + i));)\n");

	/* 
	 * 如果 MAP_PRIVATE ，那么不会 do_wp_page 的，速度逐渐加快
	 * [martins3:main:113]  3.272310
	 * [martins3:main:113]  0.429563
	 * [martins3:main:113]  0.067131
	 * [martins3:main:113]  0.040549
	 *
	 * 如果是 MAP_SHARED 然后等待 10s ，那么速度总是很慢，因为 do_wp_page :
	 * [martins3:main:120]  3.275056
	 * [martins3:main:120]  4.515000
	 * [martins3:main:120]  3.485643
	 * [martins3:main:120]  3.316294
	 */
	for (size_t i = 0; i < 10; i++) {
		// do_wp_page
		clock_t begin = clock();
		for (unsigned long i = 0; i < MAP_SIZE; i += PAGE_SIZE)
			*(char *)(ptr + i) = i;
		clock_t end = clock();
		double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("[martins3:%s:%d]  %lf\n", __FUNCTION__, __LINE__,
		       time_spent);
		// 的确，dirty memory 保存多长时间也是一门艺术，不然总是在触发 do_wp_page
		sleep(10);
	}
	return test_guest(ptr, PAGE_SIZE);
err:
	printf("%s\n", strerror(errno));
	exit(1);
}
