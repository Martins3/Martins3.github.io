# mm/percpu.c

## Principal

1. 当然，还有一点要注意，那就是在访问Per-CPU变量的时候，不能调度，当然更准确的说法是该task不能调度到其他CPU上去。
目前的内核的做法是在访问Per-CPU变量的时候disable preemptive，虽然没有能够完全避免使用锁的机制（disable preemptive也是一种锁的机制），但毫无疑问，这是一种代价比较小的锁。


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
2. 如果是NUMA 结构，那么 percpu 的内容都是在哪里的 ?
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

1、静态声明和定义Per-CPU变量的API如下表所示：

声明和定义Per-CPU变量的API	描述
| Interface                                                                            | Type                                                                                            |
|--------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| DECLARE_PER_CPU(type, name) DEFINE_PER_CPU(type, name)                               | 普通的、没有特殊要求的per cpu变量定义接口函数。没有对齐的要求
| DECLARE_PER_CPU_FIRST(type, name) DEFINE_PER_CPU_FIRST(type, name)                   | 通过该API定义的per cpu变量位于整个per cpu相关section的最前面。
| DECLARE_PER_CPU_SHARED_ALIGNED(type, name) DEFINE_PER_CPU_SHARED_ALIGNED(type, name) | 通过该API定义的per cpu变量在SMP的情况下会对齐到L1 cache line ，对于UP，不需要对齐到cachine line
| DECLARE_PER_CPU_ALIGNED(type, name) DEFINE_PER_CPU_ALIGNED(type, name)               | 无论SMP或者UP，都是需要对齐到L1 cache line
| DECLARE_PER_CPU_PAGE_ALIGNED(type, name) DEFINE_PER_CPU_PAGE_ALIGNED(type, name)     | 为定义page aligned per cpu变量而设定的API接口
| DECLARE_PER_CPU_READ_MOSTLY(type, name) DEFINE_PER_CPU_READ_MOSTLY(type, name)       | 通过该API定义的per cpu变量是read mostly的

看到这样“丰富多彩”的Per-CPU变量的API，你是不是已经醉了。这些定义使用在不同的场合，主要的factor包括：

- 该变量在section中的位置
- 该变量的对齐方式
- 该变量对SMP和UP的处理不同
- 访问per cpu的形态 (@todo 什么叫做 per cpu 的心态)

例如：如果你准备定义的per cpu变量是要求按照page对齐的，那么在定义该per cpu变量的时候需要使用DECLARE_PER_CPU_PAGE_ALIGNED。如果只要求在SMP的情况下对齐到cache line，那么使用DECLARE_PER_CPU_SHARED_ALIGNED来定义该per cpu变量。

3. 动态分配Per-CPU变量的API如下表所示：

| Interface                              | desc                                                                            |
|----------------------------------------|---------------------------------------------------------------------------------|
| alloc_percpu(type)                     | 分配类型是type的per cpu变量，返回per cpu变量的地址（注意：不是各个CPU上的副本）
| `void free_percpu(void __percpu *ptr)` | 释放ptr指向的per cpu变量空间

4. 访问动态分配Per-CPU变量的API如下表所示：

| Interface             | desc                                                                                              |
|-----------------------|---------------------------------------------------------------------------------------------------|
| get_cpu_ptr           | 这个接口是和访问静态Per-CPU变量的get_cpu_var接口是类似的，当然，这个接口是for 动态分配Per-CPU变量
| put_cpu_ptr           | 和静态相同，静态访问的另一个方法是 : get_cpu_var
| per_cpu_ptr(ptr, cpu) | 根据per cpu变量的地址和cpu number，返回指定CPU number上该per cpu变量的地址

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

## pcpu_alloc
> 现在，只是需要理解其中的一个机制


