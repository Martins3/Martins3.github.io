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

#define MAPPING_PROT PROT_READ | PROT_WRITE

#define PAGE_SIZE (4 * 1024)
static int test_mmap()
{
	int fd;
	char buf[PAGE_SIZE];
	fd = open("/dev/folio", O_RDWR, 0644);
	if (fd == -1)
		return -1;

	char *ptr =
		(char *)mmap(NULL, PAGE_SIZE, MAPPING_PROT, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
		return -1;
	sprintf(ptr, "hello\n");
	memset(buf, 0, PAGE_SIZE);
	if (read(fd, buf, PAGE_SIZE) < 0)
		return -1;

	printf("[%s]", buf);
	sleep(1000);
	return 0;
}
int main(int argc, char *argv[])
{
	if (argc <= 1)
		return test_mmap();

	if (strcmp("1", argv[1]))
		return test_mmap();

	return 0;
}
