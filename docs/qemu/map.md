# QEMU 中的 map 和 set
<!-- vim-markdown-toc GitLab -->

* [记录范围内上锁过的 page](#记录范围内上锁过的-page)
* [根据 Ram addr 找该 guest page 上关联的所有的 tb](#根据-ram-addr-找该-guest-page-上关联的所有的-tb)
* [根据 retaddr 找到其关联的 TranslationBlock](#根据-retaddr-找到其关联的-translationblock)
* [根据 physical address 计算出来 MemoryRegion](#根据-physical-address-计算出来-memoryregion)
* [根据 guest virtual address 找到 Translation Block](#根据-guest-virtual-address-找到-translation-block)
* [根据 guest physical address 找到 Translation Block](#根据-guest-physical-address-找到-translation-block)

<!-- vim-markdown-toc -->

> 从一个诡异的角度来分析 QEMU 中的源码。

既不会分析所有的种类的 map，也不会分析所有的使用位置，只是感觉老是遇到，总结一下。

## 记录范围内上锁过的 page

```c
struct page_collection {
  struct GTree *tree;
  struct page_entry *max;
};
```

在 page_collection::tree 的 (key, value) = (page_addr, page_entry)

page_collection_lock => page_entry_lock => page_lock

为了对于一个连续范围的 page 上锁而不会出现死锁，需要上锁的时候保持顺序。
具体实现的代码在 page_collection_lock 中, 利用 page_collection::tree 来记录范围中已经上过锁的 page

```c
/*
 * Lock a range of pages ([@start,@end[) as well as the pages of all
 * intersecting TBs.
 * Locking order: acquire locks in ascending order of page index.
 */
struct page_collection *
page_collection_lock(tb_page_addr_t start, tb_page_addr_t end)
{
    struct page_collection *set = g_malloc(sizeof(*set));
    tb_page_addr_t index;
    PageDesc *pd;

    start >>= TARGET_PAGE_BITS;
    end   >>= TARGET_PAGE_BITS;
    g_assert(start <= end);

    set->tree = g_tree_new_full(tb_page_addr_cmp, NULL, NULL,
                                page_entry_destroy);
    set->max = NULL;
    assert_no_pages_locked();

 retry:
    // 对于 tree 中的 page 从小达到上锁
    g_tree_foreach(set->tree, page_entry_lock, NULL);
    // 现在 tree 中的所有的 page 都是上过锁了

    for (index = start; index <= end; index++) {
        TranslationBlock *tb;
        int n;

        pd = page_find(index);
        if (pd == NULL) {
            continue;
        }
        if (page_trylock_add(set, index << TARGET_PAGE_BITS)) {
            // 进入到这里只有的情况:
            // 1. 因为 TB 跨页导致进入到 retry 中
            // 2. 对于 g_tree 中的全部上锁，假设为页面 A C ( A C 中间有 B)
            // 3. 另外 thread 的执行 B，并且将 B 上锁，那么在 page_trylock_add 将会返回 true
            g_tree_foreach(set->tree, page_entry_unlock, NULL);
            goto retry;
        }
        assert_page_locked(pd);
        // 需要特别处理 TB 跨页的情况
        // 如果一个从 start ~ end 中的页包含的 TB 跨页了，那么 TB 跨越的页也需要上锁
        PAGE_FOR_EACH_TB(pd, tb, n) {
            if (page_trylock_add(set, tb->page_addr[0]) ||
                (tb->page_addr[1] != -1 &&
                 page_trylock_add(set, tb->page_addr[1]))) {
                /* drop all locks, and reacquire in order */
                g_tree_foreach(set->tree, page_entry_unlock, NULL);
                goto retry;
            }
        }
    }
    return set;
}
```

## 根据 Ram addr 找该 guest page 上关联的所有的 tb
使用类似 page table 的方式存储，在 page_table_config_init 初始化 page table 的属性。

在 page_find_alloc 进行查询。

## 根据 retaddr 找到其关联的 TranslationBlock
对于每一段 guest 的 translation block, QEMU 都会创建一个 TranslationBlock,
TranslationBlock 就是生成的代码左侧, 但是当生成的代码(也地址为 retaddr) 中退出的时候，是无法知道其关联的 TranslationBlock 的位置的。

tcg_tb_alloc 的注释说道
```c
/*
 * Allocate TBs right before their corresponding translated code, making
 * sure that TBs and code are on different cache lines.
 */
```
通过 tcg_region_tree 可以实现从 retaddr 到 TranslationBlock 的映射

```c
struct tcg_region_tree {
  QemuMutex lock;
  struct GTree *tree;
  /* padding to avoid false sharing is computed at run-time */
};
```

```c
/*
 * Translation Cache-related fields of a TB.
 * This struct exists just for convenience; we keep track of TB's in a binary
 * search tree, and the only fields needed to compare TB's in the tree are
 * @ptr and @size.
 * Note: the address of search data can be obtained by adding @size to @ptr.
 */
struct tb_tc {
  void *ptr; /* pointer to the translated code */
  size_t size;
};
```

在 tb_gen_code 中初始化
1. `tb->tc.ptr = gen_code_buf;`
2. `tb->tc.size = gen_code_size;`
也就是，tb 所在地址和大小。

## 根据 physical address 计算出来 MemoryRegion
这个玩意设计成为多级页面的目的和页表查询的作用差不多, 都是使用 addr 中一部分逐级向下索引的。

查询的基本的调用流程如下：

- flatview_translate
  - flatview_do_translate
      - address_space_translate_internal
          - address_space_lookup_region
            - AddressSpaceDispatch::mru_section : 首先访问 mru(most recent used) 缓存，如果不命中，那么调用 phys_page_find 的 tree 中间寻找
            - phys_page_find

如何构建 PhysPageMap 这颗树:
- generate_memory_topology
  - render_memory_region / flatview_simplify  : 将 memory region 转化为了 FlatRange 的
  - flatview_add_to_dispatch : 将 FlatRange 首先使用 section_from_flat_range 转化为 MemoryRegionSection 然后添加
    - register_subpage : 如果 MemoryRegionSection 无法完整地覆盖一整个页
    - register_multipage
      - phys_section_add : 将 MemoryRegionSection 添加到 PhysPageMap::sections
      - phys_page_set : 设置对应的 PhysPageMap::nodes
        - phys_map_node_reserve : 预留空间
        - phys_page_set_level :

```c
struct AddressSpaceDispatch {
    MemoryRegionSection *mru_section;
    /* This is a multi-level map on the physical address space.
     * The bottom level has pointers to MemoryRegionSections.
     */
    PhysPageEntry phys_map;
    PhysPageMap map;
};
```
- phys_map : 相当于 x86 cr3, 每次访问首先访问第一个 PhysPageEntry
- map : 相当于管理所有的 page tabel 的页面
- mru_section : 缓存

```c
struct PhysPageEntry {
    /* How many bits skip to next level (in units of L2_SIZE). 0 for a leaf. */
    uint32_t skip : 6;
     /* index into phys_sections (!skip) or phys_map_nodes (skip) */
    uint32_t ptr : 26;
};
```
- skip : 表示还有多少级就可以到 leaf 节点
- prt : 下标
  - 如果 leaf 节点，那么就是索引 `PhysPageMap::sections` 的下标
  - 如果 non-leaf 节点，那么就是索引 `PhysPageMap::nodes` 的下标

```c
typedef PhysPageEntry Node[P_L2_SIZE];

typedef struct PhysPageMap {
    struct rcu_head rcu;

    unsigned sections_nb;
    unsigned sections_nb_alloc;
    unsigned nodes_nb;
    unsigned nodes_nb_alloc;
    Node *nodes;
    MemoryRegionSection *sections;
} PhysPageMap;
```
- Node : 定义这个结构体，相当于定义了一个 page table
- nodes : 存储所有的 Node，如果分配完了，使用 phys_map_node_reserve 来补充
- sections : 最终想要获取的

下面是一个 FlatRanges 和其对应构建出来的 tree,
在 QEMU 的 human monitor interface 中 `info mtree -f -d` 可以获取。
```txt
  Dispatch
    Physical sections
      #0 @0000000000000000..ffffffffffffffff (noname) [unassigned]
      #1 @0000000000000000..000000017fffffff pc.ram [not dirty]
      #2 @00000000000a0000..00000000000bffff vga-lowmem [ROM]
      #3 @00000000000c0000..00000001800bffff pc.ram [watch]
      #4 @00000000000cb000..00000001800cafff pc.ram
      #5 @00000000000ce000..00000001800cdfff pc.ram
      #6 @00000000000e4000..00000001800e3fff pc.ram
      #7 @00000000000f0000..00000001800effff pc.ram
      #8 @0000000000100000..00000001800fffff pc.ram
      #9 @00000000fd000000..00000000fdffffff vga.vram
      #10 @00000000fe000000..00000000fe000fff virtio-pci-common-virtio-9p
      #11 @00000000fe001000..00000000fe001fff virtio-pci-isr-virtio-9p
      #12 @00000000fe002000..00000000fe002fff virtio-pci-device-virtio-9p
      #13 @00000000fe003000..00000000fe003fff virtio-pci-notify-virtio-9p
      #14 @00000000febc0000..00000000febdffff e1000-mmio [MRU]
      #15 @00000000febf0000..00000000febf1fff nvme
      #16 @00000000febf2000..00000000febf2fff (noname)
      #17 @00000000febf2000..00000000febf240f msix-table
      #18 @00000000febf3000..00000000febf3fff (noname)
      #19 @00000000febf3000..00000000febf300f msix-pba
      #20 @00000000febf4000..00000000febf5fff nvme
      #21 @00000000febf6000..00000000febf6fff (noname)
      #22 @00000000febf6000..00000000febf640f msix-table
      #23 @00000000febf7000..00000000febf7fff (noname)
      #24 @00000000febf7000..00000000febf700f msix-pba
      #25 @00000000febf8000..00000000febf8fff (noname)
      #26 @00000000febf8000..00000000febf817f edid
      #27 @00000000febf8180..00000000febf917f vga.mmio
      #28 @00000000febf8400..00000000febf841f vga ioports remapped
      #29 @00000000febf8420..00000000febf941f vga.mmio
      #30 @00000000febf8500..00000000febf8515 bochs dispi interface
      #31 @00000000febf8516..00000000febf9515 vga.mmio
      #32 @00000000febf8600..00000000febf8607 qemu extended regs
      #33 @00000000febf8608..00000000febf9607 vga.mmio
      #34 @00000000febf9000..00000000febf9fff (noname)
      #35 @00000000febf9000..00000000febf901f msix-table
      #36 @00000000febf9800..00000000febf9807 msix-pba
      #37 @00000000fec00000..00000000fec00fff kvm-ioapic
      #38 @00000000fed00000..00000000fed00fff (noname)
      #39 @00000000fed00000..00000000fed003ff hpet
      #40 @00000000fee00000..00000000feefffff kvm-apic-msi
      #41 @00000000fffc0000..00000000ffffffff pc.bios
      #42 @0000000100000000..000000027fffffff pc.ram
    Nodes (9 bits per level, 6 levels) ptr=[3] skip=4
      [0]
          0       skip=3  ptr=[3]
          1..511  skip=1  ptr=NIL
      [1]
          0       skip=2  ptr=[3]
          1..511  skip=1  ptr=NIL
      [2]
          0       skip=1  ptr=[3]
          1..511  skip=1  ptr=NIL
      [3]
          0       skip=1  ptr=[4]
          1..2    skip=0  ptr=#8
          3       skip=1  ptr=[6]
          4..6    skip=0  ptr=#42
          7..511  skip=1  ptr=NIL
      [4]
          0       skip=1  ptr=[5]
          1..511  skip=0  ptr=#8
      [5]
          0..159  skip=0  ptr=#1
        160..191  skip=0  ptr=#2
        192..202  skip=0  ptr=#3
        203..205  skip=0  ptr=#4
        206..227  skip=0  ptr=#5
        228..239  skip=0  ptr=#6
        240..255  skip=0  ptr=#7
        256..511  skip=0  ptr=#8
      [6]
          0..487  skip=1  ptr=NIL
        488..495  skip=0  ptr=#9
        496       skip=1  ptr=[7]
        497..500  skip=1  ptr=NIL
        501       skip=1  ptr=[8]
        502       skip=1  ptr=[9]
        503       skip=1  ptr=[10]
        504..510  skip=1  ptr=NIL
        511       skip=1  ptr=[11]
      [7]
          0       skip=0  ptr=#10
          1       skip=0  ptr=#11
          2       skip=0  ptr=#12
          3       skip=0  ptr=#13
          4..511  skip=0  ptr=#0
      [8]
          0..447  skip=0  ptr=#0
        448..479  skip=0  ptr=#14
        480..495  skip=0  ptr=#0
        496..497  skip=0  ptr=#15
        498       skip=0  ptr=#16
        499       skip=0  ptr=#18
        500..501  skip=0  ptr=#20
        502       skip=0  ptr=#21
        503       skip=0  ptr=#23
        504       skip=0  ptr=#25
        505       skip=0  ptr=#34
        506..511  skip=0  ptr=#0
      [9]
          0       skip=0  ptr=#37
          1..255  skip=0  ptr=#0
        256       skip=0  ptr=#38
        257..511  skip=0  ptr=#0
      [10]
          0..255  skip=0  ptr=#40
        256..511  skip=0  ptr=#0
      [11]
          0..447  skip=0  ptr=#0
        448..511  skip=0  ptr=#41
```

QEMU 为了优化还进行了很多骚操作:
- address_space_dispatch_compact : 很多时候，一个 MemoryRegionSection 可以覆盖很大的范围，所以 tree 是可以压缩的。因为 compact 操作，tree 的 root 不在一定是 PhysPageMap::nodes 中第一个，所以需要记录在 `AddressSpaceDispatch::phys_map` 中。

## 根据 guest virtual address 找到 Translation Block
根据 GVA 查找 TB 主要使用的是一个很简单的 hash 操作:
- CPUState::tb_jmp_cache 是一个 `TranslationBlock *` 的数组
- 通过 tb_jmp_cache_hash_func 将 GVA hash 之后来索引 tb_jmp_cache

之所以，使用 tb_jmp_cache 来索引，是因为获取物理地址很麻烦，需要经过 TLB 甚至是 page walk 装换，所以使用 tb_jmp_cache 作为高速缓存。
tb_jmp_cache 因为是虚拟地址相关的，如果虚拟地址发生改变，那么需要通过调用 tb_flush_jmp_cache 将其中数据清理。

## 根据 guest physical address 找到 Translation Block
和 tb_jmp_cache 不同，TBContext::htable 是通过物理地址来索引的，因为物理地址是唯一的，所以，所有的 CPU 可以共享.
因为共享，QEMU 提出了一个高并发的 hashtable 那就是 QEMU hash table，简称 qht

在 tb_lookup 中，这次 hash 的关系清晰可见。
```c
/* Might cause an exception, so have a longjmp destination ready */
static inline TranslationBlock *tb_lookup(CPUState *cpu, target_ulong pc,
                                          target_ulong cs_base,
                                          uint32_t flags, uint32_t cflags)
{
    TranslationBlock *tb;
    uint32_t hash;

    /* we should never be trying to look up an INVALID tb */
    tcg_debug_assert(!(cflags & CF_INVALID));

    hash = tb_jmp_cache_hash_func(pc);
    tb = qatomic_rcu_read(&cpu->tb_jmp_cache[hash]); // 使用虚拟地址查询

    if (likely(tb &&
               tb->pc == pc &&
               tb->cs_base == cs_base &&
               tb->flags == flags &&
               tb->trace_vcpu_dstate == *cpu->trace_dstate &&
               tb_cflags(tb) == cflags)) {
        return tb;
    }
    tb = tb_htable_lookup(cpu, pc, cs_base, flags, cflags); // 不命中，使用物理地址查询
    if (tb == NULL) {
        return NULL;
    }
    qatomic_set(&cpu->tb_jmp_cache[hash], tb);
    return tb;
}
```

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
