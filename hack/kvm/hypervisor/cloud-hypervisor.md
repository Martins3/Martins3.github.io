# Cloud Hypervisor

这个项目的存在，意味着完全没有任何的必要看 kvmtool 的了。

- 无论如何，那么存储是如何进行的
  - 既然可以使用 docker 来构成 rootfs 的话，其实最后必然也是 qcow2 了吧
    - 所以，其将 QEMU 中的什么东西简化掉吗?

- migration 设计的好简单啊

- 是如何使用 vhost 的哇

主要的目录:
- hypervisor
- vmm

## 甚至还有 vhdx
-  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-vhdx/83e061f8-f6e2-4de1-91bd-5d518a43d477
