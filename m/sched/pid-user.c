#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../user/sysfs.h"

void update(void)
{
	sysfs_write(0);
}

int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	switch (test) {
	case 0:
		sysfs_write(0);
		pthread_t t1;
		pthread_create(&t1, NULL, (void *(*)(void *))update, NULL);
		pthread_setname_np(t1, "child");
		pthread_join(t1, NULL);
		break;
	}
	return 0;
}
