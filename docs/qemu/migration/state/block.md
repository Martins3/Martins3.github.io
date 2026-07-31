# block 热迁移

- [ ] https://qemu-project.gitlab.io/qemu/interop/bitmaps.html
- https://qemu-project.gitlab.io/qemu/interop/live-block-operations.html
	- live snapshot 和 live snapshot merge 是什么意思哇

https://wiki.qemu.org/Features/LiveBlockMigration

忽然意识到，overlayfs 可以实现 snapshot
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


## block/dirty-bitmap.c 和 migration/block-dirty-bitmap.c

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


### 那么这个 migration/block-dirty-bitmap.c 功能都是注入到哪些 level 的
本来以为是 qcow2 中做这个事情，实际上

## 非共享存储的热迁移 : codex
支持，但不再通过 migrate 命令内置的 block capability 迁移。

现在 QEMU 把两件事拆开了：

- VM migration：迁移 CPU、RAM、设备状态。
- Storage migration：使用 block layer 的 blockdev-mirror、NBD 或存储系统自身的复制能力。

典型做法是 blockdev-mirror + NBD：

源端磁盘
   |
   | blockdev-mirror
   v
目的端 NBD export
   |
   v
目的端磁盘

大致流程是：

1. 在目的端创建目标磁盘。
2. 目的端通过 NBD export 暴露目标 block node。
3. 源端添加对应的 NBD block node。
4. 源端运行 blockdev-mirror，一边运行 VM，一边复制磁盘。
5. mirror 进入 READY 后，新产生的 guest 写入会继续同步到目的端。
6. 启动普通 VM migration，迁移 RAM 和设备状态。
7. switchover 时停止源 VM并完成最后写入同步。
8. 目的端使用已经同步好的本地磁盘启动 VM。

这种设计比旧 block capability 更灵活：

- 可以分别观察、暂停和重试磁盘复制。
- 可以迁移部分磁盘。
- 可以做全量或增量同步。
- 可以使用不同的源、目的存储格式。
- 可以交给 Ceph、ZFS、SAN 等存储系统自身完成复制。

几个容易混淆的地方：

- dirty-bitmaps capability 只迁移 dirty bitmap 元数据，不复制磁盘内容。
- mapped-ram 迁移的是 RAM，只是采用固定文件偏移布局，不是磁盘迁移。
- file: migration URI 保存的是 VM state/RAM stream，默认不包含 guest 磁盘内容。
- zero-blocks 只服务于已删除的旧式 block migration，所以旧 block migration 删除后它就成了无效开关。

所以更准确的说法是：

QEMU 仍然支持在线复制和迁移存储，
但存储复制已从 VM migration stream 中拆出，改由 blockdev-mirror、NBD。

仓库中有完整的 QMP 示例：docs/interop/live-block-operations.rst。


### nbd + blockdev-mirror 的关键 QMP 操作

目标端：

{
  "execute": "nbd-server-start",
  "arguments": {
    "addr": {
      "type": "unix",
      "data": {
        "path": "/path/to/migration.nbd"
      }
    }
  }
}

用新的接口导出目标 block node：

{
  "execute": "block-export-add",
  "arguments": {
    "type": "nbd",
    "id": "export-boot1",
    "node-name": "boot1-format",
    "name": "boot1",
    "writable": true,
    "allow-inactive": true
  }
}

allow-inactive=true 对当前 QEMU 很重要：incoming QEMU 的磁盘尚未激活时，NBD mirror 仍需向其写入；

源端添加 NBD node：

{
  "execute": "blockdev-add",
  "arguments": {
    "driver": "nbd",
    "node-name": "migration-target-boot1",
    "server": {
      "type": "unix",
      "path": "/path/to/migration.nbd"
    },
    "export": "boot1"
  }
}

开始完整同步：

{
  "execute": "blockdev-mirror",
  "arguments": {
    "job-id": "mirror-boot1",
    "device": "boot1-format",
    "target": "migration-target-boot1",
    "sync": "full",
    "on-source-error": "report",
    "on-target-error": "report"
  }
}


## 源码分析 : codex
block/dirty-bitmap.c 和 migration/block-dirty-bitmap.c 的关系是什么?

两者的关系可以概括为：
- block/dirty-bitmap.c：实现 dirty bitmap 这种数据结构及其生命周期。
- migration/block-dirty-bitmap.c：把已经存在的、具名 dirty bitmap 放进 migration stream，在目标端重建。
- 两者都不复制磁盘数据。

guest write/discard
        │
        ▼
block/io.c: bdrv_set_dirty()
        │
        ▼
block/dirty-bitmap.c
BdrvDirtyBitmap / HBitmap
        │
        ├── backup/mirror/block-copy 使用
        │
        └── migration/block-dirty-bitmap.c
                │ serialize
                ▼
          migration stream
                │ deserialize
                ▼
       目标端 BdrvDirtyBitmap

### block/dirty-bitmap.c

这是 block layer 的通用脏块位图实现。

核心对象是 block/dirty-bitmap.c:28：

```txt
struct BdrvDirtyBitmap {
    BlockDriverState *bs;
    HBitmap *bitmap;
    char *name;
    int64_t size;
    bool disabled;
    bool busy;
    bool persistent;
    bool inconsistent;
    BdrvDirtyBitmap *successor;
    ...
};
```

它挂在某个 BlockDriverState 上，用一个 HBitmap 表示哪些磁盘区间发生过变化。

假设：

- 磁盘大小 1 TiB。
- bitmap granularity 是 64 KiB。
- guest 写了 offset=128 MiB、length=4 KiB。

那么 bitmap 不保存这 4 KiB 数据，只把覆盖该范围的 64 KiB granule 对应 bit 置为 1。

#### 写请求怎样进入 bitmap

block I/O 完成时，write 和 discard 会调用：

bdrv_set_dirty(bs, offset, bytes);

调用位置在 block/io.c:2028，实现位于 block/dirty-bitmap.c:656。

bdrv_set_dirty() 遍历这个 block node 上所有启用的 bitmap：

QLIST_FOREACH(bitmap, &bs->dirty_bitmaps, list) {
    if (!bdrv_dirty_bitmap_enabled(bitmap)) {
        continue;
    }
    hbitmap_set(bitmap->bitmap, offset, bytes);
}

#### 这个文件提供的主要能力

- 创建、查找、释放 bitmap。
- enable/disable：是否继续记录新写入。
- set/reset/clear：修改 dirty bits。
- iterator：遍历脏区间。
- count/query：统计 dirty 字节数。
- merge：合并两个 bitmap。
- serialize/deserialize：把 bitmap bits 转成字节流。
- persistent/inconsistent：管理持久化状态。
- busy：防止 bitmap 被 block job、migration 和 QMP 同时修改。
- successor：在旧 bitmap 暂时被冻结时，用新 bitmap 继续记录写入。

相关序列化基础接口在 block/dirty-bitmap.c:599。

#### 谁会使用它

它是一个通用基础设施：

- 增量备份使用具名 bitmap，记录“上次成功备份后哪些区间变化了”。
- blockdev-mirror 使用匿名 bitmap，记录“哪些范围还需要复制或重新复制”。
- block-copy、copy-before-write 等内部任务也会创建匿名 bitmap。
- QMP block-dirty-bitmap-add/clear/enable/disable/remove 操作具名 bitmap。
- qcow2 驱动通过 block/qcow2-bitmap.c 把 persistent bitmap 写入 qcow2。

其中匿名 bitmap：

bdrv_create_dirty_bitmap(bs, granularity, NULL, errp);

没有名字，只供 QEMU 内部使用，不会通过 QMP 暴露，也不会被 migration 迁移。

### migration/block-dirty-bitmap.c

这个文件实现一种 migration section，名称为 dirty-bitmap：

register_savevm_live("dirty-bitmap", 0, 1,
                     &savevm_dirty_bitmap_handlers,
                     &dbm_state);

注册位置见 migration/block-dirty-bitmap.c:1253。

只有启用了 migration capability：

dirty-bitmaps=on

并且源端确实有具名 bitmap，它才会激活：

return migrate_dirty_bitmaps() && !s->no_bitmaps;

#### 它迁移的内容

每个 bitmap 迁移这些信息：

- 对应的 block node/device。
- bitmap 名称。
- granularity。
- enabled 状态。
- persistent 状态。
- bitmap 中的每一个 bit。

协议格式写在文件开头，见 migration/block-dirty-bitmap.c:21。

它不发送：

- qcow2/raw 的实际扇区数据。
- backing chain。
- block node 配置。
- block device 的 I/O 内容。

因此目标端必须已经有对应的 block node，而且它代表的磁盘内容必须和源端处在一致的迁移时间点。否则 bitmap 即使成功迁过去，也没有实际意义。

### 发送端流程

#### 1. 找到要迁移的 bitmap

init_dirty_bitmap_migration() 遍历：

- 所有具名 BlockBackend。
- 其他具名 BlockDriverState。
- 每个 node 上的具名 bitmap。

实现见 migration/block-dirty-bitmap.c:601。

匿名 bitmap 会被跳过：

bitmap_name = bdrv_dirty_bitmap_name(bitmap);
if (!bitmap_name) {
    continue;
}

被选择的 bitmap 会：

- 标记为 busy，禁止 QMP 或其他 job 修改。
- 记录 node alias 和 bitmap alias。
- 保存 enabled/persistent flags。
- 设置 skip_store，表示当前 bitmap 的所有权正在通过 migration 转移。

#### 2. 发送 START

dirty_bitmap_save_setup() 对每个 bitmap 发送：

node alias
bitmap alias
granularity
enabled
persistent

见 migration/block-dirty-bitmap.c:1220。

#### 3. 发送 bitmap bits

send_bitmap_bits() 调用底层：

bdrv_dirty_bitmap_serialize_part(...)

把一段 HBitmap 转成 byte buffer，见 migration/block-dirty-bitmap.c:423。

这里的 CHUNK_SIZE = 1024 是“每个 bitmap 数据块最多约 1 KiB”，不是复制 1 KiB 磁盘内容。

如果整段 bitmap 都是零，会只发送 ZEROES 标志，不发送 buffer。

#### 4. 发送 COMPLETE

所有 bits 发送完成后，对每个 bitmap 发送 COMPLETE，最后发送 EOS。

### 为什么叫 postcopy bitmap migration

它支持目标 VM 已经运行、bitmap bits 仍未完全传完的情况。

发送端的 save_live_iterate 只有在 VM 不再运行时才参与：

return dirty_bitmap_is_active(opaque) && !runstate_is_running();

见 migration/block-dirty-bitmap.c:1243。

因此：

- 普通 precopy：源 VM 停止后发送 bitmap。
- postcopy：目标 VM 可能先启动，bitmap 数据随后继续传输。

难点在于：目标 VM 启动后又会产生新的磁盘写入，但旧 bitmap 还没接收完。这里就需要 successor。

### 目标端和 successor

目标端收到 START 后：

1. 创建一个 disabled bitmap，接收源端传来的旧 bits。
2. 如果源 bitmap 原本是 enabled，再为它创建一个 enabled successor。
3. 目标 VM 启动后，新写入进入 successor。
4. 源 bitmap bits 接收完成后，将 successor 合并进主 bitmap。

代码见 migration/block-dirty-bitmap.c:798。

逻辑上相当于：

源端迁来的历史 bitmap：  00100100
目标启动后的新写入：      00010001
                          --------
最终目标 bitmap：         00110101

如果 bitmap 在目标 VM 启动前已经接收完，直接启用主 bitmap；如果还没接收完，则先启用 successor。处理位置是 migration/block-dirty-bitmap.c:891。

收到 COMPLETE 后，dirty_bitmap_load_complete() 调用 reclaim，将 parent 和 successor 合并，见 migration/block-dirty-bitmap.c:952。

### AliasMapInnerNode 是什么

它只是为了实现两层名称映射：

source node name
    └── source bitmap name
             ↓
node alias
    └── bitmap alias
             ↓
target node name
    └── target bitmap name

结构：

typedef struct AliasMapInnerNode {
    char *string;
    GHashTable *subtree;
} AliasMapInnerNode;

外层 hash table：

node name/alias -> AliasMapInnerNode

其中：

- string：对端的 node alias/name。
- subtree：这个 node 下的 bitmap name/alias 映射。

发送端构建 name -> alias，接收端构建 alias -> name，见 migration/block-dirty-bitmap.c:192。

这允许源、目标两端的 block node 名字不同：

source: node=drive0, bitmap=backup0
alias:  node=disk-a, bitmap=incremental
target: node=target-disk, bitmap=backup-chain

### 最关键的区别

 文件                              管什么                                          保存磁盘数据吗    bitmap 范围
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━
 block/dirty-bitmap.c              bitmap 数据结构、状态和操作                                 否    具名和匿名
────────────────────────────────  ──────────────────────────────────────────────  ────────────────  ─────────────────────────
 migration/block-dirty-bitmap.c    把 bitmap 本身通过 migration stream 搬到目标                否    只迁移具名 bitmap
────────────────────────────────  ──────────────────────────────────────────────  ────────────────  ─────────────────────────
 block/mirror.c                    根据内部 bitmap 实际复制磁盘数据                            是    通常使用匿名内部 bitmap

所以，在 storage 热迁移中：

blockdev-mirror             迁移实际磁盘内容
dirty-bitmaps capability    迁移增量备份所依赖的 bitmap 状态
migrate                     迁移 RAM/CPU/device state

三者互补，不是互相替代。


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
