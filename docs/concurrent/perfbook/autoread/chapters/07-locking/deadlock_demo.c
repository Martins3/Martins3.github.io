/*
 * deadlock_demo.c: 演示死锁、锁层次结构、条件锁和局部锁层次结构
 *
 * 对应 perfbook 第7章:
 *  - 7.1.1 Deadlock
 *  - 7.1.1.1 Locking Hierarchies
 *  - 7.1.1.6 Conditional Locking
 *  - 7.1.1.8 Signal/Interrupt Handlers
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

/* ========== 1. 死锁演示 (违反锁层次结构) ========== */

pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

/* 线程1: 先获取 A 再获取 B (符合层次) */
void *thread_deadlock_1(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&lock_a);
	printf("[Deadlock-Demo] Thread 1 acquired lock A\n");
	usleep(100000); /* 增加交错概率 */
	pthread_mutex_lock(&lock_b);
	printf("[Deadlock-Demo] Thread 1 acquired lock B\n");
	pthread_mutex_unlock(&lock_b);
	pthread_mutex_unlock(&lock_a);
	return NULL;
}

/* 线程2: 先获取 B 再获取 A (违反层次，导致死锁) */
void *thread_deadlock_2(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&lock_b);
	printf("[Deadlock-Demo] Thread 2 acquired lock B\n");
	usleep(100000);
	pthread_mutex_lock(&lock_a);
	printf("[Deadlock-Demo] Thread 2 acquired lock A\n");
	pthread_mutex_unlock(&lock_a);
	pthread_mutex_unlock(&lock_b);
	return NULL;
}

/* ========== 2. 锁层次结构避免死锁 ========== */

/* 定义锁的层次: A < B，所有线程必须按相同顺序获取 */
void *thread_hierarchy_1(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&lock_a); /* 先获取低层锁 */
	printf("[Hierarchy] Thread 1 acquired A then B\n");
	pthread_mutex_lock(&lock_b);
	usleep(10000);
	pthread_mutex_unlock(&lock_b);
	pthread_mutex_unlock(&lock_a);
	return NULL;
}

void *thread_hierarchy_2(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&lock_a); /* 同样先获取低层锁 */
	printf("[Hierarchy] Thread 2 acquired A then B\n");
	pthread_mutex_lock(&lock_b);
	usleep(10000);
	pthread_mutex_unlock(&lock_b);
	pthread_mutex_unlock(&lock_a);
	return NULL;
}

/* ========== 3. 条件锁 (trylock) 避免死锁 ========== */

pthread_mutex_t lock_c = PTHREAD_MUTEX_INITIALIZER;

void *thread_conditional(void *arg)
{
	int id = *(int *)arg;
	pthread_mutex_t *first, *second;

	/* 模拟需要以任意顺序获取两个锁的场景 */
	if (id == 1) {
		first = &lock_a;
		second = &lock_c;
	} else {
		first = &lock_c;
		second = &lock_a;
	}

retry:
	pthread_mutex_lock(first);
	printf("[Conditional] Thread %d acquired first lock\n", id);
	usleep(50000);

	if (pthread_mutex_trylock(second) != 0) {
		printf("[Conditional] Thread %d trylock failed, release and retry\n", id);
		pthread_mutex_unlock(first);
		usleep(100000); /* 退避 */
		goto retry;
	}

	printf("[Conditional] Thread %d acquired both locks\n", id);
	usleep(10000);
	pthread_mutex_unlock(second);
	pthread_mutex_unlock(first);
	return NULL;
}

/* ========== 4. 信号处理器中的锁 ========== */

pthread_mutex_t sig_lock = PTHREAD_MUTEX_INITIALIZER;
volatile int sig_counter = 0;

void sig_handler(int sig)
{
	(void)sig;
	/*
	 * 在信号处理器中获取锁是危险的！
	 * 如果主线程持有此锁时被信号中断，就会死锁。
	 * 正确做法：在获取可能被信号处理器使用的锁之前屏蔽信号。
	 */
	/* pthread_mutex_lock(&sig_lock);  // 危险！不要这样做 */
	sig_counter++;
	/* pthread_mutex_unlock(&sig_lock); */
}

void demo_signal_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	printf("[Signal] Sending SIGUSR1 to self (safe counter = %d)\n", sig_counter);
	raise(SIGUSR1);
	usleep(10000);
	printf("[Signal] After signal, counter = %d\n", sig_counter);
}

/* ========== 5. 局部锁层次结构: 调用回调前释放锁 ========== */

pthread_mutex_t lib_lock = PTHREAD_MUTEX_INITIALIZER;
int shared_data = 0;

/* 模拟的 "库函数" */
void library_with_callback(void (*callback)(void))
{
	pthread_mutex_lock(&lib_lock);
	shared_data++;
	printf("[Local-Hierarchy] Library updated data to %d\n", shared_data);

	/*
	 * 黄金法则: 在调用未知代码(回调)之前释放所有锁。
	 * 这样回调函数获取的任何锁都不会与 lib_lock 形成死锁循环。
	 */
	pthread_mutex_unlock(&lib_lock);
	callback();
}

void user_callback(void)
{
	/* 用户回调也可以安全地获取它自己的锁 */
	printf("[Local-Hierarchy] Callback executing safely\n");
}

/* ========== 主函数 ========== */

int main(void)
{
	pthread_t t1, t2;
	int id1 = 1, id2 = 2;
	int rc;

	printf("=== Deadlock and Locking Hierarchy Demo ===\n\n");

	/* 1. 演示死锁 (带超时避免永久阻塞) */
	printf("--- 1. Deadlock Scenario (with 1s timeout) ---\n");
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&lock_a, &attr);
	pthread_mutex_init(&lock_b, &attr);

	pthread_create(&t1, NULL, thread_deadlock_1, NULL);
	pthread_create(&t2, NULL, thread_deadlock_2, NULL);

	/* 等待1秒，如果死锁则超时退出 */
	sleep(1);
	printf("[Deadlock-Demo] If threads did not finish, deadlock occurred.\n\n");

	/* 重新初始化锁用于后续演示 */
	pthread_mutex_init(&lock_a, NULL);
	pthread_mutex_init(&lock_b, NULL);

	/* 2. 锁层次结构避免死锁 */
	printf("--- 2. Locking Hierarchy (deadlock-free) ---\n");
	pthread_create(&t1, NULL, thread_hierarchy_1, NULL);
	pthread_create(&t2, NULL, thread_hierarchy_2, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("[Hierarchy] Success!\n\n");

	/* 3. 条件锁避免死锁 */
	printf("--- 3. Conditional Locking (trylock + retry) ---\n");
	pthread_create(&t1, NULL, thread_conditional, &id1);
	pthread_create(&t2, NULL, thread_conditional, &id2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("[Conditional] Success!\n\n");

	/* 4. 信号处理器 */
	printf("--- 4. Signal Handler Safety ---\n");
	demo_signal_handler();
	printf("\n");

	/* 5. 局部锁层次结构 */
	printf("--- 5. Local Locking Hierarchy (release before callback) ---\n");
	library_with_callback(user_callback);
	printf("[Local-Hierarchy] Success!\n\n");

	pthread_mutexattr_destroy(&attr);
	return 0;
}
