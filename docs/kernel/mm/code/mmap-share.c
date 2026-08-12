/*
 * 如果 shared mmap 一个文件，然后写会产生很多 page cache ，有如下问题:
 * 1. 但是这些 page cache 通过 echo 3 | sudo tee /proc/sys/vm/drop_caches 无法被 drop 掉, 为什么?
 * 2. 通过 /proc/meminfo | grep Shmem 可以看到，他们不算 Shmem ，为什么，Shmem 的定义是什么?
 *
 * 通过 stress-ng --vm-bytes 300M --vm-keep --vm 1 可以把他们占用的内存积压掉。
 * 注意，这里 mmap 的大小为 4G ，即便系统内存只有 1G ，也是
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include "../lib.h"

int main(int argc, char *argv[])
{
	const long PAGE_SIZE = get_page_size();
	const long MAP_SIZE = get_size(400, 'M');
	// int fd = get_file("/home/martins3/qemu.ram", MAP_SIZE);
	int fd = get_file("/tmp/a", MAP_SIZE);
	void *ptr = mmap_region(MAP_SIZE, fd, MAP_SHARED);
	loop(ptr, PAGE_SIZE, MAP_SIZE, true);
	sleep(10000);
	return EXIT_SUCCESS;
}
