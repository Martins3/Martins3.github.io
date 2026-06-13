/*
 * IORING_OP_NOP
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <liburing.h>

int main()
{
	struct io_uring ring;
	int ret;

	// 初始化 io_uring 队列，深度为 8
	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	// 获取一个 SQE（Submission Queue Entry）
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		goto cleanup;
	}

	// 准备 NOP 操作
	io_uring_prep_nop(sqe);
	sqe->user_data = 100; // 可用于标识该请求

	// 提交到内核
	ret = io_uring_submit(&ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
		goto cleanup;
	}

	printf("Submitted IORING_OP_NOP request (user_data=100)\n");

	// 等待完成事件（CQE）
	struct io_uring_cqe *cqe;
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
			strerror(-ret));
		goto cleanup;
	}

	// 检查结果
	printf("CQE received: user_data=%lld, res=%d\n", cqe->user_data,
	       cqe->res);

	if (cqe->res == 0) {
		printf("✅ IORING_OP_NOP completed successfully!\n");
	} else {
		printf("❌ IORING_OP_NOP failed with error: %s\n",
		       strerror(-cqe->res));
	}

	io_uring_cqe_seen(&ring, cqe);

cleanup:
	io_uring_queue_exit(&ring);
	return 0;
}
