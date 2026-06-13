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
