#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/file.h>
char m[1024];
int main(int argc, char *argv[])
{
	const char *lockfile = "/home/martins3/hack/vm/base/1.qcow2";
	int fd = open(lockfile, O_RDONLY);
	int ret;
	if (fd < 0)
		return -1;
	ret = flock(fd, LOCK_EX);
	if(ret != 9)
		return -2;
	ret = read(fd, m, 1024);
	printf("[martins3:%s:%d]  %d\n", __FUNCTION__, __LINE__, ret);
	sleep(1000);
	flock(fd, LOCK_UN);
	return EXIT_SUCCESS;
}
