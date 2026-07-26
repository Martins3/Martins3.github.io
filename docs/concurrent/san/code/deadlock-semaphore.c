// TSan 测不到的死锁 2: 由信号量等其他同步对象引起的死锁
//
// 和 deadlock-real.c 结构完全一样的 ABBA 顺序反转, 只是把
// pthread_mutex 换成 POSIX 信号量. TSan 的死锁检测只跟踪
// 互斥锁, 不跟踪信号量 (也不跟踪自旋锁; 读写锁的支持也很有限),
// 所以即使开了 detect_deadlocks=1 同样一片安静, 程序挂死.
//
// 运行效果: 无报告, timeout 杀掉 (退出码 124).
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

sem_t A, B;

void *t1(void *arg) {
    (void)arg;
    sem_wait(&A);
    usleep(1000);
    sem_wait(&B);               // 永远等不到
    sem_post(&B);
    sem_post(&A);
    return NULL;
}

void *t2(void *arg) {
    (void)arg;
    sem_wait(&B);
    usleep(1000);
    sem_wait(&A);               // 永远等不到
    sem_post(&A);
    sem_post(&B);
    return NULL;
}

int main(void) {
    pthread_t tid1, tid2;

    sem_init(&A, 0, 1);
    sem_init(&B, 0, 1);

    pthread_create(&tid1, NULL, t1, NULL);
    pthread_create(&tid2, NULL, t2, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("正常结束 (实际不会发生)\n");
    return 0;
}
