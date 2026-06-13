/*
 * 参考 https://unixism.net/loti/tutorial/sq_poll.html
 * 测试 sqpoll 的效果
 *
 * 配合这个测试:
 * sudo bpftrace -e 'rawtracepoint:io_uring_submit_req { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
 *
 * 顺带测试 io_uring_register_files() 的效果
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <liburing.h>
#include <string.h>

#define BUF_SIZE 512
#define FILE_NAME1 "/tmp/io_uring_sq_test.txt"
#define STR1 "What is this life if, full of care,\n"
#define STR2 "We have no time to stand and stare."

int start_sq_polling_ops(struct io_uring *ring)
{
	int fds[2];
	char buff1[BUF_SIZE];
	char buff2[BUF_SIZE];
	char buff3[BUF_SIZE];
	char buff4[BUF_SIZE];
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;

	fds[0] = open(FILE_NAME1, O_RDWR | O_TRUNC | O_CREAT, 0644);
	if (fds[0] < 0) {
		perror("open");
		return 1;
	}

	memset(buff1, 0, BUF_SIZE);
	memset(buff2, 0, BUF_SIZE);
	memset(buff3, 0, BUF_SIZE);
	memset(buff4, 0, BUF_SIZE);
	strcpy(buff1, STR1);
	strcpy(buff2, STR2);

	/*
	 * 这里演示了一个 fix file descriptor 的经典例子
	 * 1. 让 sqe->flags |= IOSQE_FIXED_FILE 中配置
	 * 2. 提交给 io_uring_prep_write 中的第二个参数就不同普通意义上的 fd 了，
	 * 而是 io_uring_register_files 中当时提交的 fd
	 */
	printf("[%d]\n", getpid());

	/*
	 * fdinfo 中到没有什么区别
	 */
	char cmd[256];
	snprintf(cmd, 256, "cat /proc/%d/fdinfo/4", getpid());
	if(system(cmd))
		return 1;
	int ret = io_uring_register_files(ring, fds, 1);
	if (ret) {
		fprintf(stderr, "Error registering buffers: %s",
			strerror(-ret));
		return 1;
	}
	if(system(cmd))
		return 1;

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "Could not get SQE.\n");
		return 1;
	}
	io_uring_prep_write(sqe, 0, buff1, sizeof(STR1), 0);
	sqe->flags |= IOSQE_FIXED_FILE;

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "Could not get SQE.\n");
		return 1;
	}
	io_uring_prep_write(sqe, 0, buff2, sizeof(STR2), sizeof(STR1));
	sqe->flags |= IOSQE_FIXED_FILE;

	io_uring_submit(ring);

	for (int i = 0; i < 2; i++) {
		int ret = io_uring_wait_cqe(ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "Error waiting for completion: %s\n",
				strerror(-ret));
			return 1;
		}
		/* Now that we have the CQE, let's process the data */
		if (cqe->res < 0) {
			fprintf(stderr, "Error in async operation: %s\n",
				strerror(-cqe->res));
		}
		printf("Result of the operation: %d\n", cqe->res);
		io_uring_cqe_seen(ring, cqe);
	}


	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "Could not get SQE.\n");
		return 1;
	}
	io_uring_prep_read(sqe, 0, buff3, sizeof(STR1), 0);
	sqe->flags |= IOSQE_FIXED_FILE;

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "Could not get SQE.\n");
		return 1;
	}
	io_uring_prep_read(sqe, 0, buff4, sizeof(STR2), sizeof(STR1));
	sqe->flags |= IOSQE_FIXED_FILE;

	io_uring_submit(ring);

	for (int i = 0; i < 2; i++) {
		int ret = io_uring_wait_cqe(ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "Error waiting for completion: %s\n",
				strerror(-ret));
			return 1;
		}
		/* Now that we have the CQE, let's process the data */
		if (cqe->res < 0) {
			fprintf(stderr, "Error in async operation: %s\n",
				strerror(-cqe->res));
		}
		printf("Result of the operation: %d\n", cqe->res);
		io_uring_cqe_seen(ring, cqe);
	}
	printf("Contents read from file:\n");
	printf("%s%s", buff3, buff4);
	return 0;
}

int main()
{
	struct io_uring ring;
	struct io_uring_params params;

	if (geteuid()) {
		fprintf(stderr,
			"You need root privileges to run this program.\n");
		return 1;
	}


	memset(&params, 0, sizeof(params));
	params.flags |= IORING_SETUP_SQPOLL;
	params.sq_thread_idle = 2000;

	int ret = io_uring_queue_init_params(8, &ring, &params);
	if (ret) {
		fprintf(stderr, "Unable to setup io_uring: %s\n",
			strerror(-ret));
		return 1;
	}
	start_sq_polling_ops(&ring);
	io_uring_queue_exit(&ring);
	return 0;
}
