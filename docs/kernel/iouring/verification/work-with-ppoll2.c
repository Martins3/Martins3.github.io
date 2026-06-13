/* 
 * 当然，iouring 也可以用类似 fs/aio-poll.c 的方式，并不去使用 epoll 来实现 io 
 * 多路复用， 而是让 io_uring_wait_cqe_timeout 来监听，也可以实现 submit 和
 * wait 互相不阻塞， 当然这种时候，只能监听一个文件
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <liburing.h>
#include <errno.h>

#define QUEUE_DEPTH 4
#define BUFFER_SIZE 4096
#define MAX_READS 3 // Number of reads before stopping
#define IO_TIMEOUT_SECONDS 2 // Timeout for io_uring_wait_cqe_timeout

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
	volatile int completed_reads; // Track completed reads
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

// Receiver thread: monitors io_uring completions with io_uring_wait_cqe_timeout
void *receiver_thread(void *arg)
{
	struct thread_data *td = (struct thread_data *)arg;
	struct io_uring *ring = td->ring;
	struct __kernel_timespec timeout = { .tv_sec = IO_TIMEOUT_SECONDS,
					     .tv_nsec = 0 };

	printf("Receiver: Monitoring io_uring completions\n");

	while (td->running && td->completed_reads < MAX_READS) {
		struct io_uring_cqe *cqe;
		// 检查 liburing 的实现，这个库函数也是通过 io_uring_enter 实现的
		int ret = io_uring_wait_cqe_timeout(ring, &cqe, &timeout);
		if (ret < 0) {
			if (ret == -ETIME)
				continue; // Timeout, try again
			if (ret != -EINTR) {
				fprintf(stderr,
					"io_uring_wait_cqe_timeout failed: %s\n",
					strerror(-ret));
			}
			continue;
		}

		struct io_data *data = (struct io_data *)cqe->user_data;
		if (cqe->res < 0) {
			fprintf(stderr, "Receiver: Async read error: %s\n",
				strerror(-cqe->res));
		} else {
			printf("Receiver: Read #%d completed, %d bytes: %.*s\n",
			       data->read_count, cqe->res, (int)cqe->res,
			       data->buffer);
		}
		free(data->buffer);
		free(data);
		io_uring_cqe_seen(ring, cqe);
		td->completed_reads++;

		if (td->completed_reads >= MAX_READS) {
			td->running = 0; // Signal sender to stop
		}
	}

	printf("Receiver thread exiting\n");
	return NULL;
}

int main(int argc, char *argv[])
{
	struct io_uring ring;
	struct thread_data td = { .running = 1, .completed_reads = 0 };
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
