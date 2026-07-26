# windows 网络
## windows 有 dpdk 和 spdk 么？

可以看看，如果虚拟机想要实现最佳性能，该如何实现?

windows 特有的:
https://npcap.com/

这里应该就是 windows 关于网络的所有的特性了吧:
https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-receive-side-scaling

## windows user mode driver

https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/overview-of-the-umdf

windows 那些驱动是卸载到用户态的?
有 vfio 么?

从这里看，应该是有的:
https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-single-root-i-o-virtualization--sr-iov-

## 调试文档
https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/user-space-and-system-space


## qemu 的存储后端有 smaba 吗?

有趣的，那么其实也可以类似的实现一个 ssh 后端，
通过 ssh 来共享目录啊:
```txt
    ``smb=dir[,smbserver=addr]``
        When using the user mode network stack, activate a built-in SMB
        server so that Windows OSes can access to the host files in
        ``dir`` transparently. The IP address of the SMB server can be
        set to addr. By default the 4th IP in the guest network is used,
        i.e. x.x.x.4.

        In the guest Windows OS, the line:

        ::

            10.0.2.4 smbserver

        must be added in the file ``C:\WINDOWS\LMHOSTS`` (for windows
        9x/Me) or ``C:\WINNT\SYSTEM32\DRIVERS\ETC\LMHOSTS`` (Windows
        NT/2000).

        Then ``dir`` can be accessed in ``\\smbserver\qemu``.

        Note that a SAMBA server must be installed on the host OS.
```

```txt
#ifdef CONFIG_SLIRP
    "-netdev user,id=str[,ipv4=on|off][,net=addr[/mask]][,host=addr]\n"
    "         [,ipv6=on|off][,ipv6-net=addr[/int]][,ipv6-host=addr]\n"
    "         [,restrict=on|off][,hostname=host][,dhcpstart=addr]\n"
    "         [,dns=addr][,ipv6-dns=addr][,dnssearch=domain][,domainname=domain]\n"
    "         [,tftp=dir][,tftp-server-name=name][,bootfile=f][,hostfwd=rule][,guestfwd=rule]"
#ifndef _WIN32
                                             "[,smb=dir[,smbserver=addr]]\n"
#endif
    "                configure a user mode network backend with ID 'str',\n"
    "                its DHCP server and optional services\n"
#endif
```

## vmbus
> 依旧完全看不懂了

GPU 虚拟化那边我记得主要有这些整活方案：
- PCIe 直通
- D3DKMT 或者 Linux DRM 这类 GPU 驱动需要的系统调用直通
- API 转译透传

目前我知道的是，就桌面那边的 GPU 虚拟化方案……VirtIO 那群人老羡慕巨硬那种说一不二在内核态统一 D3D 系统调用于是可以让 Hyper-V 直接透传 D3DKMT 接口到虚拟机

其实皮衣他不开放也没关系，毕竟 Hyper-V 的 WDDM 系统调用透传（D3DKMT）这件事 NVIDIA 还是不得不支持的，毕竟 WSL2 有 CUDA 加速用得这个

毕竟其他虚拟化阵营大多都想搞 VMBus 模拟，这样就可以不需要给 Windows Guest 做 Guest 驱动了，不需要和 Microsoft 在 WHQL 上扯皮

当然 GPU 虚拟化感觉重点应该还是在 Guest 驱动适配（想到了 Windows WDDM 那个巨坑了[流泪]）

不然就是 virgl/venus/gfxstream 这种传 api 调用的东西了

刚刚看了下相关内容，QEMU 在今年年初有个支持 virtio-gpu DRM native context 的补丁（已经合并），里面提到了当时支持的硬件：
- freedreno 已经完全上游
- amdgpu 已经完全上游
- intel i915 有 PR 了
- asahi 部分上游

## rdma

Windows 有 RDMA 支持，但是它和 Linux `rdma-core` / `libibverbs` 生态不是同一套接口。

### SMB Direct

这是 Windows Server 上最常见、最成熟的 RDMA 使用方式。应用通常不直接调用 RDMA API，而是让 SMB 在 RDMA-capable NIC 上自动走 RDMA path。

典型场景：

- Hyper-V over SMB
- SQL Server 访问远端文件服务器
- Storage Spaces Direct
- Storage Replica
- Scale-Out File Server

相关文档：

- https://learn.microsoft.com/en-us/windows-server/storage/file-server/smb-direct
- https://learn.microsoft.com/en-us/windows-server/administration/performance-tuning/role/file-server/smb-file-server

PowerShell 里可以看网卡 RDMA 状态：

```powershell
Get-NetAdapterRdma
Enable-NetAdapterRdma <name>
Disable-NetAdapterRdma <name>
```

### Network Direct

如果要写用户态 RDMA 程序，Windows 对应的是 Network Direct / Network Direct SPI。这个更接近 Linux 上直接写 `ibv_post_send`、`ibv_reg_mr`、`rdma_cm` 那种路线，但 API 和生态不同。

Network Direct 是 user-mode RDMA programming interface，设计上是 fabric agnostic，可以跑在 InfiniBand、iWARP、RoCE 上。

相关资料：

- https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/hh997033(v=ws.11)
- https://github.com/microsoft/NetworkDirect
- https://learn.microsoft.com/zh-tw/previous-versions/windows/desktop/cc904344(v=vs.85)

Microsoft 的示例仓库里有这些程序：

- `ndping`
- `ndpingpong`
- `ndcat`
- `ndmrlat`
- `ndmrrate`

如果目标是做 demo 或学习，可以先从 `ndping` 看起。

### NDKPI

NDKPI 是 Network Direct Kernel Provider Interface，属于 kernel-mode RDMA 接口。它主要面向：

- 网卡厂商实现 RDMA miniport/provider
- Windows 内核组件消费 RDMA
- 驱动开发

普通用户态应用一般不直接用 NDKPI。

相关文档：

- https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-network-direct-kernel-provider-interface--ndkpi-
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndkpi/ns-ndkpi-_ndk_sge

### 和 Linux 的区别

Linux 常见路径：

- `rdma-core`
- `libibverbs`
- `librdmacm`
- provider: `mlx5`, `irdma`, `rxe` 等

Windows 常见路径：

- 高层生产使用：SMB Direct
- 用户态 RDMA app：Network Direct
- 内核/驱动：NDKPI

所以不能直接把 Linux verbs 程序搬到 Windows 编译。需要改成 Network Direct API，或者把 RDMA 隐藏在 SMB Direct 这类系统组件后面。

### 硬件和驱动

能不能用 RDMA 主要取决于 NIC 和厂商驱动。常见方向：

- NVIDIA/Mellanox WinOF-2
- Intel Ethernet 800 系列，部分支持 iWARP/RoCEv2
- Chelsio iWARP

协议上常见的是：

- RoCE / RoCEv2
- iWARP
- InfiniBand

实际部署时需要同时确认：

- Windows 版本是否支持目标能力
- 网卡是否支持 RDMA
- 厂商 Windows driver 是否暴露 Network Direct / SMB Direct 能力
- 交换机和网络是否满足 RoCEv2/PFC/ECN 等配置要求

### 简单结论

Windows 支持 RDMA，但主流生产入口是 SMB Direct。直接写 RDMA 应用可以看 Network Direct，不过资料和生态都不如 Linux `libibverbs` 完整。如果是在写驱动或内核组件，再看 NDKPI。

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
