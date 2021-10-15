# memory model

在 https://github.com/kernelrookie/DuckBuBi/issues/35 中，
分析 `address_space_*` 以及如何检查 memory_ldst.inc.c 和
memory_ldst.inc.h 的方法。

在 v6.0 中
| file             | desc                                                            |
|------------------|-----------------------------------------------------------------|
| softmmu/memory.c | memory_region_dispatch_read 之类的各种 memory region 的管理工作 |
| softmmu/physmem  | RAMBlock 之类的管理                                             |

- flatview_for_each_range 从来不会被调用
- memory_region_read_with_attrs_accessor 从来不会被调用

分析 memory.h 吧。

| function                                                   | desc                                                                                                          |
|------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| address_space_translate                                    | 通过 hwaddr 参数找到 MemoryRegion 这里和 Flatview 有关的                                                      |
| memory_region_dispatch_read / memory_region_dispatch_write | 最关键的，访问 device, 逐步向下分发的过程                                                                     |
| memory_region_get_dirty_log_mask                           | 获取 MemoryRegion::dirty_log_mask                                                                             |
| memory_region_get_roptionrom_setupam_addr                                 |                                                                                                               |
| devend_memop                                               | 使用一个非常繁杂的宏判断，进行 IO 的时候，设备是否需要进行 endiana 调整，从现在的调用链看，应该永远返回都是 0 |
| qemu_map_ram_ptr                                           | 仔细看看注释，为了定义从 memory region 中的偏移获取 HVA, 定义了一堆函数                                       |
| invalidate_and_set_dirty                                   | 将一个范围的 TLB invalidate 利用 DirtyMemoryBlocks 标记这个区域为 dirty                                       |
| prepare_mmio_access                                        | 如果没有上 QBL 的话，那么把锁加上去                                                                           |
| memory_access_is_direct                                    | 判断内存到底是可以直接写，还是设备空间，需要重新处理一下                                                      |

按道理，memory_ldst 提供的是标准访存接口，那么:

- address_space_stw_internal
  - io_readx 是 address_space_stw_internal 的简化版，相当于直接调用 memory_region_dispatch_read, 没有处理 ram 相关的。
  - store_helper 是 address_space_stw_internal 的强化版本
    - 主要是需要处理 TLB 命中的问题
    - 以及非对其访问，因为 address_space_stw_internal 的调用者都是从 helper 哪里来的，所以要容易的多

- 从 address_space_rw 到 memory_region_dispatch_read 中间经历了什么东西?
    - 地址转换, 准确来说，是 flatview_translate

- store_helper 相对于 address_space_stw_internal 的内容对比
    - not dirty : 都有，address_space_stw_internal 也处理了
    - watch point : 多出来的
    - store_helper_unaligned : 多出来的

- [ ] kvmtool 处理地址空间之所以那么简单，是因为其不用模拟设备，
但是 QEMU 中间从 kvm 中 exit 出来，address_space_rw 的内容感觉还是比 kvmtool 复杂很多啊!

## QA
- [x] PCIe 注册的 AddressSpace 是不是因为对应的 MMIO 空间
  - [x] KVM 是如何注册这些 MMIO 空间的，还是说没有注册的空间默认为 MMIO 空间
- [x] region_add 是处理 block 的，看看 ram block 和 ptr 的处理
  - kvm_set_phys_mem : 使用 memory_region_is_ram 做了判断的
- [x] 一个 container 的 priority 会影响其 subregions 的 priority 吗? 或者说，如果 container 很 priority 很低，而 subregions 的 priority 再高也没用了
  - 从 render_memory_region 是递归的向下的, 高优先级的首先部署，所以答案是肯定的。

## QEMU Memory Model 结构分析
https://kernelgo.org/images/qemu-address-space.svg

关键结构体内容分析:
| struct               | desc                                                                                                                |
|----------------------|---------------------------------------------------------------------------------------------------------------------|
| AddressSpace         | root : 仅仅关联一个 MemoryRegion, current_map : 关联 Flatview，其余是 ioeventfd 之类的                              |
| MemoryRegion         | 主要成员 ram_block, ops, *container*, *alias*, **subregions**, 看来是 MemoryRegion 通过 subregions 负责构建树形结构 |
| Flatview             | ranges : 通过 render_memory_region 生成, 成员 nr nr_allocated 来管理其数量, root : 关联的 MemoryRegions , dispatch  |
| AddressSpaceDispatch | 保存 GPA 到 HVA 的映射关系                                                                                          |

- cpu_address_space_init : 初始化 `CPUAddressSpace *CPUState::cpu_ases`, CPUAddressSpace 的主要成员 AddressSpace + CPUState
  - address_space_init : 使用 MemoryRegion 来初始化 AddressSpace，除了调用
    - address_space_update_topology
      - memory_region_get_flatview_root
      - generate_memory_topology
        - render_memory_region
        - flatview_simplify
        - address_space_dispatch_new : 初始化 Flatview::dispatch

> `info mtree [-f][-d][-o][-D]` -- show memory tree (-f: dump flat view for address spaces;-d: dump dispatch tree, valid with -f only);-o: dump region owners/parents;-D: dump disabled regions

通过 mtree_info 函数在代码特定位置观测 memory region 的形成的过程

## AddressSpace
当提到 address space 的时候，因为要处理地址空间的变换的, 所以，实际上是来持有 Flatview 的

- 如果 memory region 添加了，但是导致 Flatview 重构，那么 AddressSpace 如何知道?
  - 在 memory_region_transaction_commit 后面紧跟着 address_space_set_flatview

- [x] 为什么使用 flat_views 这个 g_hash_table 来保存
  - 不是所有的 MemoryRegion 都是需要关联一个 Flatview 的, 实际上只有顶层的
  - AddressSpace 的确需要关联 Flatview 的，但是可能其他的 MemoryRegion 已经将其对应的 Flatview 更新了
  - 所以，其实这就是正确的操作

- [x] 为什么需要给创建多个 AddressSpace
  - KVM 中, 显然 IO 和 MMIO 是两个空间的，IO 和 MMIO 分别选择全局定义的 address_space_memory 和 address_space_io
  - tcg 中为了处理 SMM
  - [ ] KVM 处理 SMRAM 是怎么说

- cpu_address_space_init 当在 KVM 模式下, 已经没有任何必要创建出来 CPUAddressSpace 的必要
  - kvm 注册 memory listener 例如 kvm_memory_listener_register 都是直接使用 address_space_memory 的

```c
static MemoryRegion *system_memory;
static MemoryRegion *system_io;

AddressSpace address_space_io;
AddressSpace address_space_memory;
```
这两个 AddressSpace 的初始位置，都是在 memory_map_init, 将其和 system_memory 和 system_io 联系起来
而之后的一些列初始化和内容的填充都是通过这两个 MemoryRegion 完成的。

AddressSpace 关联一个 MemoryRegion, 通过 MemoryRegion 可以找到 Flatview Root, 从而找到该 as 关联的真正 flatview
而是 flatview 决定了 io 真正的地址 (address_space_set_flatview)

- 通过  `static GHashTable *flat_views;` 可以找到通过 mr 找到 flatview

## RAMBlock
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

find_ram_offset 中 RAM 的对齐至少为 0x40000
```c
        candidate = ROUND_UP(candidate, BITS_PER_LONG << TARGET_PAGE_BITS);
```
再看下面的 RAM 的 offset 既可以发现，其 RAM 就是一个个链接到一起的
```c
/*
huxueshi:ram_block_add pc.ram: offset=0 size=180000000
huxueshi:ram_block_add vga.vram: offset=180080000 size=800000
huxueshi:ram_block_add /rom@etc/acpi/tables: offset=180900000 size=200000
huxueshi:ram_block_add pc.bios: offset=180000000 size=40000
huxueshi:ram_block_add e1000.rom: offset=1808c0000 size=40000
huxueshi:ram_block_add pc.rom: offset=180040000 size=20000
huxueshi:ram_block_add virtio-vga.rom: offset=180880000 size=10000
huxueshi:ram_block_add /rom@etc/table-loader: offset=180b00000 size=10000
huxueshi:ram_block_add /rom@etc/acpi/rsdp: offset=180b40000 size=1000
```

## render_memory_region : 将 memory region 转化为 FlatRange
- memory_region_transaction_commit
  - flatviews_reset
    - generate_memory_topology : Render a memory topology into a list of disjoint absolute ranges.
      - render_memory_region : 虽然是一个很长的函数,
        1. 如果是 alias, 那么 render alias
        2. 如果存在 child，那么按照优先级 render child, memory_region_add_subregion_common 优先级是满足的
        3. 最后，Render the region itself into any gaps left by the current view.
        4. 终极目的，创建 FlatRange 出来，并且使用 flatview_insert 将 FlatRange 放到 FlatView::ranges 数组上
      - flatview_simplify
      - address_space_dispatch_new : 初始化 FlatView::dispatch
      - flatview_add_to_dispatch
      - address_space_dispatch_compact

## flatviews_reset
- flatviews_reset 的调用者总是 memory_region_transaction_commit
- flatviews_reset 总是会将之前生成的 flag_views 全部删除掉, 然后重新构建
- flat_views 中间一共只有三个 memory region 的
  - huxueshi:flatviews_reset memory
  - huxueshi:flatviews_reset I/O
  - huxueshi:flatviews_reset KVM-SMRAM

## AddressSpaceDispatch
- [ ] 应该将这些经典执行流程保护起来

#### AddressSpaceDispatch dispatch 的过程 : 百川归海
进行 pio / mmio 最后总是到达 : memory_region_dispatch_read

各种场景到达 memory_region_dispatch_read 的时候，总是会进行一个 memory_access_is_direct 的检查，否则就会进入到
qemu_map_ram_ptr 的计算中, 也就是说，memory_region_dispatch_read 总是在处理 pio / mmio

使用 memory_ldst.c 的 address_space_ldl_internal 中分析

- helper_inw
  - address_space_lduw
    - address_space_ldl_internal
      - address_space_translate : 获取具体是在那个 memory region 是为了判断当前的读写发生在哪一个 memory region 上
        - flatview_translate : 参数 Flatview, 和 hwaddr 返回 MemoryRegion
            - flatview_do_translate : 其实没有什么奇怪的，这就是利用 AddressSpaceDispatch 的基础设施查询
              - address_space_translate_internal
                - phys_page_find : 这存在一个 cache, 当没有命中的时候，需要查询一波
      - memory_region_dispatch_read : 如果进行的是 mmio, 通过持有 MemoryRegions 可以很快的找到对应的空间
        - memory_region_dispatch_read1
          - access_with_adjusted_size
      - qemu_map_ram_ptr : 如果是 RAM 的访问就很容易


- kvm_handle_io
  - address_space_rw
    - address_space_read_full
      - address_space_to_flatview : 获取  AddressSpace::current_map
      - flatview_read
        - flatview_translate : 从 flatview 到 mr
        - flatview_read_continue : 会在这里区分到底是 MMIO 还是一般的, 之所以叫做 continue 是为了处理访问在多个连续的 memory region 的情况
          - memory_region_dispatch_read : 这里，现在所有人都相同了

kvm 的 style:
```c
/*
#0  pci_host_config_read_common (pci_dev=0x5555570d4000, addr=2147483648, limit=1439872976, len=21845) at ../hw/pci/pci_host.c:88
#1  0x0000555555a49a17 in pci_data_read (s=0x5555570d4000, addr=2147483648, len=2) at ../hw/pci/pci_host.c:133
#2  0x0000555555a49b51 in pci_host_data_read (opaque=0x555556c44270, addr=0, len=2) at ../hw/pci/pci_host.c:178
#3  0x0000555555ca681c in memory_region_read_accessor (mr=0x555556c44680, addr=0, value=0x7fffe890f060, size=2, shift=0, mask=65535, attrs=...) at ../softmmu/memory.c:440
#4  0x0000555555ca6d1c in access_with_adjusted_size (addr=0, value=0x7fffe890f060, size=2, access_size_min=1, access_size_max=4, access_fn=0x555555ca67d6 <memory_region_read_accessor>, mr=0x555556c44680, attrs=...) at ../softmmu/memory.c:550
#5  0x0000555555ca9a38 in memory_region_dispatch_read1 (mr=0x555556c44680, addr=0, pval=0x7fffe890f060, size=2, attrs=...) at ../softmmu/memory.c:1427
#6  0x0000555555ca9b0e in memory_region_dispatch_read (mr=0x555556c44680, addr=0, pval=0x7fffe890f060, op=MO_16, attrs=...) at ../softmmu/memory.c:1455
#7  0x0000555555d31e77 in flatview_read_continue (fv=0x555556db8900, addr=3324, attrs=..., ptr=0x7fffeb180000, len=2, addr1=0, l=2, mr=0x555556c44680) at ../softmmu/physmem.c:2831
#8  0x0000555555d31fce in flatview_read (fv=0x555556db8900, addr=3324, attrs=..., buf=0x7fffeb180000, len=2) at ../softmmu/physmem.c:2870
#9  0x0000555555d3205b in address_space_read_full (as=0x5555567a6b00 <address_space_io>, addr=3324, attrs=..., buf=0x7fffeb180000, len=2) at ../softmmu/physmem.c:2883
#10 0x0000555555d32187 in address_space_rw (as=0x5555567a6b00 <address_space_io>, addr=3324, attrs=..., buf=0x7fffeb180000, len=2, is_write=false) at ../softmmu/physmem
```

tcg 的 style:
```c
/*
#0  pci_host_config_read_common (pci_dev=0x5555570c6c00, addr=2147483648, limit=1479011232, len=21845) at ../hw/pci/pci_host.c:88
#1  0x0000555555a49a17 in pci_data_read (s=0x5555570c6c00, addr=2147483648, len=2) at ../hw/pci/pci_host.c:133
#2  0x0000555555a49b51 in pci_host_data_read (opaque=0x555556c48e00, addr=0, len=2) at ../hw/pci/pci_host.c:178
#3  0x0000555555ca681c in memory_region_read_accessor (mr=0x555556c49210, addr=0, value=0x7fffe888db80, size=2, shift=0, mask=65535, attrs=...) at ../softmmu/memory.c:440
#4  0x0000555555ca6d1c in access_with_adjusted_size (addr=0, value=0x7fffe888db80, size=2, access_size_min=1, access_size_max=4, access_fn=0x555555ca67d6 <memory_region_read_accessor>, mr=0x555556c49210, attrs=...) at ../softmmu/memory.c:550
#5  0x0000555555ca9a38 in memory_region_dispatch_read1 (mr=0x555556c49210, addr=0, pval=0x7fffe888db80, size=2, attrs=...) at ../softmmu/memory.c:1427
#6  0x0000555555ca9b0e in memory_region_dispatch_read (mr=0x555556c49210, addr=0, pval=0x7fffe888db80, op=MO_16, attrs=...) at ../softmmu/memory.c:1455
#7  0x0000555555d332d7 in address_space_lduw_internal (as=0x5555567a6b00 <address_space_io>, addr=3324, attrs=..., result=0x0, endian=DEVICE_NATIVE_ENDIAN) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:214
#8  0x0000555555d333cc in address_space_lduw (as=0x5555567a6b00 <address_space_io>, addr=3324, attrs=..., result=0x0) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:246
#9  0x0000555555b45861 in helper_inw (env=0x555556c94340, port=3324) at ../target/i386/tcg/sysemu/misc_helper.c:48
#10 0x00007fff540080ff in code_gen_buffer ()
```

tcg 的 style : io_readx
```c
/*
>>> bt
#0  flatview_read_continue (fv=0x0, addr=384, attrs=..., ptr=0x7fffe888d8b0, len=0, addr1=93825027792176, l=1, mr=0x0) at ../softmmu/physmem.c:2818
#1  0x0000555555d31fce in flatview_read (fv=0x7ffdcc4e0850, addr=4273946630, attrs=..., buf=0x7fffe888d8b0, len=1) at ../softmmu/physmem.c:2870
#2  0x0000555555d31306 in subpage_read (opaque=0x7ffdcc52f420, addr=6, data=0x7fffe888d908, len=1, attrs=...) at ../softmmu/physmem.c:2453
#3  0x0000555555ca692e in memory_region_read_with_attrs_accessor (mr=0x7ffdcc52f420, addr=6, value=0x7fffe888da78, size=1, shift=0, mask=255, attrs=...) at ../softmmu/memory.c:462
#4  0x0000555555ca6d1c in access_with_adjusted_size (addr=6, value=0x7fffe888da78, size=1, access_size_min=1, access_size_max=8, access_fn=0x555555ca68ca <memory_region_read_with_attrs_accessor>, mr=0x7ffdcc52f420, attrs=...) at ../softmmu/memory.c:550
#5  0x0000555555ca9a76 in memory_region_dispatch_read1 (mr=0x7ffdcc52f420, addr=6, pval=0x7fffe888da78, size=1, attrs=...) at ../softmmu/memory.c:1433
#6  0x0000555555ca9b0e in memory_region_dispatch_read (mr=0x7ffdcc52f420, addr=6, pval=0x7fffe888da78, op=MO_8, attrs=...) at ../softmmu/memory.c:1455
#7  0x0000555555c6cb1b in io_readx (env=0x555556d66880, iotlbentry=0x7ffdcc01d6b0, mmu_idx=2, addr=4273946630, retaddr=140734603597128, access_type=MMU_DATA_LOAD, op=MO_8) at ../accel/tcg/cputlb.c:1359
#8  0x0000555555c6dfb9 in load_helper (env=0x555556d66880, addr=4273946630, oi=2, retaddr=140734603597128, op=MO_8, code_read=false, full_load=0x555555c6e19c <full_ldub_mmu>) at ../accel/tcg/cputlb.c:1914
#9  0x0000555555c6e1e6 in full_ldub_mmu (env=0x555556d66880, addr=4273946630, oi=2, retaddr=140734603597128) at ../accel/tcg/cputlb.c:1972
#10 0x0000555555c6e21e in helper_ret_ldub_mmu (env=0x555556d66880, addr=4273946630, oi=2, retaddr=140734603597128) at ../accel/tcg/cputlb.c:1978
```

在 [dam](#dma) 中，还有一个类似 backtrace, 其实总是到达 memory_region_dispatch_read, 而到达之前总是通过各种方法获取
mr 而已，在 kvm_handle_io 中经过了 as 到 flatview 再到 mr 的过程，在 io_readx 中几乎立刻到达，这是因为 iotlb 存储了一个地址对应的 mr

#### AddressSpaceDispatch dispatch 的过程: flatview_translate
到达 memory_region_dispatch_read 的过程中是，有一个关键的步骤是给定 AddressSpace 以及 addr 获取 memory_region
这就是靠 flatview_translate

- flatview_translate
  - flatview_do_translate : 返回 MemoryRegionSection
    - address_space_translate_internal
       - address_space_lookup_region : 通过 AddressSpaceDispatch 进行分析下去了

address_space_translate_internal 中，计算了一个关键的返回值 xlat, 表示在 MemoryRegion 中的偏移。

## 神奇的 memory_region_get_flatview_root
这个函数，其实有点硬编码, 参考其中的注释，感觉这个东西就是为了实现处理 PCIDevice 的

之所以创建这个函数，是为了更好的共享让不同的 memory_region 共享 flatview

返回值 mr = system
参数 ori = bus master container
```c
/*
address-space: e1000
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

```c
static MemoryRegion *memory_region_get_flatview_root(MemoryRegion *mr)
{
    while (mr->enabled) {
        if (mr->alias) {
            if (!mr->alias_offset && int128_ge(mr->size, mr->alias->size)) {
                /* The alias is included in its entirety.  Use it as
                 * the "real" root, so that we can share more FlatViews.
                 */
                mr = mr->alias;
                continue;
            }
        } else if (!mr->terminates) {
            unsigned int found = 0;
            MemoryRegion *child, *next = NULL;
            QTAILQ_FOREACH(child, &mr->subregions, subregions_link) {
                if (child->enabled) {
                    // 如果发现了多个 child, 一定会返回 return mr
                    if (++found > 1) {
                        next = NULL;
                        break;
                    }
                    if (!child->addr && int128_ge(mr->size, child->size)) {
                        /* A child is included in its entirety.  If it's the only
                         * enabled one, use it in the hope of finding an alias down the
                         * way. This will also let us share FlatViews.
                         */
                        next = child;
                    }
                }
            }
            if (found == 0) {
                return NULL;
            }
            if (next) {
                // 这种情况就是上面的注释说明的，只有一个 child, 那么就像是 flatview 的工作方式了
                mr = next;
                continue;
            }
        }

        return mr;
    }

    return NULL;
}
```

## alias
machine_run_board_init 中初始化 `machine->ram`, 也就是 pc.ram 这个 memory region

而分析 system 这个 memory region, 发现其中的两个 subregion ram-below-4g 和 ram-above-4g 都是
是 pc.ram 的 alias.  也即是一个 memory region 的 subregion 可以是其他的 alias

```c
/*
address-space: memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-00000000bfffffff (prio 0, ram): alias ram-below-4g @pc.ram 0000000000000000-00000000bfffffff
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff

memory-region: pc.ram
  0000000000000000-000000017fffffff (prio 0, ram): pc.ram
```
仔细想想，这么设计是很有道理的, 这样，一块物理内存是作为一个 MemoryRegion，拥有相同的属性，
而 system memory 是实际上物理内存中存在空洞的。

此外，alias 的一个例子在 isa 地址空间:
```c
/*
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
*/
```

- 现在思考一个问题，如何保证访问 alias 的时候，最后获取的是正确的地址:
  - memory_region_get_ram_ptr 中存在一个转换

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
在 [^6] 分析了下为什么 guest 需要 vIOMMU

pci_device_iommu_address_space : 如果一个 device 被用于直通，那么其进行 IO 的 address space 就不再是 address_space_memory 的，而是需要经过一层映射。

参考 [oracle 的 blog](https://blogs.oracle.com/linux/post/a-study-of-the-linux-kernel-pci-subsystem-with-qemu)

> Nowadays, IOMMU is always used by baremetal machines. QEMU is able to emulate IOMMU to help developers debug and study the Linux kernel IOMMU relevant source code, e.g., how DMA Remapping and Interrupt Remapping work.

blog 并且告诉了 iommu 打开的方法 : `-device intel-iommu` + `-machine q35`

## PCI Device AddressSpace
- PCI 设备分配空间上是 mmio 和 pio 的, 都是在 PCI 空间的，而不是 DMA 空间的

1. PCIDevice 关联了一个 AddressSpace, PCIDevice::bus_master_as, 其使用位置为
  - msi_send_message
  - pci_get_address_space : 当 pci_dma_rw 进行操作的时候需要获取当时的地址空间了
```c
/*
>>> bt
#0  pci_get_address_space (dev=0x555557d55110) at /home/maritns3/core/kvmqemu/include/hw/pci/pci.h:786
#1  pci_dma_rw (dir=DMA_DIRECTION_TO_DEVICE, len=64, buf=0x7fffffffd210, addr=3221082112, dev=0x555557d55110) at /home/maritns3/core/kvmqemu/include/hw/pci/pci.h:807
#2  pci_dma_read (len=64, buf=0x7fffffffd210, addr=3221082112, dev=0x555557d55110) at /home/maritns3/core/kvmqemu/include/hw/pci/pci.h:825
#3  nvme_addr_read (n=0x555557d55110, addr=3221082112, buf=0x7fffffffd210, size=64) at ../hw/nvme/ctrl.c:377
#4  0x0000555555955179 in nvme_process_sq (opaque=opaque@entry=0x555557d58908) at ../hw/nvme/ctrl.c:5514
#5  0x0000555555e71d38 in timerlist_run_timers (timer_list=0x55555670a060) at ../util/qemu-timer.c:573
#6  timerlist_run_timers (timer_list=0x55555670a060) at ../util/qemu-timer.c:498
#7  0x0000555555e71f47 in qemu_clock_run_all_timers () at ../util/qemu-timer.c:669
#8  0x0000555555e4ed89 in main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:542
#9  0x0000555555c5a1f1 in qemu_main_loop () at ../softmmu/runstate.c:726
#10 0x0000555555940c92 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```


想要构建如下的结构，分别发生在: pci_init_bus_master 和 do_pci_register_device
```c
/*
address-space: e1000
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

```c
struct PCIDevice {
    // ...
    PCIIORegion io_regions[PCI_NUM_REGIONS];
    AddressSpace bus_master_as;                // 因为 IOMMU 的存在，所以有
    MemoryRegion bus_master_container_region;  // container
    MemoryRegion bus_master_enable_region;     // 总是 bus_master_as->root 的 alias, 是否 enable 取决于运行时操作系统对于设备的操作
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

#### PCIIORegion
和 bus_master_as / bus_master_container_region / bus_master_enable_region 区分的是，这个就是设备的配置空间
最后都是放到 system_memory / system_io 上的

## MemoryRegionSection and RCU
[^4] 中间提到了一个非常有意思的事情，将 MemoryRegion 的 inaccessible 和 destroy 划分为两个阶段
所以使用 rcu, 其中涉及到
- memory_region_destroy / memory_region_del_subregion
- hotplug

## address_space_map 和 address_space_unmap

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
在使用 --accel=tcg 的时候，可以简化为这个

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

## dma
主要出现的文件: include/sysemu/dma.h 和 softmmu/dma-helpers.c

- dma 最重要的客户还是 pci 产生的地址空间, 其中 dma_blk_io 和 dma_buf_read 之类的都是 DMA 和 scsi / nvme 相关的, 是一个很容易的封装。
- dma_memory_rw 另一个用户当然是 fw_cfg，另一个就是 pci_dma_rw 了
- 实际上，fw_cfg 选择的 as 总是 address_space_memory 的，我甚至感觉根本没有任何必要让 fw_cfg 来走 dma

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

```c
/*
#0  flatview_read_continue (fv=0x0, addr=655360, attrs=..., ptr=0x7fffe888d7a0, len=93825001741418, addr1=93825012630272, l=16, mr=0x0) at ../softmmu/physmem.c:2818
#1  0x0000555555d31fce in flatview_read (fv=0x7ffdcc06d2e0, addr=28476, attrs=..., buf=0x7fffe888d9c0, len=16) at ../softmmu/physmem.c:2870
#2  0x0000555555d3205b in address_space_read_full (as=0x5555567a6b60 <address_space_memory>, addr=28476, attrs=..., buf=0x7fffe888d9c0, len=16) at ../softmmu/physmem.c:2883
#3  0x0000555555d32187 in address_space_rw (as=0x5555567a6b60 <address_space_memory>, addr=28476, attrs=..., buf=0x7fffe888d9c0, len=16, is_write=false) at ../softmmu/physmem.c:2911
#4  0x00005555559171ef in dma_memory_rw_relaxed (as=0x5555567a6b60 <address_space_memory>, addr=28476, buf=0x7fffe888d9c0, len=16, dir=DMA_DIRECTION_TO_DEVICE) at /home/maritns3/core/kvmqemu/include/sysemu/dma.h:88
#5  0x000055555591723c in dma_memory_rw (as=0x5555567a6b60 <address_space_memory>, addr=28476, buf=0x7fffe888d9c0, len=16, dir=DMA_DIRECTION_TO_DEVICE) at /home/maritns 3/core/kvmqemu/include/sysemu/dma.h:127
#6  0x0000555555917274 in dma_memory_read (as=0x5555567a6b60 <address_space_memory>, addr=28476, buf=0x7fffe888d9c0, len=16) at /home/maritns3/core/kvmqemu/include/sysemu/dma.h:145
#7  0x0000555555918732 in fw_cfg_dma_transfer (s=0x555556edda00) at ../hw/nvram/fw_cfg.c:360
#8  0x0000555555918b73 in fw_cfg_dma_mem_write (opaque=0x555556edda00, addr=4, value=28476, size=4) at ../hw/nvram/fw_cfg.c:469
#9  0x0000555555ca6ae5 in memory_region_write_accessor (mr=0x555556eddd80, addr=4, value=0x7fffe888db18, size=4, shift=0, mask=4294967295, attrs=...) at ../softmmu/memo ry.c:489
#10 0x0000555555ca6cc2 in access_with_adjusted_size (addr=4, value=0x7fffe888db18, size=4, access_size_min=1, access_size_max=8, access_fn=0x555555ca69f8 <memory_region_write_accessor>, mr=0x555556eddd80, attrs=...) at ../softmmu/memory.c:545
#11 0x0000555555ca9de3 in memory_region_dispatch_write (mr=0x555556eddd80, addr=4, data=28476, op=MO_32, attrs=...) at ../softmmu/memory.c:1507
#12 0x0000555555d3367a in address_space_stl_internal (as=0x5555567a6b00 <address_space_io>, addr=1304, val=1013907456, attrs=..., result=0x0, endian=DEVICE_NATIVE_ENDIAN) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:319
#13 0x0000555555d33775 in address_space_stl (as=0x5555567a6b00 <address_space_io>, addr=1304, val=1013907456, attrs=..., result=0x0) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:350
#14 0x0000555555b458a8 in helper_outl (env=0x555556d66880, port=1304, data=1013907456) at ../target/i386/tcg/sysemu/misc_helper.c:54
```

- dma 和 cpu_physical_memory_read 的关系是什么
    - 从代码逻辑上，cpu_physical_memory_read 走的 `as` 是 `address_space_memory`,  而 dma_memory_rw 是可以指定自己的 address space 的
    - 两者最后都是调用 address_space_rw 的
    - 感觉从当前的配置，实际上，dma 采用 as 显然也是 address_space_memory


## FlatRange 和 MemoryRegionSection
- section_from_flat_range : 很简单的封装

```c
static inline MemoryRegionSection section_from_flat_range(FlatRange *fr, FlatView *fv)
{
    return (MemoryRegionSection) {
        .mr = fr->mr,
        .fv = fv,
        .offset_within_region = fr->offset_in_region,
        .size = fr->addr.size,
        .offset_within_address_space = int128_get64(fr->addr.start),
        .readonly = fr->readonly,
        .nonvolatile = fr->nonvolatile,
    };
}
```
实际上，这个函数的调用者几乎就是 memory listener 了

- FlatView 持有了一堆 FlatRange，用于生成 MemoryRegionSection 插入到 AddressSpaceDispatch

## flatview_read
三个调用者:
- subpage_read : 注意 AddressSpaceDispatch 中构建的 tree 实际上只是针对于 PAGE_SIZE 大小的页面的，但是实际上，所以对于 subpage 需要重新处理，这些 subpage 都是 MMIO 的
- address_space_read_full : 只有唯一调用者 address_space_rw
- address_space_map

所以，如此看来，当不是很清楚到底是 RAM 还是 ROM 的时候，最后就会到 flatview_read 上。

那么继续向上，分析 address_space_rw 的调用者
- dma : QEMU 中的 DMA 是模拟设备的操作，其操作对象不应该是 MMIO 的，不然就是一个设备直接搬运数据到另一个设备空间，但是，那是 MMIO 空间啊.
- cpu_physical_memory_rw : CPU 调用，那么 CPU 应该是知道自己调用的位置的。

我认为，进行的是 IO 空间还是 memory 空间, 应该很早就可以发现, 而不是推迟到 flatview_read_continue 中间。

总结: flatview_read 是一个失败的设计，没有尽早区分 MMIO 和 RAM，其调用者，现在唯一一个无法区分的调用者
就是 cpu_physical_memory_rw 的位置了, 而这个相关的函数现在还是不存在的。

## Option ROM
https://en.wikipedia.org/wiki/Option_ROM

> 这下好了，似乎没有 option ROM，连 VGA 都是没有的了

好像是 vga.rom 和 e1000.rom 两个都需要的:

```txt
huxueshi:pci_update_mappings pci febe0000 vga.rom
huxueshi:pci_update_mappings pci feb80000 e1000.rom
```

- [x] 更新了，为什么在地址空间中间看不到 pci 的 rom 啊
这个地址之后被隐藏了
```txt
Option rom sizing returned febe0000 ffff0000
huxueshi:map_pcirom 0xfebe0000
```
最开始的时候，将 ROM 映射到 PCI 空间中，然后拷贝到 ROM 中，然后更新 PCI 空间, 这个 ROM 被隐藏起来了。

```txt
huxueshi:pci_add_option_rom /home/maritns3/core/kvmqemu/build/pc-bios/vgabios-stdvga.bin
huxueshi:ram_block_add vga.rom
```
vga 的源代码应该是在 : https://github.com/qemu/vgabios

## subpage : 处理 pio
从上面的构建，其实主要都是处理 page 的

通过 phys_page_find 只是可以 MemoryRegion 都是映射整个 page 的

```c
/* Called from RCU critical section */
static MemoryRegionSection *address_space_lookup_region(AddressSpaceDispatch *d,
                                                        hwaddr addr,
                                                        bool resolve_subpage)
{
    MemoryRegionSection *section = atomic_read(&d->mru_section);
    subpage_t *subpage;

    // 1: 此处保证命中 section
    if (!section || section == &d->map.sections[PHYS_SECTION_UNASSIGNED] || !section_covers_addr(section, addr)) {
        section = phys_page_find(d, addr);
        atomic_set(&d->mru_section, section);
    }

    // 2. 此时 section 指向的 mr 是在 subpage_init 的时候创建的
    if (resolve_subpage && section->mr->subpage) {
        subpage = container_of(section->mr, subpage_t, iomem);
        // 使用页内偏移获取真正的 section
        section = &d->map.sections[subpage->sub_section[SUBPAGE_IDX(addr)]];
    }
    return section;
}
```

```c
typedef struct subpage_t {
    MemoryRegion iomem;
    FlatView *fv;
    hwaddr base;
    uint16_t sub_section[]; // 在 subpage_register 的地方注册
} subpage_t;
```

```c
/*
#0  subpage_read (opaque=0x7fff6068bb60, addr=0, data=0x7fffe17f5ab8, len=1, attrs=...) at /home/maritns3/core/xqm/exec.c:2754
#1  0x0000555555881f26 in memory_region_read_with_attrs_accessor (mr=mr@entry=0x7fff6068bb60, addr=0, value=value@entry=0x7fffe17f5c00, size=size@entry=1, shift=0, mask=mask@entry=255, attrs=...) at /home/maritns3/core/xqm/memory.c:456
#2  0x0000555555881d5e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffe17f5c00, size=size@entry=1, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555881ed0 <memory_region_read_with_attrs_accessor>, mr=0x7fff6068bb60, attrs=...) at /home/maritns3/core/xqm/memory.c:544
#3  0x0000555555886231 in memory_region_dispatch_read1 (attrs=..., size=<optimized out>, pval=0x7fffe17f5c00, addr=0, mr=0x7fff6068bb60) at /home/maritns3/core/xqm/memory.c:1395
#4  memory_region_dispatch_read (mr=mr@entry=0x7fff6068bb60, addr=addr@entry=0, pval=pval@entry=0x7fffe17f5c00, op=op@entry=MO_8, attrs=...) at /home/maritns3/core/xqm/memory.c:1423
#5  0x000055555589841e in io_readx (env=env@entry=0x555556810f50, mmu_idx=mmu_idx@entry=2, addr=addr@entry=4273938432, retaddr=retaddr@entry=140735006261128, access_typ
```


[^1]: 关键参考: https://www.anquanke.com/post/id/86412
[^3]: https://wiki.osdev.org/System_Management_Mode
[^4]: https://www.linux-kvm.org/images/1/17/Kvm-forum-2013-Effective-multithreading-in-QEMU.pdf
[^5]: https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/08/11/dirty-pages-tracking-in-migration
[^6]: https://wiki.qemu.org/Features/VT-d
[^8]: [official doc](https://qemu.readthedocs.io/en/latest/devel/memory.html)
