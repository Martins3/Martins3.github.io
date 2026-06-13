## 阅读下 Documentation/atomic_t.txt

atomic_fetch_add 和 atomic_add_return : 前者返回之前的数值，后者返回被更新的数值

还是一个关键问题，什么时候使用 acquire ?

问题 1:


```txt
  atomic_{add,sub,inc,dec}()
  atomic_{add,sub,inc,dec}_return{,_relaxed,_acquire,_release}()
  atomic_fetch_{add,sub,inc,dec}{,_relaxed,_acquire,_release}()
```
为什么 atomic_*_return 和 atomic_fetch_* 是含有 acquire 和 release 操作的，
但是 atomic_add 这种没有?


问题 2:

如何理解
```txt
  smp_mb__{before,after}_atomic()
```

```txt
Therefore, if you find yourself only using
the Non-RMW operations of atomic_t, you do not in fact need atomic_t at all
and are doing it wrong.
```
找一些例子看看

问题 3 :

这样的经验规则背后的原因是什么?
```txt
The rule of thumb:

 - non-RMW operations are unordered;

 - RMW operations that have no return value are unordered;

 - RMW operations that have a return value are fully ordered;

 - RMW operations that are conditional are unordered on FAILURE,
   otherwise the above rules apply.
```

## atomic 的 api 比想象的更加复杂啊
https://docs.kernel.org/core-api/wrappers/atomic_t.html

1. atomic_dec_and_raw_lock(atomic, lock)
2. 例如 refcount_t
Reference count (but please see refcount_t):

  atomic_add_unless(), atomic_inc_not_zero()
  atomic_sub_and_test(), atomic_dec_and_test()

3. 例如 barrier 的代码:

Barriers:

  smp_mb__{before,after}_atomic()

4. 更多的 barrier 代码

Non-RMW ops:

  atomic_read(), atomic_set()
  atomic_read_acquire(), atomic_set_release()

## atomic_add_unless 是怎么实现的，又是可以判断，又是可以构造返回值

似乎 x86 中，存在 lock add 的操作直接 atomic 的增加一个数值。

但是这种 atomic_add_unless 看来是需要其他的指令了吧。

也许是底层就提供了 cmpxhg

## 思考下，atomic 在 CPU 中如何实现的?

- [x] atomic 可以跨 cache line 吗?
  - atomic 的本质是 int ，所以不能
2. x86 中的 atomic 直接实现为自带 memory barrier 的，如何验证?
3. arm 中实现为不是自带 memory barrier 的，最后大致需要如何实现的
4. linux 中的 atomic 操作自带 volatile 吗?
  - 我认为是要带的，通过 volatile 来实现，让编译器无法优化

## 单核系统中的可以自动退化吗?

**If the kernel was compiled without SMP support, the operations described are implemented in the same
way as for normal variables (only `atomic_t` encapsulation is observed) because there is no interference
from other processors**

> 似乎并没有实现 volatile 的功能啊。我觉得说法不对。
> 1. 当进程A : i ++;
> 2. 而进程B : i ++;
> 3. 但是进程A 切换到B 的时候，i 的数值没有写会，B写会之后，A再去写会，最后只有i 的数值为1 而不是 2

```c
#ifdef CONFIG_SMP
#define LOCK_PREFIX_HERE \
		".pushsection .smp_locks,\"a\"\n"	\
		".balign 4\n"				\
		".long 671f - .\n" /* offset */		\
		".popsection\n"				\
		"671:"

#define LOCK_PREFIX LOCK_PREFIX_HERE "\n\tlock; "

#else /* ! CONFIG_SMP */
#define LOCK_PREFIX_HERE ""
#define LOCK_PREFIX ""
#endif
```

由于 x86 中，总是使用的 incl 指令，所以不存在这个问题，但是如果


## local_t
`atomic_ops.txt`
`local_t` is very similar to `atomic_t`. If the counter is per CPU and only updated by one CPU, `local_t` is probably more appropriate.

The kernel provides the `local_t` data type for SMP systems. This permits atomic operations on a single
CPU. To modify variables of this kind, the kernel basically makes the same functions available as for the
`atomic_t` data type, but it is then necessary to replace atomic with local.

有趣，还一直没有注意到这个问题

## atomic bitmap : 测试内容还是在 vn/code/src/m/concurrent/atomic.c

测试 Documentation/atomic_bitops.txt 中内容，但是根本看不懂

1. In particular __clear_bit_unlock() suffers the same issue as atomic_set() ???

include/asm-generic/bitops/ 里面有三个文件，正如其名:
- instrumented-atomic.h
- instrumented-lock.h
- instrumented-non-atomic.h

才发现 clear_bit 是 atomic 版本，而 __clear_bit 是 unatomic 版本的。

### test_and_set_bit_lock 和 test_and_set_bit 有什么区别?
如果看 x86 的实现， 使用 ccls 检查，从源码分析上看似乎任何区别

看看 arm64 的实现:

在 include/asm-generic/bitops/lock.h 中:
```c
/**
 * arch_test_and_set_bit_lock - Set a bit and return its old value, for lock
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and provides acquire barrier semantics if
 * the returned value is 0.
 * It can be used to implement bit locks.
 */
static __always_inline int
arch_test_and_set_bit_lock(unsigned int nr, volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	if (READ_ONCE(*p) & mask) // TODO 这个优化，为什么 arch_test_and_set_bit 没有
		return 1;

	old = raw_atomic_long_fetch_or_acquire(mask, (atomic_long_t *)p);
	return !!(old & mask);
}
```

```c
static __always_inline int
arch_test_and_set_bit(unsigned int nr, volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	old = raw_atomic_long_fetch_or(mask, (atomic_long_t *)p);
	return !!(old & mask);
}
```

x86 的 arch_test_and_set_bit_lock 定义在
arch/x86/include/asm/bitops.h 应该是

### 什么时候，我们需要使用 test_and_set_bit ，也就是没有 lock 的版本

显然是有使用需求的，由于是 bitmap ，所以，很容易出现吧别人的
设置给清理掉了。

### test_and_clear_bit 有意义吗?
直接 clear 不可以吗？反正总是需要写成 1

并不是 test_and_clear_bit 有返回值，而 clear_bit 没有返回值。
所以，test_and_clear_bit 是一个 atomic 操作

### 具体案例 : test_and_set_bit_lock

io_worker::create_state 的使用，在函数 io_queue_worker_create 中，

其只是操作了一个 bit 来实现一个最简单互斥。

上锁:
```txt
	if (test_bit(0, &worker->create_state) ||
	    test_and_set_bit_lock(0, &worker->create_state))
		goto fail_release;
```

释放锁:
```txt
	clear_bit_unlock(0, &worker->create_state);
```

不过我奇怪的问题在于，他为什么不去直接使用 atomic ，而是使用

### 具体案例 : test_and_set_bit

__blk_mq_tag_busy 中 blk_mq_hw_ctx::state 的设置

## 问题

test_and_set_bit

既然 test_and_set_bit 是非 lock 版本，为什么反而是设置上 fully-ordered operation ?
```c
/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This is an atomic fully-ordered operation (implied full memory barrier).
 */
static __always_inline bool test_and_set_bit(long nr, volatile unsigned long *addr)
{
	kcsan_mb();
	instrument_atomic_read_write(addr + BIT_WORD(nr), sizeof(long));
	return arch_test_and_set_bit(nr, addr);
}
```

## 思考一个问题
test_bit 没有必要使用 atomic 吧

如果是 set_bit 可能是需要 atomic 的
但是 test_bit 只是读上来，然后看其中的一位，
这个时候的 atomic 的意义是什么呢?

## 这个测试让意识到
/home/martins3/data/vn/code/src/concurrent/cachesize.c

本来以为，相邻的 char 不可以同时写，否则会有问题。但是实际上，
并不是的，，bit 的操作也应该基于同样的道理

原子性不是基于 register 的大小或者 memory 操作的大小的。
而是靠 CPU 自己的支持的。

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
