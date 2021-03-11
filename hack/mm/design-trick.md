# memory 基本设计技巧，常用方法

## compaction，page reclaim, page writeback

1. `__alloc_pages_direct_reclaim` => `__perform_reclaim` => try_to_free_pages : 通过 reclaim page 进行分配
  - shrink_zones
    - shrink_node
2. `__alloc_pages_direct_compact` => try_to_compact_pages : 通过 compaction 分配
    - compact_zone_order
      - compact_zone

- kswapd
  - balance_pgdat
    - kswapd_shrink_node
      - shrink_node

kcompactd
  - kcompactd_do_work
    - compact_zone

其实 page writeback 机制和另外两个其实有点不同，
1. 似乎没有 direct 和 indirect 的两种方式
2. 使用的是 workqueue
