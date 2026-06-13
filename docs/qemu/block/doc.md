# https://www.linux-kvm.org/images/b/b5/2012-fourm-block-overview.pdf

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

# http://events17.linuxfoundation.org/sites/events/files/slides/talk_11.pdf

总是如此，直接穿过这里的:
- Data manipulation:
    - Filter drivers (e.g. throttle, quorum)
- Interpret image formats:
    - Format drivers (e.g. qcow2)
- Accessing host storage:
    - Protocol drivers (e.g. file, nbd)

- `BlockdevOptionsFile`

- [ ] 前面还是知道在干什么的，但是后面的就感觉活在梦里了。

## 源码分析
- block/block-backend.c 是一个中间层次，从 virtual device 经过此处，最后调用到具体的 BlockDriver 上，

调用 BlockDriver:: bdrv_co_preadv_part / bdrv_co_preadv / bdrv_aio_preadv / bdrv_co_readv
在 bdrv_driver_preadv 中，这个是有优先级的。

## 使用 block layer 作为例子

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
