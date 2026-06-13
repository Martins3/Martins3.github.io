## 先把基本理念搞清楚吧

- [ ] https://qemu-project.gitlab.io/qemu/interop/bitmaps.html

### https://qemu-project.gitlab.io/qemu/interop/live-block-operations.html

- live snapshot 和 live snapshot merge 是什么意思哇

### https://wiki.qemu.org/Features/LiveBlockMigration


## block.c

```c
static SaveVMHandlers savevm_block_handlers = {
    .save_setup = block_save_setup,
    .save_live_iterate = block_save_iterate,
    .save_live_complete_precopy = block_save_complete,
    .save_live_pending = block_save_pending,
    .load_state = block_load,
    .save_cleanup = block_migration_cleanup,
    .is_active = block_is_active,
};
```

- `block_load`
    - [ ] `blk_pwrite` ：似乎这就是写入 block 设备的位置，但是好奇怪啊

- `block_save_pending`

## 所有的 hook

- `block_save_pending` : 调用位置 `migration_iteration_run` => `qemu_savevm_state_pending`
    - `get_remaining_dirty`
        - 对于 `BlkMigState::bmds_list` 中的所有的成员遍历，调用 `bdrv_get_dirty_count`，从而统计 bitmap 的数量。


下面两个的区别是什么:
- `block_save_iterate` : 调用路径 `qemu_savevm_state_iterate` 核心路径上
    - `blk_mig_save_dirty_block`
        - `mig_save_device_dirty` : 参数是 async 的
            - `blk_mig_save_dirty_block` ：异步的
            - `blk_pread` + `blk_send`

- `block_save_complete` : 用于完成 precopy
    - `blk_mig_save_dirty_block` ：参数是 sync 的


## block-dirty-bitmap.c

- [ ] `AliasMapInnerNode` 都是想要表达什么?

我现在的感觉是，dirty bitmap 和 block 并不是互相替代的技术，就是存在 dirty bitmap 的信息需要被发送出去的。
如果去进一步的看 block/dirty-bitmap.c ，应该是可以验证这个想法的。

- [ ] dirty bitmap 中的保存的是谁的


关键结构体:
- DBMSaveState 持有多个 SaveBitmapState，后者一个负责一个 bitmap 的迁移。
- [ ]  BdrvDirtyBitmap

- `dirty_bitmap_save_setup`
    - `init_dirty_bitmap_migration` ：TODO 这里依赖的 block driver 的驱动让人感觉到非常迷茫
        - `blk_next`
        - `bdrv_filter_bs`
        - `bdrv_next_all_states`
        - `add_bitmaps_to_list`
            - 利用 `FOR_EACH_DIRTY_BITMAP` 逐个初始化 `SaveBitmapState` ，并且将其挂在到 DBMSaveState 上。
    - `send_bitmap_start` ：对于每一个函数


- `dirty_bitmap_save_iterate`
    - `bulk_phase`
        - `bulk_phase_send_chunk`


接受端的:
- `dirty_bitmap_load`
    - `dirty_bitmap_load_start`
        - `bdrv_create_dirty_bitmap` : 创建 bitmap
    - `dirty_bitmap_load_complete`
    - `dirty_bitmap_load_bits`
        - `bdrv_dirty_bitmap_deserialize_part` : 将 bitmap 接受过来
