/*
 * 测试内容
 * 1. madvise 对于无法 drop 正在进行 io 而被 lock 的页面
 * 2. 此外发现，io_submit 的一个 io 上线是 0x7ffff000
 */
#include "lib.h"
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <libaio.h>

int main(int argc, char *argv[])
{
	int ret;
	long size = get_size(400, 'G');

	// void *base = mmap_region_memfd(size, NULL);
	void *base = mmap_region(size, -1, false);

	printf("[martins3:%s:%d] %p\n", __FUNCTION__, __LINE__, base);
	char *m = base;
	*m = 'A';

	int fd = get_file_readonly("/dev/sde");
	io_context_t ctx;
	memset(&ctx, 0, sizeof(io_context_t));
	if (io_setup(1, &ctx) < 0)
		error("io_setup\n");
	struct iocb cb;
	struct iocb *cbs[1];
	io_prep_pread(&cb, fd, base, size, 0);
	cbs[0] = &cb;

	ret = io_submit(ctx, 1, cbs);
	if (ret != 1)
		error("io_submit\n");

	int c;
	printf("press 1 to MADV_DONTNEED\n");
	sleep(1);
	// scanf("%d", &c);
	if (madvise(base, size, MADV_DONTNEED))
		error("madvise");
	printf("press 1 to contine\n");
	ret = scanf("%d", &c);

	struct io_event ev;
	ret = io_getevents(ctx, 1, 1, &ev, NULL);
	if (ret != 1)
		error("io_getevents");
	if (ev.res != size) {
		// 提交的最大量是 0x7ffff000 ，当时的确会 pin 住那些空间
		printf("%lx\n", ev.res);
		if (ev.res != 0x7ffff000)
			error("io event");
	}

	printf("press to MADV_DONTNEED again\n");
	ret = scanf("%d", &c);
	if (madvise(base, size, MADV_DONTNEED))
		error("madvise");

	sleep(1000);
	return 0;
}
