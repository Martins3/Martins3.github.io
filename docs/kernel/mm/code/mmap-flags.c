// 如果没有 MAP_ANONYMOUS ，那么就会检查 fd ，所以这两个都会失败
// void *ptr2 = mmap_region(MAP_SIZE, -1, MAP_SHARED);
// void *ptr = mmap_region(MAP_SIZE, -1, MAP_PRIVATE);
//
// MAP_SHARED 和 MAP_PRIVATE 必需带一个
//
// 所以，这个 flags 就很简单了:
//
// MAP_PRIVATE/MAP_SHARED 是一组， MAP_ANONYMOUS 是一组，他们是正交的关系。
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "../lib.h"

//  sudo cgexec --sticky -g memory:/ ./a.out
int main(int argc, char *argv[])
{
	const long PAGE_SIZE = get_page_size();
	long MAP_SIZE;

	MAP_SIZE = get_size(23, 'G');
	MAP_SIZE = get_size(5, 'G');
	MAP_SIZE = get_size(25, 'G');
	void *ptr = mmap_region(MAP_SIZE, -1, MAP_PRIVATE | MAP_ANONYMOUS);
	touch(ptr, PAGE_SIZE, MAP_SIZE, true);
	unsigned long fast_speed = INTMAX_MAX;
	unsigned long sleep_ns = 1000;
	unsigned long alloc_size = 20;
	bool need_confirm = false;
	for (;;) {
		sleep(1000);
		MAP_SIZE = get_size(alloc_size, 'M');
		void *ptr =
			mmap_region(MAP_SIZE, -1, MAP_PRIVATE | MAP_ANONYMOUS);
		struct timespec round_start;
		struct timespec round_end;
		clock_gettime(CLOCK_MONOTONIC, &round_start);
		touch(ptr, PAGE_SIZE, MAP_SIZE, true);
		clock_gettime(CLOCK_MONOTONIC, &round_end);
		unsigned long ns =
			(round_end.tv_sec - round_start.tv_sec) * 1e9 +
			(round_end.tv_nsec - round_start.tv_nsec);

		if (ns > fast_speed * 10) {
			printf("use %ld became slow  , sleep 1s\n", ns);
			sleep_ns = 1000 * 1000 * 10;
			alloc_size = 5;
			need_confirm = true;
		}

		printf("done with %ld ms\n", ns / 1000 / 1000);
		fast_speed = fast_speed < ns ? fast_speed : ns;
		if (need_confirm)
			getchar();
		else
			usleep(sleep_ns);
	}
	return EXIT_SUCCESS;
}
