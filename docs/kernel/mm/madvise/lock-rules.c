/*
 * 测试内容
 * -
 */
#include "lib.h"
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <libaio.h>

int main(int argc, char *argv[])
{
	long size = get_size(10, 'G');
	void *base = mmap_region(size, -1, false);
	touch(base, get_page_size(), size, true);
	printf("touch finished\n");
	if (madvise(base, size, MADV_DONTNEED))
		error("madvise");
	sleep(1000);
	return 0;
}
