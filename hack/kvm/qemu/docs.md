# [Official Docs](https://qemu.readthedocs.io/en/latest/index.html)

## TODO 
- -net user 和 TAP 设备模拟网络区别 ?
- failover 机制

## System Emulation
内容快速检查:

- Quick Start
- Invocation
  - Man qemu-system(1) 还讲解了一共模拟什么设备
- Keys in the graphical frontends
- Keys in the character backend multiplexer
- QEMU Monitor
- Disk Images
  - 解释 ./block
- Network emulation
  - 解释 ./net
- QEMU virtio-net standby (net_failover)
- USB emulation
- Inter-VM Shared Memory device
  - 可以创建一个 memory device 让多个 vm 共享，而且还可以添加中断支持
- Direct Linux Boot
  - [ ] 所以非 Direct Linux Boot 是怎么操作的
- Generic Loader
  - 设置 Guest 的内存区域的数值, 具体实现在 hw/core/generic-loader.c
- Guest Loader
- VNC security
- TLS setup for network services
- GDB usage
  - [ ] 所以，当存在 kvm 时候，如何实现单步调试的
- Managed start up options
  - 介绍 --preconfig 选项，用于修复问题
- Virtual CPU hotplug
- virtio pmem
  - Virtio pmem allows to bypass the guest page cache and directly use host page cache. This reduces guest memory footprint as the host can make efficient memory reclaim decisions under memory pressure.
  - [ ] @todo 太棒了，终于可以借此分析 pmem 的内容了
  - 类似的机制还存在吗 ? 比如如果文件系统过于考虑在物理机器上运行的情况，但是实际上在虚拟机上，这些都是过度优化
- Persistent reservation managers
- QEMU System Emulator Targets
- Security
- Multi-process QEMU
- Deprecated features
- Removed features
- Supported build platforms
- License
## User Mode Emulation
## Tools
## System Emulation Management and Interoperability
## System Emulation Guest Hardware Specifications
## Developer Information
 
qemu-system-x86_64 -kernel ~/xen.git/xen/xen   -append "dom0_mem=1G,max:1G loglvl=all guest_loglvl=all"   -device guest-loader,addr=0x42000000,kernel=Image,bootargs="root=/dev/sda2 ro console=hvc0 earlyprintk=xen"   -device guest-loader,addr=0x47000000,initrd=rootfs.cpio
