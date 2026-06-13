/*
 * Qwen3-Coder 生成的，
 * 使用 io_uring 来监听多个 eventfd ，无需重复提交，
 * 可以不断的接受到任务。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <poll.h>
#include <liburing.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define MAX_EVENTFDS 4

typedef struct {
	int eventfd;
	int thread_id;
	struct io_uring *ring;
} thread_data_t;

static void *event_thread_func(void *arg)
{
	thread_data_t *data = (thread_data_t *)arg;
	int counter = 0;

	while (1) {
		uint64_t value = 1; // eventfd value
		int ret = write(data->eventfd, &value, sizeof(uint64_t));
		if (ret < 0) {
			printf("[Thread %d] write evetnfd failed\n",
			       data->thread_id);
			break;
		}
		printf("[Thread %d] sent event %d\n", data->thread_id, ++counter);
		sleep(1);
	}

	return NULL;
}

int main(int arguments_size, char *arguments[])
{
	int status = 0;
	struct io_uring ring;
	pthread_t threads[MAX_EVENTFDS];
	thread_data_t thread_data[MAX_EVENTFDS];

	status = io_uring_queue_init(2048, &ring, 0);
	if (status < 0) {
		fprintf(stderr, "queue_init: %d\n", status);
		return -12;
	}

	// Create eventfds
	int eventfds[MAX_EVENTFDS];
	for (int i = 0; i < MAX_EVENTFDS; i++) {
		eventfds[i] = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
		if (eventfds[i] < 0) {
			perror("eventfd");
			return -1;
		}

		// Setup thread data
		thread_data[i].eventfd = eventfds[i];
		thread_data[i].thread_id = i;
		thread_data[i].ring = &ring;

		// Create thread for each eventfd
		status = pthread_create(&threads[i], NULL, event_thread_func,
					&thread_data[i]);
		if (status != 0) {
			fprintf(stderr, "pthread_create failed: %d\n", status);
			return -1;
		}

		printf("Created thread %d with eventfd %d\n", i, eventfds[i]);
	}

	// Setup poll multishot for all eventfds
	for (int i = 0; i < MAX_EVENTFDS; i++) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		if (!sqe) {
			fprintf(stderr, "Failed to get SQE\n");
			return -1;
		}

		io_uring_prep_poll_multishot(sqe, eventfds[i], POLLIN);
		sqe->user_data =
			(uint64_t)(i +
				   1); // Use user_data to identify which eventfd

		printf("Setup poll multishot for eventfd %d (index %d)\n",
		       eventfds[i], i);
	}

	// Submit all SQEs
	status = io_uring_submit(&ring);
	if (status < 0) {
		fprintf(stderr, "submit failed: %d\n", status);
		return -1;
	}

	printf("  - waiting for events -  \n");

	while (1) {
		struct io_uring_cqe *cqe;

		status = io_uring_wait_cqe(&ring, &cqe);
		if (status) {
			fprintf(stderr, "wait_cqe failed: %d\n", status);
			break;
		}

		// Get the eventfd index from user_data
		int eventfd_index = (int)cqe->user_data - 1;

		if (cqe->res < 0) {
			if (cqe->res == -ENOBUFS) {
				fprintf(stderr, "ran out of buffers\n");
				io_uring_cqe_seen(&ring, cqe);
				continue;
			}

			fprintf(stderr, "Error on eventfd %d: %d\n",
				eventfds[eventfd_index], cqe->res);
			io_uring_cqe_seen(&ring, cqe);
			continue;
		}

		// Check if this is a multishot completion
		if (cqe->flags & IORING_CQE_F_MORE) {
			printf("[main] Multishot event from eventfd %d (thread %d): events=0x%x\n",
			       eventfds[eventfd_index], eventfd_index,
			       cqe->res);
		} else {
			printf("[main] Single shot event from eventfd %d (thread %d): events=0x%x\n",
			       eventfds[eventfd_index], eventfd_index,
			       cqe->res);
		}

		// Read the eventfd value to clear it
		uint64_t value;
		ssize_t bytes_read =
			read(eventfds[eventfd_index], &value, sizeof(uint64_t));
		if (bytes_read == sizeof(uint64_t)) {
			printf("[main] Read value %lu from eventfd %d\n", value, eventfds[eventfd_index]);
		} else {
			perror("[main] read eventfd");
		}

		io_uring_cqe_seen(&ring, cqe);
	}

	// Cleanup
	for (int i = 0; i < MAX_EVENTFDS; i++) {
		pthread_cancel(threads[i]);
		pthread_join(threads[i], NULL);
		close(eventfds[i]);
	}

	io_uring_queue_exit(&ring);
	return 0;
}
