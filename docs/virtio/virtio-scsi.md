# virtio scsi

## virtio-blk 和 virtio-scsi 对比
- [ ] https://mpolednik.github.io/2017/01/23/virtio-blk-vs-virtio-scsi/
- [ ] https://www.qemu.org/2021/01/19/virtio-blk-scsi-configuration/
  - 这就是最权威的

- https://wiki.qemu.org/images/c/c2/Virtio-scsi.pdf
  - designed to replace virtio-blk ，有趣的措辞

- [ ] [kvm forum : virtio scsi](https://www.linux-kvm.org/images/archive/f/f5/20110823142849!2011-forum-virtio-scsi.pdf)
  - 介绍了对比和 virtio-blk  ，一些高级话题没太看懂

- 为什么这个文章说，是可以使能 500 个 virito-blk 设备，但是上面的文章又说最多 32 个:
  - http://www.wowotech.net/linux_kenrel/509.html
  - https://github.com/spdk/spdk/issues/162

- [stackoverflow : why is virtio-scsi much slower than virtio-blk in my experiment (over and ceph rbd image)?](https://stackoverflow.com/questions/39031456/why-is-virtio-scsi-much-slower-than-virtio-blk-in-my-experiment-over-and-ceph-r)

整体来说，实际上 virtio-scsi 和 virtio-blk 不存在明显的性能差别，都是生产环境中可以使用的工具。

## 从内核的角度分析

具体实现放到 drivers/scsi/virtio_scsi.c 中，代码量很少 ，只有大约 1000 行

virtio-scsi 和 megasas 等相同，都是 HBA 卡

```c
static const struct scsi_host_template virtscsi_host_template = { /* ... */};
static const struct scsi_host_template megaraid_template = { /* ... */};
```

在 virtscsi_host_template 中注册了提供给 mq ops 的函数
```txt
#0  virtscsi_queuecommand (shost=0xffff88810b698000, sc=0xffff88810be4b3b8) at drivers/scsi/virtio_scsi.c:563
#1  0xffffffff81a5ac20 in scsi_dispatch_cmd (cmd=0xffff88810be4b3b8) at drivers/scsi/scsi_lib.c:1517
#2  scsi_queue_rq (hctx=<optimized out>, bd=<optimized out>) at drivers/scsi/scsi_lib.c:1759
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
