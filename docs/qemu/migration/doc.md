# 文档细读

## 核心资料
- docs/devel/migration/
	- https://www.qemu.org/docs/master/devel/migration/index.html
	- https://www.qemu.org/docs/master/devel/migration/main.html

```txt
  migration
     best-practices.rst : 两个小技巧
     compatibility.rst
     CPR.rst

     features.rst
     index.rst

     main.rst : 核心架构和流程

     dirty-limit.rst
     mapped-ram.rst
     postcopy.rst

     qatzip-compression.rst
     xbzrle.rst
     qpl-compression.rst
     uadk-compression.rst

     vfio.rst
     virtio.rst
```

其他资料
https://wiki.qemu.org/Features/Migration/Troubleshooting
https://www.qemu.org/docs/master/interop/vhost-user.html#migrating-backend-state

## 阅读笔记

为什么还存在 bitmap ?
```txt
  ### bitmap 是做什么的

  每个 RAMBlock 前面还有一个 bitmap：

  bit = 1：这个页面槽位里有有效的非零数据
  bit = 0：恢复时把这个页面当成零页

  非零页写入固定槽位并置位：migration/ram.c:1266。

  如果一个页后来变成零页，则不需要再写一整页，只要清除 bitmap 位：migration/ram.c:1230。

  最终 bitmap 在所有 RAM 写完后写入文件：migration/ram.c:3212。恢复端先读取 bitmap，只读取置位页面，其余按零页处理：migration/ram.c:4082。
```

就是这样的:
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


## 重新设计这个模式

```txt
   类别          模式                                  成功条件
  ━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   普通热迁移    precopy、multifd、multifd-zstd        源端 postmigrate，目标端 running
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   后拷贝        postcopy、multifd-postcopy            进入 active 后切 postcopy，最终两端 migration completed
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   文件状态      plain、mapped-ram                     源端停止，文件可恢复且 guest 状态连续
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   在线快照      background-snapshot                   migration completed，但源端必须仍是 running
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   CPR           cpr-reboot、cpr-transfer、cpr-exec    按各自协议验证 QEMU 重建和资源继承
```

## 问题
- 那些没有 touch 的页面，都是如何跳过的?

- [ ] 迁移的时候，guest 没有使用的页不用发送的?
  - 似乎比到 proc/pid/map 下去检查更加好的
    - 怀疑，qemu 中是否实现过这个功能

> 严格说，热迁移里“跳过 balloon 页”主要不是靠传统 balloon inflate 本身完成的，而是靠 virtio-balloon 的 free-page-hint 迁移优化。
>
> 传统 balloon inflate 这条路里，guest 把 PFN 交给 balloon 后，QEMU 在源端只是把对应宿主页做 discard，回收宿主内存
> balloon_inflate_page() 里直接调用 ram_block_discard_range()；底层通常走 madvise(DONTNEED) 或 fallocate(PUNCH_HOLE)，
> 是“把 host backing 扔掉”，不是直接改迁移位图。

既然如此，那么

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
