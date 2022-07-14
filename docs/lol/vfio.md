# VFIO

通过 VFIO 可以将一个物理设备直接被 Guest 使用。

## 上手操作
测试机器是小米笔记本 pro 2019 版本，其显卡为 Nvidia MX150，在 Linux 上，其实 Nvidia 的显卡一般工作就很不正常，所以直通给 Guest 使用。
参考[内核文档](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)，我这里记录一下操作:


1. 确定 GPU 的 bdf
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

3. 获取 ref ff :)
```txt
➜  vn git:(master) ✗ lspci -n -s 0000:01:00.0
01:00.0 0302: 10de:1d12 (rev ff)
```

4. 将 GPU 和 vfio 驱动绑定

如果 GPU 之前在被使用，那么首先需要解绑
```sh
sudo su
echo 0000:01:00.0 > /sys/bus/pci/devices/0000:01:00.0/driver/unbind
echo 10de 1d12 > /sys/bus/pci/drivers/vfio-pci/new_id
```

如果检查到多个 devices 的，那么上面的两个操作需要对于这个文件夹的所有的设备操作一次:
```sh
➜  vn git:(master) ✗ ls -l /sys/bus/pci/devices/0000:01:00.0/iommu_group/devices
lrwxrwxrwx root root 0 B Mon May 30 09:53:28 2022  0000:01:00.0 ⇒ ../../../../devices/pci0000:00/0000:00:1c.0/0000:01:00.0
```

5. 无需管理员权限
```sh
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

## 但是
我始终没有搞定笔记本上的 GPU 的直通，而且在台式机上直通成功的案例中，发现由于英雄联盟的翻作弊机制，也是无法成功运行的，不过可以运行原神。

相关参考:
- https://superuser.com/questions/1293112/kvm-gpu-passthrough-of-nvidia-dgpu-on-laptop-with-hybrid-graphics-without-propri
- https://github.com/jscinoz/optimus-vfio-docs

[^4]: https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing
