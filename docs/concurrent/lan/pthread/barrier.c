// 当你运行程序时，你会观察到：
// “已到达屏障” 的消息会因为随机等待而无序地、间隔地出现。
// 一旦第5个线程打印出 “已到达屏障”，所有 “已通过屏障” 的消息会几乎在同一瞬间被打印出来。
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 5

pthread_barrier_t barrier;

void *worker(void *arg)
{
	long id = (long)arg;
	int wait_sec = (rand() % 4) + 1;
	printf("线程 %ld 准备, 将等待 %d 秒\n", id, wait_sec);
	sleep(wait_sec);

	printf("线程 %ld 已到达屏障\n", id);
	int rc = pthread_barrier_wait(&barrier);
	if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
		printf("无法等待屏障\n");
		exit(-1);
	}

	printf("线程 %ld 已通过屏障\n", id);
	return NULL;
}

int main()
{
	pthread_t threads[NUM_THREADS];
	srand(time(NULL));

	if (pthread_barrier_init(&barrier, NULL, NUM_THREADS)) {
		printf("无法初始化屏障\n");
		return -1;
	}

	for (long i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, &worker, (void *)i)) {
			printf("无法创建线程\n");
			return -1;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	pthread_barrier_destroy(&barrier);
	return 0;
}
