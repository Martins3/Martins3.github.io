# qemu overview

## kvm
4.2 中 cpus.c::qemu_init_vcpu 中间分析了多种 kvm 和 tcg 的引擎, 由于 tcg 和 kvm 被放到 accel 下，但是 
whpx, hvf[^1] 和 hax 的支持都是 intel 特有的，所以在 qemu/target/i386 下面, 说着是加速，其实这应该是 exec engine, 在 latest 的版本，这一些内容都被移动到这里了

对于 kvm 支持的位置:
/home/maritns3/core/kvmqemu/hw/i386/kvm : 主要是设备模拟
/home/maritns3/core/kvmqemu/accel/kvm


## structure
- 入口应该是 ./softmmu/main.c

- virtio
  - hw/block/virtio-blk.c
  - hw/net/virtio-blk.c
  - hw/virtio

- hw/vfio

## 问题
- [ ] block 和 chardev 的作用是什么 ?
- [ ] tcg 整个到底覆盖的代码在什么位置 ?

[^1]: https://developer.apple.com/documentation/hypervisor
