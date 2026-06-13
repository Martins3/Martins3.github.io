/*
 * deepseek 生成，测试 io_uring_register_buffers 的功能
 *
 * 首先，准备环境
 * echo 1 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
 *
 * 观察执行 io_uring_register_buffers 前后的结果
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <liburing.h>
#include <errno.h>
#include <sys/mman.h>

#define QUEUE_DEPTH 1
#define BUFFER_SIZE (2 * 1024 * 1024) // 2MB for huge page
#define IOVEC_COUNT 10

int main(int argc, char *argv[])
{
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	char *buffer;
	struct iovec iov[IOVEC_COUNT];
	int fd, ret;

	/* Initialize io_uring */
	ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	/* Allocate huge page buffer */
	buffer = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (buffer == MAP_FAILED) {
		fprintf(stderr, "mmap huge page failed: %s\n", strerror(errno));
		io_uring_queue_exit(&ring);
		return 1;
	}
	memset(buffer, 0, BUFFER_SIZE);

	/* Set up iovec for buffer registration */
	for (size_t i = 0; i < IOVEC_COUNT; i++) {
		iov[i].iov_base = buffer;
		iov[i].iov_len = BUFFER_SIZE;
	}

	char cmdline[256];
	printf("Before registering huge page buffer \n");
	snprintf(cmdline, 256, "cat /proc/%d/status | grep Pin", getpid());
	if (system(cmdline))
		return 1;
	ret = io_uring_register_buffers(&ring, iov, IOVEC_COUNT);
	if (ret < 0) {
		fprintf(stderr, "io_uring_register_buffers failed: %s\n",
			strerror(-ret));
		munmap(buffer, BUFFER_SIZE);
		io_uring_queue_exit(&ring);
		return 1;
	}
	printf("Huge page buffer registered successfully\n");
	snprintf(cmdline, 256, "cat /proc/%d/status | grep Pin", getpid());
	if (system(cmdline))
		return 1;

	/* Open a file to read into the registered buffer */
	fd = open("/tmp/a", O_RDONLY | O_CREAT, 0644);
	if (fd < 0) {
		fprintf(stderr, "open failed: %s\n", strerror(errno));
		io_uring_unregister_buffers(&ring);
		munmap(buffer, BUFFER_SIZE);
		io_uring_queue_exit(&ring);
		return 1;
	}

	/* Get an SQE (submission queue entry) */
	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		close(fd);
		io_uring_unregister_buffers(&ring);
		munmap(buffer, BUFFER_SIZE);
		io_uring_queue_exit(&ring);
		return 1;
	}

	/* Prepare a read operation using the registered buffer */
	io_uring_prep_read_fixed(sqe, fd, buffer, BUFFER_SIZE, 0, 0);

	/* Submit the read operation */
	ret = io_uring_submit(&ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
		close(fd);
		io_uring_unregister_buffers(&ring);
		munmap(buffer, BUFFER_SIZE);
		io_uring_queue_exit(&ring);
		return 1;
	}

	/* Wait for completion */
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
			strerror(-ret));
		close(fd);
		io_uring_unregister_buffers(&ring);
		munmap(buffer, BUFFER_SIZE);
		io_uring_queue_exit(&ring);
		return 1;
	}

	/* Check the result of the read operation */
	if (cqe->res < 0) {
		fprintf(stderr, "Read failed: %s\n", strerror(-cqe->res));
	} else {
		printf("Read %d bytes: %.*s\n", cqe->res, cqe->res, buffer);
	}

	/* Acknowledge the CQE */
	io_uring_cqe_seen(&ring, cqe);

	/* Cleanup */
	close(fd);
	io_uring_unregister_buffers(&ring);
	munmap(buffer, BUFFER_SIZE);
	io_uring_queue_exit(&ring);
	return 0;
}
