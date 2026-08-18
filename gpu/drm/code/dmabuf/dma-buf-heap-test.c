/*
 * 参考 https://lore.kernel.org/all/20251021042022.47919-1-21cnbao@gmail.com/
 * 需要使用 sudo 权限运行快
 */
#include <fcntl.h>
#include <linux/dma-heap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define SIZE (512UL * 1024 * 1024)
#define PAGE 4096
#define STRIDE (PAGE / sizeof(int))
#define PAGES (SIZE / PAGE)

int main(void)
{
	int heap = open("/dev/dma_heap/system", O_RDONLY);
	struct dma_heap_allocation_data d = { .len = SIZE,
					      .fd_flags = O_RDWR | O_CLOEXEC };
	ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &d);

	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	int *p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, d.fd, 0);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	for (int i = 0; i < PAGES; i++)
		p[i * STRIDE] = i;
	for (int i = 0; i < PAGES; i++)
		if (p[i * STRIDE] != i) {
			fprintf(stderr, "mismatch at page %d\n", i);
			exit(1);
		}

	long ns = (t1.tv_sec - t0.tv_sec) * 1000000000L +
		  (t1.tv_nsec - t0.tv_nsec);
	printf("mmap 512MB took %.3f us, verify OK\n", ns / 1000.0);
	return 0;
}
