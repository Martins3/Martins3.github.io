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
#include <sys/wait.h>

int parent_main();
int child_main();

unsigned long MAP_SIZE = 4;
unsigned long PAGE_SIZE = 0;

void *mmap_ptr;

#define MAPPING_PROT (PROT_READ | PROT_WRITE)
static void init()
{
	PAGE_SIZE = sysconf(_SC_PAGESIZE);
	if (PAGE_SIZE == -1)
		exit(1);
}

static void *mmap_region()
{
	void *ptr = mmap(NULL, PAGE_SIZE * MAP_SIZE, MAPPING_PROT,
			 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (ptr == MAP_FAILED) {
		printf("%s\n", strerror(errno));
		exit(1);
	}
	return ptr;
}

static int unmap_region(void *ptr)
{
	if (munmap(ptr, PAGE_SIZE * MAP_SIZE) == -1) {
		printf("%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static int do_work_fork()
{
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {
		printf("Child process (PID: %d) \n", getpid());
		child_main();
	} else {
		// use uds to wait them
	}

	return 0;
}

void child_handler(const char *action)
{
	printf("[%s]: %s\n", __FUNCTION__, action);
	if (strcmp(action, "fork") == 0) {
		do_work_fork();
		return;
	}

	if (strcmp(action, "munmap") == 0) {
		unmap_region(mmap_ptr);
		return;
	}

	printf("command not found\n");
}

int main()
{
	init();
	void *ptr = mmap_region();
	if (ptr == NULL)
		return 3;
	mmap_ptr = ptr;

	do_work_fork();
	parent_main();
	return 0;
}
