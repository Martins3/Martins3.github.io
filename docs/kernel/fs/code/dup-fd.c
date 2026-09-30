#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../lib.h"

/*
 * 测试一些 dup(2) ，可以发现 dup(2) 是不会自动关闭 socket 的
 */
static int test_dup(void)
{
	const char *filename = "/tmp/test_file.txt";
	FILE *fp = fopen(filename, "w+");

	fprintf(fp, "Hello, this is a test file.\n");
	fflush(fp);

	int fd1 = fileno(fp);
	int fd2 = dup(fd1);
	printf("Original FD: %d, Duplicated FD: %d\n", fd1, fd2);

	lseek(fd1, 5, SEEK_SET);
	printf("FD1 offset after lseek: %ld\n", lseek(fd1, 0, SEEK_CUR));
	printf("FD2 offset after FD1's lseek: %ld\n", lseek(fd2, 0, SEEK_CUR));
	return 0;
}

static int test_dup2(void)
{
	const char *filename = "/tmp/test_file.txt";
	FILE *fp = fopen(filename, "w+");

	fprintf(fp, "Hello, this is a test file.\n");
	fflush(fp);

	int fd1 = fileno(fp);
	int fd2 = dup2(fd1, 10);
	printf("Original FD: %d, Duplicated FD: %d\n", fd1, fd2);

	lseek(fd1, 5, SEEK_SET);
	printf("FD1 offset after lseek: %ld\n", lseek(fd1, 0, SEEK_CUR));
	printf("FD2 offset after FD1's lseek: %ld\n", lseek(fd2, 0, SEEK_CUR));
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		return test_dup2();

	int opt = atoi(argv[1]);
	if (opt == 0)
		return -1;
	switch (opt) {
	case 0:
		test_dup();
		break;
	case 1:
		test_dup2();
		break;
	}
	return EXIT_SUCCESS;
}
