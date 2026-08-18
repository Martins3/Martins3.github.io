# qemu 中关于 page size 问题总结
<!-- 074a3276-94f9-48c9-83ee-2b05d6b05668 -->

## 一共存在那些 page size

### RAMBlock::page_size

- 文件/hugetlb 后端创建时，里赋值 block->page_size = qemu_fd_getpagesize(fd);
- 匿名 RAM 则直接设成 qemu_real_host_page_size()

考虑这些，基本上就清晰了，就是为了考虑 64k 页面或者 hugetlbfs 这两种情况:
- virtio-balloon/virtio-mem 这类按页回收、热插拔的设备逻辑，需要知道 host 侧真实粒度。
- 如果映射 rom 的时候，需要利用 RamBlock::page_size 作为最小单位

如果启动 qemu 的时候，后端使用的是 HugeTLB ，可以发现其结果 PSize 就是 2MiB 的
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

### qemu_real_host_page_size()

qemu_real_host_page_size() 物理机页面大小

和操作系统有关:
```c
/* Using intptr_t ensures that qemu_*_page_mask is sign-extended even
 * when intptr_t is 32-bit and we are aligning a long long.
 */
static inline uintptr_t qemu_real_host_page_size(void)
{
    return getpagesize();
}
```

### TARGET_PAGE_SIZE

TARGET_PAGE_SIZE 基本上是静态的，在编译期间就是确定了:

它的典型用途是:
- physmem 里的页表、dirty bitmap、地址空间切分都按 TARGET_PAGE_SIZE 做

kvm_init 中存在这个限制:

```c
    /*
     * On systems where the kernel can support different base page
     * sizes, host page size may be different from TARGET_PAGE_SIZE,
     * even with KVM.  TARGET_PAGE_SIZE is assumed to be the minimum
     * page size for the system though.
     */
    assert(TARGET_PAGE_SIZE <= qemu_real_host_page_size());
```

qemu_create_machine 中来配置的 page size:
```c
    if (machine_class->minimum_page_bits) {
        if (!set_preferred_target_page_bits(machine_class->minimum_page_bits)) {
            /* This would be a board error: specifying a minimum smaller than
             * a target's compile-time fixed setting.
             */
            g_assert_not_reached();
        }
    }
```


#### ram_save_host_page -> ram_save_target_page
target page size 和 host page size 问题是完全考虑过的
```c
/* Should be called before sending a host page */
static void pss_host_page_prepare(PageSearchStatus *pss)
{
    /* How many guest pages are there in one host page? */
    size_t guest_pfns = qemu_ram_pagesize(pss->block) >> TARGET_PAGE_BITS;

    pss->host_page_sending = true;
    if (guest_pfns <= 1) {
        /*
         * This covers both when guest psize == host psize, or when guest
         * has larger psize than the host (guest_pfns==0).
         *
         * For the latter, we always send one whole guest page per
         * iteration of the host page (example: an Alpha VM on x86 host
         * will have guest psize 8K while host psize 4K).
         */
        pss->host_page_start = pss->page;
        pss->host_page_end = pss->page + 1;
    } else {
        /*
         * The host page spans over multiple guest pages, we send them
         * within the same host page iteration.
         */
        pss->host_page_start = ROUND_DOWN(pss->page, guest_pfns);
        pss->host_page_end = ROUND_UP(pss->page + 1, guest_pfns);
    }
}
```

target page size 和 host page size 问题是完全考虑过的
```c
/* Should be called before sending a host page */
static void pss_host_page_prepare(PageSearchStatus *pss)
{
    /* How many guest pages are there in one host page? */
    size_t guest_pfns = qemu_ram_pagesize(pss->block) >> TARGET_PAGE_BITS;

    pss->host_page_sending = true;
    if (guest_pfns <= 1) {
        /*
         * This covers both when guest psize == host psize, or when guest
         * has larger psize than the host (guest_pfns==0).
         *
         * For the latter, we always send one whole guest page per
         * iteration of the host page (example: an Alpha VM on x86 host
         * will have guest psize 8K while host psize 4K).
         */
        pss->host_page_start = pss->page;
        pss->host_page_end = pss->page + 1;
    } else {
        /*
         * The host page spans over multiple guest pages, we send them
         * within the same host page iteration.
         */
        pss->host_page_start = ROUND_DOWN(pss->page, guest_pfns);
        pss->host_page_end = ROUND_UP(pss->page + 1, guest_pfns);
    }
}
```

## 从 bitmap 的角度来看

### ram_list.dirty_memory

使用 TARGET_PAGE_SIZE
```c
void physical_memory_set_dirty_range(ram_addr_t start, ram_addr_t length,
                                         uint8_t mask)
{
    DirtyMemoryBlocks *blocks[DIRTY_MEMORY_NUM];
    unsigned long end, page;
    unsigned long idx, offset, base;
    int i;

    if (!mask && !xen_enabled()) {
        return;
    }

    end = TARGET_PAGE_ALIGN(start + length) >> TARGET_PAGE_BITS;
    page = start >> TARGET_PAGE_BITS;
```

### kvm dirty

KVMSlot.dirty_bmap（accel/kvm/kvm-all.c）粒度是 host 页大小：

```c
  /* kvm_slot_init_dirty_bitmap 的注释：
   * Note: the granule of kvm dirty log is qemu_real_host_page_size. */
  hwaddr bitmap_size = ALIGN(mem->memory_size / qemu_real_host_page_size(), 64) / 8;
```

QEMU 用 physical_memory_set_dirty_lebitmap()（system/physmem.c）把 KVM 的 host 粒度位图展开成 guest 粒度：

```c
  unsigned long hpratio = qemu_real_host_page_size() / TARGET_PAGE_SIZE;
  ...
  page_number = (i * HOST_LONG_BITS + j) * hpratio;
  physical_memory_set_dirty_range(ram_addr, TARGET_PAGE_SIZE * hpratio, clients);
```

即 1 个脏 host 页 → 展开为 hpratio 个 guest 页 bit。

### RAMBlock.bmap

```c
block->bmap = bitmap_new(pages);                 // pages = used_length >> TARGET_PAGE_BITS
bitmap_zero(block->bmap, block->max_length >> TARGET_PAGE_BITS);
```

### vhost dirty bitmap

vhost 粒度固定定义

```txt
  #define VHOST_LOG_PAGE  0x1000
  #define VHOST_LOG_CHUNK (VHOST_LOG_PAGE * VHOST_LOG_BITS)
```

vhost backend 写某个 GPA 时，设置：

vhost_bit = GPA / 4096

QEMU 扫描 vhost bitmap 后，将该 bit 还原成一个 4 KiB 范围，并调用 hw/virtio/vhost.c

```txt
  page_addr = addr + bit * VHOST_LOG_PAGE;
  memory_region_set_dirty(section->mr, mr_offset, VHOST_LOG_PAGE);
```
memory_region_set_dirty 中安装 TARGET_PAGE_SIZE 来自动转换:

```txt
 TARGET_PAGE_SIZE    一个 vhost bit 的转换结果
━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 4 KiB               精确对应一个 target dirty bit
──────────────────  ────────────────────────────────────────────
 16 KiB              4 个 vhost bit 可能汇聚到同一个 target bit
──────────────────  ────────────────────────────────────────────
 1 KiB               一个 vhost bit展开成 4 个 target bits
```
其实不难理解，如果比 TARGET_PAGE_SIZE 大，就自动展开为多个，如果比 TARGET_PAGE_SIZE 小，
就仅仅标记一个

## kvm 来处理的 page size

需要注意到，在 kvm 体系下，一共存在三个映射:

- QEMU :
- Guest : GVA -> GPA
- Host : EPT

考虑这四个场景:
1. 虚拟机 64k 页 / 物理机 4k 页
2. 虚拟机 4k / 物理机使用 64k
3. 虚拟机 大页 / 物理机 小页
4. 虚拟机 小页 / 物理机 大页

虚拟机使用什么大小的页面，根本没有关系，处理起来也是非常容易的
但是 QEMU 和 EPT 的映射往往会使用相同的粒度，不是硬件的限制，
而是如果 QEMU 采用了大页面，EPT 默认会使用大页，这样性能好，
但是在热迁移的时候，ept 会被拆分为小页。

但是，热迁移的时候，QEMU 会被拆分为小页是一个问题，THP 应该类似设置。

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
