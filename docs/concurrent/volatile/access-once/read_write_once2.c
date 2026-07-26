#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define __READ_ONCE(x) (*(const volatile int *)&(x))

#define READ_ONCE(x) ({ __READ_ONCE(x); })

static void badwait(int *stop)
{
	while (!*stop) {
	}
}

static void badwait_once(int *stop)
{
	while (!READ_ONCE(*stop)) {
	}
}

static void *thread_func(void *arg)
{
	/*
	 * arm 环境 clang version 18.1.8 (Fedora 18.1.8-1.fc40)
	 *
	 * 调用的函数可以直接没优化到没有
	 *
	 * mov     w0, #0x64                       // #100
	 * ret
	 */
	badwait(arg);
	return (void *)100;
}

static void *thread_func2(void *arg)
{
	/*
	 * arm 环境 clang version 18.1.8 (Fedora 18.1.8-1.fc40)
	 *
	 * ldr     w8, [x0]
	 * cbz     w8, 0x410350 <thread_func2>
	 * mov     w0, #0xc8                       // #200
	 * ret
	 */
	badwait_once(arg);
	return (void *)200;
}

int main()
{
	int value;
	scanf("%d", &value);
	printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, value);
	pthread_t thread;
	pthread_t thread2;

	int ret = pthread_create(&thread, NULL, thread_func, &value);
	if (ret != 0) {
		perror("pthread_create\n");
		return 1;
	}

	ret = pthread_create(&thread2, NULL, thread_func2, &value);
	if (ret != 0) {
		perror("pthread_create\n");
		return 1;
	}

	void *result = NULL;
	ret = pthread_join(thread, &result);
	if (ret != 0) {
		perror("pthread_join\n");
		return 1;
	}
	ret = pthread_join(thread2, &result);
	if (ret != 0) {
		perror("pthread_join\n");
		return 1;
	}
	printf("Main thread: doubled value = %d\n", value);
	printf("%d:%d\n", getpid(), gettid());

	return 0;
}
