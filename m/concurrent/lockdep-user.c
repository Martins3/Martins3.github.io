#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../user/sysfs.h"

void update(void)
{
	pid_t pid = gettid();
	printf("%d\n", pid);
	sysfs_write(0);
}

FILE *f;
static void update_one_file(void)
{
	pid_t pid = gettid();
	printf("%d\n", pid);

	fprintf(f, "8");
	fflush(f);
	if (ferror(f)) {
		printf("failed to write \n");
		exit(1);
	}
}

static void test8()
{
	f = fopen("/sys/kernel/hacking/lockdep", "w");
	if (!f) {
		printf("open failed\n");
		exit(1);
	}

	pthread_t t1, t2;
	pthread_create(&t1, NULL, (void *(*)(void *))update_one_file, NULL);
	pthread_create(&t2, NULL, (void *(*)(void *))update_one_file, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
}

int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	switch (test) {
	case 8:
		test8();
		break;
	default:
		sysfs_write(test);
	}
	return 0;
}
