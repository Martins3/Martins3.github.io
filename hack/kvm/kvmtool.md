# kvmtool

## how to use
```
lkvm run -k ../linux/arch/x86/boot/bzImage -d /home/maritns3/core/linux-kernel-labs/tools/labs/core-image-minimal-qemux86.ext4
```

- [ ] lkvm setup minimi 可以创建一个 rootfs, 但是无法启动

https://www.96boards.org/blog/running-kvm-guest-hikey/

## vfio

device__register


## pci
- [ ]  pci__init
    - [ ] 如果 pci__init 的时候告诉了那些位置是 mmio 或者 pio 的，是不是说，在 guest 中间 /proc/meminfo 的结构相应的发生变化
    - [ ] 去 stackoverflow 问一个问题，为什么不让物理地址空间到处都是洞，我猜测是历史遗留问题，但是真的到了没有办法修改吗 ?

- [ ] virtio_pci__init : bar 寄存器设置 mmio 的大小都是 PCI_IO_SIZE, 这好吗 ?
