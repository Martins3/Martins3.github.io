## PER_CPU

```c
static void __lru_cache_add(struct page *page)
{
	struct pagevec *pvec = &get_cpu_var(lru_add_pvec);

	page_cache_get(page);
	if (!pagevec_space(pvec))
		__pagevec_lru_add(pvec);
	pagevec_add(pvec, page);
	put_cpu_var(lru_add_pvec);
}
```

**Since the function accesses a CPU-specific data structure, it must prevent the kernel from interrupting
execution** and resuming later on another CPU. This form of protection is enabled implicitly by invoking
`get_cpu_var`, which **not only disables preemption, but also returns the per-CPU variable**.

```c
/*
 * Must be an lvalue. Since @var must be a simple identifier,
 * we force a syntax error here if it isn't.
 */
#define get_cpu_var(var)						\
(*({									\
	preempt_disable();						\
	this_cpu_ptr(&var);						\
}))

/*
 * The weird & is necessary because sparse considers (void)(var) to be
 * a direct dereference of percpu variable (var).
 */
#define put_cpu_var(var)						\
do {									\
	(void)&(var);							\
	preempt_enable();						\
} while (0)
```
> 龟龟，这说的是什么东西啊。


http://www.makelinux.co.il/ldd3/chp-8-sect-5.shtml
https://lwn.net/Articles/22911/

> 接口实现还是很简单的



```c
struct per_cpu_pageset {
	struct per_cpu_pages pcp;
#ifdef CONFIG_NUMA
	s8 expire;
	u16 vm_numa_stat_diff[NR_VM_NUMA_STAT_ITEMS];
#endif
#ifdef CONFIG_SMP
	s8 stat_threshold;
	s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];
#endif
};

struct per_cpu_nodestat {
	s8 stat_threshold;
	s8 vm_node_stat_diff[NR_VM_NODE_STAT_ITEMS];
};
// 一般都是遇到的变量为文档
```

```c
__this_cpu_read(pcp->stat_threshold);
__this_cpu_write(*p, x);

// @todo 硬件是如何实现识别当前cpu 的

// @todo 可不可以写一个用户态 __percpu　的程序

// @todo 找到clang 的 __percpu 的文档是什么
```

The allocated structures are stored in a per-CPU variable, meaning that the calling function must perform the insertion before it can schedule or be moved to a different processor.
> percup 有趣的限制，同时，我们是如何保证的这一个要求的

# mm/percpu.c : 从内存管理的角度来理解下 percpu 的实现
- [ ] 想一想，cpu 是可以 hotplug 的，那么 percpu 的设计就和 tls 的难度差不多了


## Principal

1. 当然，还有一点要注意，那就是在访问 Per-CPU 变量的时候，不能调度，当然更准确的说法是该 task 不能调度到其他 CPU 上去。
目前的内核的做法是在访问 Per-CPU 变量的时候 disable preemptive，虽然没有能够完全避免使用锁的机制（disable preemptive 也是一种锁的机制），但毫无疑问，这是一种代价比较小的锁。


```c
/*
 * Must be an lvalue. Since @var must be a simple identifier,
 * we force a syntax error here if it isn't.
 */
#define get_cpu_var(var)						\
(*({									\
	preempt_disable();						\
	this_cpu_ptr(&var);						\
}))

/*
 * The weird & is necessary because sparse considers (void)(var) to be
 * a direct dereference of percpu variable (var).
 */
#define put_cpu_var(var)						\
do {									\
	(void)&(var);							\
	preempt_enable();						\
} while (0)


#define get_cpu_ptr(var)						\
({									\
	preempt_disable();						\
	this_cpu_ptr(var);						\
})
```
> 在调用对于 percpu 访问的时候，不能将当前的 task 移动其他的 thread 上，但是，否则，写回会发生错位。

2. get_cpu_ptr 和 get_cpu_var 使用的相同的 this_cpu_ptr
    1. 无论是静态还是动态，都是相同的 base + per_cpu_offset + var_offset ?
    2. 所以，动态的唯一痛苦之处只是在于动态分配而已(可能利用一下 bitmap 之类的)

## Question
1. 如何处理 cpuplug 的问题 ?
2. 如果是 NUMA 结构，那么 percpu 的内容都是在哪里的 ?
3. 本以为是闹着玩，结果似乎其可以面向大量的类似
4. chunk 和 block 的内容是什么 ?

5. 静态分配的方法 ? (应该很容易了)
    1. 分配的空间和动态分配的位置相同吗 ?

## TODO
1. percpu 的数据如何汇总
    1. 汇总的时候，谁来主导 ?
2. 访问的每一个 cpu 如何知道访问的自己的
3. 静态分配的各种配置的实现 : 对齐，section SMP/UP

3. https://0xax.gitbooks.io/linux-insides/content/Concepts/linux-cpu-1.html
4. https://lwn.net/Articles/1509/

## doc & ref

1. http://www.wowotech.net/kernel_synchronization/per-cpu.html : 分析了静态的全部内容

1、静态声明和定义 Per-CPU 变量的 API 如下表所示：

声明和定义 Per-CPU 变量的 API	描述
| Interface                                                                            | Type                                                                                            |
|--------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| DECLARE_PER_CPU(type, name) DEFINE_PER_CPU(type, name)                               | 普通的、没有特殊要求的 per cpu 变量定义接口函数。没有对齐的要求
| DECLARE_PER_CPU_FIRST(type, name) DEFINE_PER_CPU_FIRST(type, name)                   | 通过该 API 定义的 per cpu 变量位于整个 per cpu 相关 section 的最前面。
| DECLARE_PER_CPU_SHARED_ALIGNED(type, name) DEFINE_PER_CPU_SHARED_ALIGNED(type, name) | 通过该 API 定义的 per cpu 变量在 SMP 的情况下会对齐到 L1 cache line ，对于 UP，不需要对齐到 cachine line
| DECLARE_PER_CPU_ALIGNED(type, name) DEFINE_PER_CPU_ALIGNED(type, name)               | 无论 SMP 或者 UP，都是需要对齐到 L1 cache line
| DECLARE_PER_CPU_PAGE_ALIGNED(type, name) DEFINE_PER_CPU_PAGE_ALIGNED(type, name)     | 为定义 page aligned per cpu 变量而设定的 API 接口
| DECLARE_PER_CPU_READ_MOSTLY(type, name) DEFINE_PER_CPU_READ_MOSTLY(type, name)       | 通过该 API 定义的 per cpu 变量是 read mostly 的

看到这样“丰富多彩”的 Per-CPU 变量的 API，你是不是已经醉了。这些定义使用在不同的场合，主要的 factor 包括：

- 该变量在 section 中的位置
- 该变量的对齐方式
- 该变量对 SMP 和 UP 的处理不同
- 访问 per cpu 的形态 (@todo 什么叫做 per cpu 的心态)

例如：如果你准备定义的 per cpu 变量是要求按照 page 对齐的，那么在定义该 per cpu 变量的时候需要使用 DECLARE_PER_CPU_PAGE_ALIGNED。如果只要求在 SMP 的情况下对齐到 cache line，那么使用 DECLARE_PER_CPU_SHARED_ALIGNED 来定义该 per cpu 变量。

3. 动态分配 Per-CPU 变量的 API 如下表所示：

| Interface                              | desc                                                                            |
|----------------------------------------|---------------------------------------------------------------------------------|
| alloc_percpu(type)                     | 分配类型是 type 的 per cpu 变量，返回 per cpu 变量的地址（注意：不是各个 CPU 上的副本）
| `void free_percpu(void __percpu *ptr)` | 释放 ptr 指向的 per cpu 变量空间

4. 访问动态分配 Per-CPU 变量的 API 如下表所示：

| Interface             | desc                                                                                              |
|-----------------------|---------------------------------------------------------------------------------------------------|
| get_cpu_ptr           | 这个接口是和访问静态 Per-CPU 变量的 get_cpu_var 接口是类似的，当然，这个接口是 for 动态分配 Per-CPU 变量
| put_cpu_ptr           | 和静态相同，静态访问的另一个方法是 : get_cpu_var
| per_cpu_ptr(ptr, cpu) | 根据 per cpu 变量的地址和 cpu number，返回指定 CPU number 上该 per cpu 变量的地址

## API : `__alloc_percpu_gfp` `__alloc_percpu` free_percpu
1. 静态初始化的内容在哪里 ?

```c
/**
 * __alloc_percpu_gfp - allocate dynamic percpu area
 * @size: size of area to allocate in bytes
 * @align: alignment of area (max PAGE_SIZE)
 * @gfp: allocation flags
 *
 * Allocate zero-filled percpu area of @size bytes aligned at @align.  If
 * @gfp doesn't contain %GFP_KERNEL, the allocation doesn't block and can
 * be called from any context but is a lot more likely to fail. If @gfp
 * has __GFP_NOWARN then no warning will be triggered on invalid or failed
 * allocation requests.
 *
 * RETURNS:
 * Percpu pointer to the allocated area on success, NULL on failure.
 */
void __percpu *__alloc_percpu_gfp(size_t size, size_t align, gfp_t gfp)
{
	return pcpu_alloc(size, align, false, gfp);
}
EXPORT_SYMBOL_GPL(__alloc_percpu_gfp);

/**
 * __alloc_percpu - allocate dynamic percpu area
 * @size: size of area to allocate in bytes
 * @align: alignment of area (max PAGE_SIZE)
 *
 * Equivalent to __alloc_percpu_gfp(size, align, %GFP_KERNEL).
 */
void __percpu *__alloc_percpu(size_t size, size_t align)
{
	return pcpu_alloc(size, align, false, GFP_KERNEL);
}
EXPORT_SYMBOL_GPL(__alloc_percpu);
```
1. @gfp GFP_KERNEL 的作用 : allocation 可以 block，只能在特定的 context 调用


## init


| variable           | desc                                                                                                        |
|--------------------|-------------------------------------------------------------------------------------------------------------|
| pcpu_base_addr     | 利用上 `__per_cpu_offset` 就可以得到具体的位置, @todo 所以，percpu 的变量存储一个变量即可，相对于本地的偏移 |
| `__per_cpu_offset` |                                                                                                             |

```c
setup_per_cpu_areas : x86 arch 初始化
    pcpu_embed_first_chunk
    pcpu_page_first_chunk
        pcpu_setup_first_chunk : 初始化 pcpu_base_addr
```

setup_per_cpu_areas 的第一行日志结果为:
```txt
🧀  dmesg | grep NR_CPUS
[    0.029363] setup_percpu: NR_CPUS:64 nr_cpumask_bits:64 nr_cpu_ids:64 nr_node_ids:2
```
给虚拟机配置的是 32 cpu + 最多 128 个 core 。

NR_CPUS 是编译内部配置的，也就是 NR_CPUS 这个 macro
```c
unsigned long __per_cpu_offset[NR_CPUS] __ro_after_init = {
	[0 ... NR_CPUS-1] = BOOT_PERCPU_OFFSET,
};
EXPORT_SYMBOL(__per_cpu_offset);
```

n100 4 核物理机上:
```txt
[    0.048555] setup_percpu: NR_CPUS:8192 nr_cpumask_bits:4 nr_cpu_ids:4 nr_node_ids:1
[    0.102212] rcu:     RCU restricting CPUs from NR_CPUS=8192 to nr_cpu_ids=4.
```

最后
```c
static inline unsigned long cpu_kernelmode_gs_base(int cpu)
{
	return (unsigned long)per_cpu(fixed_percpu_data.gs_base, cpu);
}
```

原来 percpu 和 stack 是联系到一起的:

https://stackoverflow.com/questions/6611346/how-are-the-fs-gs-registers-used-in-linux-amd64

## pcpu_alloc
> 现在，只是需要理解其中的一个机制

### percpu alloc 的时候和 cpu hotplug 需要有一个互斥吧

类似这种，通过 DEFINE_PER_CPU 是静态分配的吗?
```c
static DEFINE_PER_CPU(int, stop);
```

## 看看 gs 寄存器的使用

arch/x86/kernel/process_64.c:__show_regs

## 对应 percpu.c 中的问题


```c
#define arch_raw_cpu_ptr(_ptr)						\
({									\
	unsigned long tcp_ptr__ = raw_cpu_read_long(this_cpu_off);	\
									\
	tcp_ptr__ += (__force unsigned long)(_ptr);			\
	(typeof(*(_ptr)) __kernel __force *)tcp_ptr__;			\
})
```

最后对应着两个指令:
```txt
 * 0xffffffffc08b76f5 <disass_percpu+5>:   mov    %gs:0x3f75e2e3(%rip),%rsi        # 0x159e0
 * 0xffffffffc08b76fd <disass_percpu+13>:  add    0xdae4(%rip),%rsi        # 0xffffffffc08c51e8 <action_fifos>
```
其实只是为了获取到一个指针而已。


为什么之前是这种写法:
```c
/*
 * Compared to the generic __my_cpu_offset version, the following
 * saves one instruction and avoids clobbering a temp register.
 */
#define arch_raw_cpu_ptr(ptr)				\
({							\
	unsigned long tcp_ptr__;			\
	asm volatile("add " __percpu_arg(1) ", %0"	\
		     : "=r" (tcp_ptr__)			\
		     : "m" (this_cpu_off), "0" (ptr));	\
	(typeof(*(ptr)) __kernel __force *)tcp_ptr__;	\
})
#else
#define __percpu_prefix		""
#endif

```

## crash 真的太强大了
```txt
crash> p cpufreq_update_util_data
PER-CPU DATA TYPE:
  struct update_util_data *cpufreq_update_util_data;
PER-CPU ADDRESSES:
  [0]: ffff889ffe81cf48
  [1]: ffff889ffe89cf48
  [2]: ffff889ffe91cf48
  [3]: ffff889ffe99cf48
  [4]: ffff889ffea1cf48
  [5]: ffff889ffea9cf48
  [6]: ffff889ffeb1cf48
  [7]: ffff889ffeb9cf48
  [8]: ffff889ffec1cf48
  [9]: ffff889ffec9cf48
  [10]: ffff889ffed1cf48
  [11]: ffff889ffed9cf48
  [12]: ffff889ffee1cf48
  [13]: ffff889ffee9cf48
  [14]: ffff889ffef1cf48
  [15]: ffff889ffef9cf48
  [16]: ffff889fff01cf48
  [17]: ffff889fff09cf48
  [18]: ffff889fff11cf48
  [19]: ffff889fff19cf48
  [20]: ffff889fff21cf48
  [21]: ffff889fff29cf48
  [22]: ffff889fff31cf48
  [23]: ffff889fff39cf48
  [24]: ffff889fff41cf48
  [25]: ffff889fff49cf48
  [26]: ffff889fff51cf48
  [27]: ffff889fff59cf48
  [28]: ffff889fff61cf48
  [29]: ffff889fff69cf48
  [30]: ffff889fff71cf48
  [31]: ffff889fff79cf48
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
