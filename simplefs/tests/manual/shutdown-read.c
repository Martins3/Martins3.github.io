#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <sys/vfs.h>
#include <unistd.h>
int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	struct statfs fs;
	if (statfs(argv[1], &fs) || fs.f_type != 0xDEADCE) {
		fputs("Create an empty file on a dedicated SimpleFS test volume first.\n", stderr);
		return 2;
	}
	char *buf = calloc(1, 1048576);
	int fd = open(argv[1], O_CREAT | O_RDWR, 0600);
	if (fd < 0 || !buf || write(fd, buf, 1048576) != 1048576) return 2;
	int rd = open(argv[1], O_RDONLY), sp = open(argv[1], O_RDONLY);
	int out = open("/dev/null", O_WRONLY);
	uint32_t flags = 2;
	if (rd < 0 || sp < 0 || out < 0 || ioctl(fd, _IOR('X', 125, uint32_t), &flags)) return 2;
	errno = 0;
	ssize_t a = read(rd, buf, 1);
	int ae = errno;
	errno = 0;
	ssize_t b = sendfile(out, sp, NULL, 1048576);
	int be = errno;
	printf("read=%zd errno=%d; sendfile=%zd errno=%d\n", a, ae, b, be);
	return !(a == -1 && ae == EIO && b == -1 && be == EIO);
}
