#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../user/sysfs.h"
#include "../user/lib.h"

static const long PAGESIZE = 4096; // Will be initialized at runtime

void f1()
{
	for (size_t i = 0; i < 100; i++) {
		printf("begin %zu\n", i);
		void *ptr = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE,
				 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
		printf("middle %zu\n", i);
		int *m = (int *)ptr;
		*m = 12;
		printf("done %zu\n", i);
		sleep(1);
	}
}

// 是的，显然，page fault 不会被阻碍
void f2()
{
	void *ptr = mmap(NULL, PAGESIZE * 100, PROT_READ | PROT_WRITE,
			 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (ptr == MAP_FAILED)
		exit(1);

	for (size_t i = 0; i < 100; i++) {
		printf("f2 begin %zu\n", i);
		printf("f2 middle %zu\n", i);
		int *m = (int *)((char *)ptr + PAGESIZE * i);
		*m = 12;
		printf("f2 done %zu\n", i);
		sleep(1);
	}
}

void write_file(FILE *file, const char *m)
{
	fprintf(file, "%s", m);
	fflush(file); // TODO 为什么 \n 也没用啊
	if (ferror(file)) {
		printf("failed %s\n", m);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	pthread_t t1, t2;
	pthread_create(&t1, NULL, (void *(*)(void *))f1, NULL);
	pthread_create(&t2, NULL, (void *(*)(void *))f2, NULL);

	sleep(3); // 2.5 seconds approximated to 3
	sysfs_write(100);
	printf("begin\n");
	sleep(8);
	printf("end\n");
	sysfs_write(101);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	return 0;
}
