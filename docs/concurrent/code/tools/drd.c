#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局共享变量
int shared_counter = 0;
// 用于保护共享变量的互斥锁
pthread_mutex_t lock;

// 线程函数，对共享变量进行递增
void *worker(void *arg) {
    // 将命令行参数（是否启用保护）传递给线程
    int use_protection = *(int*)arg;

    for (int i = 0; i < 1000000; i++) {
        // 如果启用了保护，则加锁
        if (use_protection) {
            pthread_mutex_lock(&lock);
        }

        // 临界区：读写共享变量
        shared_counter++;

        // 如果启用了保护，则解锁
        if (use_protection) {
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t tid1, tid2;
    int use_protection = 0;

    // 检查命令行参数，如果为 "protect"，则启用保护
    if (argc > 1 && strcmp(argv[1], "protect") == 0) {
        use_protection = 1;
        printf("保护模式已启用。\n");
    } else {
        printf("保护模式已关闭（数据争用将会发生）。\n");
    }

    // 初始化互斥锁
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("mutex init failed");
        return 1;
    }

    // 创建两个线程，并将 use_protection 标志传递给它们
    if (pthread_create(&tid1, NULL, worker, &use_protection) != 0) {
        perror("pthread_create failed");
        return 1;
    }
    if (pthread_create(&tid2, NULL, worker, &use_protection) != 0) {
        perror("pthread_create failed");
        return 1;
    }

    // 等待两个线程执行完毕
    if (pthread_join(tid1, NULL) != 0) {
        perror("pthread_join failed");
        return 1;
    }
    if (pthread_join(tid2, NULL) != 0) {
        perror("pthread_join failed");
        return 1;
    }

    // 销毁互斥锁
    pthread_mutex_destroy(&lock);

    // 打印最终结果
    printf("\n最终计数值: %d\n", shared_counter);
    printf("理论期望值: 2000000\n\n");

    return 0;
}
