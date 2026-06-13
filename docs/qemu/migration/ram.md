# 分析 migration/ram.c

- RAMState

- 和 memory region 的 hook memory listener 的关系

- `migrate_send_rp_recv_bitmap`
    - `migrate_send_rp_message` : 希望获取到一个 BITMAP
    - `ramblock_recv_bitmap_send` :


## [ ] zero page
- 为什么会出现？
    - xbzrle 和 postcopy 如何处理的

## compress
- `do_data_compress`

- 使用一个额外的线程来进行的

## [ ] softmmu/cpu-throttle.c
降低 Guest 的执行速度，从而让 memory dirty 的速度下降。

## `userfault_fd` : 似乎这个机制可以让 kvm 通知 QEMU guest 需要 memory 了

`migration_clear_memory_region_dirty_bitmap_range`
