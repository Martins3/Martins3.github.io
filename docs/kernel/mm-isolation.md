# isolation

似乎遇到过的地方:
- compaction : isolate_migratepages_block
- memory failer : isolate_page
- vmscan : isolate_lru_folios

但是，这些应该是只是恰好取名字是这个

page_isolation.c 中只有两个非 static 函数
- start_isolate_page_range
- test_pages_isolated

isolation 的调用源头是:
1. alloc_contig_range
2. memory unplug

## 问题
- 为什么需要使用额外的 migratetype::MIGRATE_ISOLATE
