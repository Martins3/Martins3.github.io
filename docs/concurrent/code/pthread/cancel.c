#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void cleanup_handler(void *arg)
{
	printf("清理函数被调用: %s\n", (char *)arg);
}

char msg[] = "释放资源";
void *worker_thread(void *arg)
{
	// 靠，这就是一个宏，构成了一个 while 循环
	pthread_cleanup_push(cleanup_handler, (void *)msg);

	printf("子线程启动，进入无限循环...\n");
	while (1) {
		// sleep() 是一个取消点，线程会在这里检查取消请求
		sleep(1);
		printf("子线程正在运行...\n");
	}

	// 这行代码永远不会被执行，但 push 必须和 pop 配对
	pthread_cleanup_pop(0);
	return NULL;
}

int main()
{
	pthread_t tid;
	void *thread_result;

	if (pthread_create(&tid, NULL, worker_thread, NULL) != 0) {
		perror("pthread_create");
		return 1;
	}

	printf("主线程等待 3 秒后取消子线程...\n");
	sleep(3);

	printf("主线程正在取消子线程...\n");
	if (pthread_cancel(tid) != 0) {
		perror("pthread_cancel");
		return 1;
	}

	if (pthread_join(tid, &thread_result) != 0) {
		perror("pthread_join");
		return 1;
	}

	if (thread_result == PTHREAD_CANCELED) {
		printf("主线程确认子线程已被成功取消。\n");
	} else {
		printf("子线程没有被取消。\n");
	}

	return 0;
}
