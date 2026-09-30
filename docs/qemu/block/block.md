# block

## 疑惑
- [ ] block jobs : Streaming, mirroring, commit,

忽然意识到，block 文件夹下，其实基本是这些 driver 的：
- `BlockDriver`
    - vmdk
    - dmg
    - vdi: A VDI file is a virtual disk image used by Oracle VM VirtualBox

阅读其他模块同时检测到的内容，也是比较关键的内容:
- block/block-backend.c
- block.c : 这也是我一直没有理解的地方，为什么将 block.c 不是放到 block 目录下，而是放到顶层目录中
- block/dirty-bitmap.c
- block/io.c

## 有趣的
- BlockDriverState 和 BlockBackend 的关系是什么?
```c
/*
 * Return the BlockDriverState attached to @blk if any, else null.
 */
BlockDriverState *blk_bs(BlockBackend *blk)
{
    IO_CODE();
    return blk->root ? blk->root->bs : NULL;
}
```


## block 代码基本分析

代码量比较大的问题，基本上都是各种后端。

- block/vhdx.c
- block/vvfat.c
- block/ssh.c
- block/rbd.c
- block/qed.c
- block/qcow2-refcount.c
- block/qcow2-cluster.c
- block/qapi.c
- block/nvme.c
- block/nfs.c
- block/nbd.c
- block/mirror.c
- block/iscsi.c
- block/io.c
- block/file-posix.c
- block/block-backend.c

- block/qcow2.c : 磁盘格式

## blockdev.c
1. 前面的好几千行就是用于定义这个的
2. 后面的就是各种 qmp 操作的

```c
static const BlkActionOps actions[] = {
    [TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT] = {
        .instance_size = sizeof(ExternalSnapshotState),
        .prepare  = external_snapshot_prepare,
        .commit   = external_snapshot_commit,
        .abort = external_snapshot_abort,
        .clean = external_snapshot_clean,
    },
    // ... 还有很多类似的，就不拷贝了

    /* Where are transactions for MIRROR, COMMIT and STREAM?
     * Although these blockjobs use transaction callbacks like the backup job,
     * these jobs do not necessarily adhere to transaction semantics.
     * These jobs may not fully undo all of their actions on abort, nor do they
     * necessarily work in transactions with more than one job in them.
     */
};
```

## block.c

- 几乎所有的函数都是 `bdrv` 开头的
    - 应该就是 `BlockDriverState` 的缩写了


## 一些 backtrace

### `io_uring`

```c
typedef enum BlockdevAioOptions {
    BLOCKDEV_AIO_OPTIONS_THREADS,
    BLOCKDEV_AIO_OPTIONS_NATIVE,
#if defined(CONFIG_LINUX_IO_URING)
    BLOCKDEV_AIO_OPTIONS_IO_URING,
#endif /* defined(CONFIG_LINUX_IO_URING) */
    BLOCKDEV_AIO_OPTIONS__MAX,
} BlockdevAioOptions;
```
- [ ] 其中 native 模式指的是？preadv 和 pwritev 吗？

- main
  - qemu_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - aio_ctx_dispatch
                - aio_dispatch
                  - aio_dispatch_handlers
                    - aio_dispatch_handler
                      - virtio_queue_notify_vq
                        - virtio_blk_handle_vq
                          - virtio_blk_submit_multireq
                            - submit_requests
                              - blk_aio_preadv
                                - blk_aio_prwv
                                  - aio_co_enter
                                    - qemu_aio_coroutine_enter
                                      - qemu_coroutine_switch
                                        - ??
                                          - __correctly_grouped_prefixwc
                                            - coroutine_trampoline
                                              - blk_aio_read_entry
                                                - blk_co_do_preadv_part
                                                  - bdrv_co_preadv_part
                                                    - bdrv_aligned_preadv
                                                      - qcow2_co_preadv_part
                                                        - qcow2_get_host_offset
                                                          - qcow2_cache_do_get
                                                            - bdrv_pread
                                                              - bdrv_co_pread
                                                                - bdrv_co_preadv
                                                                  - bdrv_co_preadv_part
                                                                    - bdrv_aligned_preadv
                                                                      - bdrv_driver_preadv
                                                                        - luring_co_submit
                                                                          - luring_do_submit

- [ ] 跟踪一下，这是两层的 block driver 的，首先是 qcow2，然后是 file 的

## block job
- `block_job_add_bdrv` 的调用者
  - block/commit.c
  - block/mirror.c
  - block/stream.c

## nfs 也可以直接做后端，不理解啊

## block/nvme.c 是通过 iommu 的，

## 其他的奇怪后端
- [ ] quorum

## 继续看看的
BlockDriver 似乎有很多抽象的后端，也就是后端里面还有后端了。
```c
static BlockDriver bdrv_dmg = {
    .format_name    = "dmg",
```

## [ ] 什么是 filter
- `bdrv_drop_filter`

## 关于 scsi 的代码

scsi/
block/iscsi.c
block/iscsi-opts.c

这下面有一个 scsi/qemu-pr-helper.c 做什么的?

使用 alpine.sh 中的配置之后:

如果是发起 io :
- coroutine_trampoline
  - blk_aio_write_entry
    - blk_co_do_pwritev_part
      - bdrv_co_pwritev_part
        - bdrv_aligned_pwritev
          - bdrv_driver_pwritev
            - raw_co_pwritev
              - bdrv_co_pwritev
                - bdrv_co_pwritev_part
                  - bdrv_aligned_pwritev
                    - bdrv_driver_pwritev
                      - iscsi_co_writev

如果是从 target (server) 哪里接受到消息，整个流程都是很熟悉了:

- os_host_main_loop_wait
  - glib_pollfds_poll
    - g_main_context_dispatch
      - g_main_context_dispatch_unlocked
        - aio_ctx_dispatch
          - aio_dispatch
            - aio_dispatch_handlers
              - aio_dispatch_handler
                - iscsi_process_read

```c
iscsi_process_read(void *arg)
{
    IscsiLun *iscsilun = arg;
    struct iscsi_context *iscsi = iscsilun->iscsi;

    qemu_mutex_lock(&iscsilun->mutex);
    iscsi_service(iscsi, POLLIN);
    iscsi_set_events(iscsilun);
    qemu_mutex_unlock(&iscsilun->mutex);
}
```
这里的 iscsi_service 就是调用的 libiscsi 的服务了:

/nix/store/ykvbkghvny9hfmxfzl0mlrjwxxzg34y2-libiscsi-1.20.0/include/iscsi/iscsi.h

看上去复杂的逻辑，例如 openiscs 都是在 openiscsi 中了:

# doc

## https://www.linux-kvm.org/images/b/b5/2012-fourm-block-overview.pdf

- virtual device
  - IDE, virtio-blk
- Backend
  - block driver
    - raw, qcow2
    - file, nbd(networking block device), iscsi(nternet Small Computer Systems Interface)
  - I/O throttling, copy on read
- block jobs
  - Streaming, mirroring, commit,

On the command line:
-hda test.img
...is a shortcut for...
-drive file=test.img,if=ide,cache=writeback,aio=threads
...is a shortcut for...
-drive file=test.img,id=ide0-hd0,if=none,cache=writeback,aio=threads
-device ide-drive,bus=ide.0,drive=ide0-hd0

SCSI passthrough

VMDK, VHD, VDI...
- Provided for compatibility
- Best to convert to raw or qcow2 for running VMs


- Usually you don’t want to use the host page cache
    - The guest has already a page cache
    - Data would be duplicated – waste of memory
- But it can make sense in some cases
    - Many guests sharing the host cache
    - Short-lived guests
- Must bypass host page cache for safe live migration

**External snapshots** (backing files):
    -- base -- sn1 -- sn2 -- sn3
- COW layer over backing files (of any image format) saves delta
- Cheap to create
- Deleting a snapshot means copying all data

**Internal snapshots** (savevm/loadvm, qcow2 only):
- Snapshot saved in the same image file
- Creation and deletion both with some cost
- Modify metadata, but no copy of data required
- Can contain VM state
- No live snapshots (VM stops while saving snapshot)
- Receives less testing ⇒ Stability?

- [ ] 表示并没有太看懂 Internal snapshots
    - 如果 Snapshot 都是在一个文件的，那么是不是相同的部分也拷贝了
    - 什么叫做修改 metadata 但是不用拷贝数据
    - 为什么 external VM 不能进行含有 VM state
    - 为什么不能 live snapshot 的，是不是 live snapshot 就是将运行的所有的 io 捕获下来的。

- Long-running background jobs on block devices
    - Live storage migration
    - Deleting external snapshots

**image streaing**
Use cases:
- Copy an image from a slow source in the background while running the VM
- Delete topmost external snapshots

## http://events17.linuxfoundation.org/sites/events/files/slides/talk_11.pdf

总是如此，直接穿过这里的:
- Data manipulation:
    - Filter drivers (e.g. throttle, quorum)
- Interpret image formats:
    - Format drivers (e.g. qcow2)
- Accessing host storage:
    - Protocol drivers (e.g. file, nbd)

- `BlockdevOptionsFile`

- [ ] 前面还是知道在干什么的，但是后面的就感觉活在梦里了。

### 源码分析
- block/block-backend.c 是一个中间层次，从 virtual device 经过此处，最后调用到具体的 BlockDriver 上，

调用 BlockDriver:: bdrv_co_preadv_part / bdrv_co_preadv / bdrv_aio_preadv / bdrv_co_readv
在 bdrv_driver_preadv 中，这个是有优先级的。

### 使用 block layer 作为例子

- bdrv_driver_pwritev : block/io.c 中
  * blk_aio_pwritev : 在 block-backend.c 中，这就是给 block driver 注册使用的, 例如注册到 NvmeRequest 中间去的。

```c
typedef struct NvmeRequest {
    struct NvmeSQueue       *sq;
    struct NvmeNamespace    *ns;
    BlockAIOCB              *aiocb; // 就是逐个位置了
    uint16_t                status;
    void                    *opaque;
    NvmeCqe                 cqe;
    NvmeCmd                 cmd;
    BlockAcctCookie         acct;
    NvmeSg                  sg;
    QTAILQ_ENTRY(NvmeRequest)entry;
} NvmeRequest;
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
