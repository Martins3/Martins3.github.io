# qemu
## 关联代码
- qemu/hw/vfio/pci.c

## 基本的调试方法

```diff
diff --git a/hw/vfio/listener.c b/hw/vfio/listener.c
index 2d7d3a464577..43f64e0332f7 100644
--- a/hw/vfio/listener.c
+++ b/hw/vfio/listener.c
@@ -608,6 +608,7 @@ void vfio_container_region_add(VFIOContainer *bcontainer,
         }
     }

+    printf("[martins3:%s:%d] %s %lx %lx %lx\n", __func__, __LINE__, section->mr->name, vaddr, iova, int128_getlo(llsize));
     ret = vfio_container_dma_map(bcontainer, iova, int128_get64(llsize),
                                  vaddr, section->readonly, section->mr);
     if (ret) {
@@ -711,6 +712,8 @@ static void vfio_listener_region_del(MemoryListener *listener,
         try_unmap = false;
     }

+    printf("[martins3:%s:%d] %s %lx %lx\n", __func__, __LINE__,
+           section->mr->name, iova, int128_getlo(llsize));
     if (try_unmap) {
         bool unmap_all = false;


```

然后就可以观察到这些结果了:
```txt
[martins3:vfio_container_region_add:611] mem0 7f9aa3fff000 0 c0000
[martins3:vfio_container_region_add:611] pc.rom 7f9ca8400000 c0000 20000
[martins3:vfio_container_region_add:611] pc.bios 7f9ca8620000 e0000 20000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ff000 100000 bff00000
[martins3:vfio_container_region_add:611] pc.bios 7f9ca8600000 fffc0000 40000
[martins3:vfio_container_region_add:611] mem0 7f9b63fff000 100000000 140000000
[martins3:vfio_listener_region_del:715] mem0 0 c0000
[martins3:vfio_container_region_add:611] mem0 7f9aa3fff000 0 a0000
[martins3:virtio_dummy_instance_init:120] 0x55b3a95ad790
GPU instance init
GPU Realize
[martins3:vfio_listener_region_del:715] pc.rom c0000 20000
[martins3:vfio_listener_region_del:715] pc.bios e0000 20000
[martins3:vfio_listener_region_del:715] mem0 100000 bff00000
[martins3:vfio_container_region_add:611] mem0 7f9aa40bf000 c0000 10000
[martins3:vfio_container_region_add:611] pc.rom 7f9ca8410000 d0000 10000
[martins3:vfio_container_region_add:611] pc.bios 7f9ca8620000 e0000 10000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ef000 f0000 bff10000
[martins3:vfio_listener_region_del:715] mem0 c0000 10000
[martins3:vfio_listener_region_del:715] pc.rom d0000 10000
[martins3:vfio_listener_region_del:715] pc.bios e0000 10000
[martins3:vfio_listener_region_del:715] mem0 f0000 bff10000
[martins3:vfio_container_region_add:611] mem0 7f9aa40bf000 c0000 bff40000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 0 mmaps[0] 7f9cb0014000 4240054000 2000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 0 mmaps[0] 7f9cb0017000 4240057000 1000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 4 mmaps[0] 7f9ca8780000 4240040000 10000
[martins3:vfio_container_region_add:611] gpu-fb-mem 7f9a09e00000 fd000000 1000000
[martins3:vfio_container_region_add:611] cirrus_vga.rom 7f9aa0400000 fe3a0000 10000
[martins3:vfio_listener_region_del:715] cirrus_vga.rom fe3a0000 10000
[martins3:vfio_container_region_add:611] vga.vram 7f9a0ba00000 fa000000 400000
[martins3:vfio_container_region_add:611] virtio-net-pci.rom 7f9ca8200000 fe300000 40000
[martins3:vfio_listener_region_del:715] virtio-net-pci.rom fe300000 40000
[martins3:vfio_container_region_add:611] virtio-net-pci.rom 7f9aa0600000 fe340000 40000
[martins3:vfio_listener_region_del:715] virtio-net-pci.rom fe340000 40000
qemu-system-x86_64: vfio-pci: Cannot read device rom at 0000:02:00.0
Device option ROM contents are probably invalid (check dmesg).
Skip option ROM probe with rombar=0, or load from file with romfile=
[martins3:vfio_listener_region_del:715] mem0 c0000 bff40000
[martins3:vfio_container_region_add:611] mem0 7f9aa40bf000 c0000 c000
[martins3:vfio_container_region_add:611] mem0 7f9aa40cb000 cc000 3000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ce000 cf000 1000
[martins3:vfio_container_region_add:611] mem0 7f9aa40cf000 d0000 20000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ef000 f0000 10000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ff000 100000 bff00000
[martins3:vfio_listener_region_del:715] mem0 cf000 1000
[martins3:vfio_listener_region_del:715] mem0 d0000 20000
[martins3:vfio_container_region_add:611] mem0 7f9aa40ce000 cf000 19000
[martins3:vfio_container_region_add:611] mem0 7f9aa40e7000 e8000 8000
[martins3:vfio_listener_region_del:715] 0000:02:00.0 BAR 0 mmaps[0] 4240054000 2000
[martins3:vfio_listener_region_del:715] 0000:02:00.0 BAR 0 mmaps[0] 4240057000 1000
[martins3:vfio_listener_region_del:715] 0000:02:00.0 BAR 4 mmaps[0] 4240040000 10000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 0 mmaps[0] 7f9cb0014000 4240054000 2000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 0 mmaps[0] 7f9cb0017000 4240057000 1000
[martins3:vfio_container_region_add:611] 0000:02:00.0 BAR 4 mmaps[0] 7f9ca8780000 4240040000 10000
[martins3:vfio_listener_region_del:715] vga.vram fa000000 400000
[martins3:vfio_container_region_add:611] vga.vram 7f9a0ba00000 fa000000 400000
[martins3:vfio_listener_region_del:715] gpu-fb-mem fd000000 1000000
[martins3:vfio_container_region_add:611] gpu-fb-mem 7f9a09e00000 fd000000 1000000
```
vfio 映射的 MMIO 空间会被映射到虚拟机的虚拟机地址空间，
然后让虚拟机来直接访问的，这都是直接操作虚拟机的设备地址空间了。

所以，对于这个内存在 gup ，显然是不可以的，因为这个区域根本就没有对应任何内存。

(vfio 的集中 会来 gup 这些内存吗? 可以顺着 vfio_container_dma_map 向下分析)

1. 这些虚拟机地址空间的区间如何获取的?
从 hw/vfio/region.c:vfio_region_mmap 中，提供 fd 来映射的


## 两次 gup 的区别

1. 第一次是 pin pages
2. 第二次是构建 vmx page tables

```txt
+ sudo bpftrace -e 'kprobe:__gup_longterm_locked { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...

@[
        __gup_longterm_locked+1
        pin_user_pages_remote+100
        vaddr_get_pfns+140
        vfio_pin_pages_remote+226
        vfio_pin_map_dma+244
        vfio_dma_do_map+895
        vfio_iommu_type1_ioctl+327
        __x64_sys_ioctl+147
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 8780
```

我猜测是，通过 gup 来实现一下 page walk 而已
```txt
@[
        gup_fast_pte_range+1
        gup_fast_pgd_range+688
        gup_fast+138
        gup_fast_fallback+130
        hva_to_pfn+114
        __kvm_faultin_pfn+97
        __kvm_mmu_faultin_pfn+84
        kvm_mmu_faultin_pfn+287
        kvm_tdp_page_fault+145
        kvm_mmu_do_page_fault+470
        kvm_mmu_page_fault+136
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+951
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+805
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+147
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 197440
```

## TODO
1. 才意识到 qemu_vfio_init_pci 是给 nvme 后端用的

- util/vfio-helpers.c 基本上都是 nvme 在调用

## 有待整理

### vfio 如何模拟的 bar
在 vfio_legacy_dma_map 中添加:
```c
    printf("[martins3:%s:%d] %lx %lx\n", __FUNCTION__, __LINE__, iova, size);
```
vfio_listener_region_add 中添加:
```txt
    printf("-> %s %lx %lx %d\n", section->mr->name, (uint64_t)iova, (uint64_t)llend, section->mr->ram);
```

获取到日志为:
```txt
-> 0000:00:0b.0 BAR 0 mmaps[0] febd3000 febd4000 1
[martins3:vfio_legacy_dma_map:190] febd3000 1000
```
这个 iova 正好是 GPA ， 正好和 /proc/iomem 对应的

通过这种方法，qemu 让 guest os 直接访问 bar 空间。

当然，映射的信息首先通过 vfio 获取，在 qemu 中:
```c
    ret = vfio_get_region_info(vbasedev,
                               VFIO_PCI_CONFIG_REGION_INDEX, &reg_info);
    // --> if (ioctl(vbasedev->fd, VFIO_DEVICE_GET_REGION_INFO, *info)) {
```
vfio 将 会告诉映射 vfio 文件的基本信息

观察 qemu 的 maps ，这里的 offset >> 40 就是 bar index
```txt
🧀  cat /proc/65467/maps | grep vfio
7f2ad4108000-7f2ad410c000 rw-s 00000000 00:0f 3091                       anon_inode:[vfio-device]
7f2ad4110000-7f2ad4114000 rw-s 40000000000 00:0f 3091                    anon_inode:[vfio-device]
7f2ad4118000-7f2ad4119000 rw-s 10000000000 00:0f 3091                    anon_inode:[vfio-device]
```


地址的映射建立的时机:
```txt
@[
    vfio_pci_mmap_fault+5
    __do_fault+67
    do_pte_missing+831
    handle_mm_fault+2183
    fixup_user_fault+266
    vaddr_get_pfns+352
    vfio_pin_pages_remote+259
    vfio_pin_map_dma+245
    vfio_iommu_type1_ioctl+3690
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 7
```

### vfio 如何模拟 pci config

从 vfio_intx_enable 看，还是软件模拟。
```c
    vdev->intx.pin = pin - 1; /* Pin A (1) -> irq[0] */
    pci_config_set_interrupt_pin(vdev->pdev.config, pin);

```

但是 kernel 提供了 vfio_pci_rw 和 vfio_pci_ioctl_get_region_info ，让 qemu 来获取 pci 的基本信息。

## qemu 是如何初始化一个设备的

使用 QEMU 作为，这是一切的入口。


- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_create_cli_devices
          - qemu_opts_foreach
            - device_init_func
              - qdev_device_add
                - qdev_device_add_from_qdict
                  - qdev_realize
                    - object_property_set_bool
                      - object_property_set_qobject
                        - object_property_set
                          - property_set_bool
                            - device_set_realized
                              - pci_qdev_realize
                                - vfio_realize

- vfio_realize
  - VFIODevice::ops 注册为 vfio_pci_ops
  - 通过 host 的 bdf 可以找到 group 路径
    - 传递的参数是 0000:07:00.0
    - 找到设备所在的 /sys/bus/pci/devices/0000:07:00.0/iommu_group
    - 其软连接指向 : ../../../../kernel/iommu_groups/2
    - 好的，现在知道 group id 是 2
  - vfio_get_group
    - 打开 /dev/vfio/2 放到 VFIOGroup::fd 中
    - vfio_connect_container
      - 打开 /dev/vfio/vfio 放到 VFIOContainer::fd 中
      - vfio_init_container
        - vfio_get_iommu_type
          - 通过 **VFIO_CHECK_EXTENSION** 获取到 iommu 的类型，在 AMD 上测试得到是 VFIO_TYPE1v2_IOMMU
        - **VFIO_GROUP_SET_CONTAINER** : 将 group 和 container 联系起来，也就是 /dev/vfio/2 和 /dev/vfio/vfio 联系起来
        - **VFIO_SET_IOMMU** : 设置 iommu 的类型，其实最后就是用内核中 `vfio_iommu_driver_ops_type1`
      - memory_listener_register
        - listener_add_address_space
          - vfio_listener_region_add
            - vfio_dma_map : [VFIO_IOMMU_MAP_DMA](#vfio_iommu_map_dma) ，通过内核的 `vfio_iommu_driver_ops_type1` 中注册的
  - vfio_get_device
    - **VFIO_GROUP_GET_DEVICE_FD** : 获取的到 fd 为在内核中为 `vfio_device_fops`，因为在内核中用的是 pci 的实现，所以操作都会调用到 `vfio_pci_ops` 上
    - **VFIO_DEVICE_GET_INFO** : 关于设备的基本信息
  - vfio_bars_register
    - vfio_bar_register
      - vfio_region_mmap : 映射 mmap
  - vfio_intx_enable : msi 和 msix 差不多的调用流程，但是不是在这里调用的
    - event_notifier_get_fd
    - vfio_set_irq_signaling
      - **VFIO_DEVICE_SET_IRQS** : 注册 irqfd

和哪些 mini 的 POC 测试代码很像了

## qemu 还特殊处理过 option rom 的
```c
static const MemoryRegionOps vfio_rom_ops = {
    .read = vfio_rom_read,
    .write = vfio_rom_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};
```

## vfio-pci 设备可以携带的参数

-device vfio-pci,host=01:00.0,multifunction=on,x-vga=on 中的 x-vga 是什么含义？

qemu-system-x86_64 -device virtio-blk,help
qemu-system-x86_64 -device vfio-pci,help
可以发现，都是有 multifunction 功能的

而 x-vga 是 hw/vfio/igd.c 提供的

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
