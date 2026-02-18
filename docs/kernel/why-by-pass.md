# Kernel Bypass

Linux 内核并不是为了极致性能而设计的，它追求的是通用性、兼容性、安全性，而不是专门服务某一种 workload。因此，如果我们能找到一种方式，将某些逻辑移出内核、交给用户态，自然可以获得更大的灵活性以及潜在的性能提升。

* **容错性更高**：一个用户态程序异常，重启就可以了。
* **升级方便** : 升级内核需要关机
* **开发，调试友好** : 能直接用 valgrind / gdb / sanitizers 进行调试，而不是折腾内核调试设施。你可以使用任何语言在用户态进行开发，
而在内核中，截止 2025-12-05 ，基本上只能用 C 开发，Rust 的限制还很大。
* **难以测试验证** : 内核的大部分 corner case、race condition、设备兼容性问题，往往是在超大规模才会遇到，这也意味着：
	* 少量机器根本无法覆盖内核的复杂性。
	* 很多公司根本没有工程能力长期维护一个深度魔改的内核。
* **权限问题**
	* fuse squash
	* rootless container

内核越来越多地在做“资源管理”和“隔离”，而将策略和高速路径交给用户态

复杂的文件系统，将功能放到内核中，真的好吗?
当然是不好的，但是用户态文件系统的性能实在是太差了

## 哪些内核功能可以放到用户态？

### 直接访问硬件
* **UIO（简单粗暴的 mmap BAR）**
* **VFIO（安全隔离 + IOMMU）**

相关的配套设施，vfio iommufd iommu sriov siov

整体来说，和驱动打交道，就三个问题:
1. 如何响应中断
2. 数据面 (data plane) : 如何给设备发送命令
3. 控制面 (control plane) : 如何设备发送数据

中断的解决办法:
1. 用户态中断。从 vfio 的中断直接注入的到虚拟机中就可以想到，中断也是可以直接注册到用户态的
	- [User-space interrupts](https://lwn.net/Articles/871113/)
2. 使用轮询

数据面主要使用 iommu 和 共享内存

控制面由于在低速路径上，总体来说，就比较容易了，一般通过 ioctl 来转发命令就可以了。

直接访问硬件的最经典的例子是 DPDK (用户态驱动) 和 将驱动直通到虚拟机，驱动让虚拟机中的驱动管理。
也有一些其他的有意思的例子，例如 qemu 的 block/nvme.c 中，可以直接将物理机中的 nvme 盘作为虚拟机中盘，
例如配置如下参数:
```txt
-blockdev driver=nvme,node-name=disk_backend,device=0000:03:00.0,namespace=1
-device virtio-blk-pci,drive=disk_backend
```
在虚拟机中就可以观察到:
```txt
vdb                251:16   0   1.5T  0 disk
├─vdb1             251:17   0 745.2G  0 part
└─vdb2             251:18   0 745.2G  0 part
```

* libusb : 提供对 USB 设备的直接接口，允许向任意连接的 USB 设备发送任意 USB 命令。
可以实现很多有意思的事情，例如把物理的 usb 鼠标直通到虚拟机中，把 usb 插入到一个物理机中，然后通过网络，
将这个 usb 暴露到另外一个物理机上。
* sg 和 bsg : 通过 /dev/sg* 直接给盘发送 scsi 命令，推荐使用 `libsgutils`
* I2C 和 SPI : 通过 /dev/i2c-* 进行操作
* GPIO 和 电源稳压器（power regulators）: 只需在设备树（device tree）中添加相应描述，设备就会在 sysfs 中出现，并带有可读写属性文件。
* 以 `O_DIRECT` 标志打开块设备，可绕过页缓存（page cache）以及其支持的预读（readahead）和写回（write-behind）机制，实现对设备的直接读写。
* 通过打开 TTY 设备并禁用某些 termios 设置（如 `ECHO` 和 `ICANON`），可实现对串口的直接访问。
* **io_uring uring cmd**  : 实现用户态 nvme 驱动
* ulef / uinput : 下面的实验会具体提到，这两个代码看了，我估计上面除了 io_uring uring cmd 外，其他都可以理解了。
* 某种意义上讲，qemu 作为 type2 的 vmm 相对于 type 1 vmm 而言，就是 kernel 功能放到用户态

### 提供虚拟驱动
* **文件系统**
	- FUSE 是最典型的例子
	- nfs server
	- cephfs : 后端是 ceph 集群而非物理磁盘
* **网络栈**：
	- TAP sends and receives Ethernet frames, so any networking protocol can be used with a TAP device.
	- TUN works at the IP level, which is simpler and often sufficient providing there is no need to handle non-IP protocols such as ARP. These can most obviously be used for tunneling and creating virtual private networks (VPNs) but could also be used for user-space monitoring and filtering of network traffic.
	- 对于网络设备，可通过创建 `AF_PACKET` 地址族、`SOCK_RAW` 类型的套接字实现直接访问。该套接字可绑定到特定网络接口或特定的以太网协议类型。如果使用 `AF_INET` 配合 `SOCK_RAW`，虽然仍会利用 IP 协议通用的路由等功能，但可以完全控制每个 IP 数据包的有效载荷。
	- ovs 相对于 linux bridge 来很多逻辑放到了用户态
	- AF_ALG 相关问题
* **网络设备协议**
    - nvme over tcp
    - iSCSI
    - [aoe](https://docs.kernel.org/admin-guide/aoe/index.html)
    - [nbd](https://docs.kernel.org/admin-guide/blockdev/nbd.html)
* **模拟块设备**
	* ublk
	* [tcmu](https://www.kernel.org/doc/html/latest/target/tcmu-design.html)
		- https://github.com/containerd/overlaybd/blob/main/docs/README.md
	* vduse : 让用户态实现 block / net / 其他 virtio 设备
* **userfaultfd**：用户态 page fault 处理

### 用户态策略
基于 bpf 的 struct ops 可以将部分逻辑移动到用户态，目前支持的功能为:
-  scheduler : https://github.com/sched-ext/scx
-  oom killer (未进入主线)
-  hid 驱动
-  tcp congestion control

参考 https://docs.ebpf.io/linux/program-type/BPF_PROG_TYPE_STRUCT_OPS/tcp_congestion_ops/

## 这些年在用户态驱动的进展
- ebpf struct ops
- io_uring
- vfio / iommu 替代 UIO

## 进一步的思考

1. 相同的功能其实可以放到 用户态，内核态，固件中(nvidia GSP 固件)，CPU 中，加速器中，例如对于
virtio ，可以存在一下形态
	1. kernel 中的 vhost-net vhost-scsi
	2. qemu 中 hw/net/virtio-net.c
	3. vhost-user : dpdk 实现
	4. vduse : 用户态进程实现 virtio 驱动
	5. vdpa : 硬件实现，可能是 ASIC 电路，也可能是 FPGA
	5. 智能网卡，类似的 ovs 也可以放到硬件中
3. 同样的，存储的案例，内核的 block 层的 raid / device mapper / bcache
	- 在用户态的 ceph 之类的存储产品中都有实现
	- 也可以在文件系统中实现，例如 zfs 中
5. 微内核算是一个解决的解决方案吗？我理解是可以的
3. https://github.com/theseus-os/Theseus 通过 rust 来消除用户态、内核态的操作系统
6. https://github.com/Martins3/bmbt : 将 QEMU 放到裸机上运行
8. https://engineering.fb.com/2024/03/12/data-center-engineering/building-metas-genai-infrastructure/
6. https://github.com/topics/kernel-bypass

## 为了实现用户态驱动的努力
- 硬件上，性能和功能更加强大的 iommu
- 内核中对于共享内存的开发，减少如何减少和内核的交互和数据拷贝，例如 iouring 和 dmabuf
- 更加精细的暴露到用户态的接口，例如从 vfio container 到 iommufd
- 内核各类子系统的持续优化，例如 fuse 对于 iouring cmd / iomap 的支持

## 实验

### ulef
内核新增了一个设备 `/dev/uleds` ，算是非常经典了:

- Documentation/leds/uleds.rst
- drivers/leds/uleds.c
- tools/leds/uledmon.c

```txt
sudo ./a.out mmmm
```

打开另外一个窗口，find /sys -name mmmm ，
然后就可以观察到这个目录:
```txt
/sys/class/leds/mmmm
/sys/devices/virtual/misc/uleds/mmmm
```

cd /sys/devices/virtual/misc/uleds/mmmm 中，
echo 10 | sudo tee brightness


那么之前的 a.out 就会有输出

### uinput
- drivers/input/misc/uinput.c
- Documentation/input/uinput.rst
- 测试项目，使用 https://github.com/Martins3/rust-uinput ，使用 cargo build 之后，执行 exampes/hello ，
那么 vnc 屏幕上有 hello world

## 关键参考
[Linux drivers in user space — a survey](https://lwn.net/Articles/703785/)
* **Upstream interface**：让用户态更直接地访问硬件
* **Downstream interface**：让用户态模拟硬件，供其他用户态程序使用

https://lwn.net/Articles/703785/

When one reflects on the tree-like nature of the driver model, as described in [an earlier article](https://lwn.net/Articles/645810/), it is clear that there is a chain, or path, of drivers from the root out to the leaves, each making use of services provided by the driver closer to the root (or "upstream") and providing services to the driver closer to the leaf (or "downstream").
- An upstream interface allows a user-space program to directly access services provided by the kernel that normally are only accessed by other kernel drivers.
- A downstream interface allows a user-space program to instantiate a new device for some specific kernel driver, and then provide services to it that would normally be provided by some other kernel driver.

Where upstream interfaces provide direct access to hardware, downstream interfaces allow a program to emulate some hardware and so provide access to other programs that expect to use a particular sort of interface. Rather than just providing a different sort of access to an already existing device, a downstream interface must make it possible to create a new device, configure it, and then provide whatever functionality is expected of that device type.

Similarly there is a downstream interface for writing filesystems that **is careful about how it interfaces with the page cache**,
and manages to avoid the writeback deadlocks described above: FUSE.
(如果可以解决，那么 nfs 应该也可以解决才对的啊)

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
