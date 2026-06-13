Domain U HVM Guests 利用 Qemu-DM[^1]

A new feature in Xen designed to improve overall performance and reduce the load on
the Dom 0 Guest is PCI Passthru which allows the Domain U Guest to have direct access
to local hardware without using the Domain 0 for hardware access.[^1]

Domain 0 has responsibility for all devices on the system.
Normally, as it discovers PCI devices, it passes those to drivers within the Linux kernel.
In order for a device to be accessed by a guest, the device must instead be assigned to a special domain 0 driver.
This driver is called xen-pciback in pvops kernels, and called pciback in classic kernels.
PV guests access the device via a kernel driver in the guest called xen-pcifront (pcifront in classic xen kernels),
which connects to pciback. HVM guests see the device on the emulated PCI bus presented by QEMU.[^2]

[^1]: http://www-archive.xenproject.org/files/Marketing/HowDoesXenWork.pdf
[^2]: https://wiki.xenproject.org/wiki/Xen_PCI_Passthrough

## 先看看如何使用吧
- https://www.qemu.org/docs/master/system/i386/xen.html
- https://wiki.xenproject.org/wiki/Xen_ARM_with_Virtualization_Extensions/qemu-system-aarch64

## xen 的解决方案，有趣
https://xcp-ng.org/#easy-to-install

## 我理解 vmbus 和 xenbus 和 virtio 都是类似的机制吧

## 看看
https://news.ycombinator.com/item?id=45406573

## 回答一个关键问题，为什么 xen 逐渐的衰弱了

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
