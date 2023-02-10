# VFIO

通过 VFIO 可以将一个物理设备直接被 Guest 使用。

## 上手操作
测试机器是小米笔记本 pro 2019 版本，其显卡为 Nvidia MX150，在 Linux 上，其实 Nvidia 的显卡一般工作就很不正常，所以直通给 Guest 使用。
参考[内核文档](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)，我这里记录一下操作:


1. 确定 GPU 的 bdf (*B*us *D*evice *F*unction)
```txt
➜  vn git:(master) ✗ lspci
00:00.0 Host bridge: Intel Corporation Xeon E3-1200 v6/7th Gen Core Processor Host Bridge/DRAM Registers (rev 08)
00:02.0 VGA compatible controller: Intel Corporation UHD Graphics 620 (rev 07)
00:08.0 System peripheral: Intel Corporation Xeon E3-1200 v5/v6 / E3-1500 v5 / 6th/7th/8th Gen Core Processor Gaussian Mixture Model
00:14.0 USB controller: Intel Corporation Sunrise Point-LP USB 3.0 xHCI Controller (rev 21)
00:14.2 Signal processing controller: Intel Corporation Sunrise Point-LP Thermal subsystem (rev 21)
00:15.0 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #0 (rev 21)
00:15.1 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #1 (rev 21)
00:16.0 Communication controller: Intel Corporation Sunrise Point-LP CSME HECI #1 (rev 21)
00:1c.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #1 (rev f1)
00:1c.4 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #5 (rev f1)
00:1c.7 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #8 (rev f1)
00:1d.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #9 (rev f1)
00:1f.0 ISA bridge: Intel Corporation Sunrise Point LPC Controller/eSPI Controller (rev 21)
00:1f.2 Memory controller: Intel Corporation Sunrise Point-LP PMC (rev 21)
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
00:1f.4 SMBus: Intel Corporation Sunrise Point-LP SMBus (rev 21)
01:00.0 3D controller: NVIDIA Corporation GP108M [GeForce MX150] (rev ff)
02:00.0 Non-Volatile memory controller: ADATA Technology Co., Ltd. Device 0021 (rev 01)
03:00.0 Network controller: Intel Corporation Wireless 8265 / 8275 (rev 78)
04:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd NVMe SSD Controller SM961/PM961
➜  vn git:(master) ✗
```

2. 检测 iommu 是否支持，如果这个命令失败，那么修改 grub ，内核启动参数中增加上 `intel_iommu=on` [^4]
```txt
➜  vn git:(master) ✗ readlink /sys/bus/pci/devices/0000:01:00.0/iommu_group
../../../../kernel/iommu_groups/11
```

3. 如果 GPU 之前在被使用，那么首先需要解绑
```sh
sudo su
echo 0000:01:00.0 > /sys/bus/pci/devices/0000:01:00.0/driver/unbind
```

4. lspci -nn 获取设备的 vender 和 device id
```txt
01:00.0 VGA compatible controller [0300]: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] [10de:1c02] (rev a1)
01:00.1 Audio device [0403]: NVIDIA Corporation GP106 High Definition Audio Controller [10de:10f1] (rev a1)
```

5. 创建 vfio

```sh
echo 0000:01:00.0 | sudo tee /sys/bus/pci/devices/0000:01:00.0/driver/unbind
echo 10de 1c02 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id

echo 0000:01:00.1 | sudo tee /sys/bus/pci/devices/0000:01:00.1/driver/unbind
echo 10de 10f1 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

<!--
看看这个文章: https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm
这个教程也不错：https://github.com/bryansteiner/gpu-passthrough-tutorial
 -device vfio-pci,host=01:00.0,multifunction=on,x-vga=on 中的 x-vga 是什么含义？

nixos 下非常之好的讲解: https://astrid.tech/2022/09/22/0/nixos-gpu-vfio/
-->
<!-- @todo 可以集成显卡直通吗? -->
<!-- @todo 如何实现 QEMU 的复制粘贴 -->

6. 无需管理员权限运行
```txt
➜  vn git:(master) ✗ sudo chown maritns3:maritns3 /dev/vfio/11
```
实际上，因为 ulimit 的原因，会存在如下报错:
```txt
qemu-system-x86_64: -device vfio-pci,host=00:1f.3: vfio 0000:00:1f.3: failed to setup container for group 10: memory listener initialization failed: Region pc.ram: vfio
_dma_map(0x558becc6b3b0, 0xc0000, 0x7ff40000, 0x7f958bec0000) = -12 (Cannot allocate memory)
```
而修改 hard limit 的方法参考[此处](https://docs.oracle.com/cd/E19623-01/820-6168/file-descriptor-requirements.html)，有点麻烦。

6. qemu 中运行

```sh
-device vfio-pci,host=01:00.0
```

## usb 直通
- https://unix.stackexchange.com/questions/452934/can-i-pass-through-a-usb-port-via-qemu-command-line

```txt
/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/9p, 20000M/x2
    |__ Port 8: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M
/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/16p, 480M
    |__ Port 1: Dev 21, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 0, Class=Vendor Specific Class, Driver=, 12M
    |__ Port 7: Dev 15, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 3, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 9: Dev 4, If 0, Class=Hub, Driver=hub/4p, 480M
        |__ Port 4: Dev 20, If 0, Class=Human Interface Device, Driver=usbhid, 12M
        |__ Port 4: Dev 20, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 11: Dev 5, If 0, Class=Human Interface Device, Driver=usbhid, 1.5M
    |__ Port 14: Dev 7, If 0, Class=Wireless, Driver=btusb, 12M
    |__ Port 14: Dev 7, If 1, Class=Wireless, Driver=btusb, 12M
```

```txt
Bus 002 Device 002: ID 174c:3074 ASMedia Technology Inc. ASM1074 SuperSpeed hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 001 Device 020: ID 0c45:7638 Microdia AKKO 3084BT
Bus 001 Device 004: ID 174c:2074 ASMedia Technology Inc. ASM1074 High-Speed hub
Bus 001 Device 015: ID 2717:003b Xiaomi Inc. MI Wireless Mouse
Bus 001 Device 003: ID 0b05:19af ASUSTek Computer, Inc. AURA LED Controller
Bus 001 Device 007: ID 8087:0026 Intel Corp. AX201 Bluetooth
Bus 001 Device 005: ID 17ef:6019 Lenovo M-U0025-O Mouse
Bus 001 Device 021: ID 2f68:0082 Hoksi Technology DURGOD Taurus K320
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```

## 但是
我始终没有搞定笔记本上的 GPU 的直通，而且在台式机上直通成功的案例中，发现由于英雄联盟的翻作弊机制，也是无法成功运行的，不过可以运行原神。

## 其他的 vfio 尝试
```txt
# 00:17.0 SATA controller [0106]: Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode] [8086:7ae2] (rev 11)
echo 0000:00:17.0 | sudo tee /sys/bus/pci/devices/0000:00:17.0/driver/unbind
echo 8086 7ae2 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

## 关键参考
- https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Passing_through_a_device_that_does_not_support_resetting


- https://superuser.com/questions/1293112/kvm-gpu-passthrough-of-nvidia-dgpu-on-laptop-with-hybrid-graphics-without-propri
- https://github.com/jscinoz/optimus-vfio-docs


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

[^4]: https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing
