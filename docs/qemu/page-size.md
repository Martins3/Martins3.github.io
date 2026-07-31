## qemu 中关于 page size 问题的合集
<!-- 074a3276-94f9-48c9-83ee-2b05d6b05668 -->

(2026-04-21 其实已经差不多了，现在有了 codex ，这个问题应该是很简单的)

1. TARGET_PAGE_BITS 是如何确定的?
2. 虚拟机和物理机的页大小不同 (虚拟机是 16k ，物理机是 4k 的页面)
3. 虚拟机中使用大页，但是物理机中不是
4. 哪些问题是二进制翻译特有的，哪些问题是 KVM 特有的?
  - kvm 的 stage 2 page table 还有自己独特的 tlb size 的
5. pss_host_page_prepare 中，为什么热迁移需要考虑这个问题，还是说，这个是给 loadvm 用的

包括，需要对比一下，ram_save_host_page 和 ram_save_target_page 的区别是什么?

非当 tcg 模式下，还存在这个问题吗，也就是同构场景有没有这个问题、

## 一共存在那些 page size

### RAMBlock::page_size

如果启动 qemu 的时候，后端使用的是大页，可以发现其结果 PSize 就是 2MiB 的
```txt
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0    2 MiB  0x0000000000000000 0x0000000300000000 0x0000000300000000 0x00007fe9c7c00000  rw
 0000:00:0d.0/gpu-fb-mem    4 KiB  0x0000000300100000 0x0000000001000000 0x0000000001000000 0x00007fe9aac00000  rw
    /rom@etc/acpi/tables    4 KiB  0x0000000301100000 0x0000000000020000 0x0000000000200000 0x00007fe9b4400000  ro
                 pc.bios    4 KiB  0x0000000300000000 0x0000000000040000 0x0000000000040000 0x00007fecc7e00000  ro
0000:00:05.0/virtio-net-pci.rom    4 KiB  0x0000000300080000 0x0000000000040000 0x0000000000040000 0x00007fe9c5600000  ro
0000:00:06.0/virtio-net-pci.rom    4 KiB  0x00000003000c0000 0x0000000000040000 0x0000000000040000 0x00007fe9c5400000  ro
                  pc.rom    4 KiB  0x0000000300040000 0x0000000000020000 0x0000000000020000 0x00007fe9c6200000  ro
   /rom@etc/table-loader    4 KiB  0x0000000301300000 0x0000000000001000 0x0000000000010000 0x00007fe9b4200000  ro
      /rom@etc/acpi/rsdp    4 KiB  0x0000000301340000 0x0000000000001000 0x0000000000001000 0x00007fe9abe00000  ro
```

那么 thp 如何办?

总结的很好了:
```txt
• RAMBlock::page_size 可以理解成“这块 guest RAM 在 host 上的实际 backing granularity”。它记录的不是 guest CPU 的页大小，而是这块 RAMBlock 对应的
  host 内存后端最小可操作页大小。

  它的来源很直接：

  - RAMBlock 定义里有 size_t page_size，见 include/system/ramblock.h:25
  - 文件/hugetlb 后端创建时，QEMU 在 system/physmem.c:1724 里赋值 block->page_size = qemu_fd_getpagesize(fd);
  - 匿名 RAM 则直接设成 qemu_real_host_page_size()，见 system/physmem.c:2541
  - qemu_fd_getpagesize() 对 hugetlbfs 返回 statfs.f_bsize，否则返回普通 host page size，见 util/mmap-alloc.c:60

  为什么 QEMU 需要关心它，原因是 QEMU 不是只“模拟页表”，它还真的在 host 上管理这些内存映射。host 内核对 mmap / madvise / munmap / fallocate /
  fault / discard 的约束，都是按 host backing page 来的，不按 guest page 来。

  主要用途有这几类：

  - 建立 RAMBlock 时做正确的对齐和大小检查。QEMU 要保证 align、offset、memory size 都是 page_size 的整数倍，否则映射本身就不合法，见 system/
    physmem.c:1724
  - 做 discard / balloon / poisoned-page recovery 时按正确粒度操作。比如 system/physmem.c:4094 强制要求地址和长度按 rb->page_size 对齐，而且会根据
    rb->page_size == qemu_real_host_page_size() 决定能不能走 madvise，hugetlb 场景更多要走 fallocate
  - 做单页 remap 时必须尊重 RAMBlock 的 backing page size。QEMU 在 system/physmem.c:2656 里先取 qemu_ram_pagesize(block)，再按这个粒度向下对齐和重
    建映射
  - virtio-balloon/virtio-mem 这类按页回收、热插拔的设备逻辑，需要知道 host 侧真实粒度。比如 balloon 遇到 rb_page_size > 4K 时，要先累计完整一个
    host page 才能 discard，见 hw/virtio/virtio-balloon.c:92。virtio-mem 也要求 block size 不能小于 qemu_ram_pagesize(rb)，见 hw/virtio/virtio-
    mem.c:966
  - 某些机器/KVM 接口对 host backing page size 有约束，所以 QEMU 要统计最小/最大 RAM page size。见 system/physmem.c:1530，以及 s390/spapr 的检查
    hw/s390x/s390-virtio-ccw.c:207 和 hw/ppc/spapr.c:3671

  一个关键区分是：

  - TARGET_PAGE_SIZE：guest/target 架构页大小，见 include/exec/target_page.h:42
  - qemu_real_host_page_size()：宿主普通页大小
  - RAMBlock::page_size：某个具体 RAMBlock 的 backing page size，可能是 4K，也可能是 hugetlb 的 2M/1G

  所以 RAMBlock::page_size 的本质用途不是“告诉 guest 页有多大”，而是告诉 QEMU：这块 host RAM 能以多大的最小粒度被映射、打洞、回收、重建和约束检查。
```

### qemu_real_host_page_size()

### TARGET_PAGE_SIZE

TARGET_PAGE_SIZE 不是总动态的。也就是基本上是静态的，我的天啊

它的典型用途是：
- physmem 里的页表、dirty bitmap、地址空间切分都按 TARGET_PAGE_SIZE 做，见 system/physmem.c:326 和 system/physmem.c:1227
- IOMMU 如果设备自己没给更细粒度，默认也回退到 TARGET_PAGE_SIZE，见 system/memory.c:1891

原来 TARGET_PAGE_SIZE 是写死的，既然如此，那么我就真的感觉到很奇怪了，
那么岂不是可以无视 guest 内核的 page size ?

## TODO
1. 似乎非常接近热迁移了，但是我来理解一下，为什么会有这种情况
	- dirty bitmap 为什么需要是热迁移的情况
2. 需要注意，在热迁移的时候，自动转换为 4k 页面的

## 源码分析

基于 qemu master (05e27e70df42) 的源码结论，对应回答上面的问题。

### TARGET_PAGE_BITS 是如何确定的

固定值来自各 target 的 `cpu-param.h`，不是 meson 配置。meson 会把 `configs/targets/*.mak` 的键值写进 `<target>-config-target.h`（meson.build:3365），但 `.mak` 里只有 `TARGET_ARCH` 等，没有 page bits。per-target 编译单元定义 `-DCOMPILING_PER_TARGET`（meson.build:3950）后，include/exec/target_page.h:21-24 直接包含 `cpu-param.h` 取编译期常量：

- x86 固定 12（target/i386/cpu-param.h:21）
- sparc 13 或 12、mips 12、hppa 固定 12
- arm softmmu 是**可变的**：target/arm/cpu-param.h:25-30 定义 `TARGET_PAGE_BITS_VARY` 和 `TARGET_PAGE_BITS_LEGACY 10`（ARMv5/v6 要支持 1K tiny pages）

可变架构的运行时机制：

- 全局 `TargetPageBits target_page`（page-vary-common.c:28），`set_preferred_target_page_bits()` 只能往小调，`finalize_target_page_bits()` 提交后不可再改
- `TargetInfo` 里由 configs/targets/aarch64-softmmu.c:23 设 `page_bits_vary=true`；riscv64-softmmu.c:23 这类不可变的只设 `page_bits_init`
- 非 per-target 的通用代码（没有 `COMPILING_PER_TARGET`）走 include/exec/target_page.h:26-28 的 VARY 路径，通过全局变量取值；下限 `TARGET_PAGE_BITS_MIN 9` 在 include/exec/page-vary.h:35
- system 模式的 finalize 发生在 `machine_memory_init()`（system/physmem.c:3603）；arm virt/sbsa-ref/vmapple 机器类设 `minimum_page_bits = 12`（hw/arm/virt.c:4181）；ARM CPU realize 时再按 feature 调：v7 VMSA 用 12，ARMv5 用 10（target/arm/cpu.c:2122-2150）
- user 模式直接把 host page size 当 preferred（linux-user/main.c:811，注释说 match host 最有效率）

只有 arm/aarch64（softmmu + user）真正可变；alpha/ppc/loongarch 仅 user 模式可变，softmmu 固定 12。

可变架构的热迁移兼容：migration/savevm.c:460-478 有 `configuration/target-page-bits` 小节，加载端比对（savevm.c:393）。

### TARGET_PAGE_SIZE 是写死的，那岂不是可以无视 guest 内核的 page size？

是的，而且这正是设计：

- arm guest 的 4k/16k/64k 是 **translation granule**（guest 自己写 TCR 配置），不是 QEMU 的 target page size。TCG 的 page walk 按 granule 处理（target/arm/ptw.c:398-407），但 QEMU 内部软 TLB 永远以 TARGET_PAGE_SIZE 为单位（target/arm/ptw.c:569 注释原话 "the softmmu tlb only works on units of TARGET_PAGE_SIZE"）。guest 16k 页 walk 出 `lg_page_size = 14` 后，accel/tcg/cputlb.c:1041-1061 把它拆成 TARGET_PAGE_SIZE 粒度的多个 TLB entry
- KVM 下 QEMU 完全不感知 guest granule，由内核 stage2 处理

所以 "TARGET_PAGE_SIZE 无视 guest 内核 page size" 不是缺陷，是 QEMU 内部管理粒度与 guest 页表语义的解耦。

### qemu_real_host_page_size()

- 定义在 include/qemu/osdep.h:780，就是内联返回 `getpagesize()`；`qemu_real_host_page_mask()` 在 :785
- 旧的 `qemu_host_page_size`/`qemu_host_page_mask` 全局变量只剩 bsd-user 还在用（bsd-user/main.c:61），system 模式已统一成函数
- 有 fd 时用 `qemu_fd_getpagesize(fd)`（util/mmap-alloc.c:60）：fstatfs 后若 `f_type == HUGETLBFS_MAGIC` 返回 `f_bsize`（即该 hugetlbfs 挂载点的大页尺寸），否则返回 `qemu_real_host_page_size()`
- 三层概念分清：`TARGET_PAGE_SIZE`（guest 管理粒度，编译/运行时确定）、`qemu_real_host_page_size()`（host 普通页，getpagesize）、`RAMBlock::page_size`（该块实际 backing 粒度，可能是 2M/1G 大页）

### RAMBlock::page_size 的来源链

- 读取口：`qemu_ram_pagesize()` 直接返回 `rb->page_size`（system/physmem.c:1984）
- 文件/大页后端：`file_ram_alloc()`（physmem.c:1708）在 :1718 设 `block->page_size = qemu_fd_getpagesize(fd)`，并校验 align/offset 必须是它的倍数
- 匿名内存：`qemu_ram_alloc_internal()` 在 physmem.c:2522 以 `MAX(qemu_real_host_page_size(), TARGET_PAGE_SIZE)` 对齐，:2534 设 `page_size = qemu_real_host_page_size()`
- mem-path 大页没有独立的 hugetlb 属性，完全由 mem-path 指向的文件系统类型决定（backends/hostmem-file.c 头部注释即 "Host Memory Backend for hugetlbfs"）；`share` 属性只影响 MAP_SHARED，不影响 page_size
- 机器级聚合 `find_min/max_backend_pagesize`（physmem.c:1488-1539），KVM 用 `maxrampagesize` 限制 slot 大小（accel/kvm/kvm-all.c:1595）

### 虚拟机 16k 页 / 物理机 4k 页，怎么兼容

QEMU 侧没有 guest/host page size 一致性检查。KVM 注册内存时 userspace_addr 直接传给 `KVM_SET_USER_MEMORY_REGION`（kvm-all.c:388），section 对齐只用 host page（kvm_align_section，kvm-all.c:328）。16k guest 页拆成 4 个 4k host 页映射完全在内核 KVM stage2 里完成，target/arm/kvm.c 里没有任何 granule 处理。唯一的 QEMU 侧约束是断言 `TARGET_PAGE_SIZE <= qemu_real_host_page_size()`（kvm-all.c:2920）。

### 虚拟机用大页，物理机不是（THP)

- QEMU 对**所有匿名 RAM** 默认打 `QEMU_MADV_HUGEPAGE`（physmem.c:2275，ram_block_add 里），guest 的 2M 大页只是 guest 页表项，host 侧是否合成 2M THP 由内核决定，QEMU 只做 advise
- postcopy 迁移期间关掉 THP（migration/postcopy-ram.c:883 NOHUGEPAGE），结束后恢复（:724-728）
- TCG 代码缓冲也对 TB buffer 打 MADV_HUGEPAGE（tcg/region.c:763）

### TCG 特有 vs KVM 特有的问题

TCG 特有：

- 软 TLB 以 TARGET_PAGE_SIZE 索引：`tlb_index = (addr >> TARGET_PAGE_BITS) & size_mask`（cputlb.c:128）
- 旧版固定 `CPU_TLB_SIZE` 已不存在，快 TLB 大小存在 `CPUTLBDescFast->mask` 里按命中率动态 resize（cputlb.c:205）；victim TLB 固定 8 项
- `tb_jmp_cache` 按 TARGET_PAGE_SIZE 清页（cputlb.c:553）
- guest 大页（2M section 等）到 TLB 的拆分

KVM 特有：

- dirty log 粒度 = host page：`pages = slot->memory_size / qemu_real_host_page_size()`（kvm-all.c:886），注释原话 "the granule of kvm dirty log is qemu_real_host_page_size"（kvm-all.c:918）；`KVM_CLEAR_DIRTY_LOG` 还要求 64 页对齐
- stage2 / IOMMU 页粒度 QEMU 完全不处理，都在内核
- guest_memfd 私有/共享转换按 host page 对齐（kvm-all.c:3341）

两者共用的换算点：KVM 按 host 页粒度上报 dirty bitmap 后，QEMU 用 `hpratio = host/target` 展开进 TARGET_PAGE_SIZE 粒度的 dirty bitmap（system/physmem.c:1221，hpratio==1 走整块 OR 快路径，否则逐位展开）。

同构场景（host 4k + guest 4k，x86 on x86）这些问题基本退化：hpratio==1，host page == target page，ram_save_host_page 的循环一次只发一页。page size 问题本质上是 arm（可变 granule / 64k host 页）和大页后端引入的。

### 热迁移中的 host page 与 target page

回答 "ram_save_host_page 和 ram_save_target_page 的区别"：

- `ram_save_target_page`（migration/ram.c:2045）：发送**一个 target page**，粒度 TARGET_PAGE_SIZE
- `ram_save_host_page`（migration/ram.c:2216）：发送**一整个 host page**，先调 `pss_host_page_prepare`
	算出 host page 覆盖的 target page 区间，然后 do-while 逐 target page 调 ram_save_target_page
- 为什么需要 host page 外层循环，ram.c:2312 注释原话：

"On systems where host-page-size > target-page-size it will send all the pages in a host page that are dirty"。
根本原因是 postcopy：目标端必须**原子地放置整个 host page**（可能是大页），
所以源端必须把一个 host page 的所有 target page 连续发送（目标端注释 ram.c:3860）

回答 "pss_host_page_prepare 是热迁移还是 loadvm"：

- pss 是 `struct PageSearchStatus`（ram.c:127-144），热迁移发送侧状态，每个迁移 channel 一份（precopy 和 postcopy 各一个），与 loadvm 无关
- `pss_host_page_prepare`（ram.c:2076）：`guest_pfns = qemu_ram_pagesize(block) >> TARGET_PAGE_BITS`，把当前区间 ROUND 到 host page 对齐；guest_pfns<=1 时退化单页（注释还处理了 guest psize > host psize 的 Alpha-on-x86 情形）
- 它不是 postcopy 专用，precopy 主循环也调；但"必须存在"的动机是 postcopy 的原子放置
- 切 postcopy 前要先 canonicalize dirty bitmap，保证一个 host page 内要么全脏要么全干净：`postcopy_chunk_hostpages_pass`（ram.c:2603-2668）

回答 "热迁移时自动转换为 4k 页面"：

- 迁移协议和 dirty 跟踪**永远**以 TARGET_PAGE_SIZE 为粒度，不管 host 页是 4k/64k/2M。RAMBlock dirty bitmap `rb->bmap` 一位一个 target page（ram.c:2833）；全局 dirty_memory 也是 target page 粒度（physmem.c:896）；线路上的 flags 藏在 target page 对齐地址的低位（ram.c:4339）
- host page 只是"一次连续发送/原子放置的批次"，不是传输单位
- zero page 检测、XBZRLE 压缩、multifd 页粒度（`multifd_ram_page_size() = qemu_target_page_size()`，multifd.h:394）也全部是 target page 粒度。注意 multifd 弱化了 host page 语义，所以 multifd 和 postcopy 不能同时用

postcopy 的 page size 校验（两端必须一致）：

- 源端发送 RAMBlock 列表时，若 `block->page_size != MAX(host, target)`（即用了大页），额外发送 page_size（ram.c:3130）
- 目标端 parse_ramblock 比较，不等直接拒绝 "Mismatched RAM page size"（ram.c:4245-4257）
- userfaultfd 能力检查要求 `qemu_target_page_size() <= qemu_real_host_page_size()`（postcopy-ram.c:575）；有大页时要求内核支持 `UFFD_FEATURE_MISSING_HUGETLBFS`（postcopy-ram.c:514）

目标端恢复逻辑（ram_load_postcopy，ram.c:3809-3961）：

- 每收到一个 target page 先读进临时大页缓冲，凑满 `block->page_size / TARGET_PAGE_SIZE` 个才一次性 `postcopy_place_page`（UFFDIO_COPY，一次拷 `qemu_ram_pagesize(rb)` 大小，postcopy-ram.c:1648）
- 优化：`block->page_size == TARGET_PAGE_SIZE` 时不用临时缓冲，直接放置（ram.c:3918）

dirty ring：ring 项 offset 按 host page 解释（kvm-all.c:953-972），汇总时同样走 hpratio 展开。

### 总结：一共哪些 page size

1. `TARGET_PAGE_SIZE`：QEMU 内部管理粒度（软 TLB、dirty bitmap、迁移协议），编译期确定，arm 可运行时调小。与 guest 内核实际页大小无关
2. guest translation granule：arm 的 4k/16k/64k，guest 自己选，TCG 在 page walk 里感知，KVM 下完全不可见
3. `qemu_real_host_page_size()`：host 普通页，KVM dirty log 粒度、对齐断言的基准
4. `RAMBlock::page_size`：该块的 host backing 粒度（大页时 2M/1G），决定 discard/balloon/postcopy 放置的原子单位
5. multifd 包、XBZRLE、zero page 检测全部回到 TARGET_PAGE_SIZE 粒度

## host page size vs target page size

看了相关代码，核心原因是：脏页跟踪、零页检测、发送协议都是按 target page（4K）粒度工作的，
按整个 host page（比如 2M 大页）发送会浪费大量带宽。具体来说：

- 脏位图粒度是 4K:rb->bmap 和 KVM 侧的 dirty logging 都按 TARGET_PAGE_BITS 记录
  （migration_bitmap_clear_dirty 里 test_and_clear_bit(page, rb->bmap))。一个 2M 的 host page 里通常只
  有几个 4K 子页是脏的，整页发送会把大量干净数据也发过去，拖慢收敛。

- 零页检测按 4K 做:ram_save_target_page() 里的 save_zero_page() 能把一个全零的 4K 页压缩成一个 flag。
  按 host page 整体发就丢掉了这个优化（大页里混着零页很常见）。

- 迁移协议本身按 target page 寻址：流里的 page header 携带的 offset 以 TARGET_PAGE_SIZE 为单位，目的端
  按这个 offset 逐页放置。multifd、RDMA、postcopy 都建立在这个粒度上，改成 host page 粒度要动整个协议
  和目的端逻辑。

- 限速和并发需要在子页边界生效：代码注释里明确写了，大页中间发送了数据就调 migration_rate_limit()，否
  则一个 2M/1G 大页会一次性打满带宽；postcopy preempt 模式下也要在子页之间释放 bitmap_mutex。

那为什么还要以 host page 为单位迭代（而不是纯 4K 自由扫描）？因为 host page 边界仍然有意义：

- 把一个 host page 内所有脏子页连续发完，目的端才能用 THP/hugetlb 整块 backing，避免大页被拆碎；
- postcopy 的 userfaultfd 写保护是按 host page 粒度加的，所以结尾的 ram_save_release_protection() 必须
  在整个 host page 范围发完之后才一次性解除保护；
- migration_bitmap_clear_dirty() 的注释也说明：必须在发送 chunk 内任何页之前先清掉 KVM 侧的 dirty
  bitmap，否则下次 sync dirty log 会丢新写入。

一句话：以 host page 为扫描单位是为了内存布局和写保护的正确性，以 4K 为发送单位是为了带宽效率和协议兼容。


## 不是真的

- KVM 内核侧 dirty log（KVM_GET_DIRTY_LOG、dirty ring）的粒度是 host base page size，即 QEMU
  里的 qemu_real_host_page_size()。

- QEMU 自己的 RAM dirty bitmap 才按 TARGET_PAGE_SIZE / TARGET_PAGE_BITS 编号。
- QEMU 从 KVM 同步 dirty bitmap 时，会做粒度转换。

当前代码直接说明了这一点：

- accel/kvm/kvm-all.c:918：注释明确写着 KVM dirty log granule 是
  qemu_real_host_page_size()。

- accel/kvm/kvm-all.c:922：KVM bitmap 按 memory_size / host_page_size 分配。
- system/physmem.c:1221：同步到 QEMU bitmap 时计算：

hpratio = qemu_real_host_page_size() / TARGET_PAGE_SIZE;

如果 host page 比 target page 大，一个 KVM dirty bit 会展开成多个 QEMU dirty bits：

KVM bitmap：  1 bit / host page
                   │
                   ▼
QEMU bitmap： hpratio bits / host page
              1 bit / TARGET_PAGE_SIZE

例如 ARM64 64K host 上运行 TARGET_PAGE_BITS=12 的 QEMU：

- KVM：1 bit 表示 64 KiB；
- QEMU：1 bit 表示 4 KiB；
- 一个 KVM dirty bit 会使对应的 16 个 QEMU target pages 全部变脏。

在常见的 x86 4K host 上，host PAGE_SIZE == TARGET_PAGE_SIZE == 4K，两者碰巧一致，所以容易产
生“KVM 都按 TARGET_PAGE_BITS 记录”的印象。结论是：

> KVM 侧按 host page 粒度记录；QEMU 内部按 TARGET_PAGE_BITS 保存，二者在同步时转换。


### 有点这个意思，但是不是完全对的


• 仅针对 KVM dirty-log buffer，确实应该直接使用 qemu_real_host_page_size()——而 QEMU 当前就是这么做的。

  TARGET_PAGE_SIZE 是 QEMU 公共内存层的逻辑粒度，不能换成 host page size，因为两者解决的问题不同。

   概念                          含义                                      例子
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━
   TARGET_PAGE_SIZE              QEMU 为目标架构选择的最小内存管理粒度     ARMv7 通常 4 KiB
  ────────────────────────────  ────────────────────────────────────────  ──────────────────────────
   guest OS page size            Guest 当前实际配置的页大小                AArch64 可为 4K/16K/64K
  ────────────────────────────  ────────────────────────────────────────  ──────────────────────────
   qemu_real_host_page_size()    QEMU 进程所在宿主 OS 的 base page size    ARM64 host 可能是 64 KiB
  ────────────────────────────  ────────────────────────────────────────  ──────────────────────────
   RAMBlock page size            RAM backend 的实际映射粒度                2 MiB hugepage

  注意，TARGET_PAGE_SIZE 也不一定等于 guest OS 当前使用的 page size；它一般是目标架构需要支持的最小粒度。

  ### 为什么公共层不能直接用 host page

  假设：

  TARGET_PAGE_SIZE = 4 KiB
  host PAGE_SIZE   = 64 KiB

  KVM 只能报告：

  这个 64 KiB host page 脏了

  QEMU 将它转换成：

  16 个连续的 4 KiB target pages 都脏了

  即：

  hpratio = qemu_real_host_page_size() / TARGET_PAGE_SIZE;

  对应 system/physmem.c:1221。

  这样虽然会多迁移一些数据，但不会漏掉脏页。反过来，如果 QEMU 公共 dirty bitmap 也只有 64 KiB 粒度，那么设备模拟、TCG 等能
  够精确发现的 4 KiB 写入也只能被放大为 64 KiB。

  ### TARGET_PAGE_SIZE 还有很多非 KVM 用途

  它不仅服务 dirty logging，还决定：

  - QEMU physical address space 的查找和 subpage 分割，见 system/physmem.c:1348；
  - TCG TLB、翻译块边界和代码失效粒度；
  - QEMU 公共 RAM dirty bitmap 的 bit 含义；
  - RAM live migration 的传输和 bitmap 单位；
  - migration stream 两端的兼容性检查，见 migration/savevm.c:387。

  如果这些都使用 host page size，那么同一虚拟机在 4K host 和 64K host 上会具有不同的：

  - dirty bitmap 格式；
  - migration page 编号和数据块大小；
  - TCG 地址翻译粒度；
  - MemoryRegion dispatch 布局。

  这会让虚拟机模型被宿主机实现细节污染。

  ### 为什么选择较小的公共粒度

  QEMU 采用的是“公共层使用 target 最小粒度，各数据源向它转换”：

  KVM dirty log（host page）
                │ 展开
                ▼
  QEMU dirty bitmap（target page）
                ▲
                │ 直接标记
  TCG / DMA / 设备模拟

  粗粒度信息可以安全地展开成多个细粒度 dirty bits；细粒度信息一旦存进粗粒度 bitmap，就无法恢复。

  所以可以概括为：

  > qemu_real_host_page_size() 是宿主资源和 KVM ABI 粒度；TARGET_PAGE_SIZE 是 QEMU 虚拟机模型的公共、可迁移粒度。KVM 层使
  > 用前者，进入 QEMU 公共内存层后转换成后者。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
