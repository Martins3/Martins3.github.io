/* 参考: https://noahdesu.github.io/2016/10/10/userfaultfd-hello-world.html
 */

#define _GNU_SOURCE
#include <linux/userfaultfd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../lib.h"

static inline int create_uffd()
{
	// 两个方法都是可以的，使用 syscall 或者 /dev/userfaultfd
	/* uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY); */
	// sudo chown martins3 /dev/userfaultfd
	int uffd;
	int uffd_dev = open("/dev/userfaultfd", O_RDWR | O_CLOEXEC);
	if (uffd_dev <= 0) {
		perror("open /dev/userfaultfd\n");
	}
	// UFFD_USER_MODE_ONLY
	uffd = ioctl(uffd_dev, USERFAULTFD_IOC_NEW, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1)
		error("syscall/userfaultfd");

	struct uffdio_api uffdio_api;
	uffdio_api.api = UFFD_API;
	uffdio_api.features = UFFD_FEATURE_EVENT_REMOVE;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) != 0)
		error("ioctl uffdio_api second");

	if (uffdio_api.api != UFFD_API) {
		fprintf(stderr, "unsupported userfaultfd api %llx\n",
			uffdio_api.features);
	}

	return uffd;
}

static inline void uffd_register(int uffd, void *region, long size)
{
	struct uffdio_register uffdio_register;
	uffdio_register.range.start = (unsigned long)region;
	uffdio_register.range.len = size;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING |
			       UFFDIO_REGISTER_MODE_WP;
	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		error("ioctl/uffdio_register miss mod");

	// TODO 这个检查含义是什么:
	// 这里检查不过的发
	// if ((uffdio_register.ioctls & UFFD_API_RANGE_IOCTLS) !=
	//     UFFD_API_RANGE_IOCTLS) {
	// 	fprintf(stderr, "unexpected userfaultfd ioctl set\n");
	// 	exit(1);
	// }
}

static void protect(int uffd, unsigned long start, unsigned long len, bool p)
{
	struct uffdio_writeprotect uw ={
		.range = {
			.start = start,
			.len = len,
		},
		.mode = p ? UFFDIO_WRITEPROTECT_MODE_WP : 0,
	};
	if (ioctl(uffd, UFFDIO_WRITEPROTECT, &uw)) {
		perror("ioctl/uffdio_writeprotect failed");
		exit(1);
	}
}

static volatile int stop;

struct params {
	int uffd;
	long page_size;
};

static inline uint64_t getns(void)
{
	struct timespec ts;
	int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(ret == 0);
	return (((uint64_t)ts.tv_sec) * 1000000000ULL) + ts.tv_nsec;
}

static void *handler(void *arg)
{
	struct params *p = arg;
	long page_size = p->page_size;
	char buf[page_size];

	for (;;) {
		struct uffd_msg msg;

		struct pollfd pollfd[1];
		pollfd[0].fd = p->uffd;
		pollfd[0].events = POLLIN;

		// wait for a userfaultfd event to occur
		int pollres = poll(pollfd, 1, 200);

		if (stop)
			return NULL;

		switch (pollres) {
		case -1:
			perror("poll/userfaultfd");
			continue;
		case 0:
			continue;
		case 1:
			break;
		default:
			fprintf(stderr, "unexpected poll result\n");
			exit(1);
		}

		if (pollfd[0].revents & POLLERR) {
			fprintf(stderr, "pollerr\n");
			exit(1);
		}

		if (!(pollfd[0].revents & POLLIN)) {
			continue;
		}

		int readres = read(p->uffd, &msg, sizeof(msg));
		if (readres == -1) {
			if (errno == EAGAIN)
				continue;
			perror("read/userfaultfd");
			exit(1);
		}

		if (readres != sizeof(msg)) {
			fprintf(stderr, "invalid msg size\n");
			exit(1);
		}

		// handle the page fault by copying a page worth of bytes
		printf("E : %x\n", msg.event);
		if (msg.event == UFFD_EVENT_PAGEFAULT) {
			int is_wp = (msg.arg.pagefault.flags &
				     UFFD_PAGEFAULT_FLAG_WP);
			printf("%d \n", is_wp);
			long long addr = msg.arg.pagefault.address;
			struct uffdio_copy copy;
			copy.src = (long long)buf;
			copy.dst = (long long)addr;
			copy.len = page_size;
			copy.mode = 0;
			if (is_wp) {
				printf("EVENT: WP\n");
				protect(p->uffd, (unsigned long)addr, page_size,
					false);
			} else {
				printf("EVENT: MISS\n");
				if (ioctl(p->uffd, UFFDIO_COPY, &copy) == -1) {
					perror("ioctl copy");
					exit(1);
				}
			}
		} else if (msg.event == UFFD_EVENT_REMOVE) {
			printf("EVENT : UFFD_EVENT_REMOVE\n");
		} else {
			printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		}
	}

	return NULL;
}

// read write write
int mod1(void *region, int num_pages, uint64_t *latencies, int page_size,
	 int uffd)
{
	long int value = 0;
	char *cur;
	int l = 0;
	cur = region;
	// 先读
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		int v = *((int *)cur);
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		value += v;
		cur += page_size;
	}

	protect(uffd, (unsigned long)region, page_size * num_pages, true);

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}
	printf("[%ld]\n", value);
	return l;
}

// read write write
// 差别在于，当 page table 没有建立的时候，就去设置设置 write protect
// 显然就是直接跳过的
int mod2(void *region, int num_pages, uint64_t *latencies, int page_size,
	 int uffd)
{
	long int value = 0;
	char *cur;
	int l = 0;
	cur = region;
	protect(uffd, (unsigned long)region, page_size * num_pages, true);
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		int v = *((int *)cur);
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		value += v;
		cur += page_size;
	}

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	printf("[%ld]\n", value);
	return l;
}

// write
//
// 用于测试，handler 能否正确处理
int mod3(void *region, int num_pages, uint64_t *latencies, int page_size,
	 int uffd)
{
	long int value = 0;
	char *cur;
	int l = 0;
	cur = region;
	protect(uffd, (unsigned long)region, page_size * num_pages, true);

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	printf("[%ld]\n", value);
	return l;
}

// write REMOVE write
// 测试 REMOVE 可以被监听到
int mod4(void *region, int num_pages, uint64_t *latencies, int page_size,
	 int uffd)
{
	long int value = 0;
	char *cur;
	int l = 0;
	cur = region;
	protect(uffd, (unsigned long)region, page_size * num_pages, true);

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	// 内核如何处理 thread A 在 move page ，但是 thread b 在写的情况?
	// 对于 qemu ，其实意义不大，都释放了，其他的 vCPU 不会写了
	if (madvise(region, page_size, MADV_REMOVE)) {
		printf("[martins3:%s:%d] %p %x\n", __FUNCTION__, __LINE__,
		       region, num_pages * page_size);
		perror("failed\n");
		exit(1);
	}

	cur = region;
	for (long i = 0; i < num_pages; i++) {
		uint64_t start = getns();
		*((int *)cur) = i;
		uint64_t dur = getns() - start;
		latencies[l++] = dur;
		cur += page_size;
	}

	printf("[%ld]\n", value);
	return l;
}

int main(int argc, char **argv)
{
	int uffd;
	long page_size;
	long num_pages;
	void *region;
	pthread_t uffd_thread;

	page_size = get_page_size();
	num_pages = 10;

	uffd = create_uffd();

	int fd = memfd_create("Server memfd", MFD_ALLOW_SEALING);
	if (fd == -1) {
		perror("memfd_create()");
		exit(1);
	}
	if (ftruncate(fd, page_size * num_pages) == -1) {
		perror("ftruncate()");
		exit(1);
	}
	// TODO 各种权限，是否 share 以及 mmap 文件都是需要测试一下
	/* region = mmap(NULL, page_size * num_pages, PROT_READ | PROT_WRITE, */
	/* 	      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); */

	region = mmap(NULL, page_size * num_pages, PROT_READ | PROT_WRITE,
		      MAP_SHARED, fd, 0);
	if (region == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	uffd_register(uffd, region, page_size * num_pages);

	// start the thread that will handle userfaultfd events
	stop = 0;

	struct params p;
	p.uffd = uffd;
	p.page_size = page_size;

	pthread_create(&uffd_thread, NULL, handler, &p);

	// 无需等待，反正也是放到内核中执行的
	// sleep(1);

	uint64_t *latencies = malloc(sizeof(uint64_t) * num_pages * 100);
	assert(latencies);
	memset(latencies, 0, sizeof(uint64_t) * num_pages);

	/* int l = mod1(region, num_pages, latencies, page_size, uffd); */
	/* int l = mod2(region, num_pages, latencies, page_size, uffd); */
	/* int l = mod3(region, num_pages, latencies, page_size, uffd); */
	int l = mod4(region, num_pages, latencies, page_size, uffd);
	if (l > num_pages * 100) {
		printf("fine");
		exit(1);
	}
	stop = 1;
	pthread_join(uffd_thread, NULL);
	fprintf(stdout, "latencies(ns):");
	for (long i = 0; i < l; i++) {
		fprintf(stdout, "%ld %llu\n", i,
			(unsigned long long)latencies[i]);
	}

	return 0;
}
