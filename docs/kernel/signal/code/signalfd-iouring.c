#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <liburing.h>
#include <sys/select.h>
#include <sys/signalfd.h>

#define BUFFER_SIZE sizeof(struct signalfd_siginfo)

int main()
{
	// 必须把传统的信号默认屏蔽掉才可以，不然传统的信号会更早的接受到
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT); // Ctrl+C
	sigaddset(&mask, SIGTERM); // 也可以添加其他信号

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}

	int sfd = signalfd(-1, &mask, 0);
	if (sfd == -1) {
		perror("signalfd");
		exit(EXIT_FAILURE);
	}

	// 初始化 io_uring
	struct io_uring ring;
	int ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		close(sfd);
		exit(EXIT_FAILURE);
	}

	printf("Waiting for signals (SIGINT or SIGTERM) using io_uring...\n");

	while (1) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		if (!sqe) {
			fprintf(stderr, "Failed to get SQE\n");
			break;
		}

		// 准备异步读取操作
		char buffer[BUFFER_SIZE];
		io_uring_prep_read(sqe, sfd, buffer, BUFFER_SIZE, 0);
		sqe->user_data = 1; // 标识这是 signalfd 操作

		// 提交请求
		ret = io_uring_submit(&ring);
		if (ret < 0) {
			fprintf(stderr, "io_uring_submit failed: %s\n",
				strerror(-ret));
			break;
		}

		// 等待完成事件
		struct io_uring_cqe *cqe;
		ret = io_uring_wait_cqe(&ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
				strerror(-ret));
			break;
		}

		if (cqe->res < 0) {
			fprintf(stderr, "Read failed: %s\n",
				strerror(-cqe->res));
			io_uring_cqe_seen(&ring, cqe);
			break;
		}

		if (cqe->res == sizeof(struct signalfd_siginfo)) {
			struct signalfd_siginfo *si =
				(struct signalfd_siginfo *)buffer;

			if (si->ssi_signo == SIGINT) {
				printf("Received SIGINT (Ctrl+C)\n");
			} else if (si->ssi_signo == SIGTERM) {
				printf("Received SIGTERM\n");
			} else {
				printf("Received unexpected signal %d\n",
				       si->ssi_signo);
			}

			// 如果收到 SIGINT 或 SIGTERM，可以选择退出
			if (si->ssi_signo == SIGINT ||
			    si->ssi_signo == SIGTERM) {
				printf("Exiting...\n");
			}

			io_uring_cqe_seen(&ring, cqe);

			// 如果是 SIGINT 或 SIGTERM，退出循环
			if (si->ssi_signo == SIGINT ||
			    si->ssi_signo == SIGTERM) {
				break;
			}
		} else {
			fprintf(stderr, "Unexpected read size: %d\n", cqe->res);
			return 1;
		}
	}

	// 清理
	io_uring_queue_exit(&ring);
	close(sfd);
	return 0;
}
