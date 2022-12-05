## xbzrle
```diff
Add XBZRLE to ram_save_block and ram_save_live

In the outgoing migration check to see if the page is cached and
changed, then send compressed page by using save_xbrle_page function.
In the incoming migration check to see if RAM_SAVE_FLAG_XBZRLE is set
and decompress the page (by using load_xbrle function).
```

如果对于内存的修改是稀疏的，那么就可以进行压缩。

- `migration_update_rates`
    - `migrate_use_xbzrle`
    - `migrate_use_compression`

- `ram_save_complete` : 将最后的 ram 发送出去
    - `ram_find_and_save_block` ：
        - `get_queued_page`
            - `poll_fault_page` ：应该是 userfd 机制吧
        - `find_dirty_block`
            - `migration_bitmap_find_dirty` : 这个似乎是 Tcg 才使用的路径吧
        - `ram_save_host_page`
            - `ram_save_target_page` ： 这里会进行各种可能的 save page
                - `control_save_page`
                - `save_compress_page`
                - `save_zero_page`
                - `ram_save_multifd_page`
                - `ram_save_page`
                    - `save_xbzrle_page` : 进行压缩的地方
                    - `save_normal_page`
                        - `qemu_put_buffer_async` ： 应该这里就是最常规的操作了吧。

- `ram_load`
    - `ram_load_precopy` ：其中 QEMUFile f 是数据源，`page_buffer` 是 host 的内容。
        - `RAM_SAVE_FLAG_ZERO`
            - memset
        - `RAM_SAVE_FLAG_PAGE`
            - `qemu_get_buffer`
            - `qemu_get_buffer_in_place`
        - `RAM_SAVE_FLAG_COMPRESS_PAGE`
            - `decompress_data_with_multi_threads`
    - `ram_load_postcopy` ：相对于 precopy ，分析多出来的。
        - `RAM_SAVE_FLAG_MEM_SIZE`
        - `RAM_SAVE_FLAG_XBZRLE`
            - `load_xbzrle` ： 接受到


居然整体的架构是这个样子的:
```c
static SaveVMHandlers savevm_ram_handlers = {
    .save_setup = ram_save_setup,
    .save_live_iterate = ram_save_iterate,
    .save_live_complete_postcopy = ram_save_complete,
    .save_live_complete_precopy = ram_save_complete,
    .has_postcopy = ram_has_postcopy,
    .save_live_pending = ram_save_pending,
    .load_state = ram_load,
    .save_cleanup = ram_save_cleanup,
    .load_setup = ram_load_setup,
    .load_cleanup = ram_load_cleanup,
    .resume_prepare = ram_resume_prepare,
};
```

- `SaveVMHandlers::save_state` ：注册的两个位置，slirp 和 vfio/migration.c 中的

## https://github.com/qemu/qemu/blob/master/docs/xbzrle.txt
Using XBZRLE (Xor Based Zero Run Length Encoding) allows for the reduction
of VM downtime and the total live-migration time of Virtual machines.

真的都可以降低吗?
- [ ] VM downtime
- [ ] totla live-migration time

- [ ] 这两个是一个意思
    - memory write intensive
    - sparse memory

- send a compressed version of the update

为了访问计算 update，需要 cache 数据

- KVM 只能记录 dirty 的 page，如果只是在 page 上修改了一个 byte，整个 page 需要重新传的。
- Guest 直接修改 memory ，然后通知 Host 只有 dirty bit
    - 如果命中 xbzrle 的 cache，那么可以比较新的内容和久的内容，然后将修改发送过去。
    - 因为只是发送 diff 的，需要需要保持原来的内容相同的才可以。

- [x] XBZRLE 和 postcopy 可以在一起使用吗?
    - [x] 可以先使用 XBZRLE 然后使用 postcopy
        -  从 `ram_save_page` 中可以看到，只有当前不是
- [ ] 两边是建立相同大小的 cache 才可以吗?
    - [ ] cache 的替换策略是什么？


The counter will increase after each *ram dirty bitmap* sync. When a cache conflict is
detected, XBZRLE will only evict pages in the cache that are older than
a threshold.

- [ ] 调查一下 ram dirty bitmap 更新的时间，xbzrle 的 hook 是如何联系起来的

## 压缩算法

- [ ] 没看懂最后是如何生成的?

需要参考 xbzrle.c

```sh
Example
old buffer:
1001 zeros
05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 68 00 00 6b 00 6d
3074 zeros

new buffer:
1001 zeros
01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 68 00 00 67 00 69
3074 zeros

encoded buffer:

encoded length 24
e9 07 0f 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 03 01 67 01 01 69
```

## page cache 的管理
主要的内容在 : `page_cache.c` 中

- `cache_insert` : 如果成功插入，返回 0 否则返回 -1
