## vhost
可以看看 qsd
docs/qemu/block/qsd.md

- https://kvm-forum.qemu.org/2022/libblkio-kvm-forum-2022.pdf

virtio-blk + vhost-user to communicate with a user-space storage server (e.g., SPDK, qemu-storage-daemon)

这个让我知道，似乎测试 vhost 的一个简单方法就是 qemu-storage-daemon ，而不是搭建 SPDK

https://github.com/yandex-cloud/yc-libvhost-server
和 subprojects/libvhost-user/ 是什么关系?

## 这个看看，该如何使用起来
https://github.com/rust-vmm/vhost/blob/main/vhost-user-backend/src/handler.rs

## 几个小问题
2. vhost 存在帮助保管内存的情况吗?

1. 驱动对于 vhost 有感知吗?
或者说，linux kernel 中的 virtio-blk 需要由于 blk 做修改吗?

2. vhost 的 backend 如果忽然被 skill ，恢复后，可以正常运行吗?

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
