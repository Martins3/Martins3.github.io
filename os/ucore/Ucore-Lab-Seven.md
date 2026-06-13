## 阅读文档
本次实验，主要是熟悉ucore的进程同步机制—信号量（semaphore）机制，以及基于信号量的哲学家就餐问题解决方案。
然后掌握管程的概念和原理，并参考信号量机制，实现基于管程的条件变量机制和基于条件变量来解决哲学家就餐问题。

信号量的设计已经部署，　条件变量的设计

理解底层支撑技术：禁用中断、定时器、等待队列；

根据操作系统原理的知识，我们知道如果没有在硬件级保证读内存-修改值-写回内存的原子性，*我们只能通过复杂的软件来实现同步互斥操作*
> 应该是没有办法实现吧!

通过关闭中断，可以防止对当前执行的控制流被其他中断事件处理所打断。既然不能中断，那也就意味着在内核运行的当前进程无法被打断或被重新调度，即实现了对临界区的互斥操作
> 调度是基于中断实现的，需要熟悉调度的流程是什么

意有“LAB7”的注释，主要是修改condvar.c和check_sync.c中的内容

#### 流程

#### 定时器
https://chyyuu.gitbooks.io/ucore_os_docs/content/lab7/lab7_3_2_1_timer.html

#### 等待队列
https://chyyuu.gitbooks.io/ucore_os_docs/content/lab7/lab7_3_2_3_waitqueue.html


## 阅读源代码
> kern/schedule/{sched.h,sched.c}: 增加了定时器（timer）机制，用于进程/线程的do_sleep功能。
> user/ libs/ {syscall.[ch],ulib.[ch] }与kern/sync/syscall.c：实现了进程sleep相关的系统调用的参数传递和调用关系。
> user/{ sleep.c,sleepkill.c}: 进程睡眠相关的一些测试用户程序。

> kern/sync/sync.h: 去除了lock实现（这对于不抢占内核没用）。
> kern/mm/vmm.[ch]：用信号量mm_sem取代mm_struct中原有的mm_lock。（本次实验不用管）

  之前的哪一个试验使用的mm_lock 的

> kern/sync/wait.[ch]: 定义了等待队列wait_queue结构和等待entry的wait结构以及在此之上的函数，这是ucore中的信号量semophore机制和条件变量机制的**基础**，在本次实验中你需要了解其实现。

> kern/sync/sem.[ch]:定义并实现了ucore中内核级信号量相关的数据结构和函数，本次试验中你需要了解其中的实现，并基于此完成内核级条件变量的设计与实现。
> kern/sync/monitor.[ch]:基于管程的条件变量的实现程序，在本次实验中是练习的一部分，要求完成。

> kern/sync/check_sync.c：实现了基于管程的哲学家就餐问题，在本次实验中是练习的一部分，要求完成基于管程的哲学家就餐问题。

即增加了check_sync函数的调用，而位于lab7_figs/kern/sync/check_sync.c中的check_sync函数可以理解为是实验七的起始执行点，是实验七的总控函数

## 记录
2. wait_t 的作用是什么? 两个变量用于管理queue  然后保存wait_flags 和 proc, 所以本质上wait_t 就是 处于等待状态的proc，但是wait_state 中间 wait_flags和proc 中间的wait_state 的关系是什么
3. `__down()`
```
  wait_current_set(&(sem->wait_queue), wait, wait_state);
```
wait 局部变量， schedule() 之前加入，然后当被选中之后，马上返回，结束当前的内容。

wait_flags 和 wait_state 最后会做出比较来。
`__up` 中间也有比较。 调用参数wait_state 只有 WT_KSEM

4. 信号灯的实现只是白送的而已，monitor 才是关键。

管程中的成员变量信号量next和整型变量next_count是配合进程对条件变量cv的操作而设置的，

monitor 的mutex mutex_next next_count的作用是什么?

直到进程B离开管程，进程A才能继续执行，这个同步过程是通过信号量next完成的
而next_count表示了由于发出singal_cv而睡眠的进程个数

5. 为什么总是要保证只有monitor 中间只有一个程序是 runable 的，cond_signal中间依旧是有若干时钟周期，
同时有两个进程处于 runable 的

6. 根本没有搞清楚为什么要屏蔽中断，什么时候会产生中断，已经什么位置需要屏蔽中断?





## 问题
3. 关于中断的理解，为什么中断可以造成问题，没有关中断的支持，为什么原子操作会失效，其中的原理是什么。
```
  local_intr_save(intr_flag);
  local_intr_restore(intr_flag);
```

4. 什么时候需要添加`__noinline__`关键字维持生活，什么时候不需要。

5. 这些等待的宏到底是如何使用的
```
#define WT_INTERRUPTED 0x80000000 // the wait state could be interrupted
#define WT_CHILD (0x00000001 | WT_INTERRUPTED) // wait child process
#define WT_KSEM 0x00000100                     // wait kernel semaphore
#define WT_TIMER (0x00000002 | WT_INTERRUPTED) // wait timer
```

6. 如果需要重新将list_entry_t 类型定义类型的方法有什么好的吗?
```
typedef struct {
  list_entry_t wait_head;
} wait_queue_t;
```
wait_flags 是做什么用的？

9. 如何实现信号 和 IPC 

10. `__up`中间为什么需要使用一对`{}`来包含代码

## Debug Todo
1. lab6 自己的正常运行(拷贝过entry.S 和 Makefile)，但是result_lab6 不能
2. lab7 两者都不可以，但是result_lab8 还是可以运行
> 暂时不运行了，推测还是类似的问题，毕竟以前可以运行的





## 附录
```
(THU.CST) os is loading ...
……
check_alloc_page() succeeded!
……
check_swap() succeeded!
++ setup timer interrupts
I am No.4 philosopher_condvar
Iter 1, No.4 philosopher_condvar is thinking
I am No.3 philosopher_condvar
……
I am No.1 philosopher_sema
Iter 1, No.1 philosopher_sema is thinking
I am No.0 philosopher_sema
Iter 1, No.0 philosopher_sema is thinking
kernel_execve: pid = 2, name = “matrix”.
pid 14 is running (1000 times)!.
pid 13 is running (1000 times)!.
phi_test_condvar: state_condvar[4] will eating
phi_test_condvar: signal self_cv[4]
Iter 1, No.4 philosopher_condvar is eating
phi_take_forks_condvar: 3 didn’t get fork and will wait
phi_test_condvar: state_condvar[2] will eating
phi_test_condvar: signal self_cv[2]
Iter 1, No.2 philosopher_condvar is eating
phi_take_forks_condvar: 1 didn’t get fork and will wait
phi_take_forks_condvar: 0 didn’t get fork and will wait
pid 14 done!.
pid 13 done!.
Iter 1, No.4 philosopher_sema is eating
Iter 1, No.2 philosopher_sema is eating
……
pid 18 done!.
pid 23 done!.
pid 22 done!.
pid 33 done!.
pid 27 done!.
pid 25 done!.
pid 32 done!.
pid 29 done!.
pid 20 done!.
matrix pass.
all user-mode processes have quit.
init check memory pass.
kernel panic at kern/process/proc.c:426:
    initproc exit.
Welcome to the kernel debug monitor!!
Type 'help' for a list of commands.
K> qemu: terminating on signal 2
```



## 分析
1. run_timer_list timer_list ?
2. wait_state ? 和 timer 向关联起来!

3. semaphore 实现 ?
4. monitor 的原理: mutex 和 条件变量，条件变量包含一些等待变量为真的函数。


#### sem.c
1. down 和 up 的实现 就是简单翻译 semaphore 的原理，value and wait_queue
    1. 由于是单核，单线程，只要禁止中断，就可以保证关键代码不会被重入，应该程序退出的接口只有shedule 函数

2. down
  wait_current_set
    wait_init

3. wait_queue 和 wait_t 的关联 ?
    1. 在wait_queue 上挂一个wait_t 用来保存多余关于wait_queue的信息
    2. wait_queue 只是链表entry

4. wakeup_wait
    1. del ? 被 up 调用的时候，就是 1
    2. wakep_first 和 wakeup_queue 的作用 ? 这一次试验没有用途

何处体现出来其中，队列是顺序是什么? up 的 wakeup_first说明其中内容!

#### monitor
[monitor](https://en.wikipedia.org/wiki/Monitor_(synchronization))
1. In concurrent programming, a monitor is a synchronization construct that allows threads to have both mutual exclusion and the ability to wait (block) for a certain condition to become true.
> 互斥和等待某一个条件为true
2. A monitor consists of a mutex (lock) object and condition variables.

For many applications, mutual exclusion is not enough. Threads attempting an operation may need to wait until some condition P holds true.
> 互斥是不够的，等待某一个变量需要变成一个spin lock 
> 所以 semaphore 和 monitor 的区别是什么 ?

讲解一个bounded producer/consumer problem: 分别为naive spin 和 monitor

Thus there are three main operations on condition variables:
c : conditional variable
m : mutex

1. wait c, m
2. signal c
3. broadcast c 和 signal c 没有什么区别

https://cs.stackexchange.com/questions/43721/why-would-you-use-a-monitor-instead-of-a-semaphore
1. 实际上，monitor 比 semaphore 更加容易使用，因为semaphore 的回收需要process 自己完成，如果一个程序意外结束，没有释放semaphore, GG !
2. Monitors, unlike semaphores, automatically acquire the necessary locks (我不信 !)

https://stackoverflow.com/questions/7335950/semaphore-vs-monitors-whats-the-difference/7336516#7336516

https://cseweb.ucsd.edu/classes/fa05/cse120/lectures/120-l6.pdf


> 分析一波代码
1. monitor struct
    1. mutex : 保证使用monitor 只有一个节点
    2. next ?　这是由于发出signal_cv的进程A会唤醒睡眠进程B，进程B执行会导致进程A睡眠，直到进程B离开管程，进程A才能继续执行，这个同步过
程是通过信号量next完成的 (B 离开monitor ?)
    3. next_count : 表示了由于发出singal_cv而睡眠的进程个数.


发出signal_cv 为什么会导致睡眠？ 当前进程唤醒其他进程的时候，仔细需要休眠。
因为为了保证monitor 中间只有一个进程是处于runable的 ?
monitor 保证其控制的进程总是只有一个进程运行，所以总是唤醒一个进程，然后马上休眠一个。

signal : 唤醒在条件变量上的进程，自己睡眠
wait : 唤醒在其他条件上睡眠的内容，自己睡眠在条件变量上。

2. convar.count 的作用是什么，难道不能从 semaphore 中间获取吗？ 可以


#### timer
1. wait_state
    1. 当WT_INTERRUPT 被是设置初始值，是不是只要中断之后就立刻设置 ?
    2. 当WT_CHILD 和 WT_TIMER的关系?

1. do_sleep()　-> timer_init 
  
2. timer_link 的作用: 所有的使用timer形成列表，表示经过多久之后时钟到来。
    1. 如何处理多个计时器在同一个节点上，直接距离为0
    2. del_timer 的时候让其循环执行? 不是，进程负责 和 run_timer_list 同时处理
    3. timer 如何触发功能？ 时钟

3. run_timer_list  被时钟触发!

> timer 和　锁机制类似，一旦一个进程由于锁进入到等待状态，只能靠对应的机制wake_up，调度器没有办法强行使其 wake_up


## 单核为什么屏蔽中断就可以为所欲为
1. 永远都不用给局部变量上锁
2. 检查一下，当访问全局变量的时候，一定会上锁 还是 关中断 ?
    1. **todo** : 并不是，比如trap.c:trap 中间的 current 访问
    2. 对于单核，current 指针赋值被特地保护过
3. 关中断等于上锁？
    1. 关中断之后只能说明，在打开中断之前，代码的执行不会被抢占，除非代码中间出现 schedule 函数
    2. 想要切换进程，就需要调用 schedule ，但是schedule 只能仅仅在时钟中断中间调用
    3. 为什么上锁 : 临界区不能出现多个进程。关中断，到开中断，或者schedule() 之前，没有办法切换，从而多个进程无法进入
    4. 但是为什么我们还是需要锁 来实现进餐问题 ? 如果把锁机制展开，还是中断屏蔽。
        1. 能够实现spin lock 吗 ? 可以: spin lock 需要原子支持，但是只需要使用中断开关将需要的原子的操作包围即可! 
        2. 当一个线程持有资源，睡眠，第二个线程进入，spin spin spin 知道时间片用完，回到第一个线程， 工作完成，释放资源，第二个spin 结束
        3. 进餐问题所以可以使用spin lock 解决，进餐问题让lock 机制变得明显是因为 : 单核而言，进入者就会持有资源，所以造成一个假象
    5. 所以多核为什么需要原子操作的硬件支持 ?
        1. 即使同时屏蔽了所有核的中断，但是也无法阻止多个eip 的值相同
3. 如何防止在处理中断的时候发生中断 ? 并不会防止，似乎就是支持多级中断的 ?

4. 检查fork 机制中间最后为什么需要IF 特地清空的原因
    1. 仅仅限于`copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf)` 和 load_icode
    2. 问题是 : 如果parent 本身就是就是屏蔽中断的，fork 出来的child 却是不能屏蔽中断，何如?
5. 所有的sti 和 cli 都是通过sync.c 中间的两个函数使用的。
    1. sti 在 intr_enable 出现在 init_proc 之后，如果不打开，那么新创建的进程init_main IF = 0 ，进而导致所有的IF 都是 0
    2. 只是一种措施，防止出现IF = 0 的程序
    3. 用户无法使用sti 和 cli, 从而导致其总是处于修改的状态。
