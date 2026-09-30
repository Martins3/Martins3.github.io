#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>

/*
 * 测试 openEuler 中的 CONFIG_BLK_DEV_DUMPINFO
 * */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage : ./a.out sdb\n");
		exit(1);
	}
	char path[16];
	sprintf(path, "/dev/%s", argv[1]);
	/* int fd = open(path, O_WRONLY | O_DIRECT | O_EXCL); */
	int fd = open(path, O_WRONLY | O_DIRECT);
	if (fd < 0) {
		printf("failed to open\n");
		exit(1);
	}
	/* sleep(10000); */
	return 0;
}
