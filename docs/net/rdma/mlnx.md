## MLX4 和 MLX5 的区别
CONFIG_MLX4_EN=m

https://enterprise-support.nvidia.com/s/article/mellanox-dpdk

> mlx4 is the DPDK PMD for Mellanox ConnectX-3 Pro adapters. mlx4 is included starting from DPDK 2.0.
>
> mlx5 is the DPDK PMD for Mellanox ConnectX-4/ConnectX-4 Lx/ConnectX-5 adapters. mlx5 is included starting from DPDK 2.2.

## 看看几个基本的工具
https://stackoverflow.com/questions/58622347/what-is-the-difference-between-ofed-mlnx-ofed-and-the-inbox-driver

采用这个吧:
```txt
🤒  ag -l depmod -g "*.spec"
knem.git/knem.spec
mlnx-nvme.git/mlnx-nvme.spec
kernel-mft/mft_kernel.spec
00-mlnx-ofa-kernel.git/mlnx-ofa_kernel.spec
```

## 为什么 nvme 是 mlnx-ofa_kernel 的

mlnx-nvme 依赖一个 mlx_compat 的 kernel package
```txt
[ 9160.502221] nvme_core: Unknown symbol backport_dependency_symbol (err -2)
```

mlx_compat 在 mlnx-ofa_kernel-modules 中:
```txt
[root@bogon 16:15:24 ~]$ rpm -qf /lib/modules/5.10.0-202.0.0.oe1.v10000.x86_64/extra/mlnx-ofa_kernel/compat/mlx_compat.ko
mlnx-ofa_kernel-modules-5.8-OFED.5.8.5.1.1.1.kver.5.10.0_202.0.0.oe1.v10000.x86_64.x86_64
```

mlnx-ofa_kernel-modules 提供的驱动和 mlnx-nvme 不知道为什么是有重复的:
```txt
[root@arm-server a]# rpm -qpl mlnx-ofa_kernel-modules-5.8.x86_64.x86_64.rpm
// ...
extra/mlnx-ofa_kernel/drivers/nvme
extra/mlnx-ofa_kernel/drivers/nvme/host
extra/mlnx-ofa_kernel/drivers/nvme/host/nvme-rdma.ko
extra/mlnx-ofa_kernel/drivers/nvme/target
extra/mlnx-ofa_kernel/drivers/nvme/target/nvmet-rdma.ko
// ...
```

如果都安装，就是这个场景:
```txt
[root@bogon 16:20:30 5.10.0-202.0.0.oe1.v10000.x86_64]$ find . -name "nvme*"
./kernel/drivers/nvme
./kernel/drivers/nvme/target/nvmet-fc.ko.xz
./kernel/drivers/nvme/target/nvme-loop.ko.xz
./kernel/drivers/nvme/target/nvmet-tcp.ko.xz
./kernel/drivers/nvme/target/nvmet-rdma.ko.xz
./kernel/drivers/nvme/target/nvmet.ko.xz
./kernel/drivers/nvme/target/nvme-fcloop.ko.xz
./kernel/drivers/nvme/host/nvme-fc.ko.xz
./kernel/drivers/nvme/host/nvme-rdma.ko.xz
./kernel/drivers/nvme/host/nvme.ko.xz
./kernel/drivers/nvme/host/nvme-core.ko.xz
./kernel/drivers/nvme/host/nvme-tcp.ko.xz
./kernel/drivers/nvme/host/nvme-fabrics.ko.xz
./extra/mlnx-nvme/target/nvme-fcloop.ko
./extra/mlnx-nvme/target/nvme-loop.ko
./extra/mlnx-nvme/target/nvmet-fc.ko
./extra/mlnx-nvme/target/nvmet.ko
./extra/mlnx-nvme/target/nvmet-tcp.ko
./extra/mlnx-nvme/target/nvmet-rdma.ko
./extra/mlnx-nvme/host/nvme-rdma.ko
./extra/mlnx-nvme/host/nvme-core.ko
./extra/mlnx-nvme/host/nvme.ko
./extra/mlnx-nvme/host/nvme-tcp.ko
./extra/mlnx-nvme/host/nvme-fabrics.ko
./extra/mlnx-nvme/host/nvme-fc.ko
./extra/mlnx-ofa_kernel/drivers/nvme
./extra/mlnx-ofa_kernel/drivers/nvme/target/nvmet-rdma.ko
./extra/mlnx-ofa_kernel/drivers/nvme/host/nvme-rdma.ko
```

安装参考:
https://docs.daocloud.io/network/modules/spiderpool/install/ofed_driver/

这几个驱动什么关系?
https://stacoverflow.com/questions/58622347/what-is-the-difference-between-ofed-mlnx-ofed-and-the-inbox-driver

## 常见的 mft 工具使用
```txt
mlxconfig -d 0000:81:00.1 q NUM_OF_VFS

mlxconfig -d /dev/mst/mt4119_pciconf0 set NUM_OF_VFS=16
```

## 二分内核的时候遇到内核无法启动的情况，最后发现是由于 mlnx 驱动的问题

猜测是固件和驱动版本不匹配导致的，之后讲该驱动禁用掉即可。
```txt
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: hub_init_func2 [usbcore]
workqueue mm_percpu_wq: flags=0x8
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: vmstat_update
workqueue pm: flags=0x4
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=4/256 refcnt=5
    pending: pm_runtime_work, pm_runtime_work, pm_runtime_work, pm_runtime_work
pool 0: cpus=0 node=0 flags=0x0 nice=0 hung=58s workers=7 idle: 16 5 574 256 606 581
BUG: workqueue lockup - pool cpus=0 node=0 flags=0x0 nice=0 stuck for 92s!
Showing busy workqueues and worker pools:
workqueue events: flags=0x0
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=46/256 refcnt=49
    in-flight: 576:work_for_cpu_fn BAR(518)
    pending: mlx5_timestamp_overflow [mlx5_core], irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, vmstat_shepherd, linkwatch_event, work_for_cpu_fn BAR(511), psi_avgs_work
workqueue events_power_efficient: flags=0x80
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: hub_init_func2 [usbcore]
workqueue mm_percpu_wq: flags=0x8
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: vmstat_update
workqueue pm: flags=0x4
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=4/256 refcnt=5
    pending: pm_runtime_work, pm_runtime_work, pm_runtime_work, pm_runtime_work
pool 0: cpus=0 node=0 flags=0x0 nice=0 hung=92s workers=7 idle: 16 5 574 256 606 581
EXT4-fs (sdc3): mounted filesystem with ordered data mode. Opts: (null). Quota mode: disabled.
BUG: workqueue lockup - pool cpus=0 node=0 flags=0x0 nice=0 stuck for 123s!
Showing busy workqueues and worker pools:
workqueue events: flags=0x0
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=47/256 refcnt=50
    in-flight: 576:work_for_cpu_fn BAR(518)
    pending: mlx5_timestamp_overflow [mlx5_core], irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, irq_affinity_notify, vmstat_shepherd, linkwatch_event, work_for_cpu_fn BAR(511), psi_avgs_work, free_work
workqueue events_power_efficient: flags=0x80
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: hub_init_func2 [usbcore]
workqueue mm_percpu_wq: flags=0x8
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=1/256 refcnt=2
    pending: vmstat_update
workqueue pm: flags=0x4
  pwq 0: cpus=0 node=0 flags=0x0 nice=0 active=4/256 refcnt=5
    pending: pm_runtime_work, pm_runtime_work, pm_runtime_work, pm_runtime_work
```

## ofed 相关的打包都包含了什么东西
<!-- 5f403b71-8929-43a7-b464-4608a8afaf4d -->

这么看，这个问题是相当简单的了:

内核中的包:
```txt
• mlnx-ofa_kernel-25.07.tgz 是 Mellanox/NVIDIA OFED (OpenFabrics Enterprise Distribution) 的核心驱动源代
  码包，共包含 1889 个文件。以下是详细内容结构：
  主要目录结构
   目录                             说明
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   drivers/infiniband/              InfiniBand/RDMA 核心驱动和硬件驱动
   drivers/net/ethernet/mellanox/   Mellanox 以太网驱动 (mlx5, mlxfw, mlxsw)
   drivers/nvme/                    NVMe-oF (NVMe over Fabrics) 支持
   drivers/scsi/                    SCSI RDMA 协议 (SRP) 支持
   drivers/vdpa/                    vDPA (virtio Data Path Acceleration) 驱动
   drivers/vfio/                    VFIO 支持 (用于设备直通)
   drivers/fwctl/                   固件控制接口
   backports/                       内版本兼容性补丁 (大量 BACKPORT 补丁)
   ofed_scripts/                    脚本和配置文件
   compat/                          内核兼容性层代码
   include/                         头文件
   net/                             网络协议相关代码
   debian/                          Debian/Ubuntu 打包文件
   Documentation/                   文档
```

```txt
kernel-mft-4.33.0.tgz 是 NVIDIA/Mellanox 固件工具 (MFT) 的内核驱动，共 69 个文件：
   目录                          内容
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   nnt_driver/                   NNT 主驱动（PCI 内存/配置空间访问、DMA、IOCTL 接口）
   mst_backward_compatibility/   向后兼容驱动（mst_pci, mst_pciconf, mst_ppc_pci_reset）
   mstflint/                     MSTFlint 固件烧录工具支持
   misc_drivers/bf3_livefish/    BlueField-3 Livefish 调试模式驱动
  用途：固件管理、设备调试、寄存器访问（非网络数据通路）
```

用户态的包:

```txt
1. mlnx-iproute2 (Mellanox 定制版 iproute2)
功能概述：高级 IP 路由和网络设备配置工具，基于标准 iproute2 添加 Mellanox 定制功能。
主要工具：
 工具              说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 ip                IP 路由和网络设备配置
 tc                流量控制（Traffic Control）
 bridge            以太网桥管理
 rdma              RDMA 设备管理
 mlxdevm           Mellanox 设备管理
 devlink           设备链路管理
 ss                套接字统计
 nstat / rtacct    网络统计
 arpd              ARP 守护进程
 ifstat / lnstat   网络接口统计
 vdpa              vDPA 设备管理
 dcb               数据中心桥接配置
 tipc / genl       TIPC/Generic Netlink 工具
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
2. mlnx-tools (Mellanox 用户空间工具)
功能概述：Mellanox 用户态工具和脚本，用于网卡配置、调优和管理。
主要工具：
 工具                                                                             说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 mlnxofedctl                                                                      MLNX_OFED 驱动控制工具
 mlnx-sf                                                                          子功能（Sub-Function）管理
 mlnx_bf_configure                                                                BlueField 配置工具
 mlnx_affinity                                                                    CPU 亲和性配置
 doca-hugepages                                                                   大页内存配置
 sysctl_perf_tuning                                                               性能调优 sysctl 配置
 show_gids                                                                        显示 GID（全局标识符）表
 show_counters                                                                    显示网卡计数器
 cma_roce_mode                                                                    配置 RoCE 模式
 cma_roce_tos                                                                     配置 RoCE TOS
 set_irq_affinity.sh / set_irq_affinity_cpulist.sh / set_irq_affinity_bynode.sh   IRQ 亲和性设置脚本
 show_irq_affinity.sh / show_irq_affinity_hints.sh                                显示 IRQ 亲和性
 compat_gid_gen                                                                   兼容 GID 生成
 ib2ib_setup                                                                      IB-to-IB 配置
 mlnx_dump_parser                                                                 转储解析器
 mlnx_qos                                                                         QoS 配置
 mlnx_perf                                                                        性能监控
 mlnx_tune                                                                        自动调优工具
 mlx_fs_dump                                                                      流表转储
 tc_wrap.py                                                                       TC 包装脚本
 mlnx_bf_udev                                                                     BlueField udev 规则
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
3. ofed-scripts (OFED 脚本)
功能概述：OpenFabrics Enterprise Distribution (OFED) 安装和管理脚本。
主要工具：
 工具                                                 说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 mlnxofedinstall                                      MLNX_OFED 安装脚本
 mlnxofedinstall_deb.pl / install_deb.pl              Debian 包安装脚本
 install.pl                                           通用安装脚本
 uninstall.sh / ofed_uninstall.sh                     卸载脚本
 uninstall_deb.sh                                     Debian 卸载脚本
 vendor_pre_uninstall.sh / vendor_post_uninstall.sh   供应商卸载钩子
 ofed_info                                            显示 OFED 版本信息
 ofed_rpm_info                                        显示 OFED RPM 信息
 hca_self_test.ofed                                   HCA 自检脚本
 sysinfo-snapshot.py                                  系统信息快照收集
 mlnx_add_kernel_support.sh                           添加内核支持
 is_kmp_compat.sh                                     检查 KMP 兼容性
 check_syntax                                         语法检查
 common.pl / tests/rpm_dist.pl                        公共 Perl 函数
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
4. perftest (IB 性能测试)
功能概述：InfiniBand/RDMA 性能测试工具集，用于测试带宽和延迟。
主要测试工具：
 工具                          说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 send_bw                       发送带宽测试
 send_lat                      发送延迟测试
 write_bw / read_bw            RDMA 写/读带宽测试
 write_lat / read_lat          RDMA 写/读延迟测试
 atomic_bw                     原子操作带宽测试
 atomic_lat                    原子操作延迟测试
 raw_ethernet_send_bw          原始以太网发送带宽测试
 raw_ethernet_send_lat         原始以太网发送延迟测试
 raw_ethernet_send_burst_lat   原始以太网突发延迟测试
 raw_ethernet_fs_rate          原始以太网帧速率测试
 runme                         自动化测试脚本
 run_perftest_loopback         回环测试脚本
 run_perftest_multi_devices    多设备测试脚本
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
5. rdma-core (RDMA 核心库和工具)

功能概述：RDMA 核心用户空间基础设施，包含库、诊断工具和守护进程。

	库（Libraries）
	 库名           说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 libibverbs     RDMA Verbs 库（用户态直接访问硬件）
	 libibumad      用户态管理数据报库
	 librdmacm      RDMA 连接管理库
	 libibmad       管理数据报库
	 libibnetdisc   子网发现库

	诊断工具 (infiniband-diags)
	 工具                                                        说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 ibstat / ibstatus                                           显示 IB 设备状态
	 ibnetdiscover                                               发现 IB 子网拓扑
	 ibping                                                      IB 网络 ping
	 ibportstate                                                 端口状态管理
	 ibroute                                                     路由查询
	 ibsysstat                                                   系统状态
	 ibtracert                                                   IB 路由跟踪
	 perfquery                                                   性能计数器查询
	 sminfo / smpdump / smpquery                                 子网管理器查询
	 saquery                                                     SA 查询
	 vendstat                                                    供应商特定状态
	 iblinkinfo                                                  链路信息
	 ibqueryerrors                                               错误查询
	 ibaddr                                                      地址查询
	 ibccquery / ibccconfig                                      CC（拥塞控制）查询/配置
	 dump_fts / dump_lfts.sh / dump_mfts.sh                      转发表转储
	 ibhosts / ibswitches / ibnodes / ibrouters                  拓扑显示
	 ibclearerrors / ibclearcounters                             清除错误/计数器
	 ibchecknet / ibchecknode / ibcheckport / ibcheckerrors 等   检查脚本

	libibverbs 示例工具
	 工具                                                  说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 ibv_devinfo / ibv_devices                             设备信息
	 ibv_asyncwatch                                        异步事件监控
	 ibv_rc_pingpong / ibv_uc_pingpong / ibv_ud_pingpong   各种传输类型的 ping-pong 测试
	 ibv_srq_pingpong / ibv_xsrq_pingpong                  SRQ/XSRQ ping-pong 测试

	librdmacm 示例工具
	 工具                          说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 rping                         RDMA ping
	 rdma_server / rdma_client     RDMA 连接测试
	 rdma_xserver / rdma_xclient   扩展 RDMA 连接测试
	 ucmatose                      CM 测试
	 udaddy / udpong               不可靠数据报测试
	 mckey                         组播测试
	 rcopy                         RDMA 文件拷贝
	 riostream / rstream           流测试
	 cmtime                        连接管理时间测试

	守护进程
	 守护进程               说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 ibacm                  IB 通信管理助手（类似 ARP 缓存）
	 srp_daemon / ibsrpdm   SCSI RDMA 协议守护进程
	 rdma-ndd               RDMA 节点描述守护进程
	Python 绑定
	 模块      说明
	━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	 pyverbs   Python API 封装 libibverbs
```
rdma-core 中包含了多个 rpm

### [ ] 那么这些工具是什么作用的

```txt
kudo dnf install -y rdma-core libibverbs-utils perftest infiniband-diags
sudo systemctl enable --now rdma
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
