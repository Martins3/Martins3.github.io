# vfio misc

## 给设备绑定驱动的方法

new_id 所有的

```sh
echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

```txt
@[
        vfio_pci_probe+5
        local_pci_probe+66
        pci_call_probe+91
        pci_device_probe+149
        really_probe+219
        __driver_probe_device+120
        driver_probe_device+31
        __driver_attach+186
        bus_for_each_dev+139
        new_id_store+266
        kernfs_fop_write_iter+333
        vfs_write+602
        ksys_write+107
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

- vfio_pci_probe
  - vfio_pci_core_register_device
    - `__vfio_register_dev`
      - vfio_device_set_group
        - vfio_group_find_or_alloc

是的，这个是构建出来了 vfio group

## vfio-pci 的作用
```c
static struct pci_driver vfio_pci_driver = {
    .name           = "vfio-pci",
    .id_table       = vfio_pci_table,
    .probe          = vfio_pci_probe,
    .remove         = vfio_pci_remove,
    .sriov_configure    = vfio_pci_sriov_configure,
    .err_handler        = &vfio_pci_core_err_handlers,
    .driver_managed_dma = true,
};
```
由于 vfio 还有 vfio-platform 的，所以必然可以定义一个基本机制。

vfio_pci_sriov_configure 比较有意思了，

这么操作就可以触发:
```txt
vn on  master [$!+] 😸
🧀  cd /sys/devices/pci0000:00/0000:00:06.0/0000:02:00.0
pci0000:00/0000:00:06.0/0000:02:00.0🔒 😸
🧀  echo 1 | sudo tee sriov_numvfs
```

## 每次加载 vfio 的时候的内核日志
vfio_init 中
```c
#define DRIVER_DESC	"VFIO - User Level meta-driver"
```
```txt
[68634.075962] VFIO - User Level meta-driver version: 0.3
```

## 内核中 arch/um/drivers/vfio_user.c 和 arch/um/drivers/vfio_kern.c

猜测是 uml 下，对于 vfio 机制的模拟，还是有想象力的。

## 不知道为什么现在直通都是会有报错的

当然，也不影响使用哈:
```txt
emu-system-aarch64: qemu_vfio_dma_map(0xfffda1ffd000, 8589934592) failed: VFIO_MAP_DMA failed: Cannot allocate memory
qemu-system-aarch64: qemu_vfio_dma_map(0xfffd97e00000, 67108864) failed: VFIO_MAP_DMA failed: Cannot allocate memory
qemu-system-aarch64: qemu_vfio_dma_map(0xfffd93c00000, 67108864) failed: VFIO_MAP_DMA failed: Cannot allocate memory
qemu-system-aarch64: -device virtio-blk-pci,drive=disk_backend: VFIO_MAP_DMA failed: Cannot allocate memory
```

完全不知道这个东西的规律是什么

### 即便是 VFIO_MAP_DMA 映射错误，系统也是可以正常运行的
2024-12
```txt
qemu-system-x86_64: warning: Number of hotpluggable cpus requested (128) exceeds the recommended cpus supported by KVM (32)
[martins3:virtio_dummy_instance_init:126] 0x55d4bdd62ef0
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380000000000, 0x10000000, 0x7fba00000000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380010000000, 0x2000000, 0x7fba1c000000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380000000000, 0x10000000, 0x7fba00000000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380010000000, 0x2000000, 0x7fba1c000000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380000000000, 0x10000000, 0x7fba00000000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55d4bc5924f0, 0x380010000000, 0x2000000, 0x7fba1c000000) = -22 (Invalid argument)
```

原来直通 nvme 的时候也是有 bug 的:
```txt
[martins3:virtio_dummy_instance_init:126] 0x55a8c89411b0
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x55a8c7683870, 0x3800000000
```

可以发现都是在映射一个地方的时候会出现错误，所以认为应该是一个 mmio 空间的问题了。

这个可以用切换内核到 stable 版本看看，也许过一段时间就没有这些问题了。

的确，到了 2025-12-20 的时候，还是之前的硬件，就没有问题
内核 : 6.17.7-00001-gfd23f075a322
QEMU : version 10.1.91 (v10.2.0-rc1-78-g9ef49528b528-dirty)

2026-01-11 发现还是有问题，触发原因未知。

## 和 option rom 的关系

为什么直通的时候，还需要管 option rom ?

qemu 日志:
```txt
Device option ROM contents are probably invalid (check dmesg).
Skip option ROM probe with rombar=0, or load from file with romfile=
```

内核日志:
```txt
[3189152.966714] vfio-pci 0000:02:00.0: Invalid PCI ROM header signature: expecting 0xaa55, got 0xffff
```

## [ ] 这个报错是什么意思

4.19 内核独享的报错吗?
```txt
[6644841.291571] vfio_cap_init: 0000:98:00.0 pci config conflict @0x78, was cap 0x0 now cap 0x10
[6644841.303496] vfio_cap_init: 0000:98:00.0 pci config conflict @0x79, was cap 0x0 now cap 0x10
[6644841.315363] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7a, was cap 0x0 now cap 0x10
[6644841.327169] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7b, was cap 0x0 now cap 0x10
[6644841.338848] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7c, was cap 0x0 now cap 0x10
[6644841.350513] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7d, was cap 0x0 now cap 0x10
[6644841.362170] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7e, was cap 0x0 now cap 0x10
[6644841.373703] vfio_cap_init: 0000:98:00.0 pci config conflict @0x7f, was cap 0x0 now cap 0x10
[6644841.385206] vfio_cap_init: 0000:98:00.0 pci config conflict @0x80, was cap 0x0 now cap 0x10
[6644841.396605] vfio_cap_init: 0000:98:00.0 pci config conflict @0x81, was cap 0x0 now cap 0x10
[6644841.407903] vfio_cap_init: 0000:98:00.0 pci config conflict @0x82, was cap 0x0 now cap 0x10
[6644841.419143] vfio_cap_init: 0000:98:00.0 pci config conflict @0x83, was cap 0x0 now cap 0x10
[6644841.430310] vfio_cap_init: 0000:98:00.0 pci config conflict @0x84, was cap 0x0 now cap 0x10
[6644841.441416] vfio_cap_init: 0000:98:00.0 pci config conflict @0x85, was cap 0x0 now cap 0x10
[6644841.452515] vfio_cap_init: 0000:98:00.0 pci config conflict @0x86, was cap 0x0 now cap 0x10
[6644841.463549] vfio_cap_init: 0000:98:00.0 pci config conflict @0x87, was cap 0x0 now cap 0x10
[6644841.474521] vfio_cap_init: 0000:98:00.0 pci config conflict @0x88, was cap 0x0 now cap 0x10
[6644841.485496] vfio_cap_init: 0000:98:00.0 pci config conflict @0x89, was cap 0x0 now cap 0x10
[6644841.496398] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8a, was cap 0x0 now cap 0x10
[6644841.507295] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8b, was cap 0x0 now cap 0x10
[6644841.518129] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8c, was cap 0x0 now cap 0x10
[6644841.528966] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8d, was cap 0x0 now cap 0x10
[6644841.539796] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8e, was cap 0x0 now cap 0x10
[6644841.550646] vfio_cap_init: 0000:98:00.0 pci config conflict @0x8f, was cap 0x0 now cap 0x10
[6644841.561487] vfio_cap_init: 0000:98:00.0 pci config conflict @0x90, was cap 0x0 now cap 0x10
[6644841.572319] vfio_cap_init: 0000:98:00.0 pci config conflict @0x91, was cap 0x0 now cap 0x10
[6644841.583038] vfio_cap_init: 0000:98:00.0 pci config conflict @0x92, was cap 0x0 now cap 0x10
[6644841.593638] vfio_cap_init: 0000:98:00.0 pci config conflict @0x93, was cap 0x0 now cap 0x10
[6644841.604235] vfio_cap_init: 0000:98:00.0 pci config conflict @0x94, was cap 0x0 now cap 0x10
[6644841.614724] vfio_cap_init: 0000:98:00.0 pci config conflict @0x95, was cap 0x0 now cap 0x10
[6644841.625091] vfio_cap_init: 0000:98:00.0 pci config conflict @0x96, was cap 0x0 now cap 0x10
[6644841.635344] vfio_cap_init: 0000:98:00.0 pci config conflict @0x97, was cap 0x0 now cap 0x10
[6644841.645564] vfio_cap_init: 0000:98:00.0 pci config conflict @0x98, was cap 0x0 now cap 0x10
[6644841.655673] vfio_cap_init: 0000:98:00.0 pci config conflict @0x99, was cap 0x0 now cap 0x10
[6644841.665786] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9a, was cap 0x0 now cap 0x10
[6644841.675847] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9b, was cap 0x0 now cap 0x10
[6644841.685796] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9c, was cap 0x0 now cap 0x10
[6644841.695668] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9d, was cap 0x0 now cap 0x10
[6644841.705481] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9e, was cap 0x0 now cap 0x10
[6644841.715235] vfio_cap_init: 0000:98:00.0 pci config conflict @0x9f, was cap 0x0 now cap 0x10
[6644841.724925] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa0, was cap 0x0 now cap 0x10
[6644841.734559] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa1, was cap 0x0 now cap 0x10
[6644841.744131] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa2, was cap 0x0 now cap 0x10
[6644841.753652] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa3, was cap 0x0 now cap 0x10
[6644841.763111] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa4, was cap 0x0 now cap 0x10
[6644841.772513] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa5, was cap 0x0 now cap 0x10
[6644841.781853] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa6, was cap 0x0 now cap 0x10
[6644841.791138] vfio_cap_init: 0000:98:00.0 pci config conflict @0xa7, was cap 0x0 now cap 0x10
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
