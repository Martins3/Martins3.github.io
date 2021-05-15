# nvme

默认的情况下，使用的 sata 协议访问，


Block devices are significant components of the Linux kernel.
In this article, we introduce the usage of QEMU to emulate some of these block devices, including SCSI, NVMe, Virtio and NVDIMM. [^1]

- [ ] SCSI 怎么回事 ?
- [ ] NVDIMM 怎么回事

- [ ] 完全可以制作多个 ext4 设备出来

实际上，按照[^1]的教程添加的参数并没有办法让内核检测到这个 nvme 设备，但是 info qtree 中可以看到, [^2] 提供更加详细的内容
最后，发现因为内核没有包含 nvme 模块而已。

- [ ] 实际上，nvme 存在一些设备穿透的操作

[^1]: https://blogs.oracle.com/linux/how-to-emulate-block-devices-with-qemu
[^2]: https://github.com/manishrma/nvme-qemu

## trace 一下 nvme 设备
- [ ] 没有了网络 stack 的拆解，如何对于 nvme 进行模拟

- [ ] qemu 下存在两个 nvme.c
