// TSan 测不到的场景 1: race 发生在内联汇编里
//
// TSan 靠编译器对 C/C++ 内存访问的插桩工作, 内联汇编 (以及未插桩的
// 外部库、syscall 内部的内存访问) 对它完全不可见.
//
// 这里两个线程用不带 lock 前缀的 incl 指令做 RMW, 是真实的 data race
// (从最终计数值小于 2000000 可以证实), 但 TSan 一声不吭.
//
// 反过来, 如果用内联汇编手写自旋锁来保护临界区, TSan 因为看不到锁,
// 反而会误报 race. 可见 "看不到同步原语" 既造成漏报也造成误报.
#include <pthread.h>
#include <stdio.h>

int shared_counter = 0;

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++) {
        // 等价于 shared_counter++, 但 TSan 无法插桩
        __asm__ volatile("incl %0" : "+m"(shared_counter));
    }
    return NULL;
}

int main(void) {
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, worker, NULL);
    pthread_create(&tid2, NULL, worker, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("最终计数值: %d\n", shared_counter);
    printf("理论期望值: 2000000\n");
    if (shared_counter != 2000000) {
        printf("race 真实发生了, 但 TSan 没有报告\n");
    }
    return 0;
}
