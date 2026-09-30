# vdpa sim 实验

参考:
https://www.redhat.com/en/blog/hands-vdpa-what-do-you-do-when-you-aint-got-hardware-part-1

## codex
有三个相关模块：

 模块            作用
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 vdpa_sim        公共模拟器框架，也就是你正在看的代码
──────────────  ──────────────────────────────────────────────────
 vdpa_sim_blk    模拟块设备，当前源码使用 128 MiB 内存盘
──────────────  ──────────────────────────────────────────────────
 vdpa_sim_net    模拟网络设备，把通过过滤的 TX 报文复制到 RX 队列

加载 vdpa_sim_blk 或 vdpa_sim_net 后，先得到的是用于创建设备的 management device；还需要执行 vdpa dev add 才会产生具体设备实例。

最容易体验的是：让宿主机直接使用模拟块设备。

数据路径如下：

宿主机应用读写 /dev/vdX
  → virtio_blk
  → virtio_vdpa
  → vdpasim_config_ops
  → vdpa_sim_blk
  → 内存中的磁盘数据

下面以测试环境中尚未加载这些模块、没有 vhost_vdpa 抢先绑定设备为前提。运行内核需要启用对应配置并安装匹配的模块，用户态需要 iproute2 的 vdpa 工具。

```txt
# 加载模拟器；batch_mapping=0 用来走你正在看的那张 ops 表
sudo modprobe vdpa_sim batch_mapping=0 use_va=0
sudo modprobe vdpa_sim_blk

# 让宿主机把 vDPA 设备作为 virtio 设备使用
sudo modprobe virtio_vdpa
sudo modprobe virtio_blk

# 查看用于创建设备的管理入口，应包含 vdpasim_blk
sudo vdpa mgmtdev show

# 创建一个模拟块设备实例
sudo vdpa dev add name simblk0 mgmtdev vdpasim_blk

# 查看设备和绑定的驱动
sudo vdpa dev show simblk0
readlink /sys/bus/vdpa/devices/simblk0/driver

# 查找新出现的 128 MiB 磁盘
lsblk -o NAME,SIZE,MODEL
```

预期绑定驱动是 virtio_vdpa，随后出现一个 /dev/vdX 块设备，具体名称由系统分配。它可以像普通磁盘一样用于文件系统或 I/O 测试，但数据存在内存中，默认情况下
删除设备后就会丢失。

这里 simblk0 是 vDPA 设备名，与块设备名 /dev/vdX 不同。创建逻辑对应 vdpasim_blk_dev_add() (drivers/vdpa/vdpa_sim/vdpa_sim_blk.c:402)。

用完并停止 I/O、卸载文件系统后，删除实例：

sudo vdpa dev del simblk0

上述模块参数只在首次加载时生效；对已经加载的模块再次运行 modprobe 不会更改参数。

另一种方式是：通过 vhost_vdpa 提供给虚拟机。

此时宿主机上的 vDPA 设备绑定 vhost_vdpa，产生 /dev/vhost-vdpa-N 字符设备，供 QEMU 打开：

guest 中的 virtio 驱动
  → QEMU 配置设备、建立通知通道
  → 宿主机 vhost_vdpa
  → vdpa_sim
  → 模拟器处理 virtqueue 请求

以网络设备为例，创建 vdpasim_net 实例并绑定 vhost_vdpa 后，可以向已有 QEMU 启动命令添加类似参数：

-netdev vhost-vdpa,id=net0,vhostdev=/dev/vhost-vdpa-0
-device virtio-net-pci,netdev=net0

路径中的编号要使用实际设备编号；这只是网络参数片段。QEMU 的该后端接口见 官方文档 (https://www.qemu.org/docs/master/system/qemu-manpage.html)。

同一个实例只能绑定 virtio_vdpa、vhost_vdpa 中的一个。另需注意，当前 vdpa_sim_net 实现的是报文回环，创建后不会自动获得通往外部网络的连接。对应代码见
vdpasim_net_work() (drivers/vdpa/vdpa_sim/vdpa_sim_net.c:196)。

如果目的是跟踪前面那张操作表，先使用宿主机内存盘这条路径就能观察到：创建设备时调用特性和队列配置回调，读写磁盘时进入 kick_vq → vdpasim_blk_work，完成后
通过队列回调通知 virtio_blk。


## 之前的记录

物理机中:
```txt
sudo modprobe vdpa
sudo modprobe vhost_vdpa
sudo modprobe vdpa_sim
sudo modprobe vdpa_sim_net
```

1. vdpa mgmtdev show

```txt
vdpasim_net:
  supported_classes net
  max_supported_vqs 3
  dev_features MTU MAC STATUS CTRL_VQ CTRL_MAC_ADDR ANY_LAYOUT VERSION_1 ACCESS_PLATFORM
vdpasim_blk:
  supported_classes block
  max_supported_vqs 0
  dev_features
vduse:
  supported_classes net block
  max_supported_vqs 0
  dev_features
```

sudo vdpa dev add name vdpa0 mgmtdev vdpasim_net mac 00:e8:ca:33:ba:05
sudo vdpa dev show -jp

```txt
sudo qemu-kvm \
 -drive file=/home/test/L1.qcow2,media=disk,if=virtio \
 -net nic,model=virtio \
 -net user,hostfwd=tcp::2226-:22 \
 -netdev type=vhost-vdpa,vhostdev=/dev/vhost-vdpa-0,id=vhost-vdpa0 \
 -device virtio-net-pci,netdev=vhost-vdpa0,bus=pcie.0,addr=0x7 \
 disable-modern=off,page-per-vq=on \
 -nographic \
 -m 4G \
 -smp 4 \
 -cpu host \
```

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
