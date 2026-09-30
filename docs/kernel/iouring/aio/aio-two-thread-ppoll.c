// 模拟 qemu 使用 aio 的方法，在一个 thread 中 io_submit ，然后在另外一个 thread 中通过
// io_getevents 来获取事件结果
#define _GNU_SOURCE
#include <libaio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define MAX_EVENTS 10

// Structure to pass data to threads
struct aio_data {
	io_context_t ctx;
	int efd; // Renamed from eventfd to efd to avoid conflict
	int fd;
};

// Thread 1: Submit AIO read operations
void *submit_thread(void *arg)
{
	struct aio_data *data = (struct aio_data *)arg;
	io_context_t ctx = data->ctx;
	int efd = data->efd;
	int fd = data->fd;

	struct iocb cb;
	struct iocb *cbs[] = { &cb };
	// Ensure alignment for O_DIRECT
	char buffer[BUFFER_SIZE] __attribute__((aligned(4096)));
	io_prep_pread(&cb, fd, buffer, BUFFER_SIZE, 0);
	io_set_eventfd(&cb, efd);

	if (io_submit(ctx, 1, cbs) != 1) {
		fprintf(stderr, "io_submit failed: %s\n", strerror(errno));
		return NULL;
	}
	printf("Submitted AIO read operation\n");
	return NULL;
}

// Thread 2: Monitor eventfd with ppoll and reap AIO completions
void *completion_thread(void *arg)
{
	struct aio_data *data = (struct aio_data *)arg;
	io_context_t ctx = data->ctx;
	int efd = data->efd;

	// Set up ppoll
	struct pollfd pfd = { .fd = efd, .events = POLLIN };
	struct timespec timeout = { .tv_sec = 5, .tv_nsec = 0 };
	sigset_t sigmask;
	sigemptyset(&sigmask); // No signals blocked

	int ret = ppoll(&pfd, 1, &timeout, &sigmask);
	if (ret < 0) {
		fprintf(stderr, "ppoll failed: %s\n", strerror(errno));
		return NULL;
	} else if (ret == 0) {
		printf("ppoll timed out\n");
		return NULL;
	}

	if (pfd.revents & POLLIN) {
		uint64_t event_count;
		if (read(efd, &event_count, sizeof(event_count)) !=
		    sizeof(event_count)) {
			fprintf(stderr, "eventfd read failed: %s\n",
				strerror(errno));
			return NULL;
		}
		printf("Eventfd signaled with count: %lu\n", event_count);

		// Reap completed AIO events
		struct io_event events[MAX_EVENTS];
		struct timespec aio_timeout = { .tv_sec = 1, .tv_nsec = 0 };
		ret = io_getevents(ctx, 1, MAX_EVENTS, events, &aio_timeout);
		if (ret < 0) {
			fprintf(stderr, "io_getevents failed: %s\n",
				strerror(-ret));
			return NULL;
		}

		for (int i = 0; i < ret; i++) {
			printf("AIO operation completed: res=%ld, res2=%ld\n",
			       events[i].res, events[i].res2);
		}
	}
	return NULL;
}

int main()
{
	// Initialize AIO context
	io_context_t ctx = 0;
	if (io_setup(MAX_EVENTS, &ctx) < 0) {
		fprintf(stderr, "io_setup failed: %s\n", strerror(errno));
		return 1;
	}

	// Create eventfd (renamed variable to efd)
	int efd = eventfd(0, EFD_CLOEXEC);
	if (efd < 0) {
		fprintf(stderr, "eventfd creation failed: %s\n",
			strerror(errno));
		io_destroy(ctx);
		return 1;
	}

	// Open file for AIO
	int fd = open("/tmp/test.txt", O_RDONLY | O_DIRECT);
	if (fd < 0) {
		fprintf(stderr, "open failed: %s\n", strerror(errno));
		close(efd);
		io_destroy(ctx);
		return 1;
	}

	// Prepare data for threads
	struct aio_data data = { .ctx = ctx, .efd = efd, .fd = fd };

	// Create threads
	pthread_t submit_thr, completion_thr;
	if (pthread_create(&submit_thr, NULL, submit_thread, &data) != 0) {
		fprintf(stderr, "pthread_create failed for submit_thread\n");
		goto cleanup;
	}
	if (pthread_create(&completion_thr, NULL, completion_thread, &data) !=
	    0) {
		fprintf(stderr,
			"pthread_create failed for completion_thread\n");
		goto cleanup;
	}

	// Wait for threads to complete
	pthread_join(submit_thr, NULL);
	pthread_join(completion_thr, NULL);

cleanup:
	close(fd);
	close(efd);
	io_destroy(ctx);
	return 0;
}
