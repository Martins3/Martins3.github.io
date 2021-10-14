# qemu softmmu 设计

## large page
思考一下，如果使用软件支持 hugepage，那么每次比较还要比较 TLB size，这是不可能的，所以，对于 large page, QEMU softmmu 的处理就是直接不支持。
但是 large page 需要 flush 的时候，需要将这个范围的 TLB 都 flush

CPUTLBDesc 中间存在两个 field 来记录 large TLB 的范围:
- large_page_addr
- large_page_mask

- tlb_add_large_page : 当添加 large page 的时候，需要特殊处理

- tlb_flush_page_locked / tlb_flush_range_locked : 中需要特殊检查是不是因为 large page flush 导致的

- [ ] 说实话，tlb_add_large_page 有点没看懂

## dirty page
- Dirty page tracking (for code gen, SMC detection, migration and display)

dirty_memory 划分为三种
```c
#define DIRTY_MEMORY_VGA       0
#define DIRTY_MEMORY_CODE      1
#define DIRTY_MEMORY_MIGRATION 2
#define DIRTY_MEMORY_NUM       3        /* num of dirty bits */
```
因为三种需求:
- smc
- migration
- vga

区分一下一些 dirty
1. guest os 的 kernel 的 dirty 表示，内存被修改没有被同步到文件系统中间了
2. QEMU 的 dirty 表示，这个当前虚拟机的内存被修改没有同步到远程的虚拟机中

- 请问，tcg 为什么需要 dirty ?
    - 对于 guest 的代码段，标记上 TLB_NOTDIRTY
    - 遇到了，invalid 掉对应的 tb
    - tcg 需要 dirty 的主要原因是 ： 可以防止反复 invalid 的, 因为 data 和 代码也许会被误判。
- 那么, display 为什么需要 dirty ?

关联的主要函数以及他们的调用者:
- tlb_set_dirty
- tlb_reset_dirty
- notdirty_write
   * atomic_mmu_lookup : 这是整个 atomic 机制调用的地方
   * probe_access / probe_access_flags : x86 guest 从来没有调用过，这个不是我能理解的
   * store_helper

- notdirty_write 的作用:
  - 在 store_helper 中，会处理 TLB 插入特殊 flag 的情况，例如插入 TLB_WATCHPOINT 就需要考虑 watchpoint 的，还有 TLB_MMIO, 当这个 RAM 被 TLB_NOTDIRTY 保护, 就需要 notdirty_write 特殊处理
  - cpu_physical_memory_get_dirty : 获取该位置是否发生为 dirty

- 在 cpu_physical_memory_set_dirty_lebitmap 中间, 如果没有打开 global_dirty_log 那么 client 就不会添加上 DIRTY_MEMORY_MIGRATION
- 在 memory_region_get_dirty_log_mask 中对于 DIRTY_MEMORY_CODE 和 DIRTY_MEMORY_MIGRATION 也是存在类似的特殊处理

- colo_incoming_start_dirty_log : https://wiki.qemu.org/Features/COLO
  - ramblock_sync_dirty_bitmap
    - cpu_physical_memory_sync_dirty_bitmap


- 在正常的 kvm 其中的操作过程中，cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty 都不会被调用
- 在 cpu_physical_memory_test_and_clear_dirty 作为 SMC 的基础设施, 这是为数不多的需要支持的接口

```c
/*
#0  cpu_physical_memory_set_dirty_range (start=952888, length=4, mask=5 '\005') at /home/maritns3/core/kvmqemu/include/exec/ram_addr.h:290
#1  0x0000555555c6d24f in notdirty_write (cpu=0x555556d5fc00, mem_vaddr=952888, size=4, iotlbentry=0x7ffdcc01db90, retaddr=140734602805726) at ../accel/tcg/cputlb.c:1561
#2  0x0000555555c6f601 in store_helper (env=0x555556d68480, addr=952888, val=3221225472, oi=34, retaddr=140734602805726, op=MO_32) at ../accel/tcg/cputlb.c:2451
#3  0x0000555555c6f815 in helper_le_stl_mmu (env=0x555556d68480, addr=952888, val=3221225472, oi=34, retaddr=140734602805726) at ../accel/tcg/cputlb.c:2505
#4  0x00007fff540201de in code_gen_buffer ()
#5  0x0000555555cb0254 in cpu_tb_exec (cpu=0x555556d5fc00, itb=0x7fff940200c0, tb_exit=0x7fffe888e1b0) at ../accel/tcg/cpu-exec.c:190
#6  0x0000555555cb11d8 in cpu_loop_exec_tb (cpu=0x555556d5fc00, tb=0x7fff940200c0, last_tb=0x7fffe888e1b8, tb_exit=0x7fffe888e1b0) at ../accel/tcg/cpu-exec.c:673
#7  0x0000555555cb14c9 in cpu_exec (cpu=0x555556d5fc00) at ../accel/tcg/cpu-exec.c:798
#8  0x0000555555d60555 in tcg_cpus_exec (cpu=0x555556d5fc00) at ../accel/tcg/tcg-accel-ops.c:67
#9  0x0000555555c496c3 in mttcg_cpu_thread_fn (arg=0x555556d5fc00) at ../accel/tcg/tcg-accel-ops-mttcg.c:70
#10 0x0000555555f53fcd in qemu_thread_start (args=0x555556c6dd10) at ../util/qemu-thread-posix.c:521
#11 0x00007ffff6298609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#12 0x00007ffff61bd293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

#### migration

QEMU 记录的 dirty page 已经发送到只剩下最后的 max_size 的时候，调用 migration_bitmap_sync 进行 dirty page 同步，
该函数最终会调用到 ioctl(KVM_GET_DIRTY_LOG) 上，将 dirty page 记录到 ram_list.dirty_memory 中。
- migration_bitmap_sync
  - memory_global_dirty_log_sync
    - memory_region_sync_dirty_bitmap
      - MemoryListener::log_sync
         - kvm_log_sync
            - kvm_physical_sync_dirty_bitmap
              - kvm_slot_get_dirty_log  : 使用 KVM_GET_DIRTY_LOG ioctl
              - kvm_slot_sync_dirty_pages
                - cpu_physical_memory_set_dirty_lebitmap : 这个将从 kvm 中得到的 dirty map 的信息放到 ram_list.dirty_memory
      - MemoryListener::log_sync_global
  - ramblock_sync_dirty_bitmap
    - cpu_physical_memory_sync_dirty_bitmap

总结一下，为了让 dirty log 机制是放到一起的，memory_global_dirty_log_sync 对于 tcg 是一个空函数，实际上，tcg 通过 cpu_physical_memory_sync_dirty_bitmap 将 dirty log 直接可以放到 ramlist.dirty_memory 上

#### VGA 需要 dirty tracking
使用 dirty memory 可以记录只是发生改变的地方。

- vga 注册的 hook : vga_mem_write
  - 使用 memory_region_set_dirty 来调用设置 memory dirty 的操作

- 当 vga_draw_graphic 的时候，其可以 memory_region_snapshot_and_clear_dirty

至于为什么和 migration dirty memory 分开，猜测主要是因为其中的

#### kvm ring size


```c
    if (s->kvm_dirty_ring_size) {
        kml->listener.log_sync_global = kvm_log_sync_global;
    } else {
        kml->listener.log_sync = kvm_log_sync;
        kml->listener.log_clear = kvm_log_clear;
    }
```
这些 global 接口都是 Peter Xu 在 2020 添加的, 可以参考
https://www.youtube.com/watch?v=YsQJ-Vll3sg

#### RAMList
```c
/* The dirty memory bitmap is split into fixed-size blocks to allow growth
 * under RCU.  The bitmap for a block can be accessed as follows:
 *
 *   rcu_read_lock();
 *
 *   DirtyMemoryBlocks *blocks =
 *       qatomic_rcu_read(&ram_list.dirty_memory[DIRTY_MEMORY_MIGRATION]);
 *
 *   ram_addr_t idx = (addr >> TARGET_PAGE_BITS) / DIRTY_MEMORY_BLOCK_SIZE;
 *   unsigned long *block = blocks.blocks[idx];
 *   ...access block bitmap...
 *
 *   rcu_read_unlock();
 *
 * Remember to check for the end of the block when accessing a range of
 * addresses.  Move on to the next block if you reach the end.
 *
 * Organization into blocks allows dirty memory to grow (but not shrink) under
 * RCU.  When adding new RAMBlocks requires the dirty memory to grow, a new
 * DirtyMemoryBlocks array is allocated with pointers to existing blocks kept
 * the same.  Other threads can safely access existing blocks while dirty
 * memory is being grown.  When no threads are using the old DirtyMemoryBlocks
 * anymore it is freed by RCU (but the underlying blocks stay because they are
 * pointed to from the new DirtyMemoryBlocks).
 */
#define DIRTY_MEMORY_BLOCK_SIZE ((ram_addr_t)256 * 1024 * 8)
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

RAMList::blocks 等分配在 dirty_memory_extend 函数中进行

- qemu_ram_alloc
  - qemu_ram_alloc_internal
    - ram_block_add
      - dirty_memory_extend : 应该是唯一初始化 ram_list.dirty_memory 的位置吧, 另外使用的位置在 cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty

- RAMList::blocks 中间自然是存储所有的 block 但是排序是按照 block 的大小的

所以 RAMList::dirty_memory 是如何索引的?
- 在 tlb_set_page_with_attrs 中，当 mr 是 RAM 的时候，iotlbentry::addr 存放 ram_addr
```c
iotlb = memory_region_get_ram_addr(section->mr) + xlat;
```
- 在 notdirty_write 中，获取到 ram_addr , 由此实现记录 dirty memory


## tlb flush
之后采用 HAMT 之后，这些逻辑会发生改变，但是目前还是如此了。

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

#### remote tlb shoot
很多时候，需要将 remote 的 TLB 清理掉，但是 remote 的 cpu 还在运行，所以必须确定了 remote cpu 不会使用
TLB 才可以返回。

- [ ] async_run_on_cpu : 首先将代码实现出来
  - qemu_cpu_kick
    - cpu_exit : 如果是 qemu_tcg_mttcg_enabled 那么就对于所有的 cpu 进行 cpu_exit
      - `atomic_set(&cpu_neg(cpu)->icount_decr.u16.high, -1);` : 猜测这个会导致接下来 tb 执行退出 ?
        - [ ] icount_decr 只是在 TB 开始的位置检查，怎么办 ? (tr_gen_tb_start)

## CPUNegativeOffsetState
include/exec/cpu-defs.h

- [ ] 为什么 neg 一定需要放到 CPUArchState 前面
```c
/*
 * This structure must be placed in ArchCPU immediately
 * before CPUArchState, as a field named "neg".
 */
typedef struct CPUNegativeOffsetState {
    CPUTLB tlb;
    IcountDecr icount_decr;
} CPUNegativeOffsetState;

struct X86CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUX86State env;
    // ...
}
```

## CPUTLB
```c
/*
 * The entire softmmu tlb, for all MMU modes.
 * The meaning of each of the MMU modes is defined in the target code.
 * Since this is placed within CPUNegativeOffsetState, the smallest
 * negative offsets are at the end of the struct.
 */

typedef struct CPUTLB {
    CPUTLBCommon c;
    CPUTLBDesc d[NB_MMU_MODES];
    CPUTLBDescFast f[NB_MMU_MODES];
} CPUTLB;
```

- CPUTLB
  - CPUTLBCommon : 统计数据
  - CPUTLBDesc :
    - victim tlb
    - large page
    - iotlb
  - CPUTLBDescFast : 通过 `tlb_entry` 实现访问，这就是常规的 TLB
    - mask
    - table : 需要的 page table

> IOTLB 的表项和上面的 CPUTLBEntry 分开也是一个需要理解的点。定义里有一些注释，要结合代码才会完全理解。一个 IO 访问，它的地址匹配依然是通过 CPUTLBEntry 的地址来完成，但是由于 IO 访问时 CPUTLBEntry 相关地址的低位不为 0（如果它已经被填充了的话），所以地址不会匹配成功，访存代码会走 slow path。

### CPUTLBCommon
```c
/*
 * Data elements that are shared between all MMU modes.
 */
typedef struct CPUTLBCommon {
    /* Serialize updates to f.table and d.vtable, and others as noted. */
    QemuSpin lock;
    /*
     * Within dirty, for each bit N, modifications have been made to
     * mmu_idx N since the last time that mmu_idx was flushed.
     * Protected by tlb_c.lock.
     */
    uint16_t dirty;
    /*
     * Statistics.  These are not lock protected, but are read and
     * written atomically.  This allows the monitor to print a snapshot
     * of the stats without interfering with the cpu.
     */
    size_t full_flush_count;
    size_t part_flush_count;
    size_t elide_flush_count;
} CPUTLBCommon;
```

### CPUTLBDescFast

```c
/*
 * Data elements that are per MMU mode, accessed by the fast path.
 * The structure is aligned to aid loading the pair with one insn.
 */
typedef struct CPUTLBDescFast {
    /* Contains (n_entries - 1) << CPU_TLB_ENTRY_BITS */
    uintptr_t mask;
    /* The array of tlb entries itself. */
    CPUTLBEntry *table;
} CPUTLBDescFast QEMU_ALIGNED(2 * sizeof(void *));
```
- 分析 victim_tlb_hit 的实现, 在同时替换 CPUTLBEntry 和 CPUIOTLBEntry 中的 victim

- tlb_index / tlb_entry 都是使用的 fast
- tlb_set_page_with_attrs 首先构建 CPUTLBEntry 然后使用 tlb_entry 获取地址，最后写入

### CPUTLBDesc
```c
/*
 * Data elements that are per MMU mode, minus the bits accessed by
 * the TCG fast path.
 */
typedef struct CPUTLBDesc {
    /*
     * Describe a region covering all of the large pages allocated
     * into the tlb.  When any page within this region is flushed,
     * we must flush the entire tlb.  The region is matched if
     * (addr & large_page_mask) == large_page_addr.
     */
    target_ulong large_page_addr;
    target_ulong large_page_mask;
    /* host time (in ns) at the beginning of the time window */
    int64_t window_begin_ns;
    /* maximum number of entries observed in the window */
    size_t window_max_entries;
    size_t n_used_entries;
    /* The next index to use in the tlb victim table.  */
    size_t vindex;
    /* The tlb victim table, in two parts.  */
    CPUTLBEntry vtable[CPU_VTLB_SIZE];
    CPUIOTLBEntry viotlb[CPU_VTLB_SIZE];
    /* The iotlb.  */
    CPUIOTLBEntry *iotlb;
} CPUTLBDesc;
```
#### CPUTLBEntry
```c
typedef struct CPUTLBEntry {
    /* bit TARGET_LONG_BITS to TARGET_PAGE_BITS : virtual address
       bit TARGET_PAGE_BITS-1..4  : Nonzero for accesses that should not
                                    go directly to ram.
       bit 3                      : indicates that the entry is invalid
       bit 2..0                   : zero
    */
    union {
        struct {
            target_ulong addr_read;
            target_ulong addr_write;
            target_ulong addr_code;
            /* Addend to virtual address to get host address.  IO accesses
               use the corresponding iotlb value.  */
            uintptr_t addend;
        };
        /* padding to get a power of two size */
        uint8_t dummy[1 << CPU_TLB_ENTRY_BITS];
    };
} CPUTLBEntry;
```

- addr_read / addr_write / addr_code 都是虚拟地址的，是通过 addend 来计算的
- 在 tlb_set_page_with_attrs 中，如果是 rom, 那么通过 addened 就会得到 hpa, 否则得到是 0
  - 实际上，对于 IO 空间，因为在 addr_read / addr_write / addr_code 中比较必然失败，所以这个 addened 一一不大


这是为了在生成 TLB 对比的指令中消除掉其中的关于权限对比的部分。

分析一手 addr_read / addr_write / addr_code 的调用位置:
1. 三者都是仅仅出现在 cputlb.c 中间
2. tlb_reset_dirty_range_locked : 产生一个很有意思的问题，那就是通过设置 addr_write 表示的确可写，通过设置 TLB_NOTDIRTY 实现写保护。这样做的好处是，可以区分一个页面到底是真的不可写还是因为模拟的原因不可写。
  - 如果使用上 hardware TLB，其实可以这么处理: 如果想要给 TLB 设置上 addr_write，那么就写权限去掉，当因为 write 失败，可以捕获下这个异常，然后查询 x86 page table 来看，到底是因为 SMC 的原因还是因为 guest 本身的不可写
3. tlb_set_dirty1_locked

- [ ] 找到和 addr_read / addr_write / addr_code 比较的过程

#### CPUIOTLBEntry
```c
/* The IOTLB is not accessed directly inline by generated TCG code,
 * so the CPUIOTLBEntry layout is not as critical as that of the
 * CPUTLBEntry. (This is also why we don't want to combine the two
 * structs into one.)
 */
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
从 memory_region 的
- 如果不是 RAM :  TARGET_PAGE_BITS 内可以放 section number, 其他的位置放 mr 内偏移
- 如果是 RAM : 就是 ram addr 了

使用者:
- probe_access
  - tlb_hit / victim_tlb_hit / tlb_fill : TLB 访问经典三件套
  - cpu_check_watchpoint
  - notdirty_write : 和 cpu_check_watchpoint 相同，需要 iotlbentry 作为参数
- io_writex / io_readx
  - iotlb_to_section
  - `mr_offset = (iotlbentry->addr & TARGET_PAGE_MASK) + addr;`

在 exec 中间，维护了 MemoryRegionSection (具体参考 iotlb_to_section)，这让 iotlbentry 可以找到 IO 命中的 memory_region
从而知道这个 memory_region 的对应的处理函数是什么。

插入:
  tlb_set_page_with_attrs

- [ ] attrs 的作用是什么，为什么是放到 IOTLB 中，对于 RAM 有意义吗?

- [ ] 什么叫做 physical section number ?
  - 一共出现三次，第三次是 phys_section_add
- [ ] PHYS_SECTION_NOTDIRTY
- [ ] PHYS_SECTION_ROM

## How softmmu works
Q: 其实，访问存储也是隐藏的 load，是如何被 softmmu 处理的?

A: 指令的读取都是 tb 的事情

- gen_ldst_softmmu_helper
  - `__gen_ldst_softmmu_helper_native`
    - tr_gen_lookup_qemu_tlb : TLB 比较查询
    - tr_gen_ldst_slow_path : 无奈，只能跳转到 slow path 去
      - td_rcd_softmmu_slow_path

- tr_ir2_generate
  - tr_gen_softmmu_slow_path
    - `__tr_gen_softmmu_sp_rcd`
      - helper_ret_stb_mmu : 跳转的入口通过 helper_ret_stb_mmu 实现, 当前在 accel/tcg/cputlb.c 中
        - store_helper
          - io_writex
            - memory_region_dispatch_write

## code flow
- store_helper
  - tlb_fill
    - x86_cpu_tlb_fill
      - handle_mmu_fault : 利用 x86 的页面进行 page walk
        - tlb_set_page_with_attrs : 设置页面

- [ ] 所以现在的 hamt 设计，在 tlb_fill 在 tlb refill 的位置进行修改，tlb_set_page_with_attrs 修改为真正的 tlbwr 之类的东西

下面是 tlb_fill 的几个调用者
- get_page_addr_code : 从 guest 虚拟地址的 pc 获取 guest 物理地址的 pc
  - get_page_addr_code_hostp
    - qemu_ram_addr_from_host_nofail : 通过 hva 获取 gpa
      - qemu_ram_addr_from_host
        - qemu_ram_block_from_host

## MemTxAttrs
在 `x86_*_phys` 和 helper_outb 都是通过 cpu_get_mem_attrs 来构建参数 MemTxAttrs

从目前看，MemTxAttrs 的主要作用是为了 SMM 模式，完全可以简化。

```c
static inline int x86_asidx_from_attrs(CPUState *cs, MemTxAttrs attrs)
{
    return !!attrs.secure;
}
```

- [ ] requester_id 也是使用的使用 MemTxAttrs 的位置，但是注意，对于这个数值，似乎只有赋值，没有读取，研究一下。
```c
void msi_send_message(PCIDevice *dev, MSIMessage msg)
{
    MemTxAttrs attrs = {};

    attrs.requester_id = pci_requester_id(dev);
    address_space_stl_le(&dev->bus_master_as, msg.address, msg.data,
                         attrs, NULL);
}
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

## WatchPoint 和 BreakPoint 实现
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

## tlb_set_page_with_attrs
- tlb_set_page_with_attrs 的功能就是添加一个 LTB entry 的
  - address_space_translate_for_iotlb : 因为没有 IOMMU 的支持，所以等价于调用 address_space_translate_internal

其实不区分是不是 io 还是 ram 的，都会存储到 CPUTLBDesc::iotlb 和 CPUTLBDescFast::table 中间

从 victim_tlb_hit 的实现看，也就是 victim / fast 的 tlb 的相同位置,
总是同时有相同地址的 iotlb 和 tlb

- [ ] 现在理解了其中的基本函数如何设置 CPUIOTLBEntry 和 CPUTLBEntry 的，但是中间还空出来了一大片内容的

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

## softmmu 快慢路径
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

#### fast path

#### slot path
- tr_ir2_generate
  - tr_gen_tb_start
  - tr_gen_softmmu_slow_path : slow path 的代码在每一个 tb 哪里只会生成一次
  - tr_gen_tb_end
