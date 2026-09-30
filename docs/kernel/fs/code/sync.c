#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd = open("/mnt/a", O_RDWR | O_DIRECT);
	if (fd < 0)
		goto err;
	char buf[] = "hi";
	for (;;) {
		if (write(fd, buf, 1) < 0)
			goto err;
		printf(".\n");
		sleep(1);
	}

	if (fsync(fd) < 0)
		goto err;
	return EXIT_SUCCESS;
err:
	printf("failed: %s\n", strerror(errno));
	return EXIT_FAILURE;
}
