会用 vhost-user-scsi 也是不错的

## 介绍了如何使用 vhost net 的
https://docs.nxp.com/bundle/GUID-C3A436DA-E944-4F73-9811-2335DEBD04D6/page/GUID-D87AF5A4-BC68-4F11-8F36-AEF391D91BDC.html

还是需要搭建一个网络，需要了解下



## 用用 user
name "vhost-user-blk", bus virtio-bus
name "vhost-user-blk-pci", bus PCI
name "vhost-user-blk-pci-non-transitional", bus PCI
name "vhost-user-blk-pci-transitional", bus PCI
name "vhost-user-fs-device", bus virtio-bus <--- 这个应该是 virtio fs 的实现吧
name "vhost-user-fs-pci", bus PCI
name "vhost-user-scsi", bus virtio-bus
name "vhost-user-scsi-pci", bus PCI
name "vhost-user-scsi-pci-non-transitional", bus PCI
name "vhost-user-scsi-pci-transitional", bus PCI

name "vhost-user-gpio-device", bus virtio-bus
name "vhost-user-gpio-pci", bus PCI
name "vhost-user-i2c-device", bus virtio-bus
name "vhost-user-i2c-pci", bus PCI
name "vhost-user-input", bus virtio-bus
name "vhost-user-input-pci", bus PCI
name "vhost-user-rng", bus virtio-bus
name "vhost-user-rng-pci", bus PCI

https://stackoverflow.com/questions/75906208/how-to-connect-via-virtio-gui-running-on-host-with-gpio-in-a-qemu-emulated-virtu

不对，实际上我们早就用上，那就是 virtio-fs 啊!

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
