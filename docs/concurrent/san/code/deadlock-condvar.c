// TSan 测不到的死锁 3: 与锁顺序无关的死等
//
// worker 在条件变量上等待, main 从不 signal 就去 join,
// 程序永远挂起. 这里没有锁顺序反转, 不属于死锁检测器的
// 覆盖范围 (TSan 认识 pthread_cond_wait 建立的 happens-before,
// 但"没人来 signal"不是锁顺序图里的环).
//
// 运行效果: 无报告, timeout 杀掉 (退出码 124).
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

void *worker(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m);
    while (!ready)
        pthread_cond_wait(&cond, &m);   // 永远不会被 signal
    pthread_mutex_unlock(&m);
    return NULL;
}

int main(void) {
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);
    // 忘了 signal / broadcast, 直接 join
    pthread_join(tid, NULL);

    printf("正常结束 (实际不会发生)\n");
    return 0;
}
