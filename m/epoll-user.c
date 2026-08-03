#define MAX_EVENTS 5
#define READ_SIZE 64
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
	int running = 1, event_count, i;
	ssize_t bytes_read;
	char read_buffer[READ_SIZE + 1];
	struct epoll_event event, events[MAX_EVENTS];
	int epoll_fd = epoll_create1(0);

	if (epoll_fd == -1) {
		fprintf(stderr, "Failed to create epoll file descriptor\n");
		return 1;
	}

	int fd = open("/dev/amsg", O_RDWR);
	if (fd < 0) {
		perror("cannot open /dev/amsg !");
		return 1;
	}

	event.events = EPOLLIN;
	event.data.fd = fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)) {
		fprintf(stderr, "Failed to add file descriptor to epoll\n");
		close(epoll_fd);
		return 1;
	}

	while (running) {
		printf("\nPolling for input...\n");
		event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
		printf("%d ready events\n", event_count);

		for (i = 0; i < event_count; i++) {
			printf("Reading file descriptor '%d' -- ",
			       events[i].data.fd);
			// TODO 按道理来说，输出应该从头开始才对
			lseek(events[i].data.fd, 0, SEEK_SET);
			bytes_read =
				read(events[i].data.fd, read_buffer, READ_SIZE);
			printf("%zd bytes read.\n", bytes_read);
			read_buffer[bytes_read] = '\0';
			printf("Read '%s'\n", read_buffer);

			if (!strncmp(read_buffer, "stop\n", 5))
				running = 0;
		}
	}

	return 0;
}
