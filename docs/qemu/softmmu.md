# QEMU 的 memory model 和 softmmu 高级话题分析

<!-- vim-markdown-toc GitLab -->

  - [[ ] MemOp](#-memop)
  - [lock page](#lock-page)
  - [SMC](#smc)
  - [TARGET_HAS_PRECISE_SMC](#target_has_precise_smc)
  - [流程](#流程)
  - [PageDesc](#pagedesc)
      - [PageDesc::first_tb](#pagedescfirst_tb)
      - [PageDesc::code_bitmap / PageDesc::code_write_count](#pagedesccode_bitmap-pagedesccode_write_count)
  - [page desc tree](#page-desc-tree)
  - [tb_invalidate_phys_page_fast](#tb_invalidate_phys_page_fast)
  - [unaligned access](#unaligned-access)
- [如何重新设计 QEMU 中的 memory](#如何重新设计-qemu-中的-memory)
  - [设计](#设计)
  - [需求分析](#需求分析)
  - [address_space_translate_for_iotlb 和 address_space_translate 的关系](#address_space_translate_for_iotlb-和-address_space_translate-的关系)
  - [笔记补充](#笔记补充)
  - [Softmmu](#softmmu)
    - [Overview](#overview)
    - [soft TLB](#soft-tlb)
    - [ram addr](#ram-addr)

<!-- vim-markdown-toc -->

主要是基于 tcg 分析，对于做云计算的同学应该没有什么作用。
因为 kvm 将内存虚拟化处理掉之后，softmmu 的很多复杂机制就完全消失了。

- [ ] 引用上 /home/maritns3/core/vn/docs/qemu/memory/memory-model-kvm.txt
- [ ] 整理 /home/maritns3/core/vn/docs/qemu/memory/memory-model.md
- [ ] 整理 /home/maritns3/core/vn/docs/qemu/memory/softmmu.md

## [ ] MemOp
convert_to_tcgmemop 上的注释对于
MemOp 这个 enum 做出来一些解释, 包含了 size / signed / endian / aligned

2.  的逻辑看，就是检查一下 MemOp 的 6:4 的 bit 而已
3. 目前的代码，所有的位置都没有插入过 `MO_ALIGN_x` 的

```c
static inline MemOp get_memop(TCGMemOpIdx oi) { return oi >> 4; }

static inline unsigned get_mmuidx(TCGMemOpIdx oi) { return oi & 15; }
```

## lock page
在 ./accel/tcg/translate-all.c 中我们实际上看到了一系列的 lock，比如
- page_lock_tb
- page_lock_pair
- page_lock
- page_entry_lock
- page_collection_lock

现在分析一下，page lock 的作用和实现。

首先理清楚几个基本问题:
- TB 关联的是 guest physical page
  - 如果 guest physical page 发生改变，那么生成的所有的 TB 都需要 invalidate 掉
  - 多个 guest virtual page 可以映射到同一个 guest physical page 上，当执行这些 guest virtual page 的代码的时候，将会找到相同的 TB，而不是生成出来多份
  - 执行 TB 指向，首先需要从 TB buffer 中查询 TB，使用的是 gpa 来查询的
- TB 只会 invalidate 操作掉，然后在 tb_flush 的时候将整个 TB buffer 全部删除，不会出现单独释放 tb_buffer 中的一个位置，生成的 TB 只会不断放到 tb buffer 的后面，直到 TB buffer 满了。

需要的实现的目标是，一个 guest physical page 上关联的 TB 总是和 guest physical page 上的 x86 代码总是对应的。而不要出现:
- guest physical page 上的内容被修改了，但是对应的 TB 没有被 invalidate 掉
- 多个 vCPU thread 同时生成 TB，同时关联，出现 data race，最后生成错误的数据

分析具体的代码:
- 写操作
  - tb_link_page : add a new TB and link it to the physical page tables
  - tb_phys_invalidate

- 读操作: tb_htable_lookup

这里有两个注意点:
- 因为 SMC 可能 invalidate 多个 physical page，所以有 page_collection_lock 的情况，此时需要注意上锁的时候保证从低到高逐个上锁，否则容易出现死锁
- tb_htable_lookup 使用 qht 实现，所以，tb_htable_lookup 实际上无需持有 page lock 的

## SMC
自修改代码指的是运行过程中修改执行的代码。

检查方法是存在 guest code 的 page 的 CPUTLBEntry 中插入 TLB_NOTDIRTY 的 flag, 这个导致 TLB 比较失败，然后会 invalid 掉这个 guest page 关联的所有的 tb

通过 PageDesc 可以从 ram addr 找到其关联的所有的 tb


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

## unaligned access
```c
static inline uint64_t QEMU_ALWAYS_INLINE load_helper(
    CPUArchState *env, target_ulong addr, TCGMemOpIdx oi, uintptr_t retaddr,
    MemOp op, bool code_read, FullLoadHelper *full_load) {

  unsigned a_bits = get_alignment_bits(get_memop(oi));

  /* Handle CPU specific unaligned behaviour */
  if (addr & ((1 << a_bits) - 1)) {
    cpu_unaligned_access(env_cpu(env), addr, access_type, mmu_idx, retaddr);
  }
```

如果 guest 是 x86，cpu_unaligned_access 是永远不会被调用的,
但是一些 risc 平台是赋值的。
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
原因在于，X86 的根本没有非对其访问的 exception，但是很多 RISC 平台上是有的。

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

## Softmmu

### Overview
softmmu 只有 tcg 才需要，实现基本思路是:
- 所有的访存指令前使用软件进行地址翻译，如果命中，那么获取 GPA 进行访存
- 如果不命中，慢路径，也就是 store_helper

### soft TLB
TLB 的大致结构如下, 对此需要解释一些问题:
![](./img/tlb.svg)

1. 快速路径访问的是 CPUTLBEntry 的
2. 而慢速路径访问 victim TLB 和 CPUIOTLBEntry
3. victim TLB 的大小是固定的，而正常的 TLB 的大小是动态调整的
4. CPUTLBEntry 的说明:
    - addend : GVA + addend 等于 HVA
    - 分别创建出来三个 addr_read / addr_write / addr_code 是为了快速比较，两者相等就是命中，不相等就是不命中，如果向操作系统中的 page table 将 page entry 插入 flag 描述权限，这个比较就要使用更多的指令了(移位/掩码之后比较)
5. CPUIOTLBEntry 的说明:
  - 如果不是落入 RAM : TARGET_PAGE_BITS 内可以放 AddressSpaceDispatch::PhysPageMap::MemoryRegionSection 数组中的偏移, 之外的位置放 MemoryRegion 内偏移。通过这个可以迅速获取 MemoryRegionSection 进而获取 MemoryRegion。
  - 如果是落入 RAM , 可以得到 [ram addr](#ram-addr)

### ram addr
构建 ram addr 的目的 dirty page 的记录，所有的 page 的 dirty 都是记录在 `RAMList::DirtyMemoryBlocks::blocks` 中
给出一个 ram 中的一个 page，需要找到在 blocks 数组中的下标，于是发明了 ram addr
```c
typedef struct {
    struct rcu_head rcu;
    unsigned long *blocks[];
} DirtyMemoryBlocks;

typedef struct RAMList {
    QemuMutex mutex;
    RAMBlock *mru_block;
    /* RCU-enabled, writes protected by the ramlist lock. */
    QLIST_HEAD(, RAMBlock) blocks;
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    uint32_t version;
    QLIST_HEAD(, RAMBlockNotifier) ramblock_notifiers;
} RAMList;
```
QEMU 使用 RAMBlock 来描述 ram，MemoryRegion 的类型是 ram，那么就会关联一个 RAMBlock

将所有的 RAMBlock 连续的连到一起，形成 RAMList ，一个 RAMBlock 在其中偏移量记录在 `RAMBlock::offset`, 显然，第一个 offset 为 0
```c
/*
pc.ram: offset=0 size=180000000
pc.bios: offset=180000000 size=40000
pc.rom: offset=180040000 size=20000
vga.vram: offset=180080000 size=800000
/rom@etc/acpi/tables: offset=180900000 size=200000
virtio-vga.rom: offset=180880000 size=10000
e1000.rom: offset=1808c0000 size=40000
/rom@etc/table-loader: offset=180b00000 size=10000
/rom@etc/acpi/rsdp: offset=180b40000 size=1000
```
任何一个 page 的 ram_addr = offset in RAM + `RAMBlock::offset`


<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://github.com/azru0512/slide/tree/master/QEMU
[^2]: https://qemu.weilnetz.de/w64/2012/2012-06-28/qemu-tech.html#Self_002dmodifying-code-and-translated-code-invalidation
