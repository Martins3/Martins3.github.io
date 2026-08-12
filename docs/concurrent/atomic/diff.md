## 对比 x86 arm 和 risc-v

arch/x86/include/asm/barrier.h


x86 和 arm 实现 barrier 都是如此, rsic-v 也是
```c
/* Optimization barrier */
#ifndef barrier
/* The "volatile" is due to gcc bugs */
# define barrier() __asm__ __volatile__("": : :"memory")
#endif
```

- [ ] 但是为什么 barrier 要如此实现，没有更好的办法吗?
  - 这样实现充分吗?

2. x86 中只有 smp_mb 的实现不同

1. smp_store_release 的实现是不是有点出乎意料了
```c
#define smp_store_release(p, v) do { kcsan_release(); __smp_store_release(p, v); } while (0)

#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	barrier();							\
	WRITE_ONCE(*p, v);						\
} while (0)
```

但是 arm 的实现中: 9 个 mb 和 smp_store_release 的实现都各自不同

```c
#define __smp_store_release(p, v)					\
do {									\
	typeof(p) __p = (p);						\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u =	\
		{ .__val = (__force __unqual_scalar_typeof(*p)) (v) };	\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_write(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("stlrb %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u8 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 2:								\
		asm volatile ("stlrh %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u16 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 4:								\
		asm volatile ("stlr %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u32 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 8:								\
		asm volatile ("stlr %x1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u64 *)__u.__c)		\
				: "memory");				\
		break;							\
	}								\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u;	\
	typeof(p) __p = (p);						\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_read(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("ldarb %w0, %1"				\
			: "=r" (*(__u8 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 2:								\
		asm volatile ("ldarh %w0, %1"				\
			: "=r" (*(__u16 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 4:								\
		asm volatile ("ldar %w0, %1"				\
			: "=r" (*(__u32 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 8:								\
		asm volatile ("ldar %0, %1"				\
			: "=r" (*(__u64 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	}								\
	(typeof(*p))__u.__val;						\
})
```

https://stackoverflow.com/questions/65466840/arm-stlr-memory-ordering-semantics



### [ ] 一个对比

在 arm 中，dsb 和 dmb 是分别给 rmb 和 dma_rmb 使用的

```c
#define __mb()		dsb(sy)
#define __rmb()		dsb(ld)
#define __wmb()		dsb(st)

#define __dma_mb()	dmb(osh)
#define __dma_rmb()	dmb(oshld)
#define __dma_wmb()	dmb(oshst)
```

在 x86 中
```c
#define __dma_rmb()	barrier()
#define __dma_wmb()	barrier()

#define __smp_mb()	asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")

#define __smp_rmb()	dma_rmb()
#define __smp_wmb()	barrier()
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)
```

### 如何理解 __smp_store_mb

例如 arm64 中是:
```c
#define __smp_store_mb(var, value)  do { WRITE_ONCE(var, value); __smp_mb(); } while (0)
```

x86 中是
```c
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)
```

Documentation/memory-barriers.txt
```txt
There are some more advanced barrier functions:

 (*) smp_store_mb(var, value)

     This assigns the value to the variable and then inserts a full memory
     barrier after it.  It isn't guaranteed to insert anything more than a
     compiler barrier in a UP compilation.
```

## 为什么 x86 需要三个 fence ?

既然只有 store load 的问题

```txt
asm volatile("mfence" : : : "memory");
asm volatile("lfence" : : : "memory");
```

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
