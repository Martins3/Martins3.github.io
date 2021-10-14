# QEMU 的 memory model 和 softmmu 高级话题分析

> 主要是基于 tcg 分析，应该对于做云计算的同学没有什么作用。

- [ ] 引用上 /home/maritns3/core/vn/docs/qemu/memory/memory-model-kvm.txt

# cross page check
- [ ] 在 translate-all.h 中间，为什么反而需要 references
```c
#include "../i386/LATX/include/cross-page-check.h"
```

当我们分析 cross page 的时候，应该是一个 guest 的一个 tb 正好在两个 page 上吧

- [ ] 在 core part 的地方已经处理过 cross page 了吧
  - 为什么 LATX 需要特殊处理这一个事情啊 ?

  - TranslationBlock::page_next
  - [x] 顺便问一下，PageDesc 是用于存放 x86 的还是 la 的指令的
    - 因为 PageDesc 是用于 SMC 的，在于根据 ram_addr 找到所有的 TranslationBlock，所以是 la 指令的

```c
  /* first and second physical page containing code. The lower bit
     of the pointer tells the index in page_next[].
     The list is protected by the TB's page('s) lock(s) */
  uintptr_t page_next[2];

  tb_page_addr_t page_addr[2];
```

1. tb::page_addr 分析 page_unlock_tb  就是 x86 代码所在的物理页的地址
2. PageDesc::first_tb : 在 tb_page_add 中初始化, 赋值就是 TranslationBlock

- src/i386/LATX/translator/cross-page-check.c
- src/i386/LATX/include/cross-page-check.h

## 分析一下 cross-page-check.h

- do_tb_flush
  - CPU_FOREACH(cpu) { cpu_tb_jmp_cache_clear(cpu); }
    - xtm_pf_inc_jc_clear
    - *xtm_cpt_flush* : clear Code Page Table (cpt) : 注意，这是一个可选项目
    - `atomic_set(&cpu->tb_jmp_cache[i], NULL);` : 就是直接 tb_jmp_cache 的吗? 难道不会采用更加复杂的东西吗 ?

- tb_lookup__cpu_state
  - *xtm_cpt_insert_tb*

- tb_jmp_cache_clear_page
  - *xtm_cpt_flush_page*

## 资料收集
tb_find 中的注释
- 因为我们使用虚拟地址来进行直接跳转的时候，如果一个 x86 tb 是本身是 cross page 的
那么其他的 tb 不能直接跳转到这里，也就是不能调用 tb_find 来进行跳转
- [ ] 那么间接跳转是怎么操作的
- [ ] 为什么 x86 tb 在两个 page 上的时候会出现问题

```c
  /* We don't take care of direct jumps when address mapping changes in
   * system emulation. So it's not safe to make a direct jump to a TB
   * spanning two pages because the mapping for the second page can change.
   */
```

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
在 tb_page_add 中，只要新的 tb 添加进来，那么 PageDesc::first_tb 就会指向其,
TranslationBlock::page_next[2] 中

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

## 参考
[^1]: https://github.com/azru0512/slide/tree/master/QEMU
[^2]: https://qemu.weilnetz.de/w64/2012/2012-06-28/qemu-tech.html#Self_002dmodifying-code-and-translated-code-invalidation
