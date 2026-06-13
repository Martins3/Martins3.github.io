# Folio

- [ ] 似乎需要首先了解一下 transparent 的内容

## 阅读一下各种资料
- [An introduction to compound pages](https://lwn.net/Articles/619514/)
  - Compound pages can serve as anonymous memory or be used as buffers within the kernel; they cannot, however, appear in the page cache, which is only prepared to deal with singleton pages.

> A compound page is simply a grouping of two or more physically contiguous pages into a unit that can, in many ways, be treated as a single, larger page.
They are most commonly used to create huge pages, used within hugetlbfs or the transparent huge pages subsystem, *but they show up in other contexts as well*.
*Compound pages can serve as anonymous memory or be used as buffers within the kernel*; *they cannot, however, appear in the page cache, which is only prepared to deal with singleton pages.*

- [Minimizing the use of tail pages](https://lwn.net/Articles/787388/)
- [Cramming more into struct page](https://lwn.net/Articles/565097/)
- https://lore.kernel.org/linux-mm/ZNVaEOmUUM5rR4CA@casper.infradead.org/T/#t
- https://lwn.net/Articles/619333/
- [Transparent huge page reference counting](https://lwn.net/Articles/619738/)

## Compound page 的构造过程
prep_compound_page : 初始化和 page 相关的各种参数

- asm_exc_page_fault
    - handle_page_fault
      - do_user_addr_fault
        - handle_mm_fault
          - __handle_mm_fault
            - create_huge_pmd
              - do_huge_pmd_anonymous_page
                - vma_alloc_folio
                  - __folio_alloc_node
                    - __folio_alloc
                      - __alloc_pages
                        - get_page_from_freelist
                          - prep_new_page
                            - prep_compound_page



使用 readv 读一个大文件，可以得到如下结果，看来 xfs 对于 folio 的支持的确还可以:

```bt
kfunc:page_cache_ra_order{ @order = hist(args->new_order); }
```
结果为
```txt
@order:
[0]                12813 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[1]                    0 |                                                    |
[2, 4)                 2 |                                                    |
[4, 8)              4096 |@@@@@@@@@@@@@@@@                                    |
```

整个的执行路径为:
```txt
@[
    folio_add_lru+5
    filemap_add_folio+90
    page_cache_ra_order+413
    filemap_get_pages+1246
    filemap_read+223
    xfs_file_buffered_read+79
    xfs_file_read_iter+110
    vfs_read+499
    ksys_read+111
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 420691
```

## 各种报道
- https://news.ycombinator.com/item?id=27509944

## 看看评测
### [解读 Linux 内存管理新特性 Memory folios](https://mp.weixin.qq.com/s/Zl7SaA0ZLCgZt7ZVZqTgwg)

```c
struct folio *folio_alloc(gfp_t gfp, unsigned int order)
{
	return page_rmappable_folio(alloc_pages(gfp | __GFP_COMP, order));
}
```

__GFP_COMP 的影响在于， prep_new_page -> prep_compound_page : 也就是说，

- 关于 folio_alloc 和 alloc_pages 的关系
  - 增加了 __GFP_COMP
  - 设置了释放的代码
    - https://lore.kernel.org/all/20230816151201.3655946-9-willy@infradead.org/T/#rb3e69a3ef6042dfdd423ae83426f02d8f42f3609

我不得不去学习 LWN: A discussion on folios（https://lwn.net/Articles/869942/），
LPC 2021 - File Systems MC（https://www.youtube.com/watch?v=U6HYrd85hQ8&t=1475s） 大佬关于 folio 的讨论。
然后发现 Matthew Wilcox 的主题不是《The folio》，而是《Efficient buffered I/O》。事情并不简单。

https://openanolis.cn/sig/Cloud-Kernel/doc/475049355931222178

### https://mp.weixin.qq.com/s/CfiUSsYGv8kCPiBqQydhDg

## https://mp.weixin.qq.com/s/Jp0TgD2p91m6mUczRdTh0Q

## https://mp.weixin.qq.com/s/4XnyOCSQwf6NGXY8RIAI0A

## 回答这个疑惑
- 关于 include/linux/page_idle.h 中各种 folio 的 reference ，让人迷茫啊，
为什么感觉明显有些 page 是单个 page 的，但是还是使用 folio 接口。


## https://mp.weixin.qq.com/s/Jp0TgD2p91m6mUczRdTh0Q
- Linux 社区在文件页方面，发展出多个文件系统支持 large folio。这类文件系统会通过 mapping_set_large_folios()
告诉 page cache 这层，它支持 large folio：

等等，这个总结的很好的

## 为什么文件系统需要特殊的支持 large folio ?
https://mp.weixin.qq.com/s/FeQfJ3P59f01_9UkRoKpjw

## 永恒的问题

- memory 的 folio 的问题，直接

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
