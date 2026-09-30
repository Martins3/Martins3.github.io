#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 5 // 共享缓冲区的大小
#define NUM_ITEMS 10 // 生产者要生产的物品总数

// --- 全局共享资源 ---
int buffer[BUFFER_SIZE];
int in_index = 0; // 生产者写入的位置
int out_index = 0; // 消费者读取的位置

sem_t filled_slots; // 记录已填充槽位的信号量
sem_t empty_slots; // 记录空闲槽位的信号量
pthread_mutex_t mutex; // 保护缓冲区的互斥锁

// --- 生产者线程函数 ---
void *producer(void *arg)
{
	for (int i = 0; i < NUM_ITEMS; ++i) {
		int item = rand() % 100; // 生产一个随机物品

		// 1. 等待一个空闲槽位 (P操作)
		printf("Producer: Waiting for an empty slot...\n");
		sem_wait(&empty_slots);

		// 2. 锁定互斥锁，进入临界区
		pthread_mutex_lock(&mutex);

		// --- 临界区开始 ---
		buffer[in_index] = item;
		printf("Producer: Produced item %d at index %d\n", item,
		       in_index);
		in_index = (in_index + 1) % BUFFER_SIZE;
		// --- 临界区结束 ---

		// 3. 解锁互斥锁
		pthread_mutex_unlock(&mutex);

		// 4. 通知有一个新的填充槽位 (V操作)
		sem_post(&filled_slots);

		sleep(1); // 模拟生产耗时
	}
	pthread_exit(NULL);
}

// --- 消费者线程函数 ---
void *consumer(void *arg)
{
	for (int i = 0; i < NUM_ITEMS; ++i) {
		// 1. 等待一个已填充的槽位 (P操作)
		printf("Consumer: Waiting for a filled slot...\n");
		sem_wait(&filled_slots);

		// 2. 锁定互斥锁，进入临界区
		pthread_mutex_lock(&mutex);

		// --- 临界区开始 ---
		int item = buffer[out_index];
		printf("Consumer: Consumed item %d from index %d\n", item,
		       out_index);
		out_index = (out_index + 1) % BUFFER_SIZE;
		// --- 临界区结束 ---

		// 3. 解锁互斥锁
		pthread_mutex_unlock(&mutex);

		// 4. 通知有一个新的空闲槽位 (V操作)
		sem_post(&empty_slots);

		sleep(2); // 模拟消费耗时
	}
	pthread_exit(NULL);
}

int main()
{
	pthread_t producer_thread, consumer_thread;
	srand(time(NULL));

	// --- 初始化 ---
	// 初始化互斥锁
	pthread_mutex_init(&mutex, NULL);

	// 1. sem_init(): 初始化信号量
	// 第一个参数是信号量指针
	// 第二个参数 pshared: 0 表示在当前进程的线程间共享, 非0 表示在进程间共享
	// 第三个参数 value: 信号量的初始值
	sem_init(&filled_slots, 0, 0); // 初始时没有填充的槽位
	sem_init(&empty_slots, 0, BUFFER_SIZE); // 初始时所有槽位都是空的

	printf("Starting producer and consumer threads...\n");

	// 创建线程
	pthread_create(&producer_thread, NULL, producer, NULL);
	pthread_create(&consumer_thread, NULL, consumer, NULL);

	// 等待线程结束
	pthread_join(producer_thread, NULL);
	pthread_join(consumer_thread, NULL);

	// --- 清理 ---
	// 2. sem_destroy(): 销毁信号量
	sem_destroy(&filled_slots);
	sem_destroy(&empty_slots);
	pthread_mutex_destroy(&mutex);

	printf("All items produced and consumed. Program finished.\n");

	return 0;
}
