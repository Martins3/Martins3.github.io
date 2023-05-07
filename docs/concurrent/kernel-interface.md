# lock
- [ ] qspinlock : zhihu 专栏的 lanxinyu 的文章
- [ ] 总结一下内核中带锁的位置
- [ ] 我们应该从 lock 的角度来重新思考内核的各种模块
  - vfs 的那些元素需要保护: inode dcache
  - memory 中的 : lru 的链表
- [ ] 验证一下，如果只有单核，内核中很多的 lock 都是会简化的
- [ ] 内核中 lock 和用户态的差别
  - [ ]  内核中存在 conditionnal variables 吗? 如果没有
    - 显然是可以实现的，但是似乎没有见到过
    - 似乎是被替代为 wait queue 了，如果的确是，为什么？

## qspinlock

## seqlock
dcache.c:d_lookup 的锁

简而言之:
```c
static inline void write_seqlock(seqlock_t *sl)
{
	spin_lock(&sl->lock);
	do_write_seqcount_begin(&sl->seqcount.seqcount);
}

static inline void do_raw_write_seqcount_end(seqcount_t *s)
{
	smp_wmb();
	s->sequence++;
}

static inline unsigned read_seqbegin(const seqlock_t *sl)
{
	return read_seqcount_begin(&sl->seqcount);
}
```

- 为什么需要使用 memory barrier ?
  - spin lock 可以保证只有一个 writer 存在。
  - 如果不使用，write barrier ，可以相当于 writer 不写该字段了，对于 read barrier 类似。那么就可能让 reader 直接通过了。

### QEMU 的实现

include/qemu/seqlock.h 中，更加简洁明了:

```c
/* Lock out other writers and update the count.  */
static inline void seqlock_write_lock_impl(QemuSeqLock *sl, QemuLockable *lock)
{
    qemu_lockable_lock(lock);
    seqlock_write_begin(sl);
}
```

### rwlock 和 seqlock 的差别
https://stackoverflow.com/questions/55746320/why-rwlock-is-more-popular-than-seqlock-in-linux-kernel

### 其他
- 其他的实现 https://github.com/rigtorp/Seqlock

## TODO
- [ ]  spinlock 和 spinlock_bh
- [ ]  ksoftirqd 的优先级
- [ ]  ticket spinlock


## [Unreliable Guide To Locking](https://www.kernel.org/doc/html/latest/kernel-hacking/locking.html)

介绍各种 kernel 的 lock interface

## [Local locks in the kernel](https://lwn.net/Articles/828477/)
对于访问 percpu 的数据，就像是单核执行一下，是需要屏蔽 preempt_disable 的

但是无法理解，如果允许抢占了，之后只要抢占的 process 运行到相同的位置，那么不是一定 dead lock 吗？

## mutex

实现起来比想想的复杂
