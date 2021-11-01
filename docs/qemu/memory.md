# QEMU 的 memory model

<!-- vim-markdown-toc GitLab -->

- [Overview](#overview)
- [AddressSpace](#addressspace)
  - [memory_region_get_flatview_root](#memory_region_get_flatview_root)
- [MemoryRegion](#memoryregion)
- [FlatView](#flatview)
- [AddressSpaceDispatch](#addressspacedispatch)
- [RAMBlock](#ramblock)
- [ram addr](#ram-addr)
- [subpage](#subpage)
- [MemoryListener](#memorylistener)
- [CPUAddressSpace](#cpuaddressspace)
- [memory access code flow](#memory-access-code-flow)
- [alias](#alias)
- [memory listener](#memory-listener)
    - [memory listener tcg](#memory-listener-tcg)
    - [memory listener hook 的调用位置](#memory-listener-hook-的调用位置)
    - [ioeventfd](#ioeventfd)
    - [kvm memory listener hook](#kvm-memory-listener-hook)
- [IOMMU](#iommu)
- [PCI Device AddressSpace](#pci-device-addressspace)
    - [PCIIORegion](#pciioregion)
- [dma](#dma)
- [dirty page tracking](#dirty-page-tracking)
    - [migration](#migration)
- [Option ROM](#option-rom)
- [access size](#access-size)
- [address_space_map](#address_space_map)
- [sysbus_init_mmio](#sysbus_init_mmio)
- [Appendix](#appendix)

<!-- vim-markdown-toc -->

## Overview
原图来自于 kernelgo.org, 这里进行一些小小的修改, 在浏览器中打开新的标签可以看大图。
![](../img/qemu-address-space.svg)

首先感受一下 memory model 是什么, 在 QEMU 的 monitor console 中 `info mtree` 可以下面是一个例子，guest 机器的配置使用[这个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/hack/qemu/x64-e1000/alpine.sh)生成的。

```txt
 ┌──────────► 这是一个 AddressSpace，AddressSpace 用于描述整个地址空间的映射关系。
 │                                        ┌───────────────────────► MemoryRegion 的优先级，如果一个范围两个 MemoryRegion 出现重叠，优先级高的压制优先级低的
 │                                        │
 │                                        │   ┌───────────────────► 表示这个空间的类型，一般划分为 io 和 RAM
 │                                        │   │    ┌──────────────► 这是一个 MemoryRegion，这是 Address Space 中最核心的概念，MemoryRegion 用于描述一个范围内的映射规则
address-space: memory                     │   │    │
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-00000000bfffffff (prio 0, ram): alias ram-below-4g @pc.ram 0000000000000000-00000000bfffffff ──────────────┐
    0000000000000000-ffffffffffffffff (prio -1, i/o): pci                                                                       │
      00000000000a0000-00000000000bffff (prio 1, i/o): vga-lowmem                                                               │
      00000000000c0000-00000000000dffff (prio 1, rom): pc.rom                                                                   │
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff                │
      00000000fd000000-00000000fdffffff (prio 1, ram): vga.vram                                                                 │
      00000000fe000000-00000000fe003fff (prio 1, i/o): virtio-pci                                                               │
        00000000fe000000-00000000fe000fff (prio 0, i/o): virtio-pci-common-virtio-9p                                            │
        00000000fe001000-00000000fe001fff (prio 0, i/o): virtio-pci-isr-virtio-9p                                               │
        00000000fe002000-00000000fe002fff (prio 0, i/o): virtio-pci-device-virtio-9p                                            │
        00000000fe003000-00000000fe003fff (prio 0, i/o): virtio-pci-notify-virtio-9p                                            │
      00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio                                                               │
      00000000febf0000-00000000febf3fff (prio 1, i/o): nvme-bar0                                                                │
        00000000febf0000-00000000febf1fff (prio 0, i/o): nvme                                                                   │
        00000000febf2000-00000000febf240f (prio 0, i/o): msix-table                                                             │
        00000000febf3000-00000000febf300f (prio 0, i/o): msix-pba                                                               │
      00000000febf4000-00000000febf7fff (prio 1, i/o): nvme-bar0                                                                │
        00000000febf4000-00000000febf5fff (prio 0, i/o): nvme                                                                   │
        00000000febf6000-00000000febf640f (prio 0, i/o): msix-table                                                             │
        00000000febf7000-00000000febf700f (prio 0, i/o): msix-pba                                                               │
      00000000febf8000-00000000febf8fff (prio 1, i/o): vga.mmio                                                                 │
        00000000febf8000-00000000febf817f (prio 0, i/o): edid                                                                   └── ram-below-4g 是 pc.ram 的一个 alias
        00000000febf8400-00000000febf841f (prio 0, i/o): vga ioports remapped
        00000000febf8500-00000000febf8515 (prio 0, i/o): bochs dispi interface                                                  ┌── ram-above-4g 也是 pc.ram 的一个 alias, 两者都被放到 system 这个 MemoryRegion 上
        00000000febf8600-00000000febf8607 (prio 0, i/o): qemu extended regs                                                     │
      00000000febf9000-00000000febf9fff (prio 1, i/o): virtio-9p-pci-msix                                                       │
        00000000febf9000-00000000febf901f (prio 0, i/o): msix-table                                                             │
        00000000febf9800-00000000febf9807 (prio 0, i/o): msix-pba                                                               │
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios                                                                  │
    00000000000a0000-00000000000bffff (prio 1, i/o): alias smram-region @pci 00000000000a0000-00000000000bffff                  │
    00000000000c0000-00000000000c3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000c0000-00000000000c3fff                    │
    00000000000c4000-00000000000c7fff (prio 1, ram): alias pam-rom @pc.ram 00000000000c4000-00000000000c7fff                    │
    00000000000c8000-00000000000cbfff (prio 1, ram): alias pam-rom @pc.ram 00000000000c8000-00000000000cbfff                    │
    00000000000cb000-00000000000cdfff (prio 1000, ram): alias kvmvapic-rom @pc.ram 00000000000cb000-00000000000cdfff            │
    00000000000cc000-00000000000cffff (prio 1, ram): alias pam-rom @pc.ram 00000000000cc000-00000000000cffff                    │
    00000000000d0000-00000000000d3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000d0000-00000000000d3fff                    │
    00000000000d4000-00000000000d7fff (prio 1, ram): alias pam-rom @pc.ram 00000000000d4000-00000000000d7fff                    │
    00000000000d8000-00000000000dbfff (prio 1, ram): alias pam-rom @pc.ram 00000000000d8000-00000000000dbfff                    │
    00000000000dc000-00000000000dffff (prio 1, ram): alias pam-rom @pc.ram 00000000000dc000-00000000000dffff                    │
    00000000000e0000-00000000000e3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000e0000-00000000000e3fff                    │
    00000000000e4000-00000000000e7fff (prio 1, ram): alias pam-ram @pc.ram 00000000000e4000-00000000000e7fff                    │
    00000000000e8000-00000000000ebfff (prio 1, ram): alias pam-ram @pc.ram 00000000000e8000-00000000000ebfff                    │
    00000000000ec000-00000000000effff (prio 1, ram): alias pam-ram @pc.ram 00000000000ec000-00000000000effff                    │
    00000000000f0000-00000000000fffff (prio 1, ram): alias pam-rom @pc.ram 00000000000f0000-00000000000fffff                    │
    00000000fec00000-00000000fec00fff (prio 0, i/o): ioapic                                                                     │
    00000000fed00000-00000000fed003ff (prio 0, i/o): hpet                                                                       │
    00000000fee00000-00000000feefffff (prio 4096, i/o): apic-msi                                                                │
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff ──────────────┘
```
完整的在这里[这里](https://martins3.github.io/qemu/memory/memory-model-tcg.txt)
看上去非常的长，实际上，cpu-memory-0, system, pci 这三个 MemoryRegion 逐个包含，pci 的这个 MemoryRegion 在其中打印了多次。

总体来说，MemoryRegion 用于描述一个范围内的映射规则, AddressSpace 用于描述整个地址空间的映射关系。有了映射关系，当 guest 访问到这些地址的时候，就可以知道具体应该进行什么操作了。

下面是 kvm 处理 io 端口的操作的一个调用流程图，只要给出 AddressSpace 以及 地址，最后就可以找到最后的 handler 为 `kbd_read_data`

```c
/*
#2  kbd_read_data (opaque=0x555556844d98, addr=<optimized out>, size=<optimized out>) at ../hw/input/pckbd.c:387
#3  0x0000555555cd2092 in memory_region_read_accessor (mr=mr@entry=0x555556844df0, addr=0, value=value@entry=0x7fffd9ff9130, size=size@entry=1, shift=0, mask=mask@entry=255, attrs=...) at ../softmmu/memory.c:440
#4  0x0000555555cceb1e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffd9ff9130, size=size@entry=1, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555cd2050 <memory_region_read_accessor>, mr=0x555556844df0, attrs=...) at ../softmmu/memory.c:554
#5  0x0000555555cd1ac1 in memory_region_dispatch_read1 (attrs=..., size=<optimized out>, pval=0x7fffd9ff9130, addr=0, mr=0x555556844df0) at ../softmmu/memory.c:1424
#6  memory_region_dispatch_read (mr=mr@entry=0x555556844df0, addr=0, pval=pval@entry=0x7fffd9ff9130, op=MO_8, attrs=attrs@entry=...) at ../softmmu/memory.c:1452
#7  0x0000555555c9eb89 in flatview_read_continue (fv=fv@entry=0x7ffe4402d230, addr=addr@entry=96, attrs=..., ptr=ptr@entry=0x7fffeb17d000, len=len@entry=1, addr1=<optimized out>, l=<optimized out>, mr=0x555556844df0) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:165
#8  0x0000555555c9ed43 in flatview_read (fv=0x7ffe4402d230, addr=addr@entry=96, attrs=attrs@entry=..., buf=buf@entry=0x7fffeb17d000, len=len@entry=1) at ../softmmu/physmem.c:2881
#9  0x0000555555c9ee96 in address_space_read_full (as=0x555556606880 <address_space_io>, addr=96, attrs=..., buf=0x7fffeb17d000, len=1) at ../softmmu/physmem.c:2894
#10 0x0000555555c9f015 in address_space_rw (as=<optimized out>, addr=addr@entry=96, attrs=..., attrs@entry=..., buf=<optimized out>, len=len@entry=1, is_write=is_write@entry=false) at ../softmmu/physmem.c:2922
#11 0x0000555555c8ece9 in kvm_handle_io (count=1, size=1, direction=<optimized out>, data=<optimized out>, attrs=..., port=96) at ../accel/kvm/kvm-all.c:2635
#12 kvm_cpu_exec (cpu=cpu@entry=0x555556af4410) at ../accel/kvm/kvm-all.c:2886
#13 0x0000555555cf1825 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556af4410) at ../accel/kvm/kvm-accel-ops.c:49
#14 0x0000555555e55983 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:541
#15 0x00007ffff628d609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#16 0x00007ffff61b4293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
*/
```

但是这里有一个问题，这些 MemoryRegion 逐层嵌套，如果不做简化处理，为了确定一个地址到底落到了哪一个 MemoryRegion，每次都需要
从顶层 MemoryRegion 逐个找其 child MemoryRegion，其中还需要处理 alias 和 priority 的问题，而且到底命中哪一个 MemoryRegion 需要这个比较范围。
为此，QEMU 在每次 MemoryRegion 的属性发生修改的时候都会进行两个事情:
- 将 MemoryRegion 压平为 FlatRange，避免逐级查询 MemoryRegion
- 将 FlatRange 变为树的查询，将查询从 O(n) 的查询修改为 O(log(N))

## AddressSpace
AddressSpace 用于描述整个地址空间的映射关系, 不同的地址空间的映射关系不同。guest 写相同的地址，在 io 的空间和 memory 空间的效果不同的。
在 kvm 模式下，主要是两个 AddressSpace，一个是 `address_space_memory` 另一个是 `address_space_io`, 定义在 `softmmu/physmem.c` 中间。

```c
        case KVM_EXIT_IO:
            DPRINTF("handle_io\n");
            /* Called outside BQL */
            kvm_handle_io(run->io.port, attrs,
                          (uint8_t *)run + run->io.data_offset,
                          run->io.direction,
                          run->io.size,
                          run->io.count);
            ret = 0;
            break;
        case KVM_EXIT_MMIO:
            DPRINTF("handle_mmio\n");
            /* Called outside BQL */
            address_space_rw(&address_space_memory,
                             run->mmio.phys_addr, attrs,
                             run->mmio.data,
                             run->mmio.len,
                             run->mmio.is_write);
```


```c
static MemoryRegion *system_memory;
static MemoryRegion *system_io;

AddressSpace address_space_io;
AddressSpace address_space_memory;
```
在 memory_map_init 中, AddressSpace
`address_space_memory` 和 `address_space_io` 分别关联 `system_memory` 和 `system_io` 这两个 MemoryRegion,
而之后的各种初始化都是同个将初始化 MemoryRegion 添加到这两个 MemoryRegion 上的。

通过 MemoryRegion 可以找到该 MemoryRegion 下的 Flatview
一个 AddressSpace 只会关联一个 MemoryRegion, 通过 address_space_set_flatview 缓存到 AddressSpace::current_map 上。

因为只有顶层的 MemoryRegion 才需要持有的 Flatview 的，所以没有必要在 MemoryRegion 中添加一个属性，QEMU 将 MemoryRegion 持有的 Flatview 保存在 `static GHashTable *flat_views;`

每次增删改 MemoryRegion 之后，都会调用，
memory_region_transaction_commit 进而调用 address_space_set_flatview
保证 AddressSpace::current_map 的 Flatview 和 MemoryRegion 总是同步的。

在 memory_region_transaction_commit 中调用 flatviews_reset
flatviews_reset 会将之前生成的 flag_views 全部删除掉, 然后重新构建

### memory_region_get_flatview_root

实际上, AddressSpace::root 持有的 MemoryRegion 并不一定就是顶层 MemoryRegion
是需要通过 memory_region_get_flatview_root 来获取的，比如 e1000 的顶层 MemoryRegion 就不是 bus master container
而是 system，这样就可以让多个 AddressSpace 虽然持有的 AddressSpace::root 不同，但是可以公用相同的 Flatview 了，
```c
/*
address-space: e1000
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

## MemoryRegion
MemoryRegion 用于描述一个范围内的映射规则。

例如下面的范围中描述的是，e1000-mmio 空间，对于该范围的读写最后不是在读写内存，而是在在操作设备，这会触发设备的模拟工作。
```txt
      00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio
```
而这个范围访问 ram 了。
```txt
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff
```

刚开始解除可能比较奇怪，为什么 memory region 重叠，为什么需要制作出现 alias 的概念。
其实，很简单，这样写代码比较方便，
- 如果不让 memory region 可以 overlap，那么一个模块在构建 memory region 的时候需要考虑另一个另一个模块的 memory region 的大小。
    - pci 空间是覆盖了 64 bit 的空间，和 ram-below-4g 和 ram-above-4g 是 overlap 的, 而 ram 的大小是动态确定，如果禁止 overlap, 那么 pci 的范围也需要动态调整
- 使用 alias 可以将一个 memory region 的一部分放到另一个 memory region 上，就 ram-below-4g 和 ram-above-4g 的例子而言，就是将创建 ram-below-4g 是 pc.ram 的一部分，然后放到 MemoryRegion `system` 上。
    - 如果不采用这种方法，而是创建两个分裂的 pc.ram, 那么就要 mmap 两次，之后的种种操作也都需要进行两次。

## FlatView
给出一个地址，显然需要尽快知道该地址映射到什么位置, 但是
MemoryRegion 存在 overlap，alias 的，获取这个地址上的真正的 memory region 需要进行计算，
不可能每次访存都将 memory region 的关系计算一次，而且要保存下来。这个保存下来的结果就是 FlatView 了。
[查看一个完整的 FlatRange](https://martins3.github.io/qemu/memory/memory-model-tcg-flat.txt)

FlatView 由一个个 FlatRange 组成，互相不重合，描述了最终的地址空间的样子，从 MemoryRegion 生成 FlatRange 的过程在
`generate_memory_topology`

- address_space_init : 使用 MemoryRegion 来初始化 AddressSpace，除了调用
    - address_space_update_topology
      - memory_region_get_flatview_root
      - generate_memory_topology

```c
/* Render a memory topology into a list of disjoint absolute ranges. */
static FlatView *generate_memory_topology(MemoryRegion *mr)
{
    int i;
    FlatView *view;

    view = flatview_new(mr);

    if (mr) {
        // 递归的将 MemoryRegion 中的各个 sub MemoryRegion 压平为 FlatRange
        //  1. 如果是 alias, 那么 render mr->alias
        //  2. 如果存在 child，那么按照优先级 render child, memory_region_add_subregion_common 优先级是满足的
        //  3. 最后，Render the region itself into any gaps left by the current view.
        //  4. 终极目的，创建 FlatRange 出来，并且使用 flatview_insert 将 FlatRange 放到 FlatView::ranges 数组上
        render_memory_region(view, mr, int128_zero(),
                             addrrange_make(int128_zero(), int128_2_64()),
                             false, false);
    }
    // 将可以合并的 FlatRange 合并起来
    flatview_simplify(view);

    view->dispatch = address_space_dispatch_new(view);
    for (i = 0; i < view->nr; i++) {
        // FlatRange 装换为 MemoryRegionSection 是一个很简单的装换
        MemoryRegionSection mrs =
            section_from_flat_range(&view->ranges[i], view);
        // 在其中由于一个 FlatRange 可能只会覆盖 page 的部分
        // 一个 MemoryRegionSection 可能会被拆分为多个 MemoryRegionSection
        flatview_add_to_dispatch(view, &mrs);
    }
    address_space_dispatch_compact(view->dispatch);
    // 新的 FlatView 制作出来，将老的替换掉
    g_hash_table_replace(flat_views, mr, view);

    return view;
}
```

如果 container MemoryRegion 很 priority 很低，而 subregions 的 priority 再高也没用了

## AddressSpaceDispatch
FlatView 是一个数组形式，为了加快访问，显然需要使用构成一个红黑树之类的，可以在 log(n) 的时间内访问的, QEMU 实现的
这个就是 AddressSpaceDispatch 了。

在 `generate_memory_topology` 中将 FlatRange 逐个调用 `flatview_add_to_dispatch` 创建出来的。

## RAMBlock
创建一个 RAM 的过程大致如此:
1. 创建一个 MemoryRegion / RamBlock，并且关联起来
2. mmap 出来一个 host virtual memory 当做 guest 的内存

- memory_region_init_ram : 创建出来 RAM, 但是 memory_region_set_readonly 不就让这里没有作用了
    - memory_region_init_ram_nomigrate
      - memory_region_init_ram_flags_nomigrate
        - qemu_ram_alloc
          - ram_block_add
            - dirty_memory_extend : 初始化 ram_list.dirty_memory , 使用的位置在 cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty
            - phys_mem_alloc (qemu_anon_ram_alloc)
              - qemu_ram_mmap
                - mmap : 可见 RAMBlock 在初始化的时候会在 host virtual address space 中 map 出来一个空间

RAMBlock 结构体分析:
1. RAMBlock::host : host 的虚拟地址空间，存储 mmap 的返回值
2. RAMBlock::offset : 将所有的 RAMBlock 连续的放到一起，每一个 RAMBlock 的 offset，第一个加入的 offset 为 0
    - 通过 RAMBlock::offset 可以放一个 RAM 内的 page 知道在 RAMList::dirty_memory 对应的 bit 位

看一个在综合路径中的使用:
- get_page_addr_code : 从 guest 虚拟地址的 pc 获取 guest 物理地址的 pc
  - tlb_hit : 进行虚实转换获取 hva
  - get_page_addr_code_hostp
    - qemu_ram_addr_from_host_nofail : 通过 hva 获取 gpa
      - qemu_ram_addr_from_host
        - qemu_ram_block_from_host

## ram addr
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

find_ram_offset 中 RAM 的对齐至少为 0x40000
```c
        candidate = ROUND_UP(candidate, BITS_PER_LONG << TARGET_PAGE_BITS);
```

在 ram_list 中，RAMBlock 按照大小排序的。
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

## subpage
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


## MemoryListener
当修改地址空间的映射规则的时候，有时候需要执行一下 hook 函数，最典型的就是 kvm 添加了一个 ram 的时候，这个时候是需要通知内核的 kvm 模块的，
而 tcg 的地址空间是纯粹软件模拟，就无需注册这个 hook

MemoryListener 的还有一个主要功能是 dirty memory 的记录, kvm 依赖内核模块，所以总是需要执行一下对应的通知内核操作。

## CPUAddressSpace
CPUAddressSpace 并不是因为 tcg CPU 访存存在新的映射，因为
tcg 因为使用 softmmu 的原因，cpu 的访问 ram 也是走软件的，

制作出来 CPUAddressSpace 只是为了将 tcg CPU AddressSpace 相关的东西放到一起。
```diff
tree a50a83c59f416259a423493cc996646bbeca1f7e
parent c8bc83a4dd29a9a33f5be81686bfe6e2e628097b
author Paolo Bonzini <pbonzini@redhat.com> Wed Mar 1 10:34:48 2017 +0100
committer Paolo Bonzini <pbonzini@redhat.com> Wed Jun 7 18:22:02 2017 +0200

target/i386: use multiple CPU AddressSpaces

This speeds up SMM switches.  Later on it may remove the need to take
the BQL, and it may also allow to reuse code between TCG and KVM.

Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

在 tcg_cpu_realizefn 中 tcg_cpu_machine_done 初始化 CPUAddressSpace

CPUAddressSpace 的使用主要在 address_space_translate_for_iotlb 和 iotlb_to_section 。

每一个 CPU 创建一个 CPUAddressSpace ，而不是公用一个 CPUAddressSpace, 这是因为在 tcg_commit 中，通过 CPUAddressSpace 找到对应的 cpu 然后进行 TLBFlush

## memory access code flow
总体来说

- helper_inw
  - address_space_lduw
    - address_space_ldl_internal
      - address_space_translate : 获取具体是在那个 memory region 是为了判断当前的读写发生在哪一个 memory region 上
        - flatview_translate : 参数 Flatview, 和 hwaddr 返回 MemoryRegion
            - flatview_do_translate : 这就是利用 AddressSpaceDispatch 的基础设施查询
              - address_space_translate_internal
                - phys_page_find : 这存在一个 cache, 当没有命中的时候，需要查询一波
      - memory_region_dispatch_read : 如果进行的是 mmio, 通过持有 MemoryRegions 可以很快的找到对应的空间
        - memory_region_dispatch_read1
          - access_with_adjusted_size
      - qemu_map_ram_ptr : 如果是 RAM 的访问就很容易

- kvm_handle_io / dma_memory_rw_relaxed / address_space_map
  - address_space_rw
    - address_space_read_full
      - address_space_to_flatview : 获取  AddressSpace::current_map
      - flatview_read
        - flatview_translate : 从 flatview 到 mr
        - flatview_read_continue
          - memory_region_dispatch_read : MMIO 之类的最终调用到 MemoryRegionOps
          - memcpy : RAM 直接拷贝

- io_readx / io_writex
  - memory_region_dispatch_read

其中 flatview_read 的三个调用者:
- subpage_read
- address_space_read_full : 只有唯一调用者 address_space_rw
- address_space_map

所以，如果调用者不知道将要访问的是 MMIO 还是 RAM，那么
那么就会走 flatview_read 。如果知道了，比如典型的例子
address_space_ldl_internal ，如果 MMIO，那么直接走
flatview_translate，如果是 RAM，直接 memcpy

- address_space_stw_internal 和 io_readx 开始访存的对比
  - io_readx 是 address_space_stw_internal 的简化版，相当于直接调用 memory_region_dispatch_read, 没有处理 ram 相关的。
  - store_helper 是 address_space_stw_internal 的强化版本
    - 处理 watchpoint
    - 主要是需要处理 TLB 命中的问题
    - 以及非对其访问，因为 address_space_stw_internal 的调用者都是从 helper 哪里来的，所以要容易的多
    - 两者都需要处理 dirty page 的情况

## alias
通过 alias 可以创建出来一个 MemoryRegion 的一部分。

使用 pc.ram 作为例子，物理内存中存在空洞的，用于存放 PCI 的映射空洞。
利用 alias 机制构建出来 ram-below-4g 和 ram-above-4g 两个 MemoryRegion 添加到 system_memory 上。
```c
void pc_memory_init(PCMachineState *pcms,
                    MemoryRegion *system_memory,
                    MemoryRegion *rom_memory,
                    MemoryRegion **ram_memory)
    // 创建出来 pc.ram
    memory_region_allocate_system_memory(ram, NULL, "pc.ram",
                                         machine->ram_size);
    *ram_memory = ram;
    ram_below_4g = g_malloc(sizeof(*ram_below_4g));
    memory_region_init_alias(ram_below_4g, NULL, "ram-below-4g", ram,
                             0, x86ms->below_4g_mem_size);
    memory_region_add_subregion(system_memory, 0, ram_below_4g);
    e820_add_entry(0, x86ms->below_4g_mem_size, E820_RAM);
    if (x86ms->above_4g_mem_size > 0) {
        ram_above_4g = g_malloc(sizeof(*ram_above_4g));
        memory_region_init_alias(ram_above_4g, NULL, "ram-above-4g", ram,
                                 x86ms->below_4g_mem_size,
                                 x86ms->above_4g_mem_size);
        memory_region_add_subregion(system_memory, 0x100000000ULL,
                                    ram_above_4g);
        e820_add_entry(0x100000000ULL, x86ms->above_4g_mem_size, E820_RAM);
    }
```
最后的结果:
```txt
address-space: memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-00000000bfffffff (prio 0, ram): alias ram-below-4g @pc.ram 0000000000000000-00000000bfffffff
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff

memory-region: pc.ram
  0000000000000000-000000017fffffff (prio 0, ram): pc.ram
```

此外，alias 的一个例子在 isa 地址空间:
```c
/*
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
*/
```

```c
void *memory_region_get_ram_ptr(MemoryRegion *mr)
{
    void *ptr;
    uint64_t offset = 0;

    RCU_READ_LOCK_GUARD();
    // 首先计算出来 alias MemoryRegion 在本体中的偏移 offset
    while (mr->alias) {
        offset += mr->alias_offset;
        mr = mr->alias;
    }
    assert(mr->ram_block);
    ptr = qemu_map_ram_ptr(mr->ram_block, offset);

    return ptr;
}
```

## memory listener

- kvm 根本没有注册 MemoryListener::commit

- memory_listener_register
  - 将 memory_listener 添加到全局链表 memory_listeners 和 AddressSpace::listeners
  - listener_add_address_space
    - 调用 begin region_add log_start commit 等 hook

#### memory listener tcg
- 忽然意识到，CPUAddressSpace 只是 tcg 特有的
- cpu_address_space_init 中注册 memory listener

一共注册两个 hook:
```c
    if (tcg_enabled()) {
        newas->tcg_as_listener.log_global_after_sync = tcg_log_global_after_sync;
        newas->tcg_as_listener.commit = tcg_commit;
        memory_listener_register(&newas->tcg_as_listener, as);
    }
```

- tcg_commit
  - 处理一些 RCU，iothread 的问题
  - tlb_flush
  - 当 memory_region_transaction_commit 和 将 listener 添加到 (memory_listener_register -> listener_add_address_space) 中间的时候。
- tcg_log_global_after_sync : 当 dirty map sync 之后，需要为了针对于 tcg 特殊调用的 hook

```c
/**
 * CPUAddressSpace: all the information a CPU needs about an AddressSpace
 * @cpu: the CPU whose AddressSpace this is
 * @as: the AddressSpace itself
 * @memory_dispatch: its dispatch pointer (cached, RCU protected)
 * @tcg_as_listener: listener for tracking changes to the AddressSpace
 */
struct CPUAddressSpace {
    CPUState *cpu;
    AddressSpace *as;
    struct AddressSpaceDispatch *memory_dispatch;
    MemoryListener tcg_as_listener;
};
```

#### memory listener hook 的调用位置
实际上，这些 hook 都是 KVM 注册的:
- 关于 dirty log 可以参考李强的 blog[^1]

- address_space_set_flatview 会调用两次 address_space_update_topology_pass，进而调用 log_start log_stop region_del region_add 之类的, 因为更新了新的 Flatview 之类，所以也是需要进行一下比如对于 kvm 的通知吧
- memory_listener_register -> listener_add_address_space : address_space 首次注册上 memory listener, 所以将这些 flat range 分别调用一下 listener hook 还是很有必要的
- memory_region_sync_dirty_bitmap
    - log_sync / log_sync_global
- memory_global_dirty_log_start
- memory_global_after_dirty_log_sync
    - log_global_after_sync
- address_space_add_del_ioeventfds : 将经过 memory model 变动之后还存在的 ioeventfd 保存起来
    - eventfd_add / eventfd_del

总结一下，基本就是前面两个 , dirty map 更加复杂一点还要中间几个， 最后一个处理 ioeventfd 的

#### ioeventfd
haiyonghao 的两篇 blog 对于这个问题分析非常清晰易懂
- Linux kernel 的 eventfd 实现 : https://www.cnblogs.com/haiyonghao/p/14440737.html
- QEMU 的 eventfd 实现  https://www.cnblogs.com/haiyonghao/p/14440743.html

从 memory listener 的角度，也就是当 memory_region_add_eventfd 实现添加 eventfd，而使用注册的 hook 来处理当
memory region 发生变动的时候来通知内核。

#### kvm memory listener hook
- kvm_region_add : 这个很重要，这会让 KVM 最终对于这个 memory section 调用 KVM_SET_USER_MEMORY_REGION, 也即是映射出来一个地址空间来
- kvm_log_start : 其实很容易，这是这个 region 的 flag, 从现在开始记录 kvm 了
- log_sync : 将内核的 dirty log 读去出来，调用者为 memory_region_sync_dirty_bitmap

现在分析出来，实际上，kvm 注册 memory listener 多出来的就只是 dirty log 了

## IOMMU
IOMMU 的学习可以参考 ASPLOS 提供的 ppt[^1], 简单来说，以前设备是可以直接访问物理内存，增加了 IOMMU 之后，设备访问物理内存类似 CPU 也是需要经过一个虚实地址映射。
所以，每一个 PCI 设备都会创建对应的 AddressSpace

默认没有配置 IOMMU 也就是直接访问物理内存，所以就是直接 alias 到 `system_memory`(就是 `address_space_memory` 关联的那个 MemoryRegion) 上。
```txt
address-space: nvme
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

pci_device_iommu_address_space : 如果一个 device 被用于直通，那么其进行 IO 的 address space 就不再是 address_space_memory 的，而是需要经过一层映射。

1. https://wiki.qemu.org/Features/VT-d 分析了下为什么 guest 需要 vIOMMU
2. [oracle 的 blog](https://blogs.oracle.com/linux/post/a-study-of-the-linux-kernel-pci-subsystem-with-qemu) 告诉了 iommu 打开的方法 : `-device intel-iommu` + `-machine q35`

## PCI Device AddressSpace
```c
struct PCIDevice {
    // ...
    PCIIORegion io_regions[PCI_NUM_REGIONS];
    AddressSpace bus_master_as;                // 因为 IOMMU 的存在，所以存在
    MemoryRegion bus_master_container_region;  // container
    MemoryRegion bus_master_enable_region;     // 总是 bus_master_as->root 的 alias, 是否 enable 取决于运行时操作系统对于设备的操作
```

形成下面的 AddressSpace 分别发生在: pci_init_bus_master 和 do_pci_register_device
```c
/*
address-space: e1000
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```


```c
static void pci_init_bus_master(PCIDevice *pci_dev)
{
    AddressSpace *dma_as = pci_device_iommu_address_space(pci_dev); // dma 的空间就是 address_space_memory

    memory_region_init_alias(&pci_dev->bus_master_enable_region,
                             OBJECT(pci_dev), "bus master",
                             dma_as->root, 0, memory_region_size(dma_as->root)); // 创建一个 alias 到 system_memory
    memory_region_set_enabled(&pci_dev->bus_master_enable_region, false);
    memory_region_add_subregion(&pci_dev->bus_master_container_region, 0, // 创建一个 container
                                &pci_dev->bus_master_enable_region);
}
```

- do_pci_register_device
   - `address_space_init(&pci_dev->bus_master_as, &pci_dev->bus_master_container_region, pci_dev->name);`

PCIDevice 关联了一个 AddressSpace, PCIDevice::bus_master_as 其引用位置为:
  - msi_send_message
  - pci_get_address_space
    - 当 pci_dma_rw 进行操作的时候需要获取当时的地址空间了

需要区分的是，IOMMU 让 dma_as 可能不再是 address_space_memory，但是
操作设备的 mmio 和 pio 的, 最后都会添加到 pci MemoryRegion 上

#### PCIIORegion
和 bus_master_as / bus_master_container_region / bus_master_enable_region 区分的是，这个就是设备的配置空间
最后都是放到 system_memory / system_io 上的

## dma
主要出现的文件: include/sysemu/dma.h 和 softmmu/dma-helpers.c

- dma 最重要的客户还是 pci 产生的地址空间, 其中 dma_blk_io 和 dma_buf_read 之类的都是 DMA 和 scsi / nvme 相关的, 是一个很容易的封装。
- dma_memory_rw 另一个用户当然是 fw_cfg，另一个就是 pci_dma_rw 了
  - 实际上，fw_cfg 选择的 as 总是 address_space_memory 的

```c
/*
#0  pci_dma_rw (dir=DMA_DIRECTION_TO_DEVICE, len=64, buf=0x7fffffffd210, addr=3221082112, dev=0x555557c86f30) at /home/maritns3/core/kvmqemu/include/hw/pci/pci.h:806
#1  pci_dma_read (len=64, buf=0x7fffffffd210, addr=3221082112, dev=0x555557c86f30) at /home/maritns3/core/kvmqemu/include/hw/pci/pci.h:824
#2  nvme_addr_read (n=0x555557c86f30, addr=3221082112, buf=0x7fffffffd210, size=64) at ../hw/nvme/ctrl.c:377
#3  0x00005555559550b9 in nvme_process_sq (opaque=opaque@entry=0x555557c8a728) at ../hw/nvme/ctrl.c:5514
*/

static inline MemTxResult pci_dma_rw(PCIDevice *dev, dma_addr_t addr, void *buf, dma_addr_t len, DMADirection dir) {
    return dma_memory_rw(pci_get_address_space(dev), addr, buf, len, dir);
}
```

- dma_memory_set : 只有一个用户 fw_cfg_dma_transfer，就是 DMA 版本的 memset 了
- dma_barrier : 就是一条 smp_mb，注释说的是，因为 guest 设备模拟和 guest 的执行是同步进行的, 希望让 guest 看到的内存修改就是 host 的这一侧的
    - 因为设备是被直通的，而且当前是单核，所以暂时也许不用考虑

- dma_memory_rw 和 cpu_physical_memory_read 非常相似，只是 cpu_physical_memory_read 走的 `as` 是 `address_space_memory`,  而 dma_memory_rw 是可以指定自己的 address space 的

## dirty page tracking
> 要 dirty page tracking 需要分析 migration 的整个实现，这个暂时不是我的关注重点，这里随便记录一下

dirty_memory 划分为三种
```c
#define DIRTY_MEMORY_VGA       0
#define DIRTY_MEMORY_CODE      1
#define DIRTY_MEMORY_MIGRATION 2
#define DIRTY_MEMORY_NUM       3        /* num of dirty bits */
```
因为三种需求:
- smc : TCG 中检测到 guest 的代码修改过，那么就需要 invalidate 这个 page 上管理的 TB
- migration : 当前虚拟机的内存被修改没有同步到远程的虚拟机中
- vga : 绘制屏幕的时候，记录下仅仅被修改过的部分，从而加快渲染速度

关联的主要函数以及他们的调用者:
- tlb_set_dirty
- tlb_reset_dirty
- notdirty_write
   * atomic_mmu_lookup : 这是整个 atomic 机制调用的地方
   * probe_access / probe_access_flags : x86 guest 从来没有调用过，这个不是我能理解的
   * store_helper

- 在 cpu_physical_memory_set_dirty_lebitmap 中间, 如果没有打开 global_dirty_log 那么 client 就不会添加上 DIRTY_MEMORY_MIGRATION
- 在 memory_region_get_dirty_log_mask 中对于 DIRTY_MEMORY_CODE 和 DIRTY_MEMORY_MIGRATION 也是存在类似的特殊处理

- colo_incoming_start_dirty_log : https://wiki.qemu.org/Features/COLO
  - ramblock_sync_dirty_bitmap
    - cpu_physical_memory_sync_dirty_bitmap

- 在正常的 kvm 其中的操作过程中，cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty 都不会被调用
- 在 tcg 模式下, 如果只是需要支持 SMC, 那么需要的函数不多，例如 cpu_physical_memory_test_and_clear_dirty

- vga_mem_write : 使用 memory_region_set_dirty 来调用设置 memory dirty 的操作
- 当 vga_draw_graphic 的时候，其可以 memory_region_snapshot_and_clear_dirty

#### migration

QEMU 记录的 dirty page 已经发送到只剩下最后的 max_size 的时候，调用 migration_bitmap_sync 进行 dirty page 同步，
该函数最终会调用到 ioctl(KVM_GET_DIRTY_LOG) 上，将 dirty page 记录到 ram_list.dirty_memory 中。
- migration_bitmap_sync
  - memory_global_dirty_log_sync : 调用 MemoryListener::log_sync，比如 kvm, 这是为将 dirty memory 从 kernel 中同步过来，**tcg** 因为只是操作 ramlist.dirty_memory 的，所以这一步函数对于其为空操作
    - memory_region_sync_dirty_bitmap
      - MemoryListener::log_sync
         - kvm_log_sync
            - kvm_physical_sync_dirty_bitmap
              - kvm_slot_get_dirty_log  : 使用 KVM_GET_DIRTY_LOG ioctl
              - kvm_slot_sync_dirty_pages
                - cpu_physical_memory_set_dirty_lebitmap : 这个将从 kvm 中得到的 dirty map 的信息放到 ram_list.dirty_memory
      - MemoryListener::log_sync_global
  - ramblock_sync_dirty_bitmap : 将 dirty memory 同步到 RAMState 中，准备发送出去
    - cpu_physical_memory_sync_dirty_bitmap

```c
    if (s->kvm_dirty_ring_size) {
        kml->listener.log_sync_global = kvm_log_sync_global;
    } else {
        kml->listener.log_sync = kvm_log_sync;
        kml->listener.log_clear = kvm_log_clear;
    }
```
[log_sync_global](https://www.youtube.com/watch?v=YsQJ-Vll3sg) 接口都是 Peter Xu 在 2020 添加的

## Option ROM
https://en.wikipedia.org/wiki/Option_ROM

```txt
pci_update_mappings pci febe0000 vga.rom
pci_update_mappings pci feb80000 e1000.rom
```

- [x] 更新了，为什么在地址空间中间看不到 pci 的 rom 啊
这个地址之后被隐藏了
```txt
Option rom sizing returned febe0000 ffff0000
map_pcirom 0xfebe0000
```
最开始的时候，将 ROM 映射到 PCI 空间中，然后拷贝到 ROM 中，然后更新 PCI 空间, 这个 ROM 被隐藏起来了。

```txt
pci_add_option_rom /home/maritns3/core/kvmqemu/build/pc-bios/vgabios-stdvga.bin
ram_block_add vga.rom
```
vga 的源代码应该是在 : https://github.com/qemu/vgabios

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


## address_space_map

```c
/* Map a physical memory region into a host virtual address.
 * May map a subset of the requested range, given by and returned in *plen.
 * May return NULL if resources needed to perform the mapping are exhausted.
 * Use only for reads OR writes - not for read-modify-write operations.
 * Use cpu_register_map_client() to know when retrying the map operation is
 * likely to succeed.
 */
```

```c
typedef struct {
    MemoryRegion *mr;
    void *buffer;
    hwaddr addr;
    hwaddr len;
    bool in_use;
} BounceBuffer;
```

基本可以简化为
```c
    fv = address_space_to_flatview(as);
    mr = flatview_translate(fv, addr, &xlat, &l, is_write, attrs);
    ptr = qemu_ram_ptr_length(mr->ram_block, xlat, plen, true);
```
其真正的作用在于，如果想要映射的一个空间本身不是 ram 的(memory_access_is_direct)
这个时候，将会动态的创建出来一个 memory region。
此外可以处理访问越过 MemoryRegion 的情况。

其中的一个使用者为

```c
/*
#0  address_space_map (as=0x5555563e9880 <address_space_memory>, addr=addr@entry=4320863120, plen=plen@entry=0x7fffe2ff2e70, is_write=is_write@entry=false, attrs=attrs@
entry=...) at /home/maritns3/core/xqm/exec.c:3513
#1  0x00005555558f4cd5 in dma_memory_map (dir=DMA_DIRECTION_TO_DEVICE, len=<synthetic pointer>, addr=4320863120, as=<optimized out>) at /home/maritns3/core/xqm/include/
sysemu/dma.h:135
#2  virtqueue_map_desc (vdev=vdev@entry=0x555557a02cc0, p_num_sg=p_num_sg@entry=0x7fffe2ff2ef8, addr=addr@entry=0x7fffe2ff2f90, iov=iov@entry=0x7fffe2ff4f90, max_num_sg
=max_num_sg@entry=1024, is_write=is_write@entry=false, pa=4320863120, sz=16) at /home/maritns3/core/xqm/hw/virtio/virtio.c:1314
#3  0x00005555558f553f in virtqueue_split_pop (vq=0x555557a0e970, sz=<optimized out>) at /home/maritns3/core/xqm/hw/virtio/virtio.c:1496
#4  0x00005555558f5b35 in virtqueue_pop (vq=vq@entry=0x555557a0e970, sz=sz@entry=192) at /home/maritns3/core/xqm/hw/virtio/virtio.c:1687
#5  0x00005555558bacda in virtio_blk_get_request (vq=0x555557a0e970, s=0x555557a02cc0) at /home/maritns3/core/xqm/hw/block/virtio-blk.c:775
#6  virtio_blk_handle_vq (s=0x555557a02cc0, vq=0x555557a0e970) at /home/maritns3/core/xqm/hw/block/virtio-blk.c:775
#7  0x00005555558f00a6 in virtio_queue_notify_aio_vq (vq=0x555557a0e970) at /home/maritns3/core/xqm/hw/virtio/virtio.c:2317
#8  0x0000555555caf857 in aio_dispatch_handlers (ctx=ctx@entry=0x5555566f1580) at /home/maritns3/core/xqm/util/aio-posix.c:429
#9  0x0000555555cb0526 in aio_poll (ctx=0x5555566f1580, blocking=blocking@entry=true) at /home/maritns3/core/xqm/util/aio-posix.c:731
#10 0x00005555559b5904 in iothread_run (opaque=opaque@entry=0x55555676bf00) at /home/maritns3/core/xqm/iothread.c:75
#11 0x0000555555cb28e3 in qemu_thread_start (args=<optimized out>) at /home/maritns3/core/xqm/util/qemu-thread-posix.c:519
#12 0x00007ffff5c0a609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#13 0x00007ffff5b31293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```
## sysbus_init_mmio
在 hw/intc/ioapic.c:ioapic_realize 初始化了 MemoryRegion "ioapic" 的，但是实际上是
在 ioapic_common_realize 调用 sysbus_init_mmio 注册上的

## Appendix
在 v6.0 中
| file             | desc                                                            |
|------------------|-----------------------------------------------------------------|
| softmmu/memory.c | memory_region_dispatch_read 之类的各种 memory region 的管理工作 |
| softmmu/physmem  | RAMBlock 之类的管理                                             |

| function                              | desc                                                                                      |
|---------------------------------------|-------------------------------------------------------------------------------------------|
| address_space_translate               | 通过 hwaddr 参数找到 MemoryRegion 这里和 Flatview 有关的                                  |
| qemu_map_ram_ptr                      | 给定 ram_addr 获取到 host virtual addr                                                    |
| memory_region_dispatch_ (read/ write) | 最终 dispatch 到设备注册的 MemoryRegionOps 上                                             |
| prepare_mmio_access                   | 进行 MMIO 需要持有 BQL 锁, 如果没有上 QBL 的话，那么在 prepare_mmio_access 中会把锁加上去 |
| memory_access_is_direct               | 判断内存到底是可以直接写，还是设备空间，需要重新处理一下                                  |
| memory_region_get_ram_ptr             | 返回一个 RAMBlock 在 host 中的偏移量                                                      |
| memory_region_get_ram_addr            | 获取在 ram 空间的偏移                                                                     |
| memory_region_section_get_iotlb       | 获取一个 gpa 上的 MemoryRegion，不会 resolve_subpage                                      |
| cpu_physical_memory_get_dirty         | 获取该位置是否发生为 dirty                                                                |
| qemu_ram_block_from_host              | 根据 hva 获取 RamBlock                                                                    |

| struct               | desc                                                                                                                |
|----------------------|---------------------------------------------------------------------------------------------------------------------|
| AddressSpace         | root : 仅仅关联一个 MemoryRegion, current_map : 关联 Flatview，其余是 ioeventfd 之类的                              |
| MemoryRegion         | 主要成员 ram_block, ops, *container*, *alias*, **subregions**, 看来是 MemoryRegion 通过 subregions 负责构建树形结构 |
| Flatview             | ranges : 通过 render_memory_region 生成, 成员 nr nr_allocated 来管理其数量, root : 关联的 MemoryRegions , dispatch  |
| AddressSpaceDispatch | 保存 GPA 到 HVA 的映射关系                                                                                          |

cpu_address_space_init : 初始化 `CPUAddressSpace *CPUState::cpu_ases`, CPUAddressSpace 的主要成员 AddressSpace + CPUState

在
`info mtree [-f][-d][-o][-D]` -- show memory tree (-f: dump flat view for address spaces;-d: dump dispatch tree, valid with -f only);-o: dump region owners/parents;-D: dump disabled regions

通过 mtree_info 函数在代码特定位置观测 memory region 的形成的过程

一些参考:
- https://www.anquanke.com/post/id/86412 :star:
- https://wiki.osdev.org/System_Management_Mode
- https://www.linux-kvm.org/images/1/17/Kvm-forum-2013-Effective-multithreading-in-QEMU.pdf
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/08/11/dirty-pages-tracking-in-migration
- [official doc](https://qemu.readthedocs.io/en/latest/devel/memory.html)

[^1]: [ASPLOS IOMMU tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
