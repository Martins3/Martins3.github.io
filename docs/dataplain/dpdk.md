# DPDK 基础介绍
乱七八糟的介绍:
- https://talawah.io/blog/linux-kernel-vs-dpdk-http-performance-showdown/
- https://doc.dpdk.org/guides/prog_guide/index.html#
- https://zhuanlan.zhihu.com/p/608143587

参考这个:
- https://core.dpdk.org/doc/quick-start/

## 先用起来吧
- https://github.com/IfanTsai/oceanus
- https://github.com/scylladb/seastar

## 可以从 examples/ 看看

## 如何使用 none root 来执行 dpdk
https://stackoverflow.com/questions/66571932/can-you-run-dpdk-in-a-non-privileged-container-as-as-non-root-user/69178969#comment122283933_69178969
- https://github.com/IfanTsai/oceanus : 有趣的参考

# 我认为官方文档已经清晰的不得了
- https://doc.dpdk.org/guides/prog_guide/index.html#

- [ ] 所以如何正确的编译来着

## dpdk
- [dpdk 和 network stack 的关系](https://stackoverflow.com/questions/33292630/using-dpdk-to-create-a-tcp-ip-connection)
    - dpdk 只有 API，是没有网络栈的实现的，一些具体的实现:
    - 其对应的用户态的部分: https://github.com/F-Stack/f-stack
        - 核心开发人员的 blog :  http://lovelyping.com/
- [ ] https://blog.selectel.com/introduction-dpdk-architecture-principles/ ：基本原理介绍。
- https://www.dpdk.org/wp-content/uploads/sites/35/2017/04/DPDK-India2017-RamiaJain-ArchitectureRoadmap.pdf
    - 从 17 页之后开始，

## [ ] 如何 vDPA 封装 virtio 硬件的，为什么 vDPA 还出现在 DPDK 的代码中

## PMD 是什么
poll mode driver 的简称。

- https://doc.dpdk.org/guides/prog_guide/poll_mode_drv.html

## 可以在虚拟机中安装 dpdk 然后测试吗
- [ ] 虚拟机中使用 dpdk 就是说虚拟机中需要使用 IOMMU 吧，所以可以将 IOMMU 虚拟化吗?
- [ ] 嵌套虚拟机中可以直通吗?
- [ ] 可以将 RDMA 和 DPDK 一起使用吗?

## 真的是使用 VFIO 的吗

This is opposed to a Linux kernel where we have a scheduler and interrupts for switching between processes, in the DPDK architecture the devices are accessed by constant polling.[^1]
- 有趣啊，没有 scheduler 和 interrupts 了。

[^1]: https://www.redhat.com/en/blog/how-vhost-user-came-being-virtio-networking-and-dpdk

## [ ] dpdk 的源码中只是看到了对于 NIC 的 driver，但是网络栈在什么地方不知道


## [ ] 针对于 dpdk 的这些优化，为什么内核不能搞一个对应的模式
- 内核也可以直接将分配大页，或者使用 CPU 总是 poll
    - 可能，内核的改动太大了，还是存在无法逾越的挑战。


## 值得重点关注的几个 library
- mempool
- mbuf
- ring

## [ ] 居然也有 bpf，但是其中是做啥的哇


## DPDK 中间存在 virtio，如何理解
- [ ] 之前的看法中，DPDK 直接操作硬件的，除非其操作的硬件支持 virtio。
    - vhost 本来是 virtio 中用于加速 dataplane 的

## 简单分析一下 dpdk 和 vhost 关联的部分 ： https://www.cnblogs.com/ck1020/p/8341914.html

在 `net/vhost/rte_eth_vhost.c` 中定义了：

```c
static const struct eth_dev_ops ops = {
    // ...
    .dev_configure = eth_dev_configure,
    // ...
```

- `eth_dev_configure`
    - `vhost_driver_setup`
        - `rte_vhost_driver_register` ：这个是其 driver 的哇
            -  `create_unix_socket` ： 漫长的初始化参数 vsockets 的内容，然后调用此函数。
        - `rte_vhost_driver_start`
            - `rte_ctrl_thread_create` -> `fdset_event_dispatch`
            - `vhost_user_start_server`
                - `vhost_user_server_new_connection`
                    - `vhost_user_add_connection`
                        - `vhost_user_read_cb`
                            - `vhost_user_msg_handler` ：在这里处理和 QEMU 的交互
            - `vhost_user_start_client`
                - `vhost_user_add_connection`
                    - `vhost_user_read_cb`

## memory 共享

在 `vhost_user_msg_handler` 中，`VHOST_USER_SET_MEM_TABLE` 对应位置的 hook 为: `vhost_user_set_mem_table`
```c
VHOST_MESSAGE_HANDLER(VHOST_USER_SET_MEM_TABLE, vhost_user_set_mem_table, true) \
```

## 简单分析 dpdk 架构
- `virtio_user_dev_init` ：这个结构和 QEMU 中 `vhost_dev_init` 好对称啊
    - `virtio_user_dev_setup` ：注册三种 ops
        - `virtio_ops_user`
        - `virtio_ops_kernel`
        - `virtio_ops_vdpa`
    - `::set_owner`
    - `::get_backend_features`


- [ ] 应该尝试理解一下，为什么 `virtio_ops_user` 的替代者都是谁，都是什么定位的?
```c
struct virtio_user_backend_ops virtio_ops_user = {
```

## [ ] 从 pmd 到 vhost 的 qeueu 的注入的过程



driver 使用 null 作为例子:
```c
static struct rte_vdev_driver pmd_null_drv = {
	.probe = rte_pmd_null_probe,
	.remove = rte_pmd_null_remove,
};

static const struct eth_dev_ops ops = {
	.dev_close = eth_dev_close,
	.dev_start = eth_dev_start,
	.dev_stop = eth_dev_stop,
	.dev_configure = eth_dev_configure,
	.dev_infos_get = eth_dev_info,
	.rx_queue_setup = eth_rx_queue_setup,
	.tx_queue_setup = eth_tx_queue_setup,
	.rx_queue_release = eth_rx_queue_release,
	.tx_queue_release = eth_tx_queue_release,
	.mtu_set = eth_mtu_set,
	.link_update = eth_link_update,
	.mac_addr_set = eth_mac_address_set,
	.stats_get = eth_stats_get,
	.stats_reset = eth_stats_reset,
	.reta_update = eth_rss_reta_update,
	.reta_query = eth_rss_reta_query,
	.rss_hash_update = eth_rss_hash_update,
	.rss_hash_conf_get = eth_rss_hash_conf_get
};
```

实际上，还发现了 virtio ，猜测这个时候，dpdk 是在虚拟机中，和 vDPA 联合使用，在虚拟机中通过 VFIO 直接访问 virtio 硬件。

### 真正的纯粹的 virito 驱动`net/virtio/virtio_ethdev.c`
- `set_rxtx_funcs` 设置 virtio 的接受函数

### drivers/vhost

```c
static const struct eth_dev_ops ops = {
	.dev_start = eth_dev_start,
	.dev_stop = eth_dev_stop,
	.dev_close = eth_dev_close,
	.dev_configure = eth_dev_configure,
	.dev_infos_get = eth_dev_info,
	.rx_queue_setup = eth_rx_queue_setup,
	.tx_queue_setup = eth_tx_queue_setup,
	.rx_queue_release = eth_rx_queue_release,
	.tx_queue_release = eth_tx_queue_release,
	.tx_done_cleanup = eth_tx_done_cleanup,
	.link_update = eth_link_update,
	.stats_get = eth_stats_get,
	.stats_reset = eth_stats_reset,
	.xstats_reset = vhost_dev_xstats_reset,
	.xstats_get = vhost_dev_xstats_get,
	.xstats_get_names = vhost_dev_xstats_get_names,
	.rx_queue_intr_enable = eth_rxq_intr_enable,
	.rx_queue_intr_disable = eth_rxq_intr_disable,
	.get_monitor_addr = vhost_get_monitor_addr,
};

static struct rte_vdev_driver pmd_vhost_drv = {
	.probe = rte_pmd_vhost_probe,
	.remove = rte_pmd_vhost_remove,
};
```

看来这个是提供给 lib/vhost 使用的，但问题是，似乎 lib/vhost 不是和 QEMU 通信的吗?

- `rte_vhost_driver_start` 可以一路调用到 `vhost_user_msg_handler`

```c
static struct rte_vdev_driver virtio_user_driver = {
	.probe = virtio_user_pmd_probe,
	.remove = virtio_user_pmd_remove,
	.dma_map = virtio_user_pmd_dma_map,
	.dma_unmap = virtio_user_pmd_dma_unmap,
};
```
但是 virtio 的 `eth_dev_ops` 和 `rte_vdev_driver` 不是在一个文件中的


### 为什么 vhost user 的 backend 是 kernel

```c
struct virtio_user_backend_ops virtio_ops_kernel = {
	.setup = vhost_kernel_setup,
	.destroy = vhost_kernel_destroy,
	.get_backend_features = vhost_kernel_get_backend_features,
	.set_owner = vhost_kernel_set_owner,
	.get_features = vhost_kernel_get_features,
	.set_features = vhost_kernel_set_features,
	.set_memory_table = vhost_kernel_set_memory_table,
	.set_vring_num = vhost_kernel_set_vring_num,
	.set_vring_base = vhost_kernel_set_vring_base,
	.get_vring_base = vhost_kernel_get_vring_base,
	.set_vring_call = vhost_kernel_set_vring_call,
	.set_vring_kick = vhost_kernel_set_vring_kick,
	.set_vring_addr = vhost_kernel_set_vring_addr,
	.get_status = vhost_kernel_get_status,
	.set_status = vhost_kernel_set_status,
	.enable_qp = vhost_kernel_enable_queue_pair,
	.update_link_state = vhost_kernel_update_link_state,
	.get_intr_fd = vhost_kernel_get_intr_fd,
};
```

- `vhost_kernel_set_vring_kick`
    - `vhost_kernel_ioctl` : 然后居然就是真的调用的 kernel 的 ioctl
        - [ ] 找到内核对应的内容

- `vhost_user_set_vring_kick` : 有两个实现
    - `lib/vhost_user.c` 的版本，作为和 QEMU 的 server 来通信的
    - `drivers/net/virtio/virtio_user/` 的版本，当 dpdk 在虚拟机中的用户态的时候，这就是一个驱动，和 virtio device 打交道

## 文摘
- [DPDK Graph Pipeline 框架简介与实现原理](https://mp.weixin.qq.com/s/hyb0BQXa6pqgEKLqGUEICw)
  - 想不到这个居然是 simd 的执行流

## 基于 dpdk 的应用
https://github.com/iqiyi/dpvs


## 看看 vfio 的代码

- rte_vfio_container_dma_map
  - container_dma_map
    - vfio_dma_mem_map : no-iommu mode 居然是什么都不做?

## dpdk 性能测试工具
- https://github.com/baidu/dperf

## 如何理解这些 DMA 的加速器
- https://doc.dpdk.org/guides/dmadevs/idxd.html


## 配合 dpdk 使用的


DPDK（Data Plane Development Kit）一般配合以下技术、框架和场景一起使用，才能发挥其极致的包处理性能（通常能达到线速几十 Gbps 甚至上百 Gbps）。下面按常见搭配分类总结：

### 1. 配合具体的用户态网络栈（最常见）
DPDK 本身只提供内核旁路（Kernel Bypass）的驱动和基础库，通常需要搭配一个完整的用户态协议栈：

| 协议栈/框架                    | 典型项目/产品                     | 备注                                 |
|--------------------------------|-----------------------------------|--------------------------------------|
| VPP (Vector Packet Processing) | FD.io 的 VPP                      | 最流行的组合，性能极高，广泛用于商用 |
| OVS-DPDK                       | Open vSwitch with DPDK            | OpenStack、Kubernetes 网络常用       |
| SPDK                           | Storage target (NVMe-oF 等)       | 存储方向的 DPDK                      |
| FStack / mTCP                  | 基于 DPDK 的自由 TCP/IP 栈        | 国内很多公司用                       |
| Seastar                        | 高性能异步框架（类似 DPDK + TCP） | ScyllaDB 等使用                      |
| eBPF + DPDK (XDP)              | AF_XDP / AF_PACKET with DPDK      | 新趋势，结合内核 eBPF                |

### 2. 配合虚拟化/容器平台
| 平台                       | 典型用法                                        |
|----------------------------|-------------------------------------------------|
| OpenStack + OVS-DPDK       | 虚拟机/裸金属高性能网络                         |
| Kubernetes + Multus + DPDK | 容器直通 DPDK（通过 vfio-pci）                  |
| Docker + DPDK              | 用户态容器网络（Cilium、Kube-OVS 等）           |
| SmartNIC/BlueField         | DPU 上跑 DPDK（NVIDIA BlueField、Intel IPU 等） |

### 3. 配合具体网卡（必须支持 DPDK 的驱动）
几乎所有主流高性能网卡都支持 DPDK：

| 厂商             | 常见支持型号                                  |
|------------------|-----------------------------------------------|
| Intel            | 82599、X710、XL710、E810、Columbiaville 等    |
| Mellanox/NVIDIA  | ConnectX-4/5/6/7（尤其是 ConnectX-6 Dx 以上） |
| Broadcom         | Stingray、智能网卡                            |
| Napatech         | 专业抓包卡                                    |
| Netronome/Agilio | SmartNIC（已并入 NVIDIA）                     |

### 4. 配合典型商用/开源产品
| 产品类型       | 典型代表                                       |
|----------------|------------------------------------------------|
| 虚拟交换机     | OVS-DPDK、VPP、Lagopus                         |
| 5G UPF / vEPC  | free5GC、NextEPC/Open5GS + DPDK、VPP           |
| vRouter        | Contrail/Tungsten Fabric、Cisco VPP            |
| NFV 网元       | vFirewall、vLoadBalancer、vBNG 等基本都用 DPDK |
| 高性能网关     | 6WIND、F5、A10 等商用网关                      |
| 云原生服务网格 | Cilium（eBPF+部分 DPDK）、Kuma 等              |

### 5. 典型实际部署形态（2024-2025 主流）
- 云厂商内部：VPP + DPDK（阿里云、腾讯云、AWS 部分自研）
- 运营商 5G/核心网：VPP 或 OVS-DPDK 做 UPF
- 企业私有云：OpenStack + OVS-DPDK
- SmartNIC/DPU：BlueField-3 上跑 DPDK + DOCA
- 高性能用户态应用：Nginx/DPDK、Envoy+DPDK、Redis over DPDK 等

### 总结一句话：
DPDK 本身只是“轮子+发动机”，真正跑起来需要搭配一个用户态网络协议栈（VPP、OVS-DPDK、FStack 等）+ 支持 DPDK 的高性能网卡 + 虚拟化/容器环境，才是完整的高性能网络解决方案。

目前业界最主流的组合就是：**DPDK + VPP** 或 **DPDK + OVS-DPDK**，这两套几乎占据了 80% 以上的生产部署。

## VPP 是做什么的
<!-- f7942812-4454-4c41-a298-42421bc722b4 -->

**VPP 不是“主要做路由”的工具，它是一个“万能的高性能数据面引擎”**，路由只是它能干的众多事情里最基础、最简单的一个。

- VPP 真实的生产使用场景（2025 年占比大致排名）

| 排名 | 实际用途                        | 占比（粗略估算） | 说明                                          |
|------|---------------------------------|------------------|-----------------------------------------------|
| 1    | 5G UPF（用户面）                | ~35%             | GTP-U 封装/解封装 + QoS + 计费 + SRv6 转发    |
| 2    | 云厂商 vRouter / 云内骨干转发   | ~30%             | 几十万到几百万条路由 + MPLS/SRv6 + 大规模 NAT |
| 3    | 高性能 NAT44/NAT66/CGNAT        | ~15%             | 运营商宽带出口、移动网关必备                  |
| 4    | L2/L3 VPN 网关（VPP + LDP/BGP） | ~8%              | 取代传统硬件 BNG、PE 路由器                   |
| 5    | 高性能负载均衡（LB）            | ~5%              | 纯软件 Maglev/One-arm LB，几百万连接          |
| 6    | 纯 L3 路由器（最单纯的用法）    | < 5%             | 真正只做路由的反而最少                        |

- VPP 官方自己列的“已实现功能插件”（随便挑几个你就知道它有多万能）

| 插件类别 | 具体功能举例                                      |
|----------|---------------------------------------------------|
| L2       | L2 桥接、VLAN、QinQ、L2TPv3、VXLAN、Geneve        |
| L3       | IPv4/IPv6 转发、路由（静态、BGP、IS-IS、OSPF）    |
| 隧道     | GRE、IPsec、WireGuard、SRv6、MPLS、VXLAN-GPE      |
| 安全     | ACL、IPSec、MACsec、Firewall                      |
| NAT      | NAT44、NAT66、NAT64、DS-Lite、MAP-T、LB           |
| 应用     | HTTP/TLS/DNS 终结、DPI、流量复制                  |
| 5G/移动  | GTP-U 封装解封装、PFCP（CUPS）、UPF 完整功能      |
| 其他     | In-situ OAM、IOAM、SPAN 镜像、Policing、QoS、LISP |

## DPDK 实验：dpdk-devbind.py 使用

pip3 install pyelftools
```sh
meson build
meson setup --reconfigure build --prefix $PWD/install -Dexamples=all -Dplatform=generic  # 如果是 mac
ninja -C build
echo  64 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
```


```txt
apps:
        dumpcap, graph, pdump, proc-info, test-acl, test-bbdev, test-cmdline, test-compress-perf,
        test-crypto-perf, test-dma-perf, test-eventdev, test-fib, test-flow-perf, test-gpudev, test-mldev, test-pipeline,
        test-pmd, test-regex, test-sad, test-security-perf, test,

Message:
=================
Libraries Enabled
=================

libs:
        log, kvargs, argparse, telemetry, eal, ptr_compress, ring, rcu,
        mempool, mbuf, net, meter, ethdev, pci, cmdline, metrics,
        hash, timer, acl, bbdev, bitratestats, bpf, cfgfile, compressdev,
        cryptodev, distributor, dmadev, efd, eventdev, dispatcher, gpudev, gro,
        gso, ip_frag, jobstats, latencystats, lpm, member, pcapng, power,
        rawdev, regexdev, mldev, rib, reorder, sched, security, stack,
        vhost, ipsec, pdcp, fib, port, pdump, table, pipeline,
        graph, node,

Message:
===============
Drivers Enabled
===============

common:
        cpt, dpaax, iavf, idpf, ionic, octeontx, cnxk, nfp,
        nitrox, qat, sfc_efx,
bus:
        auxiliary, cdx, dpaa, fslmc, ifpga, pci, platform, uacce,
        vdev, vmbus,
mempool:
        bucket, cnxk, dpaa, dpaa2, octeontx, ring, stack,
dma:
        cnxk, dpaa, dpaa2, hisilicon, odm, skeleton,
net:
        af_packet, ark, atlantic, avp, axgbe, bnx2x, bnxt, bond,
        cnxk, cpfl, cxgbe, dpaa, dpaa2, e1000, ena, enetc,
        enetfec, enic, failsafe, fm10k, gve, hinic, hns3, i40e,
        iavf, ice, idpf, igc, ionic, ixgbe, memif, netvsc,
        nfp, ngbe, null, octeontx, octeon_ep, pcap, pfe, qede,
        ring, sfc, softnic, tap, thunderx, txgbe, vdev_netvsc, vhost,
        virtio, vmxnet3,
raw:
        cnxk_bphy, cnxk_gpio, dpaa2_cmdif, ntb, skeleton,
crypto:
        bcmfs, caam_jr, cnxk, dpaa_sec, dpaa2_sec, ionic, nitrox, null,
        octeontx, scheduler, virtio,
compress:
        nitrox, octeontx, zlib,
regex:
        cn9k,
ml:
        cnxk,
vdpa:
        ifc, nfp, sfc,
event:
        cnxk, dpaa, dpaa2, dsw, opdl, skeleton, sw, octeontx,

baseband:
        acc, fpga_5gnr_fec, fpga_lte_fec, la12xx, null, turbo_sw,
gpu:


Message:
=================
Content Skipped
=================

apps:

libs:

drivers:
        common/mvep:    missing dependency, "libmusdk"
        common/mlx5:    missing dependency, "mlx5"
        crypto/qat:     missing dependency for Arm, libcrypto
        dma/idxd:       only supported on x86
        dma/ioat:       only supported on x86
        net/af_xdp:     missing dependency, "libxdp >=1.2.2" and "libbpf"
        net/ipn3ke:     missing dependency, "libfdt"
        net/mana:       only supported on x86 Linux
        net/mlx4:       missing dependency, "mlx4"
        net/mlx5:       missing internal dependency, "common_mlx5"
        net/mvneta:     missing dependency, "libmusdk"
        net/mvpp2:      missing dependency, "libmusdk"
        net/nfb:        missing dependency, "libnfb"
        net/ntnic:      only supported on x86_64 Linux
        raw/ifpga:      missing dependency, "libfdt"
        crypto/armv8:   missing dependency, "libAArch64crypto"
        crypto/ccp:     missing dependency, "libcrypto"
        crypto/ipsec_mb:        missing dependency, "libIPSec_MB"
        crypto/mlx5:    missing internal dependency, "common_mlx5"
        crypto/mvsam:   missing dependency, "libmusdk"
        crypto/openssl: missing dependency, "libcrypto"
        crypto/uadk:    missing dependency, "libwd"
        compress/isal:  missing dependency, "libisal"
        compress/mlx5:  missing internal dependency, "common_mlx5"
        compress/uadk:  missing dependency, "libwd"
        regex/mlx5:     missing internal dependency, "common_mlx5"
        vdpa/mlx5:      missing internal dependency, "common_mlx5"
        event/dlb2:     only supported on x86_64 Linux
        gpu/cuda:       missing dependency, "cuda.h"

```

直接给干回来了
./usertools/dpdk-devbind.py --bind=virtio-pci 00:04.0
这个才是应该高的东西
sudo ./usertools/dpdk-devbind.py --bind=vfio-pci 06:00.0

./usertools/dpdk-devbind.py --bind=igb_uio eth0

添加我们的 install 的位置:

PKGCONF := pkg-config --define-prefix --with-path=/root/dpdk/install/lib64/pkgconfig

可以编译，但是无法运行的问题:
echo /root/dpdk/install/lib64 >  /etc/ld.so.conf.d/dpdk.conf

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/dpdk/install/lib64

其实其中的 examples 已经够多了。

## /root/dpdk/build/examples/dpdk-devbind.py
可以通过 /root/dpdk/build/examples/dpdk-devbind.py 来检查 device 状态

```txt
➜  examples git:(main) ✗ dpdk-devbind.py -s
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2

Network devices using kernel driver
===================================
0000:00:02.0 'Virtio network device 1000' if=enp0s2 drv=virtio-pci unused=vfio-pci *Active*
0000:00:03.0 'Virtio network device 1000' if=enp0s3 drv=virtio-pci unused=vfio-pci *Active*
0000:00:04.0 'Virtio network device 1000' if=enp0s4 drv=virtio-pci unused=vfio-pci

No 'Baseband' devices detected
==============================

No 'Crypto' devices detected
============================

No 'DMA' devices detected
=========================

No 'Eventdev' devices detected
==============================

No 'Mempool' devices detected
=============================

No 'Compress' devices detected
==============================

Misc (rawdev) devices using kernel driver
=========================================
0000:00:01.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci
0000:00:0b.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci
0000:00:0c.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci

No 'Regex' devices detected
===========================

No 'ML' devices detected
```
但是为什么总是有

```txt
lspci: Unable to load libkmod resources: error -2
```

## 还是在虚拟机中，把这些基本的 examples 都搞完吧

## Program Reading Guide

### chapter 12

- The DPDK includes 1 Gigabit, 10 Gigabit and 40 Gigabit and para virtualized virtio Poll Mode Drivers.

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
