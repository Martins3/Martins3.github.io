// 考虑一下 io_submit 和 io_getevents 分别在两个 thread 的场景
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libaio.h>
#include <errno.h>
#include <pthread.h>

#define QUEUE_DEPTH 4
#define BUFFER_SIZE 4096
#define MAX_READS 3 // Number of reads before stopping
#define IO_TIMEOUT_SECONDS 2 // Timeout for io_getevents

static void show_event(const char *info)
{
	time_t now = time(NULL);
	char time_str[64];
	struct tm *tm_info = localtime(&now);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
	printf("%s : %s", time_str, info);
}

struct io_data {
	int fd;
	char *buffer;
	size_t len;
	off_t offset;
	int read_count;
	struct iocb *iocb;
};

struct thread_data {
	io_context_t aio_ctx;
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
	io_context_t aio_ctx = td->aio_ctx;
	int file_fd = td->file_fd;
	int read_count = 0;
	off_t offset = 0;

	while (td->running && read_count < MAX_READS) {
		// Allocate buffer and iocb
		char *buffer = malloc(BUFFER_SIZE);
		if (!buffer)
			fatal("malloc buffer");
		struct iocb *iocb = malloc(sizeof(struct iocb));
		if (!iocb)
			fatal("malloc iocb");
		struct io_data *data = malloc(sizeof(struct io_data));
		if (!data)
			fatal("malloc io_data");
		*data = (struct io_data){ .fd = file_fd,
					  .buffer = buffer,
					  .len = BUFFER_SIZE,
					  .offset = offset,
					  .read_count = read_count + 1,
					  .iocb = iocb };

		// Prepare AIO read
		io_prep_pread(iocb, file_fd, buffer, BUFFER_SIZE, offset);
		iocb->data = data;

		// Submit AIO operation
		struct iocb *iocbs[] = { iocb };
		sleep(1);
		show_event("before io_submit\n");
		int ret = io_submit(aio_ctx, 1, iocbs);
		show_event("after io_submit\n");
		if (ret < 0) {
			fprintf(stderr, "io_submit failed: %s\n",
				strerror(-ret));
			free(buffer);
			free(iocb);
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

// Receiver thread: monitors AIO completions with io_getevents
void *receiver_thread(void *arg)
{
	struct thread_data *td = (struct thread_data *)arg;
	io_context_t aio_ctx = td->aio_ctx;
	struct timespec timeout = { .tv_sec = IO_TIMEOUT_SECONDS,
				    .tv_nsec = 0 };

	printf("Receiver: Monitoring AIO completions\n");

	while (td->running && td->completed_reads < MAX_READS) {
		// Check AIO completions with io_getevents
		struct io_event events[QUEUE_DEPTH];
		show_event("before io io getevents\n");
		int ret =
			io_getevents(aio_ctx, 1, QUEUE_DEPTH, events, &timeout);
		show_event("aftre io io getevents\n");
		if (ret < 0) {
			if (ret != -EINTR) {
				fprintf(stderr, "io_getevents failed: %s\n",
					strerror(-ret));
			}
			continue;
		} else if (ret > 0) {
			for (int i = 0; i < ret; i++) {
				struct io_data *data =
					(struct io_data *)events[i].data;
				if (events[i].res < 0) {
					fprintf(stderr,
						"Receiver: Async read error: %s\n",
						strerror((int)-events[i].res));
				} else {
					printf("Receiver: Read #%d completed, %ld bytes: %.*s\n",
					       data->read_count, events[i].res,
					       (int)events[i].res,
					       data->buffer);
				}
				free(data->buffer);
				free(data->iocb);
				free(data);
				td->completed_reads++;
			}
		}
	}

	td->running = 0; // Signal sender to stop
	printf("Receiver thread exiting\n");
	return NULL;
}

int main(int argc, char *argv[])
{
	io_context_t aio_ctx = 0;
	struct thread_data td = { .running = 1, .completed_reads = 0 };
	pthread_t sender, receiver;
	int file_fd, ret;

	// Initialize AIO context
	ret = io_setup(QUEUE_DEPTH, &aio_ctx);
	if (ret < 0)
		fatal("io_setup");
	td.aio_ctx = aio_ctx;

	// Open file
	file_fd = open("/tmp/test.txt", O_RDONLY);
	if (file_fd < 0)
		fatal("open test.txt");
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
	io_destroy(aio_ctx);
	printf("Main: Program terminated\n");
	return 0;
}
