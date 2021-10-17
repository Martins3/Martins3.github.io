# QEMU 的 memory model

<!-- vim-markdown-toc GitLab -->

- [Overview](#overview)
- [AddressSpace](#addressspace)
- [MemoryRegion](#memoryregion)
- [FlatView](#flatview)
- [AddressSpaceDispatch](#addressspacedispatch)
- [RamBlock](#ramblock)
- [ram addr](#ram-addr)
- [subpage](#subpage)
- [MemoryListener](#memorylistener)
- [CPUAddressSpace](#cpuaddressspace)
- [SMM](#smm)
- [IOMMU](#iommu)
- [QA](#qa)

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

```c
/* Render a memory topology into a list of disjoint absolute ranges. */
static FlatView *generate_memory_topology(MemoryRegion *mr)
{
    int i;
    FlatView *view;

    view = flatview_new(mr);

    if (mr) {
        // 递归的将 MemoryRegion 中的各个 sub MemoryRegion 压平为 FlatRange
        //  1. 如果是 alias, 那么 render alias
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

## AddressSpaceDispatch
FlatView 是一个数组形式，为了加快访问，显然需要使用构成一个红黑树之类的，可以在 log(n) 的时间内访问的, QEMU 实现的
这个就是 AddressSpaceDispatch 了。

在 `generate_memory_topology` 中将 FlatRange 逐个调用 `flatview_add_to_dispatch` 创建出来的。

## RamBlock
创建一个 RAM 的过程大致如此:
1. 创建一个 MemoryRegion / RamBlock，并且关联起来
2. mmap 出来一个 host virtual memory 当做 guest 的内存

- memory_region_init_ram : 创建出来 RAM, 但是 memory_region_set_readonly 不就让这里没有作用了
    - memory_region_init_ram_nomigrate
      - memory_region_init_ram_flags_nomigrate
        - qemu_ram_alloc
          - ram_block_add
            - phys_mem_alloc (qemu_anon_ram_alloc)
              - qemu_ram_mmap
                - mmap : 可见 RAMBlock 在初始化的时候会在 host virtual address space 中 map 出来一个空间

RAMBlock 结构体分析:
1. RAMBlock::host : host 的虚拟地址空间，存储 mmap 的返回值
2. RAMBlock::offset : 将所有的 RAMBlock 连续的放到一起，每一个 RAMBlock 的 offset，第一个加入的 offset 为 0
    - 通过 RAMBlock::offset 可以放一个 RAM 内的 page 知道在 RAMList::dirty_memory 对应的 bit 位

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

## SMM
tcg 处理 AddressSpace 最大的不同在于为 SMM 创建一个新的地址空间。

在 do_smm_enter 中，会

```c
    env->hflags |= HF_SMM_MASK;
```

导致制作出来的 MemTxAttrs 的不同
```c
static inline MemTxAttrs cpu_get_mem_attrs(CPUX86State *env)
{
    return ((MemTxAttrs) { .secure = (env->hflags & HF_SMM_MASK) != 0 });
}
```

最后在 tlb_set_page_with_attrs 中选择不同的地址空间的。

## IOMMU
IOMMU 的学习可以参考 ASPLOS 提供的 ppt[^1], 简单来说，以前设备是可以直接访问物理内存，增加了 IOMMU 之后，设备访问物理内存类似 CPU 也是需要经过一个虚实地址映射。
所以，每一个 PCI 设备都会创建对应的 AddressSpace

默认没有配置 IOMMU 也就是直接访问物理内存，所以就是直接 alias 到 `system_memory`(就是 `address_space_memory` 关联的那个 MemoryRegion) 上。
```c
address-space: nvme
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

## QA
- 为什么 ram 不是一整块，而是拆分出来了 ram-below-4g 和 ram-above-4g 两个部分?
    - 因为中间需要留出 mmio 空洞

[^1]: [ASPLOS IOMMU tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
