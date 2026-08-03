#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include "user/sysfs.h"

// 为了测试系统调用，所以不用使用 echo 来封装
int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	const char *file_path = "/sys/kernel/hacking/signal";

	int fd = open(file_path, O_WRONLY);
	if (fd == -1) {
		perror("Failed to open sysfs file");
		return 1;
	}

	char value_str[16];
	snprintf(value_str, sizeof(value_str), "%d", test);
	const char *buf = value_str;
	size_t len = strlen(value_str);

	long written = syscall(SYS_write, fd, buf, len);
	if (written == -1) {
		perror("Failed to write via syscall");
		close(fd);
		return 1;
	}

	if ((size_t)written != len) {
		fprintf(stderr, "Warning: only %ld bytes written out of %zu\n",
			written, len);
	}

	close(fd);
	return 0;
}
