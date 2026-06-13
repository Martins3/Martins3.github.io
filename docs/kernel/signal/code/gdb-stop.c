#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
	const int MAX_EVENTS = 10;
	struct epoll_event ev, events[MAX_EVENTS];
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
		perror("epoll_ctl");
		close(epfd);
		exit(EXIT_FAILURE);
	}

	printf("等待输入（按 Ctrl+D 结束）...\n");
	printf("kill -STOP %d\n", getpid());
	printf("kill -CONT %d\n", getpid());

	while (1) {
		int nr = epoll_wait(epfd, events, MAX_EVENTS, -1);

		if (nr == -1) {
			perror("epoll_wait");
			break;
		}
	}

	close(epfd);
	return 0;
}
