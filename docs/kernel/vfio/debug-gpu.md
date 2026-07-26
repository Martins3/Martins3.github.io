# 为什么我的 GPU 直通失败了

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

## 为什么开启卡住了

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
## 为什么 QEMU 会抛出警告
```text
  qemu-system-x86_64: warning: vfio_container_dma_map(..., 0xe0200000000, 0x10000000, ...) = -22 (Invalid argument)
  0000:01:00.0: PCI peer-to-peer transactions on BARs are not supported.
```

### 为什么会报错
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

### 为什么报错没有影响

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

### 尝试直接动态解绑

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

### 直通配置方法
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
