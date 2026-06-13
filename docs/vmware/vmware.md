# vmware 简单记录

## 基本文档
- https://blogs.vmware.com/virtualblocks/2022/08/30/announcing-vsan-8-with-vsan-express-storage-architecture/
- https://www.yellow-bricks.com/2022/08/30/introducing-vsphere-8/

https://www.vmware.com/info/workstation-player/evaluation

## 好家伙，这是源码泄露了吗?
- https://github.com/mkubecek/vmware-host-modules/

- vmware 用的不是 kvm

## 问题
1. windows 中如何打开嵌套虚拟化?

## 在 vmware 中安装
1. 使用 code/qemu/alpine-action.sh:setup_vmware 中初始化配置
2. 奇怪的密码配置
  1. 首先，在安装的时候需要设置密码
  2. 在配置界面，按 F2 ，这个时候不用密码，直接 Enter ，进入到系统之后，使用:
    - 配置密码，这个是 web 界面的控制密码
    - 配置 ip

![](./img/vmware-2.png)

## 基本操作
2. 热迁移


一个虚拟机中的基本内容:
```txt
$ lspci
00:00.0 Host bridge: Intel Corporation 440BX/ZX/DX - 82443BX/ZX/DX Host bridge (rev 01)
00:01.0 PCI bridge: Intel Corporation 440BX/ZX/DX - 82443BX/ZX/DX AGP bridge (rev 01)
00:07.0 ISA bridge: Intel Corporation 82371AB/EB/MB PIIX4 ISA (rev 08)
00:07.1 IDE interface: Intel Corporation 82371AB/EB/MB PIIX4 IDE (rev 01)
00:07.3 Bridge: Intel Corporation 82371AB/EB/MB PIIX4 ACPI (rev 08)
00:07.7 System peripheral: VMware Virtual Machine Communication Interface (rev 10)
00:0f.0 VGA compatible controller: VMware SVGA II Adapter
02:00.0 Serial Attached SCSI controller: VMware PVSCSI SCSI Controller (rev 02)
02:01.0 Ethernet controller: VMware VMXNET3 Ethernet Controller (rev 01)
02:02.0 USB controller: VMware Device 077a
02:03.0 SATA controller: VMware SATA AHCI controller
```
这里的 SATA controller 可以看看

```txt
02:03.0 SATA controller: VMware SATA AHCI controller (prog-if 01 [AHCI 1.0])
        DeviceName: sata0
        Subsystem: VMware SATA AHCI controller
        Physical Slot: 35
        Flags: bus master, 66MHz, fast devsel, latency 64, IRQ 27
        Memory at fe210000 (32-bit, non-prefetchable) [size=4K]
        Expansion ROM at fdd20000 [disabled] [size=64K]
        Capabilities: <access denied>
        Kernel driver in use: ahci
        Kernel modules: ahci
```

## 了解一下 INFINIBAND_VMWARE_PVRDMA 是做什么的

drivers/infiniband/hw/vmw_pvrdma/ 其实代码量不大
但是这个项目从 2016 年就开始了。

## 好好学，好好看
https://internet.hactcm.edu.cn/fileservice/cloud/file/2024/3/13/133548125650103052.pdf

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
