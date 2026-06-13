/*
 * 用于测试 multishot-read ，提交一个 sqe ，然后可以获取到多个 cqe
 *
 * 测试程序来源
 * https://github.com/axboe/liburing/issues/1185
 *
 * 测试方法
 *
 *	首先执行:
 * 	socat pty,link=/tmp/ttyA,rawer STDIO
 * 	然后执行:
 * 	multishot-read.out /tmp/ttyA
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <termios.h>
#include <liburing.h>

#define group_id 17
#define buffers_size 4
#define buffer_mask (buffers_size - 1)
#define buffer_size 32


static int setup_terminal(int terminal_descriptor)
{
	int status = 0;
	struct termios terminal_settings;
	status = tcgetattr(terminal_descriptor, &terminal_settings);

	if (status != 0) {
		fprintf(stderr, "failed to get terminal settings %d\n", status);
		goto exit;
	}

	terminal_settings.c_cflag = CS8 | CREAD | CLOCAL;
	terminal_settings.c_cc[VMIN] = 1;
	status = cfsetospeed(&terminal_settings, B9600);
	if (status != 0) {
		fprintf(stderr, "failed to set output speed settings %d\n",
			status);

		goto exit;
	}

	status = cfsetispeed(&terminal_settings, B9600);

	if (status != 0) {
		fprintf(stderr, "failed to set input speed settings %d\n",
			status);

		goto exit;
	}

	status = tcsetattr(terminal_descriptor, TCSANOW, &terminal_settings);

	if (status != 0) {
		fprintf(stderr, "failed to set terminal settings %d\n", status);

		goto exit;
	}
exit:
	return status;
}

int main(int arguments_size, char *arguments[])
{
	int status = 0;

	if (arguments_size < 2) {
		fprintf(stderr,
			"expected terminal path to be passed as an argument\n");
	}

	int terminal_descriptor = open(arguments[1], O_RDWR, O_NONBLOCK);

	status = isatty(terminal_descriptor);

	if (status < 0) {
		printf("%s (%d) is not a terminal\n", arguments[1],
		       terminal_descriptor);
	}

	int flags = fcntl(terminal_descriptor, F_GETFL, 0);

	status = fcntl(terminal_descriptor, F_SETFL, flags | O_NONBLOCK);

	if (status != 0) {
		fprintf(stderr, "failed to set O_NONBLOCK on terminal %d\n",
			status);

		return status;
	}

	status = setup_terminal(terminal_descriptor);

	if (status < 0) {
		fprintf(stderr, "failed setting up terminal\n");

		return status;
	}

	struct io_uring ring;

	status = io_uring_queue_init(8, &ring, 0);

	if (status < 0) {
		fprintf(stderr, "queue_init: %d\n", status);

		return -12;
	}

	struct io_uring_buf_ring *buffer_ring = io_uring_setup_buf_ring(
		&ring, buffers_size, group_id, 0, &status);

	if (!buffer_ring) {
		fprintf(stderr, "failed buffer ring %d\n", status);

		return status;
	}

	char *buffer = malloc(buffers_size * buffer_size);

	for (int count = 0; count < buffers_size; count++) {
		void *buffer_segment = buffer + count * buffer_size;

		io_uring_buf_ring_add(buffer_ring, buffer_segment, buffer_size,
				      count, buffer_mask, count);
	}

	io_uring_buf_ring_advance(buffer_ring, buffers_size);

	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

	sqe->flags |= IOSQE_BUFFER_SELECT;

	io_uring_prep_read_multishot(sqe, terminal_descriptor, 0, 0, group_id);

	status = io_uring_submit(&ring);

	if (status != 1) {
		fprintf(stderr, "bad submit %d\n", status);

		return -12;
	}

	struct io_uring_cqe *cqe;

	status = io_uring_peek_cqe(&ring, &cqe);

	if (!status) {
		if (cqe->res == -EINVAL || cqe->res == -EBADF) {
			return -12;
		}
	}

	printf("  - waiting for data -  \n");

	while (1) {
		status = io_uring_wait_cqe(&ring, &cqe);

		if (status) {
			fprintf(stderr, "wait %d\n", status);

			return status;
		}

		if (cqe->res < 0) {
			if (cqe->res == -ENOBUFS) {
				fprintf(stderr, "ran out of buffers ");
			}

			fprintf(stderr, "error: %d\n", cqe->res);

			return -101;
		}

		if (!(cqe->flags & IORING_CQE_F_BUFFER)) {
			fprintf(stderr, "buffer not set\n");

			return -102;
		}

		if (!(cqe->flags & IORING_CQE_F_MORE)) {
			fprintf(stderr, "more not set\n");

			return -103;
		}

		int buffer_index = cqe->flags >> 16;

		printf("%s\n", (char *)buffer_ring->bufs[buffer_index].addr);

		io_uring_cqe_seen(&ring, cqe);
	}

	return 0;
}
