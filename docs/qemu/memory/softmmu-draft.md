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
