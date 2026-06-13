/*
 * 测试 cancel 功能
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <liburing.h>

#define TARGET_USER_DATA 12345
#define CANCEL_REQ_USER_DATA 99999

// 模拟一个长时间不会完成的请求：使用 IORING_OP_TIMEOUT
// 因为 NOP 会立即完成，不适合 cancel 测试
int submit_long_running_request(struct io_uring *ring)
{
	struct io_uring_sqe *sqe;
	struct __kernel_timespec ts = {
		.tv_sec = 10, // 10秒超时（但我们会在它完成前取消）
		.tv_nsec = 0
	};

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		return -1;
	}

	// 提交一个 timeout 请求，user_data 设为 TARGET_USER_DATA
	io_uring_prep_timeout(sqe, &ts, 0, 0);
	sqe->user_data = TARGET_USER_DATA;

	int ret = io_uring_submit(ring);

	if (ret < 0) {
		fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
		return -1;
	}

	printf("Submitted long-running timeout request (user_data=%d)\n",
	       TARGET_USER_DATA);
	return 0;
}

// 提交 cancel 请求
int submit_cancel_request(struct io_uring *ring, void *user_data_to_cancel)
{
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe for cancel failed\n");
		return -1;
	}

	// 准备 cancel 请求：取消 user_data == user_data_to_cancel 的请求
	io_uring_prep_cancel(sqe, user_data_to_cancel, 0);
	sqe->user_data = CANCEL_REQ_USER_DATA; // cancel 请求自己的 user_data

	int ret = io_uring_submit(ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit cancel failed: %s\n",
			strerror(-ret));
		return -1;
	}

	printf("Submitted cancel request for user_data=%p\n",
	       user_data_to_cancel);
	return 0;
}

void *cancel_thread_func(void *arg)
{
	struct io_uring *ring = (struct io_uring *)arg;

	// 等待一小会儿，确保 long-running 请求已提交
	usleep(100000); // 100ms

	// 提交 cancel
	submit_cancel_request(ring, (void *)TARGET_USER_DATA);

	return NULL;
}

int main()
{
	struct io_uring ring;
	int ret;

	// 初始化 io_uring，队列深度 8
	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	// 提交一个长时间运行的请求（timeout 10s）
	if (submit_long_running_request(&ring) < 0) {
		goto cleanup;
	}

	// 启动一个线程，在 100ms 后取消该请求
	pthread_t cancel_thread;
	pthread_create(&cancel_thread, NULL, cancel_thread_func, &ring);

	// 等待两个 CQE：一个是被取消的请求，一个是 cancel 请求本身
	struct io_uring_cqe *cqe;
	int completed = 0;
	while (completed < 2) {
		ret = io_uring_wait_cqe(&ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
				strerror(-ret));
			break;
		}

		if (cqe->user_data == TARGET_USER_DATA) {
			printf("Original request completed with res=%d\n",
			       cqe->res);
			if (cqe->res == -ECANCELED) {
				printf("✅ Original request was successfully canceled!\n");
			} else if (cqe->res == -ETIME) {
				printf("⚠️ Original request timed out (cancel may have failed or been too late)\n");
			}
		} else if (cqe->user_data == CANCEL_REQ_USER_DATA) {
			printf("Cancel request completed with res=%d\n",
			       cqe->res);
			if (cqe->res == 0) {
				printf("✅ Cancel request succeeded (found and canceled a request)\n");
			} else if (cqe->res == -ENOENT) {
				printf("❌ Cancel request failed: target request not found (already completed?)\n");
			}
		}

		io_uring_cqe_seen(&ring, cqe);
		completed++;
	}

	pthread_join(cancel_thread, NULL);

cleanup:
	io_uring_queue_exit(&ring);
	return 0;
}
