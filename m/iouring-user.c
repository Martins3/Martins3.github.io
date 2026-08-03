/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <liburing.h>

#define QD 64

static int setup_context(unsigned entries, struct io_uring *ring)
{
	int ret;

	ret = io_uring_queue_init(entries, ring, 0);
	if (ret < 0) {
		fprintf(stderr, "queue_init: %s\n", strerror(-ret));
		return -1;
	}

	return 0;
}

static void queue_prepped(struct io_uring *ring, int outfd, void *data)
{
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;

	sqe = io_uring_get_sqe(ring);
	assert(sqe);
	io_uring_prep_read(sqe, outfd, NULL, 0, 0);
	sqe->cmd_op = 0;
	sqe->opcode = IORING_OP_URING_CMD;
	io_uring_sqe_set_data(sqe, data);
	if (io_uring_submit(ring) < 0)
		goto err;
	if(io_uring_wait_cqe(ring, &cqe) < 0)
		goto err;
	io_uring_cqe_seen(ring, cqe);
	return;
err:
	perror(strerror(errno));
}

int main(int argc, char *argv[])
{
	struct io_uring ring;

	int fd = open("/dev/iouring", O_RDWR);
	if (setup_context(QD, &ring))
		return 1;

	queue_prepped(&ring, fd, NULL);

	close(fd);
	io_uring_queue_exit(&ring);
	return 0;
}
