# khugepaged

## 基本接口
```txt
mm/transparent_hugepage/khugepaged🔒
🧀  grep "" -r
defrag:1
max_ptes_shared:256
scan_sleep_millisecs:10000
max_ptes_none:511
pages_to_scan:4096
max_ptes_swap:64
alloc_sleep_millisecs:60000
pages_collapsed:52938
full_scans:33
```

## khugepaged

kcompactd 用来 defrag，而 khugepaged 来扫描，确定到底那些已经映射的可以设置为 page table

1. /sys/kernel/mm/transparent_hugepage/enabled => start_stop_khugepaged => khugepaged => khugepaged_do_scan => khugepaged_scan_mm_slot => khugepaged_scan_pmd
2. in `khugepaged_scan_pmd`, we will check pages one by one, if enough base pages are found, call `collapse_huge_page` to merge base page to huge page
3. `collapse_huge_page` = `khugepaged_alloc_page` + `__collapse_huge_page_copy` + many initialization for huge page + `__collapse_huge_page_isolate` (free base page)

- [x] it seems khugepaged scan pages and collapse it into huge pages, so what's difference between kcompactd
  - khugepaged is consumer of hugepage, it's scan base pages and collapse them
  - [ ] khugepaged 是用于扫描 base page 的 ? It’s the responsibility of khugepaged to then install the THP pages.

```txt
#0  prep_transhuge_page (page=0xffffea000d998000) at mm/huge_memory.c:582
#1  0xffffffff81394907 in hpage_collapse_alloc_page (nmask=0xffffc9000234fe28, node=<optimized out>, gfp=1844426, hpage=0xffffc9000234fd20) at mm/khugepaged.c:808
#2  alloc_charge_hpage (hpage=hpage@entry=0xffffc9000234fd20, mm=mm@entry=0xffff888341a3bdc0, cc=cc@entry=0xffffffff82d75980 <khugepaged_collapse_control>) at mm/khugepaged.c:957
#3  0xffffffff81394c3b in collapse_huge_page (mm=mm@entry=0xffff888341a3bdc0, address=address@entry=140576452247552, referenced=referenced@entry=512, unmapped=unmapped@entry=0, cc=cc@entry=0xffffffff82d75980 <khugepaged_collapse_control>) at mm/khugepaged.c:989
#4  0xffffffff813963e9 in hpage_collapse_scan_pmd (mm=mm@entry=0xffff888341a3bdc0, vma=vma@entry=0xffff8883d8e1c130, address=140576452247552, mmap_locked=mmap_locked@entry=0xffffc9000234fe97, cc=cc@entry=0xffffffff82d75980 <khugepaged_collapse_control>) at mm/khugepaged.c:1275
#5  0xffffffff81398edb in khugepaged_scan_mm_slot (cc=0xffffffff82d75980 <khugepaged_collapse_control>, result=<synthetic pointer>, pages=4096) at mm/khugepaged.c:2316
#6  khugepaged_do_scan (cc=0xffffffff82d75980 <khugepaged_collapse_control>) at mm/khugepaged.c:2422
#7  khugepaged (none=<optimized out>) at mm/khugepaged.c:2478
#8  0xffffffff811556a4 in kthread (_create=0xffff88834128b300) at kernel/kthread.c:376
#9  0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#10 0x0000000000000000 in ?? ()
```

- [ ] 似乎在 page fault 的时候就会构造，khugepaged 来制作 thp 的意义是什么
  - 应该是存在开始的时候，没有 thp ，之后被 khugepaged 合并上的。

## 触发方法


## 核心参数

## 基本流程

  - khugepaged_scan_mm_slot : madvise_collapse 也会向下调用
    - hpage_collapse_scan_pmd
      - collapse_huge_page
        - `__collapse_huge_page_isolate`

## khugepaged 和 kcompactd 的区别

- khugepaged 是逐个扫描各个地址空间的，其作用是将已经的映射的 page 都放到一起。

- kcompactd 扫描空闲页的页，然后搬动。

## 从 vmstat 看 khugepaged

```txt
🧀  cat /proc/vmstat | grep khugepaged
pgsteal_khugepaged 6758901
pgdemote_khugepaged 0
pgscan_khugepaged 6948435
```

居然存在 pgscan 和 pgsteal

这个说明了 thp 会导致系统中内存使用增多，shrink_inactive_list

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
