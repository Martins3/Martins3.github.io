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
  - 指向更多的和板子相关的文档，比如 x86 架构的 microvm 和 pc-i440fx
- Security
  - [ ] 似乎阐述了一些和安全可能存在关联的位置
- Multi-process QEMU
  - [ ] 看看 devel/multi-process 的内容
- Deprecated features
- Removed features
- Supported build platforms
- License
## User Mode Emulation
三件事应该是非常复杂的:
1. syscall 模拟
2. signal handling
3. clone

- [ ] 至于为什么信号处理很麻烦，暂时并不是很清楚啊!

## Tools
> Qemu 每次每次编译完成之后还会生成一堆工具，这里讲解这些工具都是怎么使用的

- QEMU disk image utility
- QEMU Storage Daemon
- QEMU Disk Network Block Device Server
- QEMU persistent reservation helper
- QEMU SystemTap trace tool
- QEMU 9p virtfs proxy filesystem helper
- QEMU virtio-fs shared file system daemon

## System Emulation Management and Interoperability
- Dirty Bitmaps and Incremental Backup
  - [ ] 应该就是分析 /block/dirty-bitmap.c 位置的代码
  - [ ] 按道理这个和 migration 可以存在一些联系，毕竟内存需要迁移，disk 也是需要迁移的
- D-Bus
  - Using a bus, helper processes can discover and communicate with each other easily, without going through QEMU. 
  - [ ] https://github.com/makercrew/dbus-sample 教程
  - [ ] 但是 dbus 需要考虑安全问题
- D-Bus VMState
- Live Block Device Operations
- Persistent reservation helper protocol
- QEMU Guest Agent
- QEMU Guest Agent Protocol Reference
- QEMU QMP Reference Manual
- QEMU Storage Daemon QMP Reference Manual
  - [ ] :star2: 非常长的文档
  - [ ] 和上面的 Live Block Device Operations 的内容，应该是对应 block.c / blockdev-nbd.c / blockdev.c / blockjob.c 几个文件
- Vhost-user Protocol
- Vhost-user-gpu Protocol
- Vhost-vdpa Protocol
## System Emulation Guest Hardware Specifications
## Developer Information
- Code of Conduct
- Conflict Resolution Policy
- The QEMU build system architecture
- QEMU Coding Style
- QEMU and Kconfig
- Testing in QEMU
- Fuzzing
- Control-Flow Integrity (CFI)
- Load and Store APIs
  - [ ] 干啥的 ？
- The memory API
  - [ ] 干啥的 ?
- Migration
  - [ ] 值得分析一下
- Atomic operations in QEMU
  - [ ] ?
- QEMU and the stable process
- QTest Device Emulation Testing Framework
- Decodetree Specification
- Secure Coding Practices
- Translator Internals
  - [ ] 全文背诵
- TCG Instruction Counting
- Tracing
- Multi-Thread TCG
- QEMU TCG Plugins
- Bitwise operations
- Reset in QEMU: the Resettable interface
- Modelling a clock tree in QEMU
  - [ ] 分析 clock 的构建，既不知道为什么叫做 clock tree 也不知道为什么构建这个需要这么多内容
- The QEMU Object Model (QOM)
  - 绝对的重点
- block-coroutine-wrapper
- Multi-process QEMU
