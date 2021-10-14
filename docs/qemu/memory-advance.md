# QEMU 的 memory model 和 softmmu 高级话题分析

<!-- vim-markdown-toc GitLab -->

- [subpage](#subpage)
- [cross page check](#cross-page-check)
    - [SMC](#smc)
- [SMC](#smc-1)
  - [lock page](#lock-page)
  - [TARGET_HAS_PRECISE_SMC](#target_has_precise_smc)
  - [流程](#流程)
  - [PageDesc](#pagedesc)
      - [PageDesc::first_tb](#pagedescfirst_tb)
      - [PageDesc::code_bitmap / PageDesc::code_write_count](#pagedesccode_bitmap-pagedesccode_write_count)
  - [page desc tree](#page-desc-tree)
  - [tb_invalidate_phys_page_fast](#tb_invalidate_phys_page_fast)
- [非对其访问](#非对其访问)
- [如何重新设计 QEMU 中的 memory](#如何重新设计-qemu-中的-memory)
  - [设计](#设计)
  - [需求分析](#需求分析)
  - [address_space_translate_for_iotlb 和 address_space_translate 的关系](#address_space_translate_for_iotlb-和-address_space_translate-的关系)
  - [笔记补充](#笔记补充)

<!-- vim-markdown-toc -->

主要是基于 tcg 分析，对于做云计算的同学应该没有什么作用。
因为 kvm 将内存虚拟化处理掉之后，softmmu 的很多复杂机制就完全消失了。

- [ ] 引用上 /home/maritns3/core/vn/docs/qemu/memory/memory-model-kvm.txt
- [ ] 整理 /home/maritns3/core/vn/docs/qemu/memory/memory-model.md
- [ ] 整理 /home/maritns3/core/vn/docs/qemu/memory/softmmu.md

# subpage
之前分析过 flatview_translate 的流程，其作用在于根据 hwaddr 在 AddressSpace 中找到对应的 MemoryRegion
查询过程是采用类似 page walk 的过程，具体参考函数 phys_page_find

但是如果一个 MemoryRegion 无法占据整个 TARGET_PAGE_SIZE 的时候，例如各种 port io 对应的 MemoryRegion，是如何通过
flatview_translate 访问到的?

QEMU 为此设计出来了一个 subpage MemoryRegion:

```txt
|  subpage MR   |
|mr1|mr2|mr3|mr4|
```
通过 phys_page_find 只是获取了 subpage MemoryRegion，可以通过 addr 在页内偏移获取获取找到真正的 MemoryRegion

```c
/* Called from RCU critical section */
static MemoryRegionSection *address_space_lookup_region(AddressSpaceDispatch *d,
                                                        hwaddr addr,
                                                        bool resolve_subpage)
{
    MemoryRegionSection *section = atomic_read(&d->mru_section);
    subpage_t *subpage;

    // 1. 如果 section 没有命中 AddressSpaceDispatch::mru_section
    if (!section || section == &d->map.sections[PHYS_SECTION_UNASSIGNED] ||
        !section_covers_addr(section, addr)) {
        // 2. 那么就老实的查询一次
        section = phys_page_find(d, addr);
        atomic_set(&d->mru_section, section);
    }
    // 3. 根据参数 resolve_subpage 查询是否查询到下面的 mr 的
    if (resolve_subpage && section->mr->subpage) {
        subpage = container_of(section->mr, subpage_t, iomem);
        section = &d->map.sections[subpage->sub_section[SUBPAGE_IDX(addr)]];
    }
    return section;
}
```

这样很多问题都可以被解释了:
1. iotlb 可以加速 softmmu 处理 mmio 的情况，将这个 CPUIOTLBEntry 直接存储这个 gpa 上的 MemoryRegion，实际上上一个 guest page 上可能存在多个物理页面，那么 IOTLB 是如何处理的?
    - 在 tlb_set_page_with_attrs 中调用 memory_region_section_get_iotlb 获取的是 subpage MemoryRegion 索引，然后在 io_readx / io_writex 调用 memory_region_dispatch_read  进行访问的时候的参数也是 subpage MemoryRegion
    - 然后逐步调用到 memory_region_dispatch_read => memory_region_dispatch_read1 => memory_region_read_with_attrs_accessor => subpage_read，然后在 subpage_read 中在进行常规路径的调用
2. address_space_translate 调用 address_space_translate_internal 参数 is_mmio 总是 true，而 address_space_get_iotlb_entry 调用的时候 is_mmio 总是 false ?
  - 因为  address_space_get_iotlb_entry 需要向 IOTLB 上填充的是 subpage MemoryRegion

最后说明一下 subpage 的初始化过程:
- register_subpage
  - subpage_init : 初始化 subpage MemoryRegion，注册其 MemoryRegionOps 为 subpage_ops
  - phys_section_add : 向 PhysPageMap::sections 添加上一个 MemoryRegionSection
  - subpage_register : 让 `subpage->sub_section[SUBPAGE_IDX(addr)]` 索引 `d->map.sections`


# cross page check
是 guest physical page 持有 tb 而不是 guest virtual page 持有 tb

执行代码首先持有的是虚拟地址，在 tb_find => tb_lookup__cpu_state 中进行查找，可以保证 tb_jmp_cache 中找到的绝对有效，但是 TLB flush 之后，tb_jmp_cache 也是会被刷新的。


存在两种情况的 cross page，
1. tb A 跳转到 tb B 上，A B 分别在两个页面上
2. tb B 本身横跨两个页面

- [ ] 从 tb_find 看，其会处理第二种情况，就是直接不跳转的，但是对于第一种情况，没有处理，总是默认使用 tb_add_jump 添加上两者的
  - [ ] 找不到证据说明会处理第一种情况啊
  - tb_set_jmp_target : 设置为跳转到下一个位置


- 当 cross page 的时候，cross 的是 virtual page
  - 虽然 virtual page 的 cross page 就是 physical page 的 cross page
  - QEMU 对于 corss page 的处理就是不做处理，所有跳转到 cross page 的页面都是只能退出，然后

xqm 增加的处理:

- do_tb_flush
  - CPU_FOREACH(cpu) { cpu_tb_jmp_cache_clear(cpu); }
    - xtm_pf_inc_jc_clear
    - *xtm_cpt_flush* : clear Code Page Table (cpt)
    - `atomic_set(&cpu->tb_jmp_cache[i], NULL);` : 就是直接 tb_jmp_cache 的吗? 难道不会采用更加复杂的东西吗 ?
- tb_lookup__cpu_state
  - *xtm_cpt_insert_tb*
- tb_jmp_cache_clear_page
  - *xtm_cpt_flush_page*

### SMC
自修改代码指的是运行过程中修改执行的代码。

检查方法是存在 guest code 的 page 的 CPUTLBEntry 中插入 TLB_NOTDIRTY 的 flag, 这个导致 TLB 比较失败，然后会 invalid 掉这个 guest page 关联的所有的 tb

通过 PageDesc 可以从 ram addr 找到其关联的所有的 tb


# SMC

## lock page
```c
/**
 * struct page_entry - page descriptor entry
 * @pd:     pointer to the &struct PageDesc of the page this entry represents
 * @index:  page index of the page
 * @locked: whether the page is locked
 *
 * This struct helps us keep track of the locked state of a page, without
 * bloating &struct PageDesc.
 *
 * A page lock protects accesses to all fields of &struct PageDesc.
 *
 * See also: &struct page_collection.
 */
struct page_entry {
    PageDesc *pd;
    tb_page_addr_t index;
    bool locked;
};
```
> This struct helps us keep track of the locked state of a page, without bloating &struct PageDesc.

page_collection_lock => page_entry_lock

```c
/**
 * struct page_collection - tracks a set of pages (i.e. &struct page_entry's)
 * @tree:   Binary search tree (BST) of the pages, with key == page index
 * @max:    Pointer to the page in @tree with the highest page index
 *
 * To avoid deadlock we lock pages in ascending order of page index.
 * When operating on a set of pages, we need to keep track of them so that
 * we can lock them in order and also unlock them later. For this we collect
 * pages (i.e. &struct page_entry's) in a binary search @tree. Given that the
 * @tree implementation we use does not provide an O(1) operation to obtain the
 * highest-ranked element, we use @max to keep track of the inserted page
 * with the highest index. This is valuable because if a page is not in
 * the tree and its index is higher than @max's, then we can lock it
 * without breaking the locking order rule.
 *
 * Note on naming: 'struct page_set' would be shorter, but we already have a few
 * page_set_*() helpers, so page_collection is used instead to avoid confusion.
 *
 * See also: page_collection_lock().
 */
struct page_collection {
    GTree *tree;
    struct page_entry *max;
};
```
page_collection 收集一堆 page_entry ，通过一个 page_entry 来索引 PageDesc 的

- page_collection_lock 中通过调用 page_trylock_add 来将一个范围的 block 来 lock, 同时保证如果一个 page 上锁了，比它小的 page 不能直接上锁。
  - 出现 out of order lock 的主要情况 : cross page 的时候，可能先找到 tb 的上半部分所在的 page，然后才找到的下半部分的
  - page_trylock_add 这个函数写的实际上很糟糕，和其调用者 page_collection_lock 的逻辑强耦合，在 page_collection_lock 中，首先会将 tree 中间的全部 lock 一遍，然后才会开始逐个扫描，所以只要

- 如果 page_collection_lock 了，这一块的代码还可以同时被另一个 CPU 执行吗, 应该是可以，主要的目的是屏蔽那些调用 page_lock 的位置, 其中一个重要的用户就是 tb_link_page 了，tb_link_page 通过调用 page_lock_pair 将 tb 所需要的两个 tb 保护起来。

## TARGET_HAS_PRECISE_SMC
TARGET_HAS_PRECISE_SMC 的使用位置只有 tb_invalidate_phys_page_range__locked

为了处理当前的 tb 正好被 SMC 了

## 流程
- 用户态是如此处理的 通过信号机制(SEGV)，系统态直接在 softmmu 的位置检查

保护代码的流程:

- tb_link_page
  - tb_page_add
    - tlb_protect_code
      - cpu_physical_memory_test_and_clear_dirty : 这是 ram_addr.h 中一个处理 dirty page 的标准函数
        - memory_region_clear_dirty_bitmap
        - tlb_reset_dirty_range_all
          - tlb_reset_dirty : 将这个范围内的 TLB 全部添加上 TLB_NOTDIRTY
            - tlb_reset_dirty_range_locked : 这就是设置保护的位置

触发错误的流程:
- store_helper
  - notdirty_write : 当写向一个 dirty 的位置的处理
    - cpu_physical_memory_get_dirty_flag
    - tb_invalidate_phys_page_fast :
    - cpu_physical_memory_set_dirty_range : Set both VGA and migration bits for simplicity and to remove the notdirty callback faster.
    - tlb_set_dirty

## PageDesc
```c
typedef struct PageDesc {
    /* list of TBs intersecting this ram page */
    uintptr_t first_tb;
#ifdef CONFIG_SOFTMMU
    /* in order to optimize self modifying code, we count the number
       of lookups we do to a given page to use a bitmap */
    unsigned long *code_bitmap;
    unsigned int code_write_count;
#else
    unsigned long flags;
    void *target_data;
#endif
#ifndef CONFIG_USER_ONLY
    QemuSpin lock;
#endif
} PageDesc;
```

#### PageDesc::first_tb
从 tb_page_add 可以看到
- TranslationBlock::page_addr : guest 的物理页面地址
- TranslationBlock::page_next : 通过 page_next 将 TranslationBlock 挂到 PageDesc::first_tb 上，这样 PageDesc 就可以找到所有的 TranslationBlock 了


> 注意，TranslationBlock::page_next[2] 存在两个项目，当一个 tb 跨页之后，那么这个 tb 就需要分别添加到
> 两个 PageDesc::first 的数组中。

只要 PageDesc::first_tb page_already_protected

- PageDesc::first_tb 指针的最低两位可以保存这个 PageDesc 是 tb 的第一个 PageDesc 还是第二个。
  - 比如 build_page_bitmap 就需要这个的。

- tb_link_page (exec.c) 把新的 TB 加進 tb_phys_hash 和 l1_map 二級頁表。
tb_find_slow 會用 pc 對映的 GPA 的哈希值索引 tb_phys_hash。

- tb_page_add (exec.c) 設置 TB 的 page_addr 和 page_next，並在 l1_map 中配置 PageDesc 給 TB。

tb_invalidate_phys_page_fast : 一个 PageDesc 并不会立刻创建 bitmap, 而是发现 tb_invalidate_phys_page_fast 多次被调用才会创建
创建 bitmap 的作用是为了精准定位出来到底是哪一个 page 需要被 invalid。

- [ ] page_flush_tb

- [ ] 结构体 PageDesc 的作用是什么 ?
  - 难道时候首先分配 page，然后这些 tb 都是 page
  - 对于连续的物理空间或者虚拟地址空间，感觉并没有必要如此必要吧
  - TranslationBlock::page_addr
    - 记录了一个 TB 所在的页面
    - 如果页面是连续的，就不应该申请两个

- [ ] SMC_BITMAP_USE_THRESHOLD
  - 和 highwater 什么关系?

- tb_gen_code : 这是一个关键核心
  - get_page_addr_code : 将虚拟地址的 pc 装换为物理地址
    - get_page_addr_code_hostp
      - 如果命中，就是 TLB 的翻译 `p = (void *)((uintptr_t)addr + entry->addend);`
      - qemu_ram_addr_from_host_nofail
  - tb_link_page : 将 tb 纳入到 QEMU 的管理中
    - tb_page_add
      - [ ] invalidate_page_bitmap : 根本无法理解，link page 的时候为什么会将 bitmap disable 掉
      - tlb_protect_code : 指向 exec.c 中间，应该是通过 dirty / clean 的方式来防止代码被修改 ?
        - [ ] 原则上，guest 代码段被修改必然需要让对应的 tb 也是被 invalidate 的呀

#### PageDesc::code_bitmap / PageDesc::code_write_count
主要的使用位置 : tb_invalidate_phys_page_fast

逻辑非常简单，如果一个 PageDesc 对应的 page 反复被 invalidate 的时候，那么就会建立 bitmap 将其中真正有代码的位置确认，
只有命中了翻译了 tb 的位置，才会真正的 invalidate 的，而一般的处理是，直接 invalidate 所有的。

## page desc tree
- PageDesc 的 lock 是基于什么的 ?

创建:
- page_lock_pair
  - page_find_alloc

查询:
- page_find
  - page_find_alloc

将 tb 放到 PageDesc 的管理中:
- tb_link_page
  - tb_page_add : 需要将 bitmap invalidate 掉

```c
/* Size of the L2 (and L3, etc) page tables.  */
#define V_L2_BITS 10
#define V_L2_SIZE (1 << V_L2_BITS)

# define L1_MAP_ADDR_SPACE_BITS  TARGET_PHYS_ADDR_SPACE_BITS

/*
 * L1 Mapping properties
 */
static int v_l1_size;
static int v_l1_shift;
static int v_l2_levels;

/* The bottom level has pointers to PageDesc, and is indexed by
 * anything from 4 to (V_L2_BITS + 3) bits, depending on target page size.
 */
#define V_L1_MIN_BITS 4
#define V_L1_MAX_BITS (V_L2_BITS + 3)
#define V_L1_MAX_SIZE (1 << V_L1_MAX_BITS)

static void *l1_map[V_L1_MAX_SIZE];

static void page_table_config_init(void)
{
    uint32_t v_l1_bits;

    assert(TARGET_PAGE_BITS);
    /* The bits remaining after N lower levels of page tables.  */
    v_l1_bits = (L1_MAP_ADDR_SPACE_BITS - TARGET_PAGE_BITS) % V_L2_BITS;

    if (v_l1_bits < V_L1_MIN_BITS) {
        v_l1_bits += V_L2_BITS;
    }

    v_l1_size = 1 << v_l1_bits;
    v_l1_shift = L1_MAP_ADDR_SPACE_BITS - TARGET_PAGE_BITS - v_l1_bits;
    v_l2_levels = v_l1_shift / V_L2_BITS - 1;

    assert(v_l1_bits <= V_L1_MAX_BITS);
    assert(v_l1_shift % V_L2_BITS == 0);
    assert(v_l2_levels >= 0);
}
```

- 需要覆盖架构支持的所有的物理地址
- 保证 V_L2_BITS 总是 10

## tb_invalidate_phys_page_fast
- tb_invalidate_phys_page_fast
  - page_find
    - [ ] page_find_alloc(tb_page_addr_t index, int alloc)
      - 分配空间，还需要考虑 level 什么的
      - [ ] page_find_alloc 中间为什么需要使用 rcu
  - build_page_bitmap
  - tb_invalidate_phys_page_range__locked
    - tb_phys_invalidate__locked
      - do_tb_phys_invalidate
        - do_tb_phys_invalidate(在 chen 的笔记中叫做 tb_phys_invalidate)，在这里完成真正的工作, 将 tb 从 hash 中间移除之类的

- tb_invalidate_phys_page_range__locked : 这是真正进行工作的位置
  * tb_invalidate_phys_range
      * invalidate_and_set_dirty : 调用这个的位置超级多
      * tb_check_watchpoint
  * tb_invalidate_phys_page_range
    * tb_invalidate_phys_addr : 没有用户, 或者说是一个很奇怪的架构需要这个东西

# 非对其访问

- [x] 所以，x86 为什么不害怕这个非对其访问的问题 ?

1. 搜索符号，convert_to_tcgmemop 上的注释对于
MemOp 这个 enum 做出来一些解释, 包含了 size / signed / endian / aligned
2. get_alignment_bits 的逻辑看，就是检查一下 MemOp 的 6:4 的 bit 而已
3. 目前的代码，所有的位置都没有插入过 `MO_ALIGN_x` 的

所以 addr & ((1 << a_bits) - 1) 的检查永远失败啊


cpu_unaligned_access : x86 对应的 handler 没有赋值
但是一些 risc 平台是赋值的，
```c
void riscv_cpu_do_unaligned_access(CPUState *cs, vaddr addr,
                                   MMUAccessType access_type, int mmu_idx,
                                   uintptr_t retaddr)
{
    RISCVCPU *cpu = RISCV_CPU(cs);
    CPURISCVState *env = &cpu->env;
    switch (access_type) {
    case MMU_INST_FETCH:
        cs->exception_index = RISCV_EXCP_INST_ADDR_MIS;
        break;
    case MMU_DATA_LOAD:
        cs->exception_index = RISCV_EXCP_LOAD_ADDR_MIS;
        break;
    case MMU_DATA_STORE:
        cs->exception_index = RISCV_EXCP_STORE_AMO_ADDR_MIS;
        break;
    default:
        g_assert_not_reached();
    }
    env->badaddr = addr;
    env->two_stage_lookup = riscv_cpu_virt_enabled(env) ||
                            riscv_cpu_two_stage_lookup(mmu_idx);
    riscv_raise_exception(env, cs->exception_index, retaddr);
}
```
原因在于，X86 的根本没有非对其访问的 exception 啊

```c
static inline MemOp get_memop(TCGMemOpIdx oi) { return oi >> 4; }

static inline unsigned get_mmuidx(TCGMemOpIdx oi) { return oi & 15; }
```

- [ ] 如果 x86 本身是非对其访问，但是我们又是不支持非对其，那怎么办 ?
    - 我猜测是被模拟成为两次访问

# 如何重新设计 QEMU 中的 memory

## 设计
AddressSpace 中增加两个函数?

1. segments 直接初始化为固定大小的数组
2. 所有的 MemoryRegion 在按照顺序排放
3. 使用二分查找来找到 MemoryRegion
4. 以后增加一个 mru 的 cache 维持生活这个样子的

- 对于 SMM 的处理，首先判断是否命中 SMM 空间
  - 常规路径
  - 查看是否是 SMM 的那个 MemoryRegion 的，如果是，再看 attrs 的结果

- 对于 0xfffff 下面的那些 memory region 都是直接静态的定义下来的

## 需求分析
- [ ] 原本的 QEMU 初始化 memory 的函数全部都列举出来
  - [ ] x86_bios_rom_init : 进行 bios 相关的初始 *完全没有移植*
    - [ ] rom_add_file_fixed : 如何和 pc.bios 联系起来的
  - [ ] pc_memory_init : 将其重新 copy 将不需要的部分适应 BMBT 包围起来
  - 很多函数都在被过于快速的删除了

- [x] 在 PAM 完全没有打开的时候，是使用 read only 来表示还是直接这个 MemoryRegion 就是不存在的
  - 最开始，走的是 PCI 的

- [x] 映射到 00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios 的部分是可写的吗?
  - 不是的，在 x86_bios_rom_init 中，会设置 rom memory_region_set_readonly(bios, true); 在 mtree_info 上的显示也是可以区分的

- 修改那些位置是需要进行 tcg_commit 的
  - 修改 AddressSpace::segments 的时候，也就是 io_add_memory_region 和 mem_add_memory_region
  - smm / pam 修改映射关系的时候，对应原来的代码  memory_region_set_enabled 会调用 memory_region_transaction_commit 的情况啊!

```c
/*
 * SMRAM memory area and PAM memory area in Legacy address range for PC.
 * PAM: Programmable Attribute Map registers
 *
 * 0xa0000 - 0xbffff compatible SMRAM
 *
 * 0xc0000 - 0xc3fff Expansion area memory segments // 每一个 segments
 * 0xc4000 - 0xc7fff
 * 0xc8000 - 0xcbfff
 * 0xcc000 - 0xcffff
 * 0xd0000 - 0xd3fff
 * 0xd4000 - 0xd7fff
 * 0xd8000 - 0xdbfff
 * 0xdc000 - 0xdffff
 * 0xe0000 - 0xe3fff Extended System BIOS Area Memory Segments
 * 0xe4000 - 0xe7fff
 * 0xe8000 - 0xebfff
 * 0xec000 - 0xeffff
 *
 * 0xf0000 - 0xfffff System BIOS Area Memory Segments
 */
```

其实，PAM
```txt
0000000000000000-ffffffffffffffff (prio -1, i/o): pci
  00000000000c0000-00000000000dffff (prio 1, rom): pc.rom
  00000000000e0000-00000000000fffff (prio 1, i/o): alias isa-bios @pc.bios 0000000000020000-000000000003ffff
  00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
```
- [x] 分析一下 pc.rom 是如何初始化，是不是完全没有任何的作用
  - 是的

- [x] 修改 PAM 的时候，是不是所有的 CPU 都可以看到的
  - 是的

- 恐怕，RAMBlock 显然是不能只有一块的

- [x] 我们是如何通过 RamBlock 实现空洞的处理的
  - alias
  - 在 memory_region_get_ram_ptr 首先计算出来一个 MemoryRegion 所在的偏移

- 如何构建 MemoryRegion 和 RamBlock 的关系啊?
  - 从 memory_region_get_ram_ptr 中，应该出现的是，


- 将 dirty memory 的 ram_addr

- memory_region_get_ram_ptr : 返回一个 RAMBlock 在 host 中的偏移量
- memory_region_get_ram_addr : 获取在 ram 空间的偏移
- memory_region_section_get_iotlb : 如果是一个

- 在 tlb_set_page_with_attrs 的 xlat 是 MemoryRegion 内的偏移
  - 需要靠 address_space_translate_for_iotlb 同时返回 MemoryRegion 和 xlat

## address_space_translate_for_iotlb 和 address_space_translate 的关系

- address_space_translate
  - address_space_to_flatview : 获取 Flatview
  - flatview_translate : 返回 MemoryRegionSection
    - flatview_do_translation
      - flatview_to_dispatch : 从 Flatview 获取 dispatch
        - address_space_translate_internal
    - 从 MemoryRegionSection 获取 mr

- address_space_translate_for_iotlb
  - `atomic_rcu_read(&cpu->cpu_ases[asidx].memory_dispatch)` : 直接获取 dispatch
  - address_space_translate_internal

| function                          | para                                            | res                       |
|-----------------------------------|-------------------------------------------------|---------------------------|
| address_space_translate_for_iotlb | [cpu asidx](获取地址) addr [attrs prot](没用的) | MemoryRegionSection, xlat |
| address_space_translate           | as addr is_write attrs ()                       | MemoryRegion, xlat, len   |
| address_space_translate_internal  | d, addr, resolve_subpage                        | xlat, plen                |




- [x] attrs 有用?
- [x] address_space_translate 返回 len : 在 read continue 中使用的
  - 只有 ram 的时候会遇到
  - 因为访问的时候可能越过 MemoryRegion 的范围
  - 估计当前的项目中不会遇到，增加上 duck_check 的方法
  - mmio 中也是需要考虑 size 的问题: 会使用 prepare_mmio_access 计算
  - 注释中也是如此说明的
    - [x] 注释说，When such registers exist (e.g. I/O ports 0xcf8 and 0xcf9 on most PC chipsets), MMIO regions overlap wildly.

- [ ] address_space_rw : 是不是只有 dma 的位置在使用?
  - [ ] 重新看看访存辅助函数集合 ?
  - [ ] 所以到底如何处理 flatview_write_continue，其中只有 ram 的访问吗?

-  memory_access_size 中会进行返回值调整的就只有一个 hpet 了

这个空间居然是重合的，鬼鬼:
```c
    0000000000000cf8-0000000000000cfb (prio 0, i/o): pci-conf-idx
    0000000000000cf9-0000000000000cf9 (prio 1, i/o): piix3-reset-control
    0000000000000cfc-0000000000000cff (prio 0, i/o): pci-conf-data
```

在 Flatview 中的效果的确如此:
```c
  0000000000000cf8-0000000000000cf8 (prio 0, i/o): pci-conf-idx
  0000000000000cf9-0000000000000cf9 (prio 1, i/o): piix3-reset-control
  0000000000000cfa-0000000000000cfb (prio 0, i/o): pci-conf-idx @0000000000000002
```

## 笔记补充
分析出来 i440fx_update_memory_mappings 的每一次调用


1. 在 i440fx_init 中间自然会初始化出来一个

2. 这个实际上，将整个空间都打开了, 这个会调用两次
```c
__make_bios_writable_intel(u16 bdf, u32 pam0)
{
    // Read in current PAM settings from pci config space
    union pamdata_u pamdata;
    pamdata.data32[0] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4));
    pamdata.data32[1] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4) + 4);
    u8 *pam = &pamdata.data8[pam0 & 0x03];

    // Make ram from 0xc0000-0xf0000 writable
    int i;
    for (i=0; i<6; i++)
        pam[i + 1] = 0x33;

    // Make ram from 0xf0000-0x100000 writable
    int ram_present = pam[0] & 0x10;
    pam[0] = 0x30;

    // Write PAM settings back to pci config space
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4), pamdata.data32[0]);
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4) + 4, pamdata.data32[1]);

    if (!ram_present)
        // Copy bios.
        memcpy(VSYMBOL(code32flat_start)
               , VSYMBOL(code32flat_start) + BIOS_SRC_OFFSET
               , SYMBOL(code32flat_end) - SYMBOL(code32flat_start));
```

3. 最后这个会调用两次，应该是全部设置为 readonly 的，但是操作上不是如此的:
```c
static void
make_bios_readonly_intel(u16 bdf, u32 pam0)
{
    // Flush any pending writes before locking memory.
    wbinvd();

    // Read in current PAM settings from pci config space
    union pamdata_u pamdata;
    pamdata.data32[0] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4));
    pamdata.data32[1] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4) + 4);
    u8 *pam = &pamdata.data8[pam0 & 0x03];

    // Write protect roms from 0xc0000-0xf0000
    u32 romlast = BUILD_BIOS_ADDR, rommax = BUILD_BIOS_ADDR;
    if (CONFIG_WRITABLE_UPPERMEMORY)
        romlast = rom_get_last();
    if (CONFIG_MALLOC_UPPERMEMORY)
        rommax = rom_get_max();
    int i;
    for (i=0; i<6; i++) {
        u32 mem = BUILD_ROM_START + i * 32*1024;
        if (romlast < mem + 16*1024 || rommax < mem + 32*1024) {
            if (romlast >= mem && rommax >= mem + 16*1024)
                pam[i + 1] = 0x31;
            break;
        }
        pam[i + 1] = 0x11;
    }

    // Write protect 0xf0000-0x100000
    pam[0] = 0x10;

    // Write PAM settings back to pci config space
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4), pamdata.data32[0]);
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4) + 4, pamdata.data32[1]);
}
```
- 从 romlast 和 rommax 让 set readonly 似乎比想象的更加复杂
  - 0xe4000 ~ 0xeffff 之间的区间的确会被豁免
  - 的确是因为从 bios 的哪里设置的原因

```txt
    00000000000cb000-00000000000cdfff (prio 1000, i/o): alias kvmvapic-rom @pc.ram 00000000000cb000-00000000000cdfff
```

```txt
  00000000000c0000-00000000000cafff (prio 0, rom): pc.ram @00000000000c0000
  00000000000cb000-00000000000cdfff (prio 0, ram): pc.ram @00000000000cb000 // kvmvapic-rom
  00000000000ce000-00000000000e3fff (prio 0, rom): pc.ram @00000000000ce000
  00000000000e4000-00000000000effff (prio 0, ram): pc.ram @00000000000e4000 // 被豁免的
  00000000000f0000-00000000000fffff (prio 0, rom): pc.ram @00000000000f0000
```


3. 所以，中间的那一次是怎么回事啊?
  - smram-region 被 disable 掉了 ?
  - 实际上，i440fx_update_memory_mappings 本身就是具有更新 SMRAM 的功能的

4. 三个作用分别是啥?
  - smram_region : 让 vga-low 暴露出来
  - smram-low 是 pc.ram 的 alias 放到 smram 下
```c
struct PCII440FXState {
    MemoryRegion smram_region;
    MemoryRegion smram, low_smram;
};
```

```txt
memory-region: smram
  0000000000000000-00000000ffffffff (prio 0, i/o): smram
    00000000000a0000-00000000000bffff (prio 0, i/o): alias smram-low @pc.ram 00000000000a0000-00000000000bffff
```

好的，找到了中间的两次

```c
// This code is hardcoded for PIIX4 Power Management device.
static void piix4_apmc_smm_setup(int isabdf, int i440_bdf)
{
    /* check if SMM init is already done */
    u32 value = pci_config_readl(isabdf, PIIX_DEVACTB);
    if (value & PIIX_DEVACTB_APMC_EN)
        return;

    /* enable the SMM memory window */
    pci_config_writeb(i440_bdf, I440FX_SMRAM, 0x02 | 0x48);

    smm_save_and_copy();

    /* enable SMI generation when writing to the APMC register */
    pci_config_writel(isabdf, PIIX_DEVACTB, value | PIIX_DEVACTB_APMC_EN);

    /* enable SMI generation */
    value = inl(acpi_pm_base + PIIX_PMIO_GLBCTL);
    outl(value | PIIX_PMIO_GLBCTL_SMI_EN, acpi_pm_base + PIIX_PMIO_GLBCTL);

    smm_relocate_and_restore();

    /* close the SMM memory window and enable normal SMM */
    pci_config_writeb(i440_bdf, I440FX_SMRAM, 0x02 | 0x08);
}
```

```c
    int smram_region = !(pd->config[I440FX_SMRAM] & SMRAM_D_OPEN);
    int smram = pd->config[I440FX_SMRAM] & SMRAM_G_SMRAME;
    printf("huxueshi:%s %d %d\n", __FUNCTION__, smram_region, smram);
```

1 0 ==> 所有的 CPU 都是无法访问到 SMM 空间的
0 8 ==> 所有的 CPU 访问到的都是 smm 模式
1 8 ==> 正常的 smm 空间，smm 模式下的 CPU 和正常的 CPU 走不同的位置

正常模式下，访问流程：
- smram_region 是否打开，如果是，那么就会发生两个

smm 模式下:
  - smram 打开了，一定走 ram 的
  - smram 未打开，和普通相同的


<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://github.com/azru0512/slide/tree/master/QEMU
[^2]: https://qemu.weilnetz.de/w64/2012/2012-06-28/qemu-tech.html#Self_002dmodifying-code-and-translated-code-invalidation
