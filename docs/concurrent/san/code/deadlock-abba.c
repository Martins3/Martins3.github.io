// TSan 能检测的死锁: 互斥锁获取顺序不一致造成的潜在死锁 (ABBA)
//
// 前提: TSAN_OPTIONS="detect_deadlocks=1", 且必须用 clang 的 runtime
// (gcc 的 TSan 不支持死锁检测, 见 README.md).
//
// 这里刻意让 t1 跑完再跑 t2, 程序本身不会死锁.
// 但 TSan 在锁顺序图上先后看到 A->B 和 B->A 两条边, 构成环,
// 于是报告 lock-order-inversion (potential deadlock).
//
// 注意: 边是在"持有一把锁时又成功拿到另一把锁"时添加的,
// 所以它报告的是运行中观察到的危险顺序, 不要求死锁真实发生.
// 反过来, 如果两种顺序在时间上真的重叠, 程序就真挂了,
// 那时反而什么都报不出来 (见 deadlock-real.c).
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

void *t1(void *arg) {
    (void)arg;
    pthread_mutex_lock(&A);
    pthread_mutex_lock(&B);   // 锁顺序 A->B
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

void *t2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&B);
    pthread_mutex_lock(&A);   // 锁顺序 B->A, 与 t1 相反
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}

int main(void) {
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, t1, NULL);
    pthread_join(tid1, NULL);   // t1 完整跑完, 不会真死锁
    pthread_create(&tid2, NULL, t2, NULL);
    pthread_join(tid2, NULL);

    printf("正常结束 (但 TSan 已报告潜在死锁)\n");
    return 0;
}
