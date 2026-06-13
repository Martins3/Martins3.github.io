#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void *thread_func(void *arg)
{
	int *data = (int *)arg;
	*data = *data * 2;
	sleep(1);
	printf("Thread doubled the value to: %d\n", *data);
	printf("%d:%d\n", getpid(), gettid());

	return (void *)100;
}

int main()
{
	int value = 5;
	pthread_t thread;

	int ret = pthread_create(&thread, NULL, thread_func, &value);
	if (ret != 0) {
		perror("pthread_create\n");
		return 1;
	}

	/*
	 * pthread 居然使用的 futex 来等待，而不是 waitpid 来等待的
	 */
	void *result = NULL;
	ret = pthread_join(thread, &result);
	if (ret != 0) {
		perror("pthread_join\n");
		return 1;
	}
	printf("Main thread: doubled value = %d\n", value);
	printf("%d:%d\n", getpid(), gettid());

	return 0;
}
