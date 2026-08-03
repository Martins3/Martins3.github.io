/*
 * deepseek 生成
 *
 * 用这个例子来说明，当 eventfd 也可以用于用户态的通信。
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <stdint.h>

#define NUM_EVENTS 5

/**
 * @brief 生产者线程函数
 * * @param arg 指向 eventfd 文件描述符的指针
 * @return NULL
 */
static void *producer_thread(void *arg)
{
	int efd = *(int *)arg;

	printf("[Producer] Starting. Will send %d events.\n", NUM_EVENTS);

	for (int i = 0; i < NUM_EVENTS; i++) {
		// 模拟一些工作
		sleep(1);

		uint64_t event_value = 1;
		printf("[Producer] Signaling event #%d...\n", i + 1);

		// 向 eventfd 写入一个值，这将唤醒正在 read() 的消费者
		ssize_t s = write(efd, &event_value, sizeof(event_value));
		if (s != sizeof(event_value)) {
			perror("write to eventfd");
			break;
		}
	}

	printf("[Producer] Finished.\n");
	return NULL;
}

/**
 * @brief 消费者线程函数
 * * @param arg 指向 eventfd 文件描述符的指针
 * @return NULL
 */
static void *consumer_thread(void *arg)
{
	int efd = *(int *)arg;
	int events_consumed = 0;

	printf("[Consumer] Starting. Waiting for events...\n");

	while (events_consumed < NUM_EVENTS) {
		uint64_t event_value;

		printf("[Consumer] Waiting for an event...\n");

		// 从 eventfd 读取。如果内核计数器为0，这个调用会阻塞。
		ssize_t s = read(efd, &event_value, sizeof(event_value));
		if (s != sizeof(event_value)) {
			perror("read from eventfd");
			break;
		}

		// 默认情况下，read() 会返回计数器的值并将其清零
		printf("[Consumer] Woke up! Received event with value: %llu\n",
		       (unsigned long long)event_value);
		events_consumed += event_value;
	}

	printf("[Consumer] Finished after consuming %d events.\n",
	       events_consumed);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t producer, consumer;
	int efd;

	// 创建一个 eventfd 对象
	// 第一个参数是初始计数值
	// 第二个参数是标志位，0 表示默认行为
	efd = eventfd(0, 0);
	if (efd == -1) {
		perror("eventfd");
		return 1;
	}

	// 创建生产者和消费者线程，并将 eventfd 传递给它们
	if (pthread_create(&producer, NULL, producer_thread, &efd) != 0) {
		perror("pthread_create producer");
		close(efd);
		return 1;
	}
	if (pthread_create(&consumer, NULL, consumer_thread, &efd) != 0) {
		perror("pthread_create consumer");
		close(efd);
		return 1;
	}

	// 等待线程结束
	pthread_join(producer, NULL);
	pthread_join(consumer, NULL);

	// 关闭文件描述符
	close(efd);

	return 0;
}
