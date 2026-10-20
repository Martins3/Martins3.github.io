# c++ 的同步设计

## 谈谈 C++ 中的内存顺序
### https://luyuhuang.tech/2022/06/25/cpp-memory-order.html

这六种内存顺序相互组合可以实现三种顺序模型 (ordering model)

- Sequencial consistent ordering. 实现同步, 且保证全局顺序一致 (single total order) 的模型. 是一致性最强的模型, 也是默认的顺序模型.
- Acquire-release ordering. 实现同步, 但不保证保证全局顺序一致的模型.
- Relaxed ordering. 不能实现同步, 只保证原子性的模型.

- [ ] 如果这六种组合为 3 个，测试一个不是组合的模式吧

- [ ] 修改循序相同，如何理解?
  - 似乎有个 textbook 讲过，这个问题的确是需要注意的，修改传递
    - 需要一个例子分析，如果没有修改顺序不同，那么如何处理的
  - 如果 store cache 的时候，需要通知其他的 cache invalid ，如果这个时候有读请求，会如何处理，应该会 stall 吧
  - cache 支持并发写操作吗?
    - 如果 L3 / L2 cache 不支持，性能的有多低啊
    - 如果 L3 / L2 cache 支持，大致该如何设计?
    - 如果 cache 支持并发读写，那么如何保证修改的顺序所有的 core 看到一致


## 一些资料
- https://liam.page/2021/12/11/memory-order-cpp-02/

- https://weakyon.com/2023/07/23/understanding-of-the-cpp-memory-order.html

## [C++ 并发编程实战 2](https://github.com/downdemo/Cpp-Concurrency-in-Action-2ed)

前三章是关于 cpp 的线程的基本的使用，对于不是特别了解 cpp 的人来说（例如我），说实话，难度也是很高的。

### chapter 4

> std::atomic_flag 是一个原子的布尔类型，也是唯一保证 lock-free 的原子类型，只能用 ATOMIC_FLAG_INIT 初始化为 false

std::atomic_flag 功能过于局限，甚至无法像布尔类型一样使用，相比之下，std::atomic<bool> 更易用，它不保证 lock-free，可以用 is_lock_free 检验在当前平台上是否 lock-free

- atomic_flag 和 atomic<bool> 有区别?
- 什么平台上，atomic_flag 不是 lock free 的?
  - 检查下其源码

https://stackoverflow.com/questions/39329311/difference-between-standards-atomic-bool-and-atomic-flag

- [ ] https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange : 如何理解 compare_exchange_weak 和 compare_exchange_strong ?

## 看看官方文档

## https://en.cppreference.com/w/cpp/atomic/memory_order

> std::memory_order specifies how memory accesses, including regular, non-atomic memory accesses, are to be ordered around an atomic operation.

https://stackoverflow.com/questions/67620813/alternatives-to-stdatomic-ref

但是，实际上是存在单独的 fence 工具的:
https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence

## 继续挖掘

- CppCoreGuidelines
  - https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-concurrency
- ABA 问题
  - https://en.wikipedia.org/wiki/ABA_problem
  - https://en.cppreference.com/w/cpp/atomic/atomic_wait

### lock_tag

参考 https://en.cppreference.com/w/cpp/thread/lock_tag

```txt
Type	Effect(s)
defer_lock_t		do not acquire ownership of the mutex
try_to_lock_t	try to acquire ownership of the mutex without blocking
adopt_lock_t		assume the calling thread already has ownership of the mutex
```

- https://stackoverflow.com/questions/18520983/is-stdlock-ill-defined-unimplementable-or-useless
- https://stackoverflow.com/questions/20516773/stdunique-lockstdmutex-or-stdlock-guardstdmutex
- https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes

## https://blog.kernel.love/memory-model.html : really nice blog with cpp perspective

## 类似的这种接口还有多少?
counting_semaphore

## cpp 11 memory model

其实简而言之，就是这样定义的就可以了:
```c
enum memory_order
{
    memory_order_relaxed,
    memory_order_consume, // 这个暂时不用考虑了
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
};
```

```txt
  - __ATOMIC_RELAXED：只保证原子性，不保证额外顺序
  - __ATOMIC_ACQUIRE：成功后，后续读写不能跑到它前面
  - __ATOMIC_RELEASE：成功前，之前的写不能跑到它后面
  - __ATOMIC_ACQ_REL：同时有 acquire + release 效果
  - __ATOMIC_SEQ_CST：最强，提供全局一致的顺序感
```


## cpp 标准库中是没有实现 spinlock 的

## lambda 在并发示例中的两种用法

这里的几个启动方法也是需要学习下的:

这些示例里的 lambda 并不都是"用来创建线程"的，可以分成两类。

1. 作为线程执行体（大多数）, lambda 被交给 thread / jthread / async，在另一个线程里执行：
    - `std::thread t([&] {...})`、`std::jthread worker([] {...})`：直接作为线程入口
      - `cpp-std/locks/mutex/1-lock_tag.cpp`
      - `cpp-std/threading/async-results/promise.cpp`
      - `cpp-std/threading/cancellation/stop_callback.cpp`
    - `threads.emplace_back([&counter]() {...})`：放进 `std::vector<std::thread>` 的也是线程
      - `cpp-std/atomic/basics/atomic_ref.cpp`
    - `std::async(..., [] {...})`：底层同样启动异步任务
      - `cpp-std/atomic/wait-notify/1-wait.cpp`
      - `cpp-std/threading/async-results/1-future.cpp`
      - `cpp-std/threading/overview/1-thread.cpp`
1. 不是线程，只是回调 / 谓词 / 普通函数对象（少数），这里 lambda 本身不创建线程，而是被别的机制调用：
    - `std::call_once` 的回调：只在首次执行，不新开线程
      - `cpp-std/locks/coordination/1-once.cpp`
    - 先存成普通变量，之后才被多个 `std::thread` 传进去执行：
      - `cpp-std/locks/mutex/1-lock_tag.cpp` 的 `auto increment = [...](...) {...}`
      - `cpp-std/locks/rw/rwlock.cpp` 的 `auto increment_and_print = [&counter]() {...}`
    - `std::packaged_task` 的任务体：lambda 是"被打包的任务"，线程稍后才来取
      - `cpp-std/threading/async-results/1-future.cpp`
      - `cpp-std/threading/async-results/1-thread-2.cpp`
    - 条件变量的 wait 谓词：用来判断"条件是否成立"
      - `cpp-std/threading/cancellation/stop_callback.cpp` 的 `wait(lock, stoken, [&stoken] {...})`
    - `std::stop_callback` 的钩子：由 `request_stop()` 触发，不是线程执行体
      - `cpp-std/threading/cancellation/stop_callback.cpp`

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
