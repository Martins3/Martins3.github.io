/*
 * 通过这个 demo 来验证，io_uring 可以异步的提交 rename 操作。
 * 以前只有数据的异步提交，而 io_uring 现在可以实现元数据的异步提交，
 * 而且是批量的提交
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>
#include <errno.h>
#include <sys/stat.h>

#define QUEUE_DEPTH 1

int main()
{
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int ret;

	// Initialize io_uring
	ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
		return 1;
	}

	// Create a test file to rename
	int fd = open("oldname.txt", O_CREAT | O_WRONLY, 0644);
	if (fd < 0) {
		perror("open");
		io_uring_queue_exit(&ring);
		return 1;
	}
	close(fd);

	// Get a submission queue entry
	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		io_uring_queue_exit(&ring);
		return 1;
	}

	// Prepare renameat operation with flags
	const char *oldpath = "/tmp/oldname.txt";
	const char *newpath = "/tmp/newname.txt";
	io_uring_prep_renameat(sqe, AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);

	// Submit the operation
	ret = io_uring_submit(&ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
		io_uring_queue_exit(&ring);
		return 1;
	}

	// Wait for completion
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
		io_uring_queue_exit(&ring);
		return 1;
	}

	// Check the result
	if (cqe->res < 0) {
		fprintf(stderr, "Async renameat failed: %s\n",
			strerror(-cqe->res));
	} else {
		printf("File successfully renamed from %s to %s\n", oldpath,
		       newpath);
	}

	// Mark CQE as seen
	io_uring_cqe_seen(&ring, cqe);

	// Cleanup
	io_uring_queue_exit(&ring);
	return 0;
}
