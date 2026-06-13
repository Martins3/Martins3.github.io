/*
 * 用于制作 TLB_REMOTE_WRONG_CPU 的
 *
 *
 *   多线程 + 频繁内存操作 + 多核系统（128核） 的组合导致：
 *
 *  1. 线程频繁创建/销毁，每个线程有独立的 mm_struct
 *  2. mprotect 触发 flush_tlb_mm_range()，发送 IPI 给其他 CPU
 *  3. 目标 CPU 在 IPI 到达前发生了上下文切换
 *  4. 结果：IPI 到达时 CPU 已不在目标 mm 上下文中 → TLB_REMOTE_WRO
 *     CPU
 *
 * 你的 128 核系统让这种竞态条件更容易发生，因为：
 *  - 更多 CPU 同时运行不同线程
 *  - IPI 传播延迟更大
 *  - 上下文切换更频繁
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sched.h>

#define NUM_THREADS 16
#define ITERATIONS 1000
#define MAP_SIZE (4096 * 4)

static volatile int start_flag = 0;

void *worker(void *arg)
{
    int id = *(int*)arg;

    while (!start_flag)
        sched_yield();

    for (int i = 0; i < ITERATIONS; i++) {
        void *addr = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) continue;

        memset(addr, id & 0xFF, MAP_SIZE);

        mprotect(addr, MAP_SIZE, PROT_READ);
        mprotect(addr, MAP_SIZE, PROT_READ|PROT_WRITE);

        munmap(addr, MAP_SIZE);

        if ((i & 0x1F) == 0) {
            sched_yield();
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    printf("TLB 测试: %d 线程 x %d 迭代\n", NUM_THREADS, ITERATIONS);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    printf("开始!\n");
    start_flag = 1;

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("完成\n");
    return 0;
}

