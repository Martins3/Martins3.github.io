# QEMU 的 softmmu 设计

主要是基于 tcg 分析，对于做云计算的同学应该没有什么作用。
因为 kvm 将内存虚拟化处理掉之后，softmmu 的很多复杂机制就完全消失了。

## Overview
softmmu 只有 tcg 才需要，实现基本思路是:
- 所有的访存指令前使用软件进行地址翻译，如果命中，那么获取 GPA 进行访存
- 如果不命中，慢路径，也就是 store_helper

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

CPUNegativeOffsetState : include/exec/cpu-defs.h
- [ ] 为什么 neg 一定需要放到 CPUArchState 前面

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

- [ ] attrs 的作用是什么，为什么是放到 IOTLB 中，对于 RAM 有意义吗?

- [ ] 什么叫做 physical section number ?
  - 一共出现三次，第三次是 phys_section_add
- [ ] PHYS_SECTION_NOTDIRTY
- [ ] PHYS_SECTION_ROM


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

## access size
如果一次访问同时横跨了两个 MemoryRegion 怎么办?

address_space_translate_internal 中的注释解释很不错:
- 进行 address_space_translate_internal 的一个参数 plen 其实是用于返回实际上可以访问的范围
- MMIO 访问的时候会在 memory_access_size 中调整访问的大小为最大为 4，而且 MMIO 很少出现 MemoryRegion 的
- MMIO 的 MemoryRegion 有时候会出现完全重合的情况，所以不能因为一个访问的延伸到了另外一个
- flatview_read_continue / flatview_write_continue 中会循环调用 flatview_translate memory_access_size memory_region_dispatch_write，从而可以保证即使是访问跨 MemoryRegion 的，其 MemoryRegionOps 也是自动变化的

```c
    /* MMIO registers can be expected to perform full-width accesses based only
     * on their address, without considering adjacent registers that could
     * decode to completely different MemoryRegions.  When such registers
     * exist (e.g. I/O ports 0xcf8 and 0xcf9 on most PC chipsets), MMIO
     * regions overlap wildly.  For this reason we cannot clamp the accesses
     * here.
     *
     * If the length is small (as is the case for address_space_ldl/stl),
     * everything works fine.  If the incoming length is large, however,
     * the caller really has to do the clamping through memory_access_size.
     */
```

补充若干内容:
1. 重合的 MMIO
```txt
0000000000000cf8-0000000000000cfb (prio 0, i/o): pci-conf-idx
0000000000000cf9-0000000000000cf9 (prio 1, i/o): piix3-reset-control
0000000000000cfc-0000000000000cff (prio 0, i/o): pci-conf-data
```
flatview 的样子就非常有趣了。
```txt
0000000000000cf8-0000000000000cf8 (prio 0, i/o): pci-conf-idx
0000000000000cf9-0000000000000cf9 (prio 1, i/o): piix3-reset-control
0000000000000cfa-0000000000000cfb (prio 0, i/o): pci-conf-idx @0000000000000002
```

2. MMIO 的 access size 除了通过 memory_access_size 将 access_size 限制为 4 以内，而且在 access_with_adjusted_size 也进行了处理，不过目的不在于处理 cross MemoryRegion，单纯的为了保证访问的时候 size 为 4
- flatview_read_continue
  - memory_access_size : 将访问的大小调整为最多为 4，比如 hpet 的
  - memory_region_dispatch_read

- io_writex
    - memory_region_dispatch_write
        - access_with_adjusted_size : 将一次 access 的 size 压缩为 1 ~ 4 之间，然后多次调用 access_fn，比如 vga_mem_write

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

## Notes
- 从 tb_page_add 可以看到, TranslationBlock::page_addr 保存到是 guest 的物理页面地址

## TODO
- [ ] 找到 CPUTLBEntry::addr_read / addr_write / addr_code 比较的过程

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://github.com/azru0512/slide/tree/master/QEMU
[^2]: https://qemu.weilnetz.de/w64/2012/2012-06-28/qemu-tech.html#Self_002dmodifying-code-and-translated-code-invalidation
