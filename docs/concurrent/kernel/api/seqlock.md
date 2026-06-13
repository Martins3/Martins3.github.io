## seqlock
<!-- 5a35e8c1-0181-4093-a29d-ef721fae46e9 -->

https://www.kernel.org/doc/html/latest/locking/seqlock.html

基本想法很简单:
1. writer 首先互斥
2. reader 原理，如果在 read 的过程中发现了中间存在 writer 修改过内容，那么就重试。

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

细节的考虑，似乎不仅仅有 spinlock ，但是如果是 mutex 或者 rwlock 的话，
那么岂不是，也可以有两个 lock 吗？


dcache.c:d_lookup 的锁

- 为什么需要使用 memory barrier ?
  - spin lock 可以保证只有一个 writer 存在。
  - 如果不使用，write barrier ，可以相当于 writer 不写该字段了
  ，对于 read barrier 类似。那么就可能让 reader 直接通过了。

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

### raw_write_seqcount_latch 上写了好长的注释哦

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
