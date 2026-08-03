#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stddef.h>
#include "lib.h"

void *map_region(const unsigned long pages)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	void *ptr = mmap(NULL, pages * pagesize, PROT_READ | PROT_WRITE,
			 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	printf("mmap addr %p\n", ptr);
	return ptr;
}

void *map_file(const unsigned long pages)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	long MAP_SIZE = pages * pagesize;
	int fd = open("/home/martins3/qemu.ram", O_RDWR | O_CREAT, 0644);
	if (fd == -1) {
		perror("create file");
		exit(EXIT_FAILURE);
	}

	if (ftruncate(fd, MAP_SIZE) < 0) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}

	void *ptr =
		mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	printf("mmap addr %p\n", ptr);
	return ptr;
}
