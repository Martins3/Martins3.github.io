#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <liburing.h>

#define TARGET_USER_DATA 54321

int main()
{
	struct io_uring ring;
	int ret;

	// 初始化 io_uring
	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	// 准备一个长时间 timeout 请求（10秒）
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "get sqe failed\n");
		goto exit;
	}

	struct __kernel_timespec ts = { .tv_sec = 10, .tv_nsec = 0 };
	io_uring_prep_timeout(sqe, &ts, 0, 0);
	sqe->user_data = TARGET_USER_DATA;

	ret = io_uring_submit(&ring);
	if (ret <= 0) {
		fprintf(stderr, "submit failed: %s\n", strerror(-ret));
		goto exit;
	}

	printf("Submitted timeout request (user_data=%d), will cancel it in 100ms...\n",
	       TARGET_USER_DATA);
	usleep(100000); // 等待 100ms，确保请求已进入内核 pending 队列

	// === 执行同步取消 ===
	struct io_uring_sync_cancel_reg cancel_reg = {
		.fd = -1,
		.flags = IORING_ASYNC_CANCEL_ANY,
		.timeout = { .tv_sec = 0, .tv_nsec = 0 }
	};

	// 调用 io_uring_register(..., IORING_REGISTER_SYNC_CANCEL, ...)
	ret = io_uring_register_sync_cancel(&ring, &cancel_reg);
	if (ret == -ENOENT) {
		printf("❌ Cancel failed: request not found (maybe already completed?)\n");
	} else if (ret < 0) {
		printf("❌ Sync cancel error: %s\n", strerror(-ret));
	} else {
		printf("✅ Sync cancel succeeded! (ret=%d)\n", ret);
		// 注意：成功返回值是被取消的请求数（通常为1）
	}

	// 现在等待原始请求的 CQE
	struct io_uring_cqe *cqe;
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "wait cqe failed: %s\n", strerror(-ret));
		goto exit;
	}

	printf("Original request CQE: user_data=%lld, res=%d\n", cqe->user_data,
	       cqe->res);
	if (cqe->res == -ECANCELED) {
		printf("✅ Request was canceled!\n");
	} else if (cqe->res == -ETIME) {
		printf("⚠️ Request timed out (cancel may be too late)\n");
	}

	io_uring_cqe_seen(&ring, cqe);

exit:
	io_uring_queue_exit(&ring);
	return 0;
}
