## 可以总结一下各个版本更新

参考这个东西:
https://blog.vmsplice.net/2025/12/whats-new-in-virtio-14.html
(为什么 virtio 还要添加 spi 和 can 总线的支持啊)

## 如何理解 hw/virtio/virtio-bus.c

启动的时候有这个输出：
```txt
#0  virtio_bus_set_host_notifier (bus=0x555558de37b0, n=n@entry=0, assign=assign@entry=true) at /home/martins3/core/qemu/include/hw/virtio/virtio-bus.h:135
#1  0x0000555555c303f2 in virtio_blk_start_ioeventfd (vdev=<optimized out>) at ../hw/block/virtio-blk.c:1667
#2  0x0000555555a87e87 in virtio_bus_start_ioeventfd (bus=bus@entry=0x555558de37b0) at ../hw/virtio/virtio-bus.c:236
#3  0x0000555555a8ab74 in virtio_pci_start_ioeventfd (proxy=0x555558ddb3d0) at ../hw/virtio/virtio-pci.c:375
#4  virtio_pci_common_write (opaque=0x555558ddb3d0, addr=<optimized out>, val=<optimized out>, size=<optimized out>) at ../hw/virtio/virtio-pci.c:1615
```
以前 arm 环境中 qemu 启动的过程中存在如下问题，存在这个日志，但是现在无法复现了

```txt
(qemu) qemu-system-aarch64: virtio_bus_set_host_notifier: unable to init event notifier: Too many open files (-24)
virtio-blk failed to set host notifier (-24)
qemu-system-aarch64: virtio_bus_start_ioeventfd: failed. Fallback to userspace (slower).
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
