#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static void write_all(int fd, const char *buf, size_t len)
{
	while (len > 0) {
		ssize_t n = write(fd, buf, len);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			die("write");
		}

		buf += n;
		len -= (size_t)n;
	}
}

static void read_once(const char *name, int fd)
{
	char buf[128];
	ssize_t n = read(fd, buf, sizeof(buf) - 1);

	if (n < 0)
		die("read");

	buf[n] = '\0';
	printf("%s: %s", name, buf);
}

int main(void)
{
	int p1[2];
	int p2[2];
	const char payload[] = "tee duplicates pipe buffers without consuming p1\n";
	ssize_t n;

	if (pipe(p1) < 0)
		die("pipe p1");
	if (pipe(p2) < 0)
		die("pipe p2");

	write_all(p1[1], payload, strlen(payload));

	n = tee(p1[0], p2[1], strlen(payload), 0);
	if (n < 0)
		die("tee");
	printf("tee duplicated %zd bytes from p1 to p2\n", n);

	close(p1[1]);
	close(p2[1]);

	read_once("read original p1", p1[0]);
	read_once("read copied   p2", p2[0]);

	close(p1[0]);
	close(p2[0]);

	return EXIT_SUCCESS;
}
