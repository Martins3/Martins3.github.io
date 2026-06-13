# memblock

buddy 需要用链表之类的数据结构管理内存，初始化这些数据结构是需要另一个内存管理器，那就是 memblock 。
而 memblock 因为特别简单，其管理结构可以静态分配，所以就不存在鸡生蛋的问题了。

## 记录
- 总体来说 : 将 free 和 allocated 的保存在两个数组中间，然后使用 for_each_free_mem_range 来遍历数组，找到合适的位置来进行 free 和 alloc

- 使用 struct memblock_region 描述一个 region
- struct memblock_type - collection of memory regions of certain type
- struct memblock 持有所有内容

- 使用 memblock=debug 来调试
- memblock_double_array : 数组的倍增

## 和 hugepage 的关系
如果配置了 gigantic 的大页，这些页无法从 buddy 中分配，需要提前从 memblock 中分配

memblock_free_all 将 page 给 buddy allocator，使用 QEMU 的调试可以发现，
首先调用的 hugepages_setup，然后才是调用的 memblock_free_all 。

```txt
#0  memblock_free_all () at mm/memblock.c:2128
#1  0xffffffff833532ce in mem_init () at arch/x86/mm/init_64.c:1338
#2  0xffffffff83338ff0 in mm_init () at init/main.c:843
#3  start_kernel () at init/main.c:997
#4  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#5  0x0000000000000000 in ?? ()
```

## memblock 是如何被初始化的

在 x86 中，在函数 `e820__memblock_setup` 中将 e820
```txt
#0  e820__memblock_setup () at arch/x86/kernel/e820.c:1328
#1  0xffffffff833426bf in setup_arch (cmdline_p=cmdline_p@entry=0xffffffff82a03f10) at arch/x86/kernel/setup.c:1133
#2  0xffffffff83338c7d in start_kernel () at init/main.c:959
#3  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
```

## 问题
- [ ] acpi 和 e820 的关系 : 为什么不是 ACPI 来提供系统资源
- [ ] aarch64 是如何初始化 memblock 的
- [ ] 什么是 mirror memory

## 参考资料

- [A quick history of early-boot memory allocators](https://lwn.net/Articles/761215/)

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
