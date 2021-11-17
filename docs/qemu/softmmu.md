# QEMU 的 softmmu 设计

主要是基于 tcg 分析，对于做云计算的同学应该没有什么作用。
因为 kvm 将内存虚拟化处理掉之后，softmmu 的很多复杂机制就完全消失了。

## Overview
softmmu 只有 tcg 才需要，实现基本思路是:
- 所有的访存指令前使用软件进行地址翻译，如果命中，那么获取 GPA 进行访存
- 如果不命中，慢路径，也就是 store_helper

TLB 在 code 中的经典路径:
```c
/*
 * Since the addressing of x86 architecture is complex, we
 * remain the original processing that exacts the final
 * x86vaddr from IR1_OPND(x86 opnd).
 * It is usually done by functions load_ireg_from_ir1_mem()
 * and store_ireg_from_ir1_mem().
```
- gen_ldst_softmmu_helper
  - `__gen_ldst_softmmu_helper_native` : 其中的注释非常清晰，首先查询 TLB，如果不成功，进入慢路径
    - tr_gen_lookup_qemu_tlb : TLB 比较查询
    - tr_gen_ldst_slow_path : 无奈，只能跳转到 slow path 去
      - td_rcd_softmmu_slow_path

- tr_ir2_generate
  - tr_gen_tb_start
  - tr_gen_softmmu_slow_path : slow path 的代码在每一个 tb 哪里只会生成一次
    - `__tr_gen_softmmu_sp_rcd`
      - helper_ret_stb_mmu : 跳转的入口通过 helper_ret_stb_mmu 实现, 当前在 accel/tcg/cputlb.c 中
        - store_helper
          - io_writex
            - memory_region_dispatch_write
  - tr_gen_tb_end

进行 TLB 填充的经典路径:
- store_helper
  - tlb_fill
    - x86_cpu_tlb_fill
      - handle_mmu_fault : 利用 x86 的页面进行 page walk
        - tlb_set_page_with_attrs : 设置页面

## soft TLB
TLB 的大致结构如下, 对此需要解释一些问题:
![](./img/tlb.svg)

1. 快速路径访问的是 CPUTLBEntry 的
2. 而慢速路径访问 victim TLB 和 CPUIOTLBEntry
3. victim TLB 的大小是固定的，而正常的 TLB 的大小是动态调整的
4. CPUTLBEntry 的说明:
    - addr_read / addr_write / addr_code 都是 GVA
    - 分别创建出来三个 addr_read / addr_write / addr_code 是为了快速比较，两者相等就是命中，不相等就是不命中，如果向操作系统中的 page table 将 page entry 插入 flag 描述权限，这个比较就要使用更多的指令了(移位/掩码之后比较)
    - addend : GVA + addend 等于 HVA
5. CPUIOTLBEntry 的说明:
  - 如果不是落入 RAM : TARGET_PAGE_BITS 内可以放 AddressSpaceDispatch::PhysPageMap::MemoryRegionSection 数组中的偏移, 之外的位置放 MemoryRegion 内偏移。通过这个可以迅速获取 MemoryRegionSection 进而获取 MemoryRegion。
  - 如果是落入 RAM , 可以得到 [ram addr](#ram-addr)

### CPUIOTLBEntry
```c
typedef struct CPUIOTLBEntry {
    /*
     * @addr contains:
     *  - in the lower TARGET_PAGE_BITS, a physical section number
     *  - with the lower TARGET_PAGE_BITS masked off, an offset which
     *    must be added to the virtual address to obtain:
     *     + the ram_addr_t of the target RAM (if the physical section
     *       number is PHYS_SECTION_NOTDIRTY or PHYS_SECTION_ROM)
     *     + the offset within the target MemoryRegion (otherwise)
     */
    hwaddr addr;
    MemTxAttrs attrs;
} CPUIOTLBEntry;
```
这个注释，其实有点误导，我以为 lower TARGET_PAGE_BITS 中总是存放的 physical section number
实际上，
- physical section number 就是 MemoryRegionSection 在 AddressSpaceDispatch::map::sections 中的 index
- 对于 is_ram 而言，直接就是翻译为 ram_addr 了，lower TARGET_PAGE_BITS 没有任何东西
- 对于 IO 而言，lower TARGET_PAGE_BITS 保存 physical section number, upper 部分是 MemoryRegion 的偏移，两者一并计算出来需要访问的是 MemoryRegion 的哪里

```c
void tlb_set_page_with_attrs(CPUState *cpu, target_ulong vaddr,
                             hwaddr paddr, MemTxAttrs attrs, int prot,
                             int mmu_idx, target_ulong size)
{

    // 因使用的是 paddr_page 来查询的，所以 xlat 必然也页对齐的
    section = address_space_translate_for_iotlb(cpu, asidx, paddr_page,
                                                &xlat, &sz, attrs, &prot);

    if (is_ram) {
        iotlb = memory_region_get_ram_addr(section->mr) + xlat;
        // ...
    } else {
        /* I/O or ROMD */
        iotlb = memory_region_section_get_iotlb(cpu, section) + xlat;
        // ...
    }
}
```

- CPUIOTLBEntry 的创建位置:
  - tlb_set_page_with_attrs

- CPUIOTLBEntry 的使用位置:
  - probe_access
    - tlb_hit / victim_tlb_hit / tlb_fill : TLB 访问经典三件套
    - cpu_check_watchpoint
    - notdirty_write : 和 cpu_check_watchpoint 相同，需要 iotlbentry 作为参数
  - io_writex / io_readx
    - iotlb_to_section
    - `mr_offset = (iotlbentry->addr & TARGET_PAGE_MASK) + addr;`

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
用户态是通过信号机制(SEGV)，系统态直接在 softmmu 的位置检查

在系统态中，存在 guest code 的 page 的 CPUTLBEntry 中插入 TLB_NOTDIRTY 的 flag, 这个导致 TLB 比较失败，
通过 PageDesc 可以从 ram addr 找到其关联的所有的 tb，
进而 invalidate 掉这个 guest page 关联的所有的 tb

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


### PageDesc
如果找到 guest physical page 上的所有的 TB ，这就是通过 PageDesc 进行的

#### PageDesc::first_tb
- TranslationBlock::page_next : 通过 page_next 将 TranslationBlock 挂到 PageDesc::first_tb 上
- TranslationBlock::page_next[2] 存在两个项目，当一个 tb 跨页之后，那么这个 tb 就需要分别添加到两个 PageDesc::first 的数组中。 只要 PageDesc::first_tb 那么 page_already_protected

- PageDesc::first_tb 和 TranslationBlock::page_next 指针的最低一位可以用于 PageDesc p 知道这个 TB 的那一部分在 p 中

在 tb_page_add 中初始化:
```c
static inline void tb_page_add(PageDesc *p, TranslationBlock *tb,
                               unsigned int n, tb_page_addr_t page_addr)
{

    tb->page_next[n] = p->first_tb;
    // ...
    p->first_tb = (uintptr_t)tb | n;
```

构建出来一个标准处理的 macro, macro 的使用者将会获得三个变量:
- p : PageDesc
- tb : 在 p 上的一个 TranslationBlock
- n
  - n == 0 : 说明 tb 要么不跨页，要么是上半部分跨页
  - n == 1: 下半部分跨页

```c
/* list iterators for lists of tagged pointers in TranslationBlock */
#define TB_FOR_EACH_TAGGED(head, tb, n, field)                          \
    for (n = (head) & 1, tb = (TranslationBlock *)((head) & ~1);        \
         tb; tb = (TranslationBlock *)tb->field[n], n = (uintptr_t)tb & 1, \
             tb = (TranslationBlock *)((uintptr_t)tb & ~1))

#define PAGE_FOR_EACH_TB(pagedesc, tb, n)                       \
    TB_FOR_EACH_TAGGED((pagedesc)->first_tb, tb, n, page_next)
```

当构建一个 page 的 bitmap 的时候，就可以通过 n == 1 可以快速的计算出来这个 tb 到底那些是落到 page 上的：
```c
static void build_page_bitmap(PageDesc *p) {
  int n, tb_start, tb_end;
  TranslationBlock *tb;

  assert_page_locked(p);
  p->code_bitmap = bitmap_new(TARGET_PAGE_SIZE);

  PAGE_FOR_EACH_TB(p, tb, n) {
    /* NOTE: this is subtle as a TB may span two physical pages */
    if (n == 0) {
      /* NOTE: tb_end may be after the end of the page, but
         it is not a problem */
      tb_start = tb->pc & ~TARGET_PAGE_MASK;
      tb_end = tb_start + tb->size;
      if (tb_end > TARGET_PAGE_SIZE) {
        tb_end = TARGET_PAGE_SIZE;
      }
    } else {
      tb_start = 0;
      tb_end = ((tb->pc + tb->size) & ~TARGET_PAGE_MASK);
    }
    bitmap_set(p->code_bitmap, tb_start, tb_end - tb_start);
  }
}
```

#### code_bitmap
在默认情况下，如果一个 page 只要一个写，那么就会将整个 page 关联的 TB 全部清理，实际上:
- 一般来说，反复修改代码段的情况比较少
- 如果一个 page 同时含有数据和代码，修改数据的时候可能导致 spurious 的 TB 修改

一个 PageDesc 并不会立刻创建 bitmap, 而是发现 `tb_invalidate_phys_page_fast` 多次被调用才会创建
创建 bitmap 的作用是为了精准定位出来 page 中到底是那些 位置需要 code，从而避免 spurious SMC

- PageDesc::code_write_count : 将 write 的字数保存到此处，如果 write count 超过 SMC_BITMAP_USE_THRESHOLD，那么就会构建 code_bitmap
- PageDesc::code_bitmap

#### precise smc
TARGET_HAS_PRECISE_SMC 的使用位置只有 tb_invalidate_phys_page_range__locked

为了处理当前的 tb 正好被 SMC 了

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

## TCGMemOpIdx
TCGMemOpIdx
- 0 ~ 4 : mmuid
- 5 ~   : MemOp

```c
static inline MemOp get_memop(TCGMemOpIdx oi) { return oi >> 4; }

static inline unsigned get_mmuidx(TCGMemOpIdx oi) { return oi & 15; }
```

而 MemOp 的进一步的编码如下:
```c
/*
 * MemOp in tcg/tcg.h
 *
 * [1:0] op size
 *     = 0 : MO_8
 *     = 1 : MO_16
 *     = 2 : MO_32
 *     = 3 : MO_64
 *    mask : MO_SIZE = 3 = 0b11
 *
 * [  2] sign or unsign
 *     = 1 : signed
 *     = 0 : unsigned
 *    mask : MO_SIGN = 4 = 0b100
 *
 * [  3] host reverse endian
 *   if host is big endian
 *     = 1 : MO_LE
 *     = 0 : MO_BE
 *   if host is little endian
 *     = 1 : MO_BE
 *     = 0 : MO_LE
 *
 * [6:4] aligned or unaligned
 *    = 1 : MO_ALIGN_2  = 0b001000
 *    = 2 : MO_ALIGN_4  = 0b010000
 *    = 3 : MO_ALIGN_8  = 0b011000
 *    = 4 : MO_ALIGN_16 = 0b100000
 *    = 5 : MO_ALIGN_32 = 0b101000
 *    = 6 : MO_ALIGN_64 = 0b110000
 *   mask : MO_AMASK    = 0b111000
 */
```

### mmu idx
MemTxAttrs 中主要是为了正确选择 AddressSpace, 使用 mmu idx 的原因是为了区分 kernel, user，SMAP[^1] 之类的
两者的类似指出就是都是通过 env 来构建的

```c
#define NB_MMU_MODES 3

typedef struct CPUTLB {
    CPUTLBCommon c;
    CPUTLBDesc d[NB_MMU_MODES];
    CPUTLBDescFast f[NB_MMU_MODES];
} CPUTLB;
```

- [x] 理解一下 tlb_hit 和 victim_tlb_hit
  - tlb_hit 的实现很容易，通过 cpu_mmu_index 获取 mmu_idx, 然后就可以得到对应的 TLB entry 了，然后比较即可
  - victim_tlb_hit 是一个全相连的 TLB

```c
static inline int cpu_mmu_index(CPUX86State *env, bool ifetch)
{
    return (env->hflags & HF_CPL_MASK) == 3 ? MMU_USER_IDX :
        (!(env->hflags & HF_SMAP_MASK) || (env->eflags & AC_MASK))
        ? MMU_KNOSMAP_IDX : MMU_KSMAP_IDX;
}
```

两个 flush 的接口， tlb_flush_page_by_mmuidx 和 tlb_flush_by_mmuidx 一个用于 flush 一个，一个用于 flush 全部 tlb


## x86_stl_phys_notdirty
在 target/i386/helper.c 中间的, 提供一系列的类似的 helper，但是这一个唯一一个要求 notdirty 的

```c
/* warning: addr must be aligned. The ram page is not masked as dirty
   and the code inside is not invalidated. It is useful if the dirty
   bits are used to track modified PTEs */
```
其使用位置是唯一的，在 `mmu_translate` 中的。

一般 write 是: invalidate_and_set_dirty

cpu_physical_memory_set_dirty_range 的处理:

```c
        dirty_log_mask = memory_region_get_dirty_log_mask(mr);
        dirty_log_mask &= ~(1 << DIRTY_MEMORY_CODE);
        cpu_physical_memory_set_dirty_range(memory_region_get_ram_addr(mr) + addr,
                                            4, dirty_log_mask);
```
因为不会考虑 migration 的问题，所以这个操作永远都是空的

实际上，这个只是一个普通的优化，那就是明明知道这个空间是 PTE，中间不可能放 code, 那么就没有必要去和 code 打交道了

优雅!

## WatchPoint & BreakPoint
```c
struct CPUState{
    /* ice debug support */
    QTAILQ_HEAD(, CPUBreakpoint) breakpoints;

    QTAILQ_HEAD(, CPUWatchpoint) watchpoints;
    CPUWatchpoint *watchpoint_hit;
}
```

在 tlb_set_page_with_attrs 中如果 cpu_watchpoint_address_matches, 那么该 TLB 将会插入 watchpoints，而

在 store_helper 中间，检查 TLB_WATCHPOINT, 调用 cpu_check_watchpoint

```c
/* Return flags for watchpoints that match addr + prot.  */
int cpu_watchpoint_address_matches(CPUState *cpu, vaddr addr, vaddr len)
{
    CPUWatchpoint *wp;
    int ret = 0;

    QTAILQ_FOREACH(wp, &cpu->watchpoints, entry) {
        if (watchpoint_address_matches(wp, addr, len)) {
            ret |= wp->flags;
        }
    }
    return ret;
}
```

而 breakpoints 知道一定是发生在代码段上的，所以只是需要向代码段上加上标记就可以了。

## large page
思考一下，如果使用软件支持 hugepage，那么每次比较还要比较 TLB size，这是不可能的，所以，对于 large page, QEMU softmmu 的处理就是直接不支持。
但是 large page 需要 flush 的时候，需要将这个范围的 TLB 都 flush

CPUTLBDesc 中间存在两个 field 来记录 large TLB 的范围:
- large_page_addr
- large_page_mask

- tlb_add_large_page : 当添加 large page 的时候，需要特殊处理

- tlb_flush_page_locked / tlb_flush_range_locked : 中需要特殊检查是不是因为 large page flush 导致的


## tlb flush
- tlb_flush_one_mmuidx_locked
    - tlb_mmu_resize_locked : 只有当在 TLB 发生 flush 的时候，才可以 TLB 大小的调整
    - tlb_mmu_flush_locked : flush 就是 table 清空，并且将统计数据重置

- 为什么 flush TLB 这种事情有的情况必须让这个 cpu 做:
  - TLB Update (update a CPUTLBEntry, via tlb_set_page_with_attrs) - This is a per-vCPU table - by definition can’t race - updated by its own thread when the slow-path is forced

- 或者说，如果一个 CPU A 正在运行，另外一个 CPU B 如何修改 A 的 TLB 只有一种可能，那就是 remote TLB shoot
- 之所以需要 remote TLB shoot 是因为 B 修改了 page table 所以需要通知其他的 cpu 这件事情。
stackoverflow : [Who performs the TLB shootdown](https://stackoverflow.com/questions/50256740/who-performs-the-tlb-shootdown) 这个回答正确，虽然 x86 不存在专门的 remote TLB shoot 但是一些操作可以导致这些行为。

#### sync
- tlb_flush_page_by_mmuidx
  - tlb_flush_page_by_mmuidx_async_0 : 如果是自己
  - tlb_flush_page_by_mmuidx_async_1 : 如果 mmuidx <= TARGET_PAGE_SIZE 的 bit 位数
  - tlb_flush_page_by_mmuidx_async_2 : 如果 mmuidx > TARGET_PAGE_SIZE 的 bit 位数
- tlb_flush_page_by_mmuidx_all_cpus
- tlb_flush_page_by_mmuidx_all_cpus_synced

- synced 的作用是什么?
  - 调用的函数 async_run_on_cpu / async_safe_run_on_cpu 的差别
      - qemu_work_item::exclusive 如果设置为 true 就是 safe 的
      - 这会导致 process_queued_cpu_work 执行的时候保持互斥

只是 ARM 需要 sync 版本的 flush 函数。


## Notes
- 从 tb_page_add 可以看到, TranslationBlock::page_addr 保存到是 guest 的物理页面地址

## TODO
- [ ] 找到 CPUTLBEntry::addr_read / addr_write / addr_code 比较的过程
- [ ] 说实话，tlb_add_large_page 有点没看懂

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://github.com/azru0512/slide/tree/master/QEMU
[^2]: https://qemu.weilnetz.de/w64/2012/2012-06-28/qemu-tech.html#Self_002dmodifying-code-and-translated-code-invalidation
