# memory barrier


## 问题
1. when UP ， 需要 mb ?
    1. 需要的，但是效果不同
    
2. 到底在什么地方使用这个 ?
    1. 一共 mb rmb 和 wmb

3. 硬件的上的支持是什么 ?
4. 这种东西的bug 简直就是要人命啊 !

## doc
1. Documentation/memory-barriers.txt 3000 行的注释
2. https://people.cs.pitt.edu/~xianeizhang/notes/cpp11_mem.html
3. wowotech
4. perfbook : chapter 14.2

```c
#ifdef CONFIG_SMP

#ifndef smp_mb
#define smp_mb()	__smp_mb()
#endif

#ifndef smp_rmb
#define smp_rmb()	__smp_rmb()
#endif

#ifndef smp_wmb
#define smp_wmb()	__smp_wmb()
#endif

#ifndef smp_read_barrier_depends
#define smp_read_barrier_depends()	__smp_read_barrier_depends()
#endif

#else	/* !CONFIG_SMP */

#ifndef smp_mb
#define smp_mb()	barrier()
#endif

#ifndef smp_rmb
#define smp_rmb()	barrier()
#endif

#ifndef smp_wmb
#define smp_wmb()	barrier()
#endif

#ifndef smp_read_barrier_depends
#define smp_read_barrier_depends()	do { } while (0)
#endif

#endif	/* CONFIG_SMP */


#ifdef CONFIG_X86_32
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%esp)" ::: "memory", "cc")
#else
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%rsp)" ::: "memory", "cc")
#endif

/* The following are for compatibility with GCC, from compiler-gcc.h,
 * and may be redefined here because they should not be shared with other
 * compilers, like ICC.
 */
#define barrier() __asm__ __volatile__("" : : : "memory")
```

