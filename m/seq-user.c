#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SEQ_PATH "/sys/kernel/debug/martins3_seq"
#define LINE_SIZE 512
char line[LINE_SIZE];

static void print2(void)
{
	int fd = open(SEQ_PATH, O_RDWR | O_CREAT, 0644);
	if (fd == -1)
		exit(1);
	read(fd, line, LINE_SIZE);
	printf("%s", line);
}

int main(int argc, char *argv[])
{
	print2();
	return EXIT_SUCCESS;
}
