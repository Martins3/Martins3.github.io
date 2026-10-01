# GPU 直通的两个问题

## 直通的虚拟机自动的触发错误

2026-10-01 我发现 GPU 直通的虚拟机启动会失败:

```txt
[256138.011263] pcieport 0000:00:01.0: AER: Multiple Uncorrectable (Non-Fatal) error message received from 0000:01:00.0
[256138.011275] vfio-pci 0000:01:00.0: PCIe Bus Error: severity=Uncorrectable (Non-Fatal), type=Transaction Layer, (Receiver ID)
[256138.011276] vfio-pci 0000:01:00.0:   device [10de:2d04] error status/mask=00100000/00400000
[256138.011277] vfio-pci 0000:01:00.0:    [20] UnsupReq               (First)
[256138.011279] vfio-pci 0000:01:00.0: AER:   TLP Header: 0x40000001 0x0000000c 0x8808000c 0x00000000
[256138.011341] pcieport 0000:00:01.0: AER: device recovery successful
```

```txt
qemu-system-x86_64: vfio_err_notifier_handler(0000:01:00.0) Unrecoverable error detected. Please collect any data possible and then kill the guest
runstate_set current_run_state 9 (running) new_state 2 (internal-error)
qemu-system-x86_64: vfio_err_notifier_handler(0000:01:00.1) Unrecoverable error detected. Please collect any data possible and then kill the guest
```

今天 08:01:19、08:02:12 两次启动都出现：

vfio-pci 0000:01:00.0: PCIe Bus Error: severity=Uncorrectable (Non-Fatal)
[20] UnsupReq (First)
TLP Header: 0x40000001 0x0000000c 0x8808000c 0x00000000

设备是 RTX 5060 Ti。还有一个具体线索：报错地址 0x8808000c 落在它的 HDMI 音频
功能 01:00.1 的 BAR0 内，偏移 0xc，音频功能本身也保留着 UnsupReq 状态。因此要
重点查显卡及音频功能的初始化、复位和电源状态。

QEMU 随后报：

vfio_err_notifier_handler(...) Unrecoverable error detected
running → internal-error

我核对了 QEMU 源码 (/home/martins3/data/qemu/hw/vfio/pci.c) 的
vfio_err_notifier_handler()：收到通知就暂停 VM。因此宿主机即使打印 device
recovery successful，VM 也不会自动恢复。Non-Fatal
也不代表事务成功，内核文档说明它仍表示该事务不可靠。AER 文档
(https://www.kernel.org/doc/html/latest/PCI/pcieaer-howto.html)

关于你怀疑的升级，时间线是：

• 9 月 25 日：安装宿主机内核 7.2.7-200.fc44。
• 9 月 28 日 08:53：宿主机重启，开始运行这个内核。
• 9 月 30 日 14:09：同一显卡已经出现 D3cold → D0 失败、device inaccessible，
  继而出现 Fatal MalfTLP，等待约 32 秒后才恢复并重置链路。

• 9 月 30 日 15:33：当前 QEMU 二进制重新构建。
• 今天：启动 VM 稳定触发上述错误。

新内核值得怀疑，但目前还不能认定是内核回归；昨天显卡从低功耗状态恢复失败，是
更直接的异常线索。 下一步适合用保留的 7.1.10 内核做同配置对照，并检查显卡电源
状态恢复。此次只做了读取检查，没有重启或修改配置。



## 为什么我的 GPU 直通失败了

启动 qemu 有警告，而且启动失败了:
```text
  qemu-system-x86_64: warning: vfio_container_dma_map(..., 0xe0200000000, 0x10000000, ...) = -22 (Invalid argument)
  0000:01:00.0: PCI peer-to-peer transactions on BARs are not supported.
```

我发现直通 GPU 到虚拟机中，虚拟机开机卡死，perf 观察 qemu ，结果如下:
这是 1060 Ti 的观察结果
```txt
 - 21.69% address_space_write
    - 21.26% flatview_write
       - 20.78% flatview_write_continue_step
          - 19.50% memory_region_dispatch_write
             - 19.20% access_with_adjusted_size
                - 18.76% memory_region_write_accessor
                   - 18.26% vfio_region_write
                      - 17.74% vfio_device_io_region_write
                         - 17.10% __libc_pwrite
                            - 16.78% entry_SYSCALL_64_after_hwframe
                               - do_syscall_64
                                  - 16.44% __x64_sys_pwrite64
                                     - 15.90% vfs_write
                                        - 15.26% vfio_pci_rw
                                           - 14.44% vfio_pci_bar_rw
                                              - 14.26% vfio_pci_core_do_io_rw
                                                   13.67% iowrite32
                                             0.58% __pm_runtime_resume
```

使用 5070Ti 观察的结果为:
```txt
@[
        handle_ud+5
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+2102
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+358
        kvm_vcpu_ioctl+876
        __x64_sys_ioctl+185
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 1435096
```

### 问题 1 : 为什么开启卡住了

观察 seabios 日志 ，可以发现，这是在执行 nvidia 的 option rom ，也就是 VBIOS 的代码，
这个，显然我们没有什么操作空间了:
```txt
Copying MPTABLE from 0x00006de8/befe26e0 to 0x000f53d0
Copying SMBIOS from 0x00006de8 to 0x000f5200
table(50434146)=0xbffe34d7 (via rsdt)
ACPI: parse DSDT at 0xbffe0040 (len 13463)
Scan for VGA option rom
Running option rom at c000:0003
```

显然是 NV 的 VBIOS 代码实现的有问题，解决办法是屏蔽掉 option ROM ，对应的 QEMU 的启动的参数为:
```txt
-device vfio-pci,host=0000:01:00.0,rombar=0
```

### 问题 2 : 为什么 QEMU 会抛出警告
```text
  qemu-system-x86_64: warning: vfio_container_dma_map(..., 0xe0200000000, 0x10000000, ...) = -22 (Invalid argument)
  0000:01:00.0: PCI peer-to-peer transactions on BARs are not supported.
```

#### 为什么会报错
vfio_container_dma_map(..., IOVA, size, HVA) 是把 guest 物理地址（IOVA）映射到 QEMU 进程地址（HVA），再交给 host IOMMU 做 DMA 翻译。

-22 是 EINVAL。IOVA 0xe0200000000 约等于 14.5 TB，已经远超 host IOMMU 能翻译的范围。

```bash
dmesg | grep -i dmar
```
```text
  Address Translation Service Device: dmar5
  Address Width: 39
```
Intel IOMMU 只支持 39-bit = 512GB。所以 guest 里任何 PCI BAR 放到 0x8000000000 以上，VFIO 就映射不了。

这和 seabios 中的日志是对应的:
```text
  phys-bits=46
  PCI: 64: 00000e0000000000 - 00000e0240000000
  PCI: map device bdf=00:07.0  bar 1, addr 00000e0200000000, size 10000000 [prefmem]
```

SeaBIOS 直接开了个 64-bit prefetchable 窗口在 0xe0000000000，GPU 的 256MB BAR 落到了 0xe0200000000。

#### 为什么报错没有影响

回想一下 vfio 的工作流程 ./internal-kernel.md

对于 mmap 的 vfio_region_fd ，这是为了让 Guest 直接访问设备的 mmio 的:
```txt
region->mmaps[i].mmap = mmap(..., vfio_region_fd, ...);
```
例如:
```txt
QEMU HVA 0x7f8490000000 → Host GPU BAR1 0x6000000000
```

会把这个区域再通过 VFIO_IOMMU_MAP_DMA ，
也就是设备可以通过 iommu 来访问

```txt
vfio_container_dma_map(
    iova  = 0xe0200000000,
    size  = 0x10000000,
    vaddr = 0x7f8490000000
);
```
也就是设备在访问 0xe0200000000 ，会访问到 0x7f8490000000 所对应的物理地址，
而 0x7f8490000000 所对应的物理地址正好就是设备的 mmio 空间。

这是无所谓的，hw/vfio/listener.c 对于这种映射的失败只有警告:
```c
    ret = vfio_container_dma_map(bcontainer, iova, int128_get64(llsize),
                                 vaddr, section->readonly, section->mr);
    if (ret) {
        error_setg(&err, "vfio_container_dma_map(%p, 0x%"HWADDR_PRIx", "
                   "0x%"HWADDR_PRIx", %p) = %d (%s)",
                   bcontainer, iova, int128_get64(llsize), vaddr, ret,
                   strerror(-ret));
    mmio_dma_error:
        if (memory_region_is_ram_device(section->mr)) {
            /* Allow unexpected mappings not to be fatal for RAM devices */
            VFIODevice *vbasedev =
                vfio_get_vfio_device(memory_region_owner(section->mr));
            vfio_device_error_append(vbasedev, &err);
            warn_report_err_once(err);
            return;
        }
        goto fail;
    }
```

## 附录

### 如何正确的绑定
#### 尝试直接动态解绑

运行脚本，直接失败:
```txt
🤒  sudo cat /proc/38657/stack
[sudo] password for martins3:
[<0>] nv_sleep_ms.isra.0+0xf8/0x250 [nvidia]
[<0>] os_delay+0x12/0x20 [nvidia]
[<0>] nv_pci_remove_helper+0x3eb/0x520 [nvidia]
[<0>] pci_device_remove+0x4a/0xc0
[<0>] device_release_driver_internal+0x19e/0x200
[<0>] unbind_store+0xa4/0xb0
[<0>] kernfs_fop_write_iter+0x171/0x220
[<0>] vfs_write+0x281/0x570
[<0>] ksys_write+0x7b/0x110
[<0>] do_syscall_64+0x109/0x6e0
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

此时:
```txt
 ls /dev/nvidia*
/dev/nvidia-modeset  /dev/nvidia-uvm-tools  /dev/nvidiactl
/dev/nvidia-uvm      /dev/nvidia0

/dev/nvidia-caps:
nvidia-cap1  nvidia-cap2
```

#### 直通配置方法
自己修改了一次，但是失败了，让 codex 直接修改，结果如下:
1. /etc/modules-load.d/vfio.conf
```txt
🧀  cat /etc/modules-load.d/vfio.conf
vfio
vfio_iommu_type1
vfio_pci
```

2. /etc/modprobe.d/vfio-pci.conf
```txt
# Bind RTX 5060 Ti GPU and its HDMI audio function to vfio-pci.
options vfio-pci ids=10de:2d04,10de:22eb disable_vga=1
```

3. 修改 grub ，添加如下内容:
```txt
vfio-pci.ids=10de:2d04,10de:22eb
```

没有 disable 这些服务
nvidia-powerd.service、nvidia-cdi-refresh.service、nvidia-cdi-refresh.path

现在可以观察到这些错误:
```txt
[Sat Jun 27 19:59:02 2026] nvidia-nvlink: Nvlink Core is being initialized, major device number 508
[Sat Jun 27 19:59:02 2026] NVRM: GPU 0000:01:00.0 is already bound to vfio-pci.
[Sat Jun 27 19:59:02 2026] NVRM: The NVIDIA probe routine was not called for 1 device(s).
[Sat Jun 27 19:59:02 2026] NVRM: This can occur when another driver was loaded and
                           NVRM: obtained ownership of the NVIDIA device(s).
[Sat Jun 27 19:59:02 2026] NVRM: Try unloading the conflicting kernel module (and/or
                           NVRM: reconfigure your kernel without the conflicting
                           NVRM: driver(s)), then try loading the NVIDIA kernel module
                           NVRM: again.
[Sat Jun 27 19:59:02 2026] NVRM: No NVIDIA devices probed.
[Sat Jun 27 19:59:02 2026] nvidia-nvlink: Unregistered Nvlink Core, major device number 508
[Sat Jun 27 19:59:02 2026] nvidia-nvlink: Nvlink Core is being initialized, major device number 508
[Sat Jun 27 19:59:02 2026] NVRM: GPU 0000:01:00.0 is already bound to vfio-pci.
[Sat Jun 27 19:59:02 2026] NVRM: The NVIDIA probe routine was not called for 1 device(s).
[Sat Jun 27 19:59:02 2026] NVRM: This can occur when another driver was loaded and
                           NVRM: obtained ownership of the NVIDIA device(s).
[Sat Jun 27 19:59:02 2026] NVRM: Try unloading the conflicting kernel module (and/or
                           NVRM: reconfigure your kernel without the conflicting
                           NVRM: driver(s)), then try loading the NVIDIA kernel module
                           NVRM: again.
[Sat Jun 27 19:59:02 2026] NVRM: No NVIDIA devices probed.
[Sat Jun 27 19:59:02 2026] nvidia-nvlink: Unregistered Nvlink Core, major device number 508
[Sat Jun 27 19:59:07 2026] nvidia-nvlink: Nvlink Core is being initialized, major device number 508
[Sat Jun 27 19:59:07 2026] NVRM: GPU 0000:01:00.0 is already bound to vfio-pci.
[Sat Jun 27 19:59:07 2026] NVRM: The NVIDIA probe routine was not called for 1 device(s).
[Sat Jun 27 19:59:07 2026] NVRM: This can occur when another driver was loaded and
                           NVRM: obtained ownership of the NVIDIA device(s).
[Sat Jun 27 19:59:07 2026] NVRM: Try unloading the conflicting kernel module (and/or
                           NVRM: reconfigure your kernel without the conflicting
                           NVRM: driver(s)), then try loading the NVIDIA kernel module
                           NVRM: again.
[Sat Jun 27 19:59:07 2026] NVRM: No NVIDIA devices probed.
[Sat Jun 27 19:59:07 2026] nvidia-nvlink: Unregistered Nvlink Core, major device number 508
[Sat Jun 27 19:59:07 2026] nvidia-nvlink: Nvlink Core is being initialized, major device number 508
[Sat Jun 27 19:59:07 2026] NVRM: GPU 0000:01:00.0 is already bound to vfio-pci.
[Sat Jun 27 19:59:07 2026] NVRM: The NVIDIA probe routine was not called for 1 device(s).
[Sat Jun 27 19:59:07 2026] NVRM: This can occur when another driver was loaded and
                           NVRM: obtained ownership of the NVIDIA device(s).
[Sat Jun 27 19:59:07 2026] NVRM: Try unloading the conflicting kernel module (and/or
                           NVRM: reconfigure your kernel without the conflicting
                           NVRM: driver(s)), then try loading the NVIDIA kernel module
                           NVRM: again.
[Sat Jun 27 19:59:07 2026] NVRM: No NVIDIA devices probed.
[Sat Jun 27 19:59:07 2026] nvidia-nvlink: Unregistered Nvlink Core, major device number 508
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
