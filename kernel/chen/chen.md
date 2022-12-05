# 用"芯"探核 基于龙芯的 Linux 内核探索解析

P69
> rest_init() 函数的主要作用是通过 kernel_thread() 来创建 1 号进程 kernel_init 和 2 号进程 kthreadd(实际上是两个内核线程).
> 1 号进程执行函数是 kernel_init(), 它完成接下来的大部分初始化工作，后面的章节会详细介绍它。2 号进程则是除 0,1,2号进程之外其他所有内河线程的祖先。

使用 for_each_process 将进程的名称和 pid 出来，可以看到:
```
[258364.051244] systemd [1]
[258364.051254] kthreadd [2]
```
所以，0 号进程完成部分初始化，变成 idle
2 号产生 kernel_thread 但是 orphan 挂到 1 号上
1 号是 systemd
