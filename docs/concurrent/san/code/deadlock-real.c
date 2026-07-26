// TSan 测不到的死锁 1: 真实发生的死锁
//
// 和 deadlock-abba.c 完全相同的 ABBA 顺序反转, 只是让两个线程
// 在时间上重叠 (各自拿着第一把锁睡 1ms, 再去抢第二把),
// 程序几乎必然真死锁.
//
// 死锁发生后 TSan 自己也随进程一起卡住, 什么报告都打不出来:
// 锁顺序图的边只在"成功拿到嵌套锁"时添加, 两个线程都卡在
// 第二次 lock 上, 边根本没建成, 检测器无从发现环.
//
// 运行效果: 即使开了 detect_deadlocks=1 也一片安静,
// 只能靠 timeout 杀掉 (退出码 124).
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

void *t1(void *arg) {
    (void)arg;
    pthread_mutex_lock(&A);
    usleep(1000);               // 让 t2 有时间拿到 B
    pthread_mutex_lock(&B);     // 永远等不到
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

void *t2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&B);
    usleep(1000);               // 让 t1 有时间拿到 A
    pthread_mutex_lock(&A);     // 永远等不到
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}

int main(void) {
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, t1, NULL);
    pthread_create(&tid2, NULL, t2, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("正常结束 (实际不会发生)\n");
    return 0;
}
