# sparse vmemmap


## 原理
使用 page struct 数组来记录物理页面 (page frame) 的元数据，内核管理物理页面的过程中需要:
- 以通过 page frame 的物理地址找到 page struct : `pfn_to_page`
- 找到 page struct 对应的 page frame : `page_to_pfn`

```c
/* memmap is virtually contiguous.  */
#define __pfn_to_page(pfn)	(vmemmap + (pfn))
#define __page_to_pfn(page)	(unsigned long)((page) - vmemmap)
```

但是物理内存部署连续的，而是有大片的空洞的[^1]

## memory model
这个 memory model 和体系结构中的 memory model 没有任何关系，指的是内核如何构建 page struct 和 page frame 的关系，内核提供三种 memory model 来构建 page frame  和 page struct 的关系:

1. FLAT：ucore 中间使用的模型。Back in the beginning of Linux, memory was flat: it was a simple linear sequence with physical addresses starting at zero and ending at several megabytes.实现从一个 page frame 的 PFN 转化为其对应的 page struct 很容易且高效。
2. DISCONTIG：为了应对物理内存中间出现空洞，以及 NUMA 而产生的。其提出一个重要的概念，memory node。每一个 memory node 中间含有独立 buddy system，各种统计信息等。在每一个 memory node 中间，其中是管理连续的物理地址空间，并且含有一个 page struct 数组和一个物理页面一一对应。虽然解决问题，但是也带来了 PFN 难以查询 page struct 的问题。
3. SPARSE：在 64 位系统上，At the cost of additional page table entries, page_to_pfn(), and pfn_to_page() became as simple as with the flat model. (其他我就真的没有看懂
  - 在这个模式下，启用 vmemmap，也是目前主流的配置

关于三者更加细节的讨论参考这里: [Memory: the flat, the discontiguous, and the sparse](https://lwn.net/Articles/789304/)

## 内核管理内存的粒度
- memsection
- pageblock : 用于进行 memory 的
- page
- slab

- 内核 27bit 也就是 128M 形成一个 memseciton
```c
# define SECTION_SIZE_BITS  27 /* matt - 128 is convenient right now */
// 获取页面所在的section
static inline unsigned long pfn_to_section_nr(unsigned long pfn)
{
    return pfn >> PFN_SECTION_SHIFT;
}
```

- 一个 pageblock 中一般包含 1024 个页面
```c
#define MAX_ORDER 11
/*
 * Huge pages are a constant size, but don't exceed the maximum allocation
 * granularity.
 */
#define pageblock_order		min_t(unsigned int, HUGETLB_PAGE_ORDER, MAX_ORDER - 1)
#define pageblock_nr_pages	(1UL << pageblock_order)
```

### mem_section
```c
struct mem_section {
	/*
	 * This is, logically, a pointer to an array of struct
	 * pages.  However, it is stored with some other magic.
	 * (see sparse.c::sparse_init_one_section())
	 *
	 * Additionally during early boot we encode node id of
	 * the location of the section here to guide allocation.
	 * (see sparse.c::memory_present())
	 *
	 * Making it a UL at least makes someone do a cast
	 * before using it wrong.
	 */
	unsigned long section_mem_map;

	struct mem_section_usage *usage;
	/*
	 * WARNING: mem_section must be a power-of-2 in size for the
	 * calculation and use of SECTION_ROOT_MASK to make sense.
	 */
};
```
1. section_mem_map : 类似 "Flat Memory" 状态下的 mem_map，指向 struct page 数组
2. usage:
  - 存储了每一个 pageblock 的 flags，pageblock 是用于处理 page compaction 的基础
  - subsection

```c
static __always_inline int get_pfnblock_migratetype(const struct page *page,
					unsigned long pfn)
{
	return __get_pfnblock_flags_mask(page, pfn, MIGRATETYPE_MASK);
}
```

- 一个 page 可以知道自己所在的 pageblock，一个 pageblock 是 migrate 的基本单元。
- 一个 memsection 中存储该 memsection 所有 pageblock 的属性

- subsection : 看上去是管理
  - section_activate
  - section_inactivate

```diff
commit f46edbd1b1516da1fb34c917775168d5df576f78
Author: Dan Williams <dan.j.williams@intel.com>
Date:   Thu Jul 18 15:58:04 2019 -0700

    mm/sparsemem: add helpers track active portions of a section at boot

    Prepare for hot{plug,remove} of sub-ranges of a section by tracking a
    sub-section active bitmask, each bit representing a PMD_SIZE span of the
    architecture's memory hotplug section size.

    The implications of a partially populated section is that pfn_valid()
    needs to go beyond a valid_section() check and either determine that the
    section is an "early section", or read the sub-section active ranges
    from the bitmask.  The expectation is that the bitmask (subsection_map)
    fits in the same cacheline as the valid_section() / early_section()
    data, so the incremental performance overhead to pfn_valid() should be
    negligible.

    The rationale for using early_section() to short-ciruit the
    subsection_map check is that there are legacy code paths that use
    pfn_valid() at section granularity before validating the pfn against
    pgdat data.  So, the early_section() check allows those traditional
    assumptions to persist while also permitting subsection_map to tell the
    truth for purposes of populating the unused portions of early sections
    with PMEM and other ZONE_DEVICE mappings.

```


## 内存初始化过程
- paging_init
  - sparse_init
    - sparse_init_nid
      - `__populate_section_memmap`
      - sparse_init_one_section


当在 memory hotplug 的时候:
- sparse_add_section
  - section_activate
    - populate_section_memmap : vmemmap 进行填充

```txt
#0  sparse_buffer_alloc (size=2097152) at mm/sparse.c:484
#1  0xffffffff81faff88 in vmemmap_alloc_block_buf (size=size@entry=2097152, node=node@entry=1, altmap=altmap@entry=0x0 <fixed_percpu_data>) at mm/sparse-vmemmap.c:85
#2  0xffffffff81faddf6 in vmemmap_populate_hugepages (altmap=<optimized out>, node=<optimized out>, end=<optimized out>, start=<optimized out>) at arch/x86/mm/init_64.c:1565
#3  vmemmap_populate (start=start@entry=18446719884455837696, end=end@entry=18446719884457934848, node=node@entry=1, altmap=altmap@entry=0x0 <fixed_percpu_data>) at arch/x86/mm/init_64.c:1615
#4  0xffffffff81fb062f in __populate_section_memmap (pfn=pfn@entry=32768, nr_pages=nr_pages@entry=32768, nid=nid@entry=1, altmap=altmap@entry=0x0 <fixed_percpu_data>, pgmap=pgmap@entry=0x0 <fixed_percpu_data>) at mm/sparse-vmemmap.c:392
#5  0xffffffff83366fba in sparse_init_nid (nid=1, pnum_begin=pnum_begin@entry=0, pnum_end=pnum_end@entry=40, map_count=32) at mm/sparse.c:527
#6  0xffffffff833673ed in sparse_init () at mm/sparse.c:580
#7  0xffffffff83353299 in paging_init () at arch/x86/mm/init_64.c:816
#8  0xffffffff83342b40 in setup_arch (cmdline_p=cmdline_p@entry=0xffffffff82a03f10) at arch/x86/kernel/setup.c:1253
#9  0xffffffff83338c7d in start_kernel () at init/main.c:959
#10 0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#11 0x0000000000000000 in ?? ()
```

## 如何理解 vmemmap
- 物理内存不是连续的，为空洞分配 page struct 非常划不来。

- 物理内存分配: for each node 的构建的，通过 memblock 的分配器来保证 page frame 和对应的 page struct 在同一个 node 中
- 虚拟内存分配: 从 `pfn_to_page` 可以看到是从 vmemmap 开始的

## vmemmap 的基本执行流程
```txt
#0  vmemmap_populate (start=start@entry=18446719884453740544, end=end@entry=18446719884455837696, node=node@entry=1, altmap=altmap@entry=0x0 <fixed_percpu_data>) at arch/x86/mm/init_64.c:1612
#1  0xffffffff81fb063f in __populate_section_memmap (pfn=pfn@entry=0, nr_pages=nr_pages@entry=32768, nid=nid@entry=1, altmap=altmap@entry=0x0 <fixed_percpu_data>, pgmap=pgmap@entry=0x0 <fixed_percpu_data>) at mm/sparse-vmemmap.c:392
#2  0xffffffff83366fc1 in sparse_init_nid (nid=1, pnum_begin=pnum_begin@entry=0, pnum_end=pnum_end@entry=40, map_count=32) at mm/sparse.c:527
#3  0xffffffff833673f4 in sparse_init () at mm/sparse.c:580
#4  0xffffffff833532a0 in paging_init () at arch/x86/mm/init_64.c:816
#5  0xffffffff83342b47 in setup_arch (cmdline_p=cmdline_p@entry=0xffffffff82a03f10) at arch/x86/kernel/setup.c:1253
#6  0xffffffff83338c7d in start_kernel () at init/main.c:959
#7  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#8  0x0000000000000000 in ?? ()
```

- vmemmap_populate : 处理自动选择 hugepages 的，在 arch/x86/mm/init_64.c
  - vmemmap_populate_basepages : 使用 basepage 来实现映射
  - vmemmap_populate_hugepages

hugepages 的初始化是在此之后的
```txt
#0  hugepages_setup (s=0xffff88823fff51ea "4") at mm/hugetlb.c:4165
#1  0xffffffff833388f0 in obsolete_checksetup (line=0xffff88823fff51e0 "hugepages=4") at init/main.c:221
#2  unknown_bootoption (param=0xffff88823fff51e0 "hugepages=4", val=val@entry=0xffff88823fff51ea "4", unused=unused@entry=0xffffffff827b3bc4 "Booting kernel", arg=arg@entry=0x0 <fixed_percpu_data>) at init/main.c:541
#3  0xffffffff81131dc3 in parse_one (handle_unknown=0xffffffff83338856 <unknown_bootoption>, arg=0x0 <fixed_percpu_data>, max_level=-1, min_level=-1, num_params=748, params=0xffffffff82992e20 <__param_initcall_debug>, doing=0xffffffff827b3bc4 "Booting kernel", val=0xffff88823fff51ea "4", param=0xffff88823fff51e0 "hugepages=4") at kernel/params.c:153
#4  parse_args (doing=doing@entry=0xffffffff827b3bc4 "Booting kernel", args=0xffff88823fff51ec "hugepagesz=2M hugepages=512 systemd.unified_cgroup_hierarchy=1 ", params=0xffffffff82992e20 <__param_initcall_debug>, num=748, min_level=min_level@entry=-1, max_level=max_level@entry=-1, arg=0x0 <fixed_percpu_data>, unknown=0xffffffff83338856 <unknown_bootoption>) at kernel/params.c:188
#5  0xffffffff83338e27 in start_kernel () at init/main.c:974
#6  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#7  0x0000000000000000 in ?? ()
```

### sparsemap_buf

- vmemmap_populate_basepages
  - vmemmap_pte_populate : 分配位置来自于

如果我们观察 `vmemmap_populate_address` 的调用

pagetable 使用的 page 走的这个路径:
- vmemmap_alloc_block_zero
  - vmemmap_alloc_block
    - `__earlyonly_bootmem_alloc`
      - memblock_alloc_try_nid_raw

- 而 pagetable 的 leaf 最终指向内容，也就是 page struct 是通过 vmemmap_alloc_block_buf 分配的:

```c
/* need to make sure size is all the same during early stage */
void * __meminit vmemmap_alloc_block_buf(unsigned long size, int node,
					 struct vmem_altmap *altmap)
{
	void *ptr;

	if (altmap)
		return altmap_alloc_block_buf(size, altmap);

	ptr = sparse_buffer_alloc(size);
	if (!ptr)
		ptr = vmemmap_alloc_block(size, node);
	return ptr;
}
```

下面是对于原文的总结性的翻译：

## 小知识
- vmemmap_alloc_block 中为什么会出现 slab_is_available 的判断，是因为内存的热插拔
在内核启动的时候 vmemmap_alloc_block_buf 最后调用 memblock 上

[^1]: https://stackoverflow.com/questions/23626165/what-is-meant-by-holes-in-the-memory-linux

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
