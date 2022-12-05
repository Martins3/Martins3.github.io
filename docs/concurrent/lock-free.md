# wait free，lockfree 和 obstruction free 区分
## 1 定义
使用[这个](http://www.cs.swan.ac.uk/~csdavec/HPC/11LockFreedom.pdf)ppt 的定义:
1. wait free

> Every operation is bounded on the number of steps before completion.
> 或者定义为:
> A method is wait-free if it guarantees that every call finishes its execution in a finite number of steps.

2. lock-free

> - At least one thread must be able to make progress at any given time
> - Eventually, all threads must make progress
> - Given infinite time, infinitely many threads will progress

3. obstruction-free

> A single thread, with all other threads paused, may complete its work.

说明: [The Art of Multiprocessor Programming](https://book.douban.com/subject/24700553/) 对于 lock-free 的定义有点模糊，内容如下，但是表达的意思是类似的，一个函数有可能在有限步骤内无法返回。
例子就是，无锁通用构造(多处理器编程艺术 ch6)中间某一个线程的 apply 函数由于其他线程竞争成功始终失败，将其转变为无等待通用构造的方法则是利用"助人为乐"，保证所有线程调用 apply 总是可以有限步返回。
> A method is lock-free if it guarantees that infinitely often some method call finishes in a finite number of steps.


## 2 性质
1. 所有 wait free 都是 lock free，所有的 lock free 都是 obstruction free。
2. [是否能举一个是 obstruction-free 但是不是 lock free 的例子 ?](https://stackoverflow.com/questions/46489805/what-is-an-example-of-obstruction-free-algorithm-that-is-not-lock-free) 

存在，详细的内容请查看链接，此处简单描述。如果对于一个 stack 两个线程分别进行 push 和 pop，而且 push 和 pop  由于某些需要两步完成，那么这两个操作会因为各自执行了第一个操作而导致对方的第二步操作无法进行。但是当系统中间只有一个进程的时候，该线程可以完成工作。

## 3 为什么要使用 wait free
利用 cas 构成各种 wait free 算法中间其实还是含有大量的循环（失败的线程需要不反复尝试，直到成功），这和 spin lock 的循环似乎没有什么区别，即不能带来性能提升，而且 wait free 算法似乎更加难以设计，所以为什么还是要使用 wait free 呢？
[参考](https://stackoverflow.com/questions/5680869/do-lock-free-algorithms-really-perform-better-than-their-lock-full-counterparts)，我的想法是:
1. 性能取决于 wait free 具体算法的实现，运行的程序，可能高，可能低。

> There are so many variants... tagged pointers, epoch based reclamation, RCU/quiescent state, hazard pointers, general process-wide garbage collection, etc. All these strategies have performance implications, and some also place restrictions on how your application in general can be designed.

> If preemption in the middle of a critical section is sufficiently rare, then dependent blocking progress conditions are effectively indistinguishable from their nonblocking counterparts. If preemption is common enough to cause concern, or if the cost of preemption-based delay is sufficiently high, then it is sensible to consider nonblocking progress conditions.

2. wait free 的核心是:防止出现死锁，即为当持有锁的 thread 不释放锁，其他的线程只能干看着。但是 wait free 不会出现，所有的线程只会不停的执行 CAS 指令，如果失败重新尝试。

> not all threads can be blocked by a sudden delay of one or more other threads. 

> The absolute wait-free and lock-free progress properties have good
theoretical properties, they work on just about any platform, and they provide
real-time guarantees useful to applications such as music, electronic games, and
other interactive applications. The dependent obstruction-free, deadlock-free,
and starvation-free properties rely on guarantees provided by the underlying
platform. **Given those guarantees, however, the dependent properties often admit
simpler and more efficient implementations**
