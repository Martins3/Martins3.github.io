// TSan 测不到的场景 2: race 所在的代码路径在这次运行中没有执行
//
// TSan 是动态分析工具, 只能发现"本次运行实际执行到"的 race.
// 潜在的 race 如果躲在没跑到的分支里, TSan 给不出任何警告.
//
// 不带参数运行: TSan 安静退出, 但代码里其实埋着一个 race.
// 带 "racy" 参数运行: 同一份代码, TSan 立刻报告 race.
//
// 推论: TSan 报告干净不等于没有 race, 只等于"覆盖到的路径上没有 race".
// 测试用例的覆盖率决定了 TSan 结论的可信度.
#include <pthread.h>
#include <stdio.h>
#include <string.h>

int shared = 0;

void *writer1(void *arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++)
        shared++;
    return NULL;
}

void *writer2(void *arg) {
    // 潜在的 race 藏在这个条件分支里
    if (arg && strcmp((char *)arg, "racy") == 0) {
        for (int i = 0; i < 1000000; i++)
            shared++;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t tid1, tid2;
    char *mode = argc > 1 ? argv[1] : NULL;

    pthread_create(&tid1, NULL, writer1, NULL);
    pthread_create(&tid2, NULL, writer2, mode);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("最终计数值: %d\n", shared);
    return 0;
}
