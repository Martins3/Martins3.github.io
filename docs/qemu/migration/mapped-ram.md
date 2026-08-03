## 阅读笔记

每个 RAMBlock 前面还有一个 bitmap：

bit = 1：这个页面槽位里有有效的非零数据
bit = 0：恢复时把这个页面当成零页

非零页写入固定槽位并置位：migration/ram.c:1266。

如果一个页后来变成零页，则不需要再写一整页，只要清除 bitmap 位：migration/ram.c:1230。

最终 bitmap 在所有 RAM 写完后写入文件：migration/ram.c:3212。恢复端先读取 bitmap，只读取置位页面，其余按零页处理：migration/ram.c:4082。


代码证据:
```c
/*
 * directly send the page to the stream
 *
 * Returns the number of pages written.
 *
 * @pss: current PSS channel
 * @block: block that contains the page we want to send
 * @offset: offset inside the block for the page
 * @buf: the page to be sent
 * @async: send to page asyncly
 */
static int save_normal_page(PageSearchStatus *pss, RAMBlock *block,
                            ram_addr_t offset, uint8_t *buf, bool async)
{
    QEMUFile *file = pss->pss_channel;

    if (migrate_mapped_ram()) {
        qemu_put_buffer_at(file, buf, TARGET_PAGE_SIZE,
                           block->pages_offset + offset);
        set_bit(offset >> TARGET_PAGE_BITS, block->file_bmap);
    } else {
        ram_transferred_add(save_page_header(pss, pss->pss_channel, block,
                                             offset | RAM_SAVE_FLAG_PAGE));
        if (async) {
            qemu_put_buffer_async(file, buf, TARGET_PAGE_SIZE,
                                  migrate_release_ram() &&
                                  migration_in_postcopy());
        } else {
            qemu_put_buffer(file, buf, TARGET_PAGE_SIZE);
        }
    }
    ram_transferred_add(TARGET_PAGE_SIZE);
    qatomic_add(&mig_stats.normal_pages, 1);
    return 1;
}
```
如果一个页后来变成零页，则不需要再写一整页，只要清除 bitmap 位：migration/ram.c:1230。

为什么仅仅 mmap-map 才支持这个东西?
```c
static int save_zero_page(RAMState *rs, PageSearchStatus *pss,
                          ram_addr_t offset)
{
    uint8_t *p = pss->block->host + offset;
    QEMUFile *file = pss->pss_channel;
    int len = 0;

    if (migrate_zero_page_detection() == ZERO_PAGE_DETECTION_NONE) {
        return 0;
    }

    if (!buffer_is_zero(p, TARGET_PAGE_SIZE)) {
        return 0;
    }

    qatomic_add(&mig_stats.zero_pages, 1);

    if (migrate_mapped_ram()) {
        /* zero pages are not transferred with mapped-ram */
        clear_bit_atomic(offset >> TARGET_PAGE_BITS, pss->block->file_bmap);
        return 1;
    }
```

save_zero_page 是只有 zero page 才会使用吗?

- 支持 O_DIRECT
  RAM 区域按 1 MiB 对齐，因此可以绕过 page cache 做 direct I/O：migration/ram.c:3043。

## file_bmap 的作用

对，就是这个作用。file_bmap 是 "file bitmap"，每个 bit 对应 RAMBlock 里的一页，语义是：这页的数据有没有被写进迁移文件。

- bit = 1：页内容已通过 pwrite 写到文件的 pages_offset + 页偏移 处，目的端要从文件读出来；
- bit = 0：文件里没有这页，目的端直接清零即可（全零页，或从没被发过的页）。

置位/清零的两个来源：

- 单通道路径 save_normal_page()(migration/ram.c)：写完一页就 set_bit；
- multifd 路径 multifd_set_file_bitmap()(migration/multifd-nocomp.c):normal_num 以内的正常页置位，零页显式清零——清 bit 不是
  多余的，因为同一页可能上一轮以非零页发过（bit 已是 1)，这一轮变成了零页，必须把旧 bit 清掉，否则目的端会去读文件里那份过时的旧数据。

bitmap 本身由 bitmap_new() 分配（migration/ram.c)，初始全 0，所以从未发送的页天然就是 0。

目的端对应逻辑在 read_ramblock_mapped_ram()(migration/ram.c)：扫描这个位图，bit=1 的页从 pages_offset + 页偏移 处读文件
，bit=0 的连续区间交给 handle_zero_mapped_ram() 直接 memset 清零。这样零页和未变页完全不用占迁移文件的带宽和空间（文件里就是稀疏空洞）。

### 和 background-snapshot 是冲突的
1. 但最终 bitmap 没有写入

Mapped-ram 在发送页面时只更新内存中的 file_bmap：

写页面固定槽位
更新 file_bmap

真正把最终 bitmap 写进迁移文件，是在 ram_save_complete() 中完成的：

if (migrate_mapped_ram()) {
    ram_save_file_bmap(f);
}

问题在于 background-snapshot 使用自己的完成路径，它只反复调用：

qemu_savevm_state_iterate(...)

见 migration/migration.c:3564，完成后直接把预先保存的设备状态追加到 stream，
并不会调用 ram_save_complete()。

结果是：

```txt
RAM 页面已经写入固定槽位
          ↓
file_bmap 只存在于内存
          ↓
没有写入 migration file
          ↓
恢复端读到的 bitmap 全零
          ↓
将页面误认为零页/不存在
```

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
