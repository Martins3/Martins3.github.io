# lock

## seqlock
dcache.c:d_lookup 的锁


## lockless
让人想起了 slab 的内容:
https://lwn.net/SubscriberLink/827180/a1c1305686bfea67/

## RCU
What is Rcu, Really[^1]:
RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete. 


## futex
基本介绍 [^2]

这个解释了，既然可以使用 userspace 的 spinlock，为什么还是要使用内核:
https://linuxplumbersconf.org/event/4/contributions/286/attachments/225/398/LPC-2019-OptSpin-Locks.pdf


用户态的lock:
```c
void lock() {
  while (__sync_lock_test_and_set(&exclusion, 1)) {
    // Do nothing. This GCC builtin instruction
    // ensures memory barrier.
  }
}

void unlock() {
  __sync_synchronize(); // Memory barrier.
  exclusion = 0;
}
```

## set_tid_address
从 man 和 [^3] 看，似乎是配合 pthread 用于 pthread_join，而且会进一步依赖于 futex 来操作




## TODO
考试:
1. spinlock 和 spinlock_bh
2. ksoftirqd 的优先级
3. memcg 如何操作 slab (本来认为 slab 作为内核的部分，不会被 memcg 控制)
4. ticket spinlock


[^1]: https://lwn.net/Articles/262464/
[^2]: https://eli.thegreenplace.net/2018/basics-of-futexes/
[^3]: https://stackoverflow.com/questions/6975098/when-is-the-system-call-set-tid-address-used
