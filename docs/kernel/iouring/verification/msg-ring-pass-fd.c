/*
 * 参考 liburing 的 test/fd-pass.c 一个简单的 fd 传递操作
 *
 * 我理解这个并不是类似 unix domain socket 将一个 fd 跨进程传输。
 * 而是在多个线程，每一个线程都有自己的 ring ，ring 之间传递 fd 。
 * 而且 fd 也是 io_uring 自己注册的
 *
 * 从 commit e6130eba8a84 ("io_uring: add support for passing fixed file descriptors") 看，
 * 应该是支持的跨 process 的
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <liburing.h>

enum t_test_result {
	T_EXIT_PASS = 0,
	T_EXIT_FAIL = 1,
	T_EXIT_SKIP = 77,
};

#define USER_DATA 0x89
#define FSIZE 16
#define PAT 0x9a

static int verify_fixed_read(struct io_uring *ring, int fixed_fd, int fail)
{
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	unsigned char buf[FSIZE];

	sqe = io_uring_get_sqe(ring);
	io_uring_prep_read(sqe, fixed_fd, buf, FSIZE, 0);
	sqe->flags |= IOSQE_FIXED_FILE;
	io_uring_submit(ring);

	io_uring_wait_cqe(ring, &cqe);
	if (cqe->res != FSIZE) {
		if (fail && cqe->res == -EBADF)
			return 0;
		fprintf(stderr, "Read: %d\n", cqe->res);
		return 1;
	}
	io_uring_cqe_seen(ring, cqe);

	printf("===\n");
	printf("%s\n", buf);
	printf("===\n");
	return 0;
}

static int test(const char *filename, int source_fd, int target_fd,
		unsigned int ring_flags)
{
	struct io_uring sring, dring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int ret;

	ret = io_uring_queue_init(8, &sring, ring_flags);
	if (ret) {
		fprintf(stderr, "ring setup failed: %d\n", ret);
		return T_EXIT_FAIL;
	}
	ret = io_uring_queue_init(8, &dring, 0);
	if (ret) {
		fprintf(stderr, "ring setup failed: %d\n", ret);
		return T_EXIT_FAIL;
	}

	ret = io_uring_register_files_sparse(&sring, 8);
	if (ret) {
		fprintf(stderr, "register files failed %d\n", ret);
		return T_EXIT_FAIL;
	}
	ret = io_uring_register_files_sparse(&dring, 8);
	if (ret) {
		fprintf(stderr, "register files failed %d\n", ret);
		return T_EXIT_FAIL;
	}

	/* open direct descriptor */
	sqe = io_uring_get_sqe(&sring);
	io_uring_prep_openat_direct(sqe, AT_FDCWD, filename, 0, 0644,
				    source_fd);
	io_uring_submit(&sring);
	ret = io_uring_wait_cqe(&sring, &cqe);
	if (ret) {
		fprintf(stderr, "wait cqe failed %d\n", ret);
		return T_EXIT_FAIL;
	}
	if (cqe->res) {
		fprintf(stderr, "cqe res %d\n", cqe->res);
		return T_EXIT_FAIL;
	}
	io_uring_cqe_seen(&sring, cqe);

	/* send direct descriptor to destination ring */
	sqe = io_uring_get_sqe(&sring);
	io_uring_prep_msg_ring_fd(sqe, dring.ring_fd, source_fd, target_fd,
				  USER_DATA, 0);
	io_uring_submit(&sring);
	ret = io_uring_wait_cqe(&sring, &cqe);
	if (ret) {
		fprintf(stderr, "wait cqe failed %d\n", ret);
		return T_EXIT_FAIL;
	}
	if (cqe->res < 0) {
		fprintf(stderr, "msg_ring failed %d\n", cqe->res);
		return T_EXIT_FAIL;
	}
	io_uring_cqe_seen(&sring, cqe);

	/* get posted completion for the passing */
	ret = io_uring_wait_cqe(&dring, &cqe);
	if (ret) {
		fprintf(stderr, "wait cqe failed %d\n", ret);
		return T_EXIT_FAIL;
	}
	if (cqe->user_data != USER_DATA) {
		fprintf(stderr, "bad user_data %ld\n", (long)cqe->res);
		return T_EXIT_FAIL;
	}
	if (cqe->res < 0) {
		fprintf(stderr, "bad result %i\n", cqe->res);
		return T_EXIT_FAIL;
	}
	io_uring_cqe_seen(&dring, cqe);

	/* now verify we can read the sane data from the destination ring */
	if (verify_fixed_read(&dring, target_fd, 0))
		return T_EXIT_FAIL;

	return T_EXIT_PASS;
}

int main(int argc, char *argv[])
{
	char name[PATH_MAX];
	snprintf(name, PATH_MAX , "./%s", __FILE__);
	return test(name, 0, 1, 0);
}
