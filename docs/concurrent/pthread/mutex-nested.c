// pthread mutex 的 nested（同一线程重复加锁）示例
//
// 编译运行:
//   gcc -Wall -Wextra -O2 mutex-nested.c -o mutex-nested.out -pthread
//   ./mutex-nested.out
//
// pthread_mutex_t 默认是非递归 mutex:
//   同一个线程第一次 pthread_mutex_lock() 成功后，第二次再锁同一把锁
//   会阻塞（PTHREAD_MUTEX_NORMAL 的典型行为），不能拿它做 nested lock。
//
// 如果确实需要 nested lock，要在初始化前把 mutex 类型设为
// PTHREAD_MUTEX_RECURSIVE。它会记录 owner 和递归次数：同一线程每成功
// 加锁一次，计数加一；必须用同样次数的 pthread_mutex_unlock() 才真正释放。

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t recursive_mutex;
static pthread_mutex_t errorcheck_mutex;

static void check_pthread(const char *operation, int rc)
{
	if (rc != 0) {
		fprintf(stderr, "%s: %s\n", operation, strerror(rc));
		exit(EXIT_FAILURE);
	}
}

// 递归调用会在同一个 worker 线程中连续锁住同一把 mutex。
static void nested_function(int level)
{
	check_pthread("recursive lock", pthread_mutex_lock(&recursive_mutex));
	printf("level %d: lock 成功\n", level);

	if (level < 3)
		nested_function(level + 1);

	printf("level %d: unlock\n", level);
	check_pthread("recursive unlock", pthread_mutex_unlock(&recursive_mutex));
}

static void show_recursive_mutex(void)
{
	puts("[1] PTHREAD_MUTEX_RECURSIVE");
	nested_function(1);
	puts("    三次 lock 对应三次 unlock，最后一次 unlock 后 mutex 才可被其他线程获取。\n");
}

static void show_non_recursive_mutex(void)
{
	int rc;

	puts("[2] PTHREAD_MUTEX_ERRORCHECK");
	check_pthread("errorcheck lock (first)",
		      pthread_mutex_lock(&errorcheck_mutex));
	puts("第一次 lock 成功");

	// ERRORCHECK 不会让示例卡死，而是报告同一线程重复加锁的错误。
	rc = pthread_mutex_lock(&errorcheck_mutex);
	if (rc == EDEADLK) {
		puts("第二次 lock 返回 EDEADLK：这把 mutex 不是递归 mutex");
	} else if (rc == 0) {
		puts("第二次 lock 也成功：实现把它当作递归 mutex 处理");
		check_pthread("errorcheck unlock (extra)",
			      pthread_mutex_unlock(&errorcheck_mutex));
	} else {
		fprintf(stderr, "second lock: %s\n", strerror(rc));
	}

	check_pthread("errorcheck unlock (first)",
		      pthread_mutex_unlock(&errorcheck_mutex));
	puts("普通 PTHREAD_MUTEX_NORMAL 的第二次 lock 通常会永久等待，因此这里不直接执行。\n");
}

static void *worker(void *arg)
{
	(void)arg;
	show_recursive_mutex();
	show_non_recursive_mutex();
	return NULL;
}

int main(void)
{
	pthread_mutexattr_t attr;
	pthread_t thread;

	check_pthread("mutexattr init", pthread_mutexattr_init(&attr));
	check_pthread("set recursive type",
		      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
	check_pthread("recursive mutex init",
		      pthread_mutex_init(&recursive_mutex, &attr));
	check_pthread("set errorcheck type",
		      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
	check_pthread("errorcheck mutex init",
		      pthread_mutex_init(&errorcheck_mutex, &attr));
	check_pthread("mutexattr destroy", pthread_mutexattr_destroy(&attr));

	check_pthread("pthread_create", pthread_create(&thread, NULL, worker, NULL));
	check_pthread("pthread_join", pthread_join(thread, NULL));

	check_pthread("recursive mutex destroy",
		      pthread_mutex_destroy(&recursive_mutex));
	check_pthread("errorcheck mutex destroy",
		      pthread_mutex_destroy(&errorcheck_mutex));
	return 0;
}
