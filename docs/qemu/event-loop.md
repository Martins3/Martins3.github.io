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


## IOThread

## Main Loop

## io thread 和 main loop 交互

[^1]: [Towards Multi-threaded Device Emulation in QEMU](https://www.linux-kvm.org/images/a/a7/02x04-MultithreadedDevices.pdf)


