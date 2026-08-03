/*
 * 和 eventfd-user.cpp 做对比，这里的 eventfd 可以用于内核和用户态的通信。
 *
 * TODO 可以补充一下功能，现在只有用户态监听内核态的事件，但是也可以 write eventfd 来通知
 * 内核态，也就是对于 irqfd 功能的 POC
 * */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/eventfd.h>

int main(int argc, char *argv[])
{
	int efd; 
	uint64_t eftd_ctr;

	int retval;
	fd_set rfds;

	int s;
	efd = eventfd(0, 0);
	if (efd == -1) {
		printf("\nUnable to create eventfd! Exiting...\n");
		exit(EXIT_FAILURE);
	}
	FILE *file = fopen("/sys/kernel/hacking/eventfd", "w");
	if (!file) {
		perror("Failed to open /sys/kernel/hacking/eventfd");
		printf("Please ensure CONFIG_TEST_EVENTFD is enabled in config.h\n");
		close(efd);
		return 1;
	}
	fprintf(file, "%d", efd);
	fclose(file);

	FD_ZERO(&rfds);
	FD_SET(efd, &rfds);

	printf("Now waiting on select()...\n");
	retval = select(efd + 1, &rfds, 0, 0, 0);

	if (retval == -1) {
		printf("select() error. Exiting...\n");
		exit(EXIT_FAILURE);
	} else if (retval > 0) {
		printf("select() says data is available now. Exiting...\n");
		printf("returned from select(), now executing read()...\n");
		s = read(efd, &eftd_ctr, sizeof(uint64_t));
		if (s != sizeof(uint64_t)) {
			printf("\neventfd read error. Exiting...");
		} else {
			printf("\nReturned from read(), value read = %ld",
			       eftd_ctr);
		}
	} else if (retval == 0) {
		printf("\nselect() says that no data was available");
	}
	close(efd);
	return 0;
}
