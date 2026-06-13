/*
 * 本来用来测试 buffer io 的，
 * 其实基本思路可以测试 io_uring_submit 和 io_uring_wait_cqe 的时间
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>
#include <errno.h>
#include <sys/stat.h>

#define QUEUE_DEPTH 32

int main()
{
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;

	int mode = O_RDONLY;
	char *FILENAME;
	int io_size = 16;

	int PAGE_SIZE = sysconf(_SC_PAGESIZE);
	if (PAGE_SIZE == -1) {
		perror("sysconf/pagesize");
		exit(1);
	}
	printf("PAGE_SIZE=%d\n", PAGE_SIZE);

	int ret;
	char *buffer;
	int fd;

	/*
	 * 参数调整
	 */
	FILENAME = "/dev/zero";
	FILENAME = "/tmp/testfile.txt";
	FILENAME = "/home/martins3/data/testfile.txt";

	// mode |= O_DIRECT;

	io_size = PAGE_SIZE;

	/*
	 * 1. 读取哪里的文件
	 * 2. 是否 O_DIRECT
	 * 3. 读取的大小
	 *
	 * 发现了有些有趣的东西:
	 * 1. /tmp 下文件，对于 O_DIRECT 没有 size 的要求
	 * 2. /dev/zero 完全不支持 O_DIRECT
	 * 3. xfs 下的文件，支持 O_DIRECT ，但是有 size 的
	 */
	printf("%s %s size=%d", FILENAME,
	       mode & O_DIRECT ? "O_DIRECT" : "PAGE_CACHE", io_size);

	if (posix_memalign((void **)&buffer, io_size, io_size) != 0) {
		perror("posix_memalign");
		return 1;
	}

	// Create and write test data to file (so we have something to read)
	fd = open(FILENAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open (write)");
		free(buffer);
		return 1;
	}
	const char *test_data = "Hello, io_uring! This is a test file.\n";
	if (write(fd, test_data, strlen(test_data)) < 0)
		return 1;
	close(fd);

	fd = open(FILENAME, mode);
	if (fd < 0) {
		perror("open (read)");
		return 1;
	}

	ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
		close(fd);
		free(buffer);
		return 1;
	}

	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		return 1;
	}

	io_uring_prep_read(sqe, fd, buffer, io_size, 0);

	ret = io_uring_submit(&ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
		return 1;
	}
	// sleep(1);

	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
		return 1;
	}

	// Check the result
	if (cqe->res < 0) {
		fprintf(stderr, "Async read failed: %s\n", strerror(-cqe->res));
	} else {
		// Null-terminate the buffer for safe printing
		buffer[cqe->res] = '\0';
		printf("Read %d bytes:\n%s", cqe->res, buffer);
	}
	return 0;
}
