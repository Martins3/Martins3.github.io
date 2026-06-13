## QEMU Event Loop

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

才意识到为了处理 /home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2, 不仅仅需要
block/qcow2.c 而且需要 block/file-posix.c 来进行文件的 IO


# block

也许了解一下这个经典设备可以帮助我来理解 aio 之类的蛇皮到底在干什么

- [ ] 将 glib.md 中的测试分析首先放一下吧, 先分析 block/null.c

## https://www.linux-kvm.org/images/b/b5/2012-fourm-block-overview.pdf

- virtual device
  - IDE, virtio-blk
- Backend
  - block driver
    - raw, qcow2
    - file, nbd(networking block device), iscsi(nternet Small Computer Systems Interface), gluster(Gluster is a scalable network filesystem. Using common off-the-shelf hardware, you can create large, distributed storage solutions for media streaming, data ...)
  - I/O throttling, copy on read
- block jobs
  - Streaming, mirroring, commit,

```txt
aio=aio
  aio is "threads", or "native" and selects between pthread based disk I/O and native Linux AIO.
```

## 源码分析
- block/block-backend.c 是一个中间层次，从 virtual device 经过此处，最后调用到具体的 BlockDriver 上，

调用 BlockDriver:: bdrv_co_preadv_part / bdrv_co_preadv / bdrv_aio_preadv / bdrv_co_readv
在 bdrv_driver_preadv 中，这个是有优先级的。

## qemu_notify_event 这个函数是做什么的

```c
static void notify_event_cb(void *opaque)
{
    /* No need to do anything; this bottom half is only used to
     * kick the kernel out of ppoll/poll/WaitForMultipleObjects.
     */
}
```

[^1]: [Towards Multi-threaded Device Emulation in QEMU](https://www.linux-kvm.org/images/a/a7/02x04-MultithreadedDevices.pdf)
