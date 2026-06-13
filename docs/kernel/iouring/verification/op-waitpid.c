/*
 * 测试 io_uring/waitid.c
 *
 * strace -e trace=io_uring_setup,io_uring_enter,waitid ./op-waitpid.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <liburing.h>
#include <string.h>
#include <errno.h>

int test_waitid_basic(struct io_uring *ring)
{
	pid_t pid;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	siginfo_t info;
	int ret;

	// 创建子进程
	pid = fork();
	if (pid == 0) {
		// 子进程：立即退出
		exit(42);
	} else if (pid > 0) {
		// 父进程：使用 io_uring 异步等待
		sqe = io_uring_get_sqe(ring);
		if (!sqe) {
			perror("io_uring_get_sqe");
			return -1;
		}

		// 设置 waitid 操作
		io_uring_prep_waitid(sqe, P_PID, pid, &info, WEXITED, 0);
		sqe->user_data = 123;

		ret = io_uring_submit(ring);
		if (ret != 1) {
			fprintf(stderr, "io_uring_submit failed: %d\n", ret);
			return -1;
		}

		// 等待完成
		ret = io_uring_wait_cqe(ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
				strerror(-ret));
			return -1;
		}

		printf("CQE result: %d, user_data: %lld\n", cqe->res,
		       cqe->user_data);

		if (cqe->res == 0) {
			printf("Child pid: %d, exit status: %d\n", info.si_pid,
			       info.si_status);
			if (info.si_status == 42) {
				printf("Basic test PASSED\n");
				io_uring_cqe_seen(ring, cqe);
				return 0;
			}
		}

		io_uring_cqe_seen(ring, cqe);
		return -1;
	} else {
		perror("fork");
		return -1;
	}
}

int main()
{
	struct io_uring ring;
	int ret;

	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	printf("Testing io_uring waitid functionality...\n");

	if (test_waitid_basic(&ring) == 0) {
		printf("✓ Basic waitid test passed\n");
	} else {
		printf("✗ Basic waitid test failed\n");
	}

	io_uring_queue_exit(&ring);
	return 0;
}
