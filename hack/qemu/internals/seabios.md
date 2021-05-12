# seabios
https://www.seabios.org/SeaBIOS

- [ ] 从 QEMU fw_cfg 的是如何被 seabios 的 romfile_add



kvm_detect 中使用一条 cpuid 指令，来知道当前在 KVM 中间运行

- [x] 如何实现 dma 的，如何实现 region 的操作的
```
0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
```

P69 : 在 fw_cfg_io_realize 调用 memory_region_init_io 来实现分配 FW_CFG_CTL_SIZE 的

总结下，fw_cfg : 存在一些数值是常规定义的，没有定义过的为 file，fw_cfg_add_file_callback 会计算出其偏移 file 的偏移，并且将文件信息放到 `FWCfgFiles *files;` 中间:

- [ ] 内核是提供给 fw_cfg 的，bios 怎么最后切换到 kernel 的位置开始执行的 ?

## FW_CFG_IO_BASE

```c
#define FW_CFG_IO_BASE     0x510
```

fw_cfg_add_acpi_dsdt : 制作 acpi table 的时候会包含这个信息

在这里初始化:
```c
static void fw_cfg_io_realize(DeviceState *dev, Error **errp)
{
    ERRP_GUARD();
    FWCfgIoState *s = FW_CFG_IO(dev);

    fw_cfg_file_slots_allocate(FW_CFG(s), errp);
    if (*errp) {
        return;
    }

    /* when using port i/o, the 8-bit data register ALWAYS overlaps
     * with half of the 16-bit control register. Hence, the total size
     * of the i/o region used is FW_CFG_CTL_SIZE */
    memory_region_init_io(&s->comb_iomem, OBJECT(s), &fw_cfg_comb_mem_ops,
                          FW_CFG(s), "fwcfg", FW_CFG_CTL_SIZE);

    if (FW_CFG(s)->dma_enabled) {
        memory_region_init_io(&FW_CFG(s)->dma_iomem, OBJECT(s),
                              &fw_cfg_dma_mem_ops, FW_CFG(s), "fwcfg.dma",
                              sizeof(dma_addr_t));
    }

    fw_cfg_common_realize(dev, errp);
}
```

fw_cfg_comb_mem_ops 中对应的 read / write 实现，首先选择地址，然后操作:

- dma 的操作: fw_cfg_dma_transfer 中的，根据配置的地址，最后调用 dma_memory_read / dma_memory_write

## qemu_detect
在 qemu_detect 中，通过检测 host bridge, 可以发现判断当前在 qemu 中间运行。

其在 Qemu 中设置的位置是，而检测正好是 device_id
```c
static void i440fx_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = i440fx_realize;
    k->config_write = i440fx_write_config;
    k->vendor_id = PCI_VENDOR_ID_INTEL;
    k->device_id = PCI_DEVICE_ID_INTEL_82441;
    k->revision = 0x02;
    k->class_id = PCI_CLASS_BRIDGE_HOST;
    dc->desc = "Host bridge";
    dc->vmsd = &vmstate_i440fx;
    /*
     * PCI-facing part of the host bridge, not usable without the
     * host-facing part, which can't be device_add'ed, yet.
     */
    dc->user_creatable = false;
    dc->hotpluggable   = false;
}
```

- [ ] 猜测，通过 pci 的 CONFIG_ADDR 和 CONFIG_DATA 可以来访问这些所有的数据:
