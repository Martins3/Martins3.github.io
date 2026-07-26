#ifndef MARTINS3_MM_LIB_H
#define MARTINS3_MM_LIB_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

static inline void error(const char *msg)
{
	perror(msg);
	exit(1);
}

static inline long get_page_size(void)
{
	long ret = sysconf(_SC_PAGESIZE);
	if (ret == -1) {
		perror("sysconf(_SC_PAGESIZE)");
		exit(1);
	}
	return ret;
}

// 未经测试，效果未知
static inline void get_tmpfile(off_t size)
{
	char name[] = "/tmp/hugeXXXXXX";
	int fd = mkstemp(name);
	if (fd < 0)
		error("mkstemp");
	if (unlink(name))
		error("unlink");
}

static inline int get_file(const char *file_path, off_t size)
{
	/* fd = open("/dev/hugepages/", O_RDWR | O_CREAT, 0644); */
	// fd = open(file_path, O_RDWR | O_CREAT, 0644);
	int fd = open(file_path, O_RDWR | O_CREAT | O_DIRECT, 0644);
	/* fd = open("/dev/shm/x", O_RDWR | O_CREAT, 0644); */
	if (fd == -1)
		goto err;

	if (ftruncate(fd, size) < 0)
		goto err;

	return fd;
err:
	printf("%s\n", strerror(errno));
	exit(1);
}

static inline int get_file_readonly(const char *file_path)
{
	int fd = open(file_path, O_RDONLY | O_DIRECT);
	if (fd == -1)
		error("open");
	return fd;
}

#define MAPPING_PROT PROT_READ | PROT_WRITE
static inline void *mmap_region_internal(size_t size, int fd, bool share,
					 bool huge)
{
	// void *ptr = mmap(NULL, MAP_SIZE, MAPPING_PROT, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	// void *ptr = mmap(NULL, MAP_SIZE, MAPPING_PROT, MAP_ANONYMOUS | MAP_PRIVATE, 1, 0);
	// void *ptr = mmap(NULL, MAP_SIZE, MAPPING_PROT, MAP_PRIVATE, get_file(), 0);
	int flags = share ? MAP_SHARED : MAP_PRIVATE;
	if (fd == -1)
		flags |= MAP_ANONYMOUS;
	if (huge)
		flags |= MAP_HUGETLB;
	void *ptr = mmap(NULL, size, MAPPING_PROT, flags, fd, 0);
	if (ptr == MAP_FAILED) {
		printf("mmap failed : [%s]\n", strerror(errno));
		exit(1);
	}
	return ptr;
}

// fd == -1 : anonymous
static inline void *mmap_region(size_t size, int fd, bool share)
{
	return mmap_region_internal(size, fd, share, false);
}

static inline void *mmap_region_hugetlb(size_t size, int fd, bool share)
{
	return mmap_region_internal(size, fd, share, true);
}

static inline void *mmap_region_memfd(size_t size, int *fd_ptr)
{
	int fd = memfd_create("memfd", MFD_ALLOW_SEALING);
	if (fd == -1)
		error("memfd_create");

	if (fd_ptr != NULL)
		*fd_ptr = fd;

	if (ftruncate(fd, size) == -1)
		error("ftruncate()");
	return mmap_region_internal(size, fd, true, false);
}

static inline long get_size(int s, char g)
{
	long m;
	switch (g) {
	case 'K':
		m = 1024l * s;
		break;
	case 'M':
		m = 1024l * 1024 * s;
		break;
	case 'G':
		m = 1024l * 1024 * 1024 * s;
		break;
	}
	return m;
}

static inline int touch(char *ptr, int granularity, long size, bool write)
{
	char m = '1';
	for (unsigned long i = 0; i < size; i += granularity) {
		if (write)
			*((char *)(ptr + i)) = m + i;
		else
			m += *((char *)(ptr + i));
	}
	return m;
}

static inline int loop(char *ptr, int granularity, long size, bool write)
{
	int x = 0;
	for (size_t i = 0;; i++) {
		x += touch(ptr, granularity, size, write);
		printf("%ld : %d\n", i, x);
		sleep(1);
	}
	exit(1);
}

#endif
