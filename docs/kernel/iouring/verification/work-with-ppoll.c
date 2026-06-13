/* 用这个例子来测试的 qemu 的 event model
 *
 * qemu 可以让 io 的提交和 io 的完成处理在两个 thread 中 
 * 提交其实是 vCPU thread ，而 io 的接受可以是 iothread 或者 main loop 中
 *
 * 在这里 io uring 可以自己只是一个 io 引擎
 * epoll 来监听 io_uring::ring_fd
 *
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <liburing.h>
#include <errno.h>

#define QUEUE_DEPTH 4
#define BUFFER_SIZE 4096
#define MAX_READS 3 // Number of reads before sender stops

struct io_data {
	int fd;
	char *buffer;
	size_t len;
	off_t offset;
	int read_count;
};

struct thread_data {
	struct io_uring *ring;
	int file_fd;
	volatile int running;
};

void fatal(const char *msg)
{
	perror(msg);
	exit(1);
}

// Sender thread: submits async read operations
void *sender_thread(void *arg)
{
	struct thread_data *td = (struct thread_data *)arg;
	struct io_uring *ring = td->ring;
	int file_fd = td->file_fd;
	int read_count = 0;
	off_t offset = 0;

	while (td->running && read_count < MAX_READS) {
		// Allocate buffer for each read
		char *buffer = malloc(BUFFER_SIZE);
		if (!buffer)
			fatal("malloc buffer");

		// Prepare io_uring read operation
		struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
		if (!sqe) {
			free(buffer);
			fprintf(stderr, "io_uring_get_sqe failed\n");
			break;
		}

		struct io_data *data = malloc(sizeof(struct io_data));
		if (!data)
			fatal("malloc io_data");
		*data = (struct io_data){ .fd = file_fd,
					  .buffer = buffer,
					  .len = BUFFER_SIZE,
					  .offset = offset,
					  .read_count = read_count + 1 };

		io_uring_prep_read(sqe, file_fd, buffer, BUFFER_SIZE, offset);
		sqe->user_data = (unsigned long)data;

		// Submit operation
		int ret = io_uring_submit(ring);
		if (ret < 0) {
			fprintf(stderr, "io_uring_submit failed: %s\n",
				strerror(-ret));
			free(buffer);
			free(data);
			break;
		}

		printf("Sender: Submitted read #%d at offset %ld\n",
		       read_count + 1, offset);
		offset += BUFFER_SIZE;
		read_count++;
		sleep(1); // Simulate delay between submissions
	}

	printf("Sender thread exiting\n");
	return NULL;
}

// Receiver thread: monitors stdin and io_uring completions with ppoll
void *receiver_thread(void *arg)
{
	struct thread_data *td = (struct thread_data *)arg;
	struct io_uring *ring = td->ring;
	struct pollfd pfds[2];
	sigset_t sigmask;

	// Set up ppoll
	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = ring->ring_fd; // Use ring->ring_fd directly
	pfds[1].events = POLLIN;

	sigemptyset(&sigmask);

	printf("Receiver: Monitoring stdin and io_uring\n");

	while (td->running) {
		int ret = ppoll(pfds, 2, NULL, &sigmask);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			fatal("ppoll");
		}

		// Check stdin
		if (pfds[0].revents & POLLIN) {
			char buf[256];
			ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
			if (n > 0) {
				buf[n] = '\0';
				printf("Receiver: Stdin input: %s", buf);
				if (strncmp(buf, "quit\n", 5) == 0) {
					td->running =
						0; // Signal threads to exit
				}
			}
		}

		// Check io_uring completions
		if (pfds[1].revents & POLLIN) {
			struct io_uring_cqe *cqe;
			ret = io_uring_peek_cqe(ring, &cqe);
			if (ret == 0) {
				struct io_data *data =
					(struct io_data *)cqe->user_data;
				if (cqe->res < 0) {
					fprintf(stderr,
						"Receiver: Async read error: %s\n",
						strerror(-cqe->res));
				} else {
					printf("Receiver: Read #%d completed, %d bytes: %.*s\n",
					       data->read_count, cqe->res,
					       (int)cqe->res, data->buffer);
				}
				free(data->buffer);
				free(data);
				io_uring_cqe_seen(ring, cqe);
			}
		}
	}

	printf("Receiver thread exiting\n");
	return NULL;
}

int main(int argc, char *argv[])
{
	struct io_uring ring;
	struct thread_data td = { .running = 1 };
	pthread_t sender, receiver;
	int file_fd, ret;

	// Initialize io_uring
	ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
	if (ret < 0)
		fatal("io_uring_queue_init");
	td.ring = &ring;

	// Open file
	file_fd = open("/tmp/test.txt", O_RDONLY);
	if (file_fd < 0)
		fatal("open /tmp/test.txt");
	td.file_fd = file_fd;

	// Create sender and receiver threads
	ret = pthread_create(&sender, NULL, sender_thread, &td);
	if (ret)
		fatal("pthread_create sender");
	ret = pthread_create(&receiver, NULL, receiver_thread, &td);
	if (ret)
		fatal("pthread_create receiver");

	// Wait for threads to complete
	pthread_join(sender, NULL);
	pthread_join(receiver, NULL);

	// Cleanup
	close(file_fd);
	io_uring_queue_exit(&ring);
	printf("Main: Program terminated\n");
	return 0;
}
