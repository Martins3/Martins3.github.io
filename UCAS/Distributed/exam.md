分布式：
考五个题

1. RPC相关
	CDK第5章
	ppt: 2plus3chapter21第43张开始

2. 传染病协议
	分布式系统原理与范型6.4.3
	ppt: 2plus3chapter21第13张开始
3. Raft 协议  记功能和特征
	只能参考论文
	ppt: 6chapter4第40页ppt开始

4. Chord 协议  比较细 要会给出例子写步骤
	ppt: 5plus6chapter4第13,20张

5. 分布式系统的可伸缩性，针对某些分布式系统，要求你改进它的可伸缩性
	ppt: 1chapter11第37页开始

并行：
1.openmp的各种directive，包括omp parallel, omp for，private，critical section等。
2. MPI的消息发送与接收函数，及课上讲过的聚合通信函数。
3. pthread的线程create, join、信号量的用法。
4. HPL里spread和roll的流程。

```c
  int i;
  int counter = 0;
#pragma omp parallel private( counter)
  for (i = 0; i < 3; ++i) { // 将此处的i 申明为 parallel for 为局部变量，那么执行的次数将会是 6 
  // 否则指向的次数为4，因为i 是被两个变量共享的
  // 如果是添加 private for 那么 private 中间放不放 i 就没有任何具体的意义了
    counter ++;
    printf("%d  counter : %d\n", i, counter);
```

## Epidemic
两个公式的来源

230
## RPC
不知道5个字是什么，感觉 at-least-once 和 at-most-once

## scability
29

## Chord
89
finger table 中间 up to m
两个公式的内容

## Raft

