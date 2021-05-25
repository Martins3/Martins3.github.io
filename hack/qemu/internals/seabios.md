# seabios
- [ ] 从 QEMU fw_cfg 的是如何被 seabios 的 romfile_add

- [ ] seabios 是如何处理 acpi 的

- [ ] layoutrom.S 是怎么安装的 ？
  - 硬件应该保证，当其 int 0x19 的时候， pc 直接指向对应的位置

- [ ] 如果通过 boot_disk 中加载的 MBR，那么 MBR 中是什么

## 结论

kvm_detect 中使用一条 cpuid 指令，来知道当前在 KVM 中间运行

- [x] 如何实现 dma 的，如何实现 region 的操作的
```
0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
```

P69 : 在 fw_cfg_io_realize 调用 memory_region_init_io 来实现分配 FW_CFG_CTL_SIZE 的

总结下，fw_cfg : 存在一些数值是常规定义的，没有定义过的为 file，fw_cfg_add_file_callback 会计算出其偏移 file 的偏移，并且将文件信息放到 `FWCfgFiles *files;` 中间:


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

## acpi 在 seabios 中是怎么处理的
1. ACPI 在 seabios 中是一个标准定义项目
```c
#define QEMU_CFG_ACPI_TABLES            (QEMU_CFG_ARCH_LOCAL + 0)
```
2. qemu_cfg_legacy : 中通过约定好的位置读去 acpi 的代码
  - qemu_cfg_read_entry(&cnt, QEMU_CFG_ACPI_TABLES, sizeof(cnt));
  - qemu_romfile_add(name, QEMU_CFG_ACPI_TABLES, offset, len);

3. qemu_platform_setup
  - pci_setup
  - smbios_setup
    - smbios_romfile_setup : 参考 smbios.c
  - [ ] romfile_loader_execute : 这个 table loader 是做什么的
  - find_acpi_rsdp : 在一个范围 `find_acpi_rsdp f48c0 --> f50c0` 内比对字符串
    - [ ] 其实让我疑惑的问题在于，rsdp 不是通过 fw_cfg 传递过来的吗?
  - acpi_dsdt_parse
    - `struct fadt_descriptor_rev1 *fadt = find_acpi_table(FACP_SIGNATURE);` : 通过 rdsp 找到 fadt
    - `u8 *dsdt = (void*)(fadt->dsdt);`
    - 然后会遍历所有的设备
  - virtio_mmio_setup_acpi
    - acpi_dsdt_find_string  : `static const char *virtio_hid = "LNRO0005";` 使用 hid 在 `static struct hlist_head acpi_devices VARVERIFY32INIT;` 中查询
    - 因为没有配置 virtio，所以，这里并没有找到设备
    - [ ] virtio 为什么需要 bios 支持，是不是为了枚举出来这个 virtio 设备
  - acpi_setup : fw_cfg 当没有提供 acpi table 的时候，这个函数将会让 seabios 重新构建一次 acpi 的内容


- [ ] 不能理解，rsdp_descriptor 放到地址为很低，而 rsdt 放到很高的位置，
```c
rsdp=0x000f4d40
rsdt=0xbffe18fe
```
通过查看 guest 机器的 /proc/iomem 这两个区域都是 Reserved


在 seabios/src/std/acpi.h 中定义了 RSDP 和 RSDT/XSDT
```c
struct rsdp_descriptor {        /* Root System Descriptor Pointer */
    u64 signature;              /* ACPI signature, contains "RSD PTR " */
    u8  checksum;               /* To make sum of struct == 0 */
    u8  oem_id [6];             /* OEM identification */
    u8  revision;               /* Must be 0 for 1.0, 2 for 2.0 */
    u32 rsdt_physical_address;  /* 32-bit physical address of RSDT */
    u32 length;                 /* XSDT Length in bytes including hdr */
    u64 xsdt_physical_address;  /* 64-bit physical address of XSDT */
    u8  extended_checksum;      /* Checksum of entire table */
    u8  reserved [3];           /* Reserved field must be 0 */
};

/* Table structure from Linux kernel (the ACPI tables are under the
   BSD license) */

#define ACPI_TABLE_HEADER_DEF   /* ACPI common table header */ \
    u32 signature;          /* ACPI signature (4 ASCII characters) */ \
    u32 length;                 /* Length of table, in bytes, including header */ \
    u8  revision;               /* ACPI Specification minor version # */ \
    u8  checksum;               /* To make sum of entire table == 0 */ \
    u8  oem_id [6];             /* OEM identification */ \
    u8  oem_table_id [8];       /* OEM table identification */ \
    u32 oem_revision;           /* OEM revision number */ \
    u8  asl_compiler_id [4];    /* ASL compiler vendor ID */ \
    u32 asl_compiler_revision;  /* ASL compiler revision number */

/*
 * ACPI 1.0 Root System Description Table (RSDT)
 */
#define RSDT_SIGNATURE 0x54445352 // RSDT
struct rsdt_descriptor_rev1
{
    ACPI_TABLE_HEADER_DEF       /* ACPI common table header */
    u32 table_offset_entry[0];  /* Array of pointers to other */
    /* ACPI tables */
} PACKED;

/*
 * ACPI 2.0 eXtended System Description Table (XSDT)
 */
#define XSDT_SIGNATURE 0x54445358 // XSDT
struct xsdt_descriptor_rev2
{
    ACPI_TABLE_HEADER_DEF       /* ACPI common table header */
    u64 table_offset_entry[0];  /* Array of pointers to other */
    /* ACPI tables */
} PACKED;
```
length : 从而知道 table_offset_entry 到底存在多少项了.

## ACPI: dumping dsdt devices
- [ ] 就算是在 seabios 中间枚举了所有的设备，又怎么样, 需要干什么吗?
- [ ] 为什么需要靠 acpi 枚举 pci 设备，pci 在 acpi 之前就搞定了
```
ACPI: dumping dsdt devices
    SF8
    SF0
    SE8
    SE0
    SD8
    SD0
    SC8
    SC0
    SB8
    SB0
    SA8
    SA0
    S98
    S90
    S88
    S80
    S78
    S70
    S68
    S60
    S58
    S50
    S48
    S40
    S38
    S30
    S28
    S20
    S18
    S10
    S00
    FWCF, hid, sta (0x8), crs
        i/o 0x510 -> 0x51b
    PHPR, hid, sta (0x8), crs
        i/o 0xae00 -> 0xae17  // acpi-pci-hotplug
    GPE0, hid, sta (0x8), crs // acpi-gpe0
        i/o 0xafe0 -> 0xafe3
    \_SB.CPUS, hid
    \_SB.PCI0.PRES, hid, crs // acpi-cpu-hotplug
        i/o 0xaf00 -> 0xaf0b
    LNKS, hid, sta (0x14)
    LNKD, hid, sta (0x14)
    LNKC, hid, sta (0x14)
    LNKB, hid, sta (0x14)
    LNKA, hid, sta (0x14)
    RTC, hid, crs
        i/o 0x70 -> 0x77
        irq 8
    COM1, hid, sta (0x8), crs // 串口
        i/o 0x3f8 -> 0x3ff
        irq 4
    LPT1, hid, sta (0x8), crs // 并口
        i/o 0x378 -> 0x37f
        irq 7
    FLPA
    FDC0, hid, crs            // 软盘
        i/o 0x3f2 -> 0x3f5
        i/o 0x3f7 -> 0x3f7
        irq 6
parse_resource: small: 0x5 (len 3)
    MOU, hid, sta (0x8), crs
        irq 12
    KBD, hid, sta (0x8), crs
        i/o 0x60 -> 0x60
        i/o 0x64 -> 0x64
        irq 1
    ISA
    HPET, hid, sta (0x14), crs
        mem 0xfed00000 -> 0xfed003ff
    PCI0, hid
```
- [x] 前面的 `S` 之类的东西都是搞什么的 ?
  - build_append_pci_bus_devices 中定义的
- [x] 那些 LNK 之类的是搞什么的 ?
  - qemu :  build_piix4_pci0_int 中创建了一堆这些玩意儿，但是不知道有什么作用
  - 似乎是 routing table 之类的东西

在 seabios/src/fw/dsdt_parser.c: parse_resource 将资源划分为 small resource, 所以打印分为两种。


#### qemu 是如何制作这个东西的
参考 ./hack/acpi/acpi.md 中 ACPI considerations for PCI host bridges, 大概可以知道 acpi 的 crs 就是用于记录 io 空间和 irq 的

在 https://github.com/disdi/ACPI/blob/master/debian/ssdt.dsl 似乎找到了对应的 QEMU 对应的 dsl

- [ ] 其实，我有点想知道，这些 acpi 的内容会真的影响操作系统？
  - [ ] 比如键盘的 keyboard 的 irq 是什么 ?
- [ ] 试图拦截一下 GPE 的内容

总体来说，是从 build_dsdt 的位置触发的。

#### FDC0 / MOU / KBD 是如何添加进去的
```c
/*
#0  i8042_build_aml (isadev=0x5555566eb400, scope=0x55555699a990) at ../hw/input/pckbd.c:564
#1  0x000055555590c474 in isa_build_aml (bus=<optimized out>, scope=scope@entry=0x55555699a990) at ../hw/isa/isa-bus.c:214
#2  0x0000555555a6e08d in build_isa_devices_aml (table=table@entry=0x555556909540) at /home/maritns3/core/kvmqemu/include/hw/isa/isa.h:17
#3  0x0000555555a71281 in build_dsdt (machine=0x5555566c0400, pci_hole64=<synthetic pointer>, pci_hole=<synthetic pointer>, misc=<synthetic pointer>, pm=0x7fffffffd450,
 linker=0x5555568d6bc0, table_data=0x555556aae0a0) at ../hw/i386/acpi-build.c:1403
#4  acpi_build (tables=tables@entry=0x7fffffffd530, machine=0x5555566c0400) at ../hw/i386/acpi-build.c:2374
#5  0x0000555555a73e8e in acpi_setup () at /home/maritns3/core/kvmqemu/include/hw/boards.h:24
#6  0x0000555555a5fd1f in pc_machine_done (notifier=0x5555566c0598, data=<optimized out>) at ../hw/i386/pc.c:789
#7  0x0000555555d27e67 in notifier_list_notify (list=list@entry=0x5555564c0a58 <machine_init_done_notifiers>, data=data@entry=0x0) at ../util/notify.c:39
#8  0x00005555558ff94b in qdev_machine_creation_done () at ../hw/core/machine.c:1280
#9  0x0000555555bae301 in qemu_machine_creation_done () at ../softmmu/vl.c:2567
#10 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2590
#11 0x0000555555bb1e62 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#12 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```
原来是通过 isa 总线将描述信息添加进去的。

#### acpi_pm1_cnt_write
当关机的时候:
```c
/*
#0  acpi_pm1_cnt_write (val=1, ar=0x555557b97d00) at ../hw/acpi/core.c:602
#1  acpi_pm_cnt_write (opaque=0x555557b97d00, addr=0, val=1, width=2) at ../hw/acpi/core.c:602
#2  0x0000555555b8d820 in memory_region_write_accessor (mr=mr@entry=0x555557b97f30, addr=0, value=value@entry=0x7fffd9ff90a8, size=size@entry=2, shift=<optimized out>,
mask=mask@entry=65535, attrs=...) at ../softmmu/memory.c:491
#3  0x0000555555b8c14e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffd9ff90a8, size=size@entry=2, access_size_min=<optimized out>, access_siz
e_max=<optimized out>, access_fn=access_fn@entry=0x555555b8d790 <memory_region_write_accessor>, mr=0x555557b97f30, attrs=...) at ../softmmu/memory.c:552
#4  0x0000555555b8fca7 in memory_region_dispatch_write (mr=mr@entry=0x555557b97f30, addr=0, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softm
mu/memory.c:1502
#5  0x0000555555bd70f0 in flatview_write_continue (fv=fv@entry=0x7ffdc4082740, addr=addr@entry=1540, attrs=..., ptr=ptr@entry=0x7fffeaf82000, len=len@entry=2, addr1=<op
timized out>, l=<optimized out>, mr=0x555557b97f30) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:164
#6  0x0000555555bd7306 in flatview_write (fv=0x7ffdc4082740, addr=addr@entry=1540, attrs=attrs@entry=..., buf=buf@entry=0x7fffeaf82000, len=len@entry=2) at ../softmmu/p
hysmem.c:2786
#7  0x0000555555bd9fc6 in address_space_write (as=0x5555564e0ba0 <address_space_io>, addr=addr@entry=1540, attrs=..., buf=0x7fffeaf82000, len=len@entry=2) at ../softmmu
/physmem.c:2878
#8  0x0000555555bda05e in address_space_rw (as=<optimized out>, addr=addr@entry=1540, attrs=..., attrs@entry=..., buf=<optimized out>, len=len@entry=2, is_write=is_writ
e@entry=true) at ../softmmu/physmem.c:2888
#9  0x0000555555bb8899 in kvm_handle_io (count=1, size=2, direction=<optimized out>, data=<optimized out>, attrs=..., port=1540) at ../accel/kvm/kvm-all.c:2256
#10 kvm_cpu_exec (cpu=cpu@entry=0x5555569c1d20) at ../accel/kvm/kvm-all.c:2507
#11 0x0000555555b771e5 in kvm_vcpu_thread_fn (arg=arg@entry=0x5555569c1d20) at ../accel/kvm/kvm-accel-ops.c:49
#12 0x0000555555d248e3 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:521
#13 0x00007ffff6096609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#14 0x00007ffff5fbb293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

#### 在 Guest 中间 disassembly acpi
参考 : https://01.org/linux-acpi/utilities

#### [ ] 为什么在 seabios 中间会调用 `acpi_build_update`
调用位置在 `romfile_loader_execute` 中间的

```
[rom : kvmvapic.bin]
[rom : linuxboot_dma.bin]
[rom : /home/maritns3/core/seabios/out/bios.bin]
[rom : etc/acpi/tables]
[rom : etc/table-loader]
[rom : etc/acpi/rsdp]
```
下面三个都是在 `acpi_setup` 中初始化的，通过函数 `acpi_add_rom_blob`

- acpi_add_rom_blob
  - rom_add_blob
    - fw_cfg_add_file_callback
      - ...(多次调用，跟踪 acpi_build_update 的传递)  fw_cfg_add_bytes_callback 中注册这个函数指针
  
- [ ] 暂时搞不清楚了，只是知道在 fw_cfg 的 dma select 的时候最后会调用

#### [ ] 看懂 dsdt.dsl
- [ ] Notify 和 hotplug

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


```c
/**
#0  do_pci_register_device (errp=<optimized out>, devfn=<optimized out>, name=<optimized out>, pci_dev=<optimized out>) at ../hw/pci/pci.c:1016
#1  pci_qdev_realize (qdev=0x555556892a90, errp=0x7fffffffd330) at ../hw/pci/pci.c:2110
#2  0x0000555555cade57 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd3b0) at ../hw/core/qdev.c:761
#3  0x0000555555c9b1aa in property_set_bool (obj=0x555556892a90, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd3b0) at ../qom/object.c :2257
#4  0x0000555555c9d6bc in object_property_set (obj=obj@entry=0x555556892a90, name=name@entry=0x555555ed7fb6 "realized", v=v@entry=0x555556ac7f00, errp=errp@entry=0x5555 564e2e30 <error_fatal>) at ../qom/object.c:1402
#5  0x0000555555c9f9f4 in object_property_set_qobject (obj=obj@entry=0x555556892a90, name=name@entry=0x555555ed7fb6 "realized", value=value@entry=0x5555568bc0f0, errp=e rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#6  0x0000555555c9d909 in object_property_set_bool (obj=0x555556892a90, name=0x555555ed7fb6 "realized", value=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../ qom/object.c:1472
#7  0x0000555555cacd23 in qdev_realize_and_unref (dev=0x555556892a90, bus=<optimized out>, errp=<optimized out>) at ../hw/core/qdev.c:396
#8  0x000055555596d5f0 in pci_create_simple_multifunction (bus=0x555556e09800, devfn=<optimized out>, multifunction=<optimized out>, name=<optimized out>) at ../hw/pci/ pci.c:2190
#9  0x000055555589012f in i440fx_init (host_type=host_type@entry=0x555555d80e54 "i440FX-pcihost", pci_type=pci_type@entry=0x555555dbe5ad "i440FX", pi440fx_state=pi440fx _state@entry=0x7fffffffd540, address_space_mem=address_space_mem@entry=0x5555566f6400, address_space_io=address_space_io@entry=0x55555668f800, ram_size=8589934592, belo w_4g_mem_size=3221225472, above_4g_mem_size=5368709120, pci_address_space=0x55555681a700, ram_memory=0x555556602c60) at ../hw/pci-host/i440fx.c:266
#10 0x0000555555a65b2d in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:202
#11 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#12 0x0000555555bae1de in qemu_init_board () at ../softmmu/vl.c:2514
#13 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#14 0x0000555555bb1e32 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3612
#15 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```

每一个地址空间都存在:
```c
struct PCIDevice {
    DeviceState qdev;
    bool partially_hotplugged;

    /* PCI config space */
    uint8_t *config;

    PCIConfigReadFunc *config_read;
    PCIConfigWriteFunc *config_write;
```

- [x] 猜测，通过 pci 的 CONFIG_ADDR 和 CONFIG_DATA 可以来访问这些所有的数据
    - [x] 既然所有的 pci 都是可以统一寻址，那么应该存在一个机制将所有的 PCIDevice::config 挂到一起。
      - 通过 `PCIDevice *pci_dev = pci_dev_find_by_addr(s, addr);`

```c
[0] from 0x0000555555878040 in pci_host_config_read_common+0 at ../hw/pci/pci_host.c:88
[1] from 0x00005555558782ee in pci_data_read+62 at ../hw/pci/pci_host.c:133
[2] from 0x0000555555878323 in pci_host_data_read+35 at ../hw/pci/pci_host.c:178
[3] from 0x0000555555b8da11 in memory_region_read_accessor+65 at ../softmmu/memory.c:442
[4] from 0x0000555555b8c10e in access_with_adjusted_size+174 at ../softmmu/memory.c:552
[5] from 0x0000555555b8fb61 in memory_region_dispatch_read1+41 at ../softmmu/memory.c:1422
[6] from 0x0000555555b8fb61 in memory_region_dispatch_read+113 at ../softmmu/memory.c:1450
[7] from 0x0000555555bd9ba9 in flatview_read_continue+233 at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:164
[8] from 0x0000555555bd9d63 in flatview_read+99 at ../softmmu/physmem.c:2849
[9] from 0x0000555555bd9eb6 in address_space_read_full+86 at ../softmmu/physmem.c:2862
```

```c
/**
 *
#0  pci_default_read_config (d=0x5555568923a0, address=0, len=2) at ../hw/pci/pci.c:1419
#1  0x00005555558780f8 in pci_host_config_read_common (pci_dev=0x5555568923a0, addr=0, limit=<optimized out>, limit@entry=256, len=2) at ../hw/pci/pci_host.c:104
#2  0x00005555558782ee in pci_data_read (s=<optimized out>, addr=<optimized out>, len=<optimized out>) at ../hw/pci/pci_host.c:133
#3  0x0000555555878323 in pci_host_data_read (opaque=<optimized out>, addr=<optimized out>, len=<optimized out>) at ../hw/pci/pci_host.c:178
#4  0x0000555555b8da11 in memory_region_read_accessor (mr=mr@entry=0x5555569aa1a0, addr=0, value=value@entry=0x7fffd9ff9130, size=size@entry=2, shift=0, mask=mask@entry
=65535, attrs=...) at ../softmmu/memory.c:442
#5  0x0000555555b8c10e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffd9ff9130, size=size@entry=2, access_size_min=<optimized out>, access_siz
e_max=<optimized out>, access_fn=0x555555b8d9d0 <memory_region_read_accessor>, mr=0x5555569aa1a0, attrs=...) at ../softmmu/memory.c:552
#6  0x0000555555b8fb61 in memory_region_dispatch_read1 (attrs=..., size=<optimized out>, pval=0x7fffd9ff9130, addr=0, mr=0x5555569aa1a0) at ../softmmu/memory.c:1422
#7  memory_region_dispatch_read (mr=mr@entry=0x5555569aa1a0, addr=0, pval=pval@entry=0x7fffd9ff9130, op=MO_16, attrs=attrs@entry=...) at ../softmmu/memory.c:1450
#8  0x0000555555bd9ba9 in flatview_read_continue (fv=fv@entry=0x555556b98730, addr=addr@entry=3324, attrs=..., ptr=ptr@entry=0x7fffeaf82000, len=len@entry=2, addr1=<opt
imized out>, l=<optimized out>, mr=0x5555569aa1a0) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:164
#9  0x0000555555bd9d63 in flatview_read (fv=0x555556b98730, addr=addr@entry=3324, attrs=attrs@entry=..., buf=buf@entry=0x7fffeaf82000, len=len@entry=2) at ../softmmu/ph
ysmem.c:2849
#10 0x0000555555bd9eb6 in address_space_read_full (as=0x5555564e0ba0 <address_space_io>, addr=3324, attrs=..., buf=0x7fffeaf82000, len=2) at ../softmmu/physmem.c:2862
#11 0x0000555555bda035 in address_space_rw (as=<optimized out>, addr=addr@entry=3324, attrs=..., attrs@entry=..., buf=<optimized out>, len=len@entry=2, is_write=is_writ
e@entry=false) at ../softmmu/physmem.c:2890
#12 0x0000555555bb8869 in kvm_handle_io (count=1, size=2, direction=<optimized out>, data=<optimized out>, attrs=..., port=3324) at ../accel/kvm/kvm-all.c:2256
#13 kvm_cpu_exec (cpu=cpu@entry=0x555556a9fca0) at ../accel/kvm/kvm-all.c:2507
#14 0x0000555555b771a5 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556a9fca0) at ../accel/kvm/kvm-accel-ops.c:49
#15 0x0000555555d248b3 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:521
#16 0x00007ffff6096609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#17 0x00007ffff5fbb293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
*/
```

我草，这不就是想要找到的位置吗?
```c
static void i440fx_pcihost_initfn(Object *obj)
{
    PCIHostState *s = PCI_HOST_BRIDGE(obj);

    memory_region_init_io(&s->conf_mem, obj, &pci_host_conf_le_ops, s,
                          "pci-conf-idx", 4);
    memory_region_init_io(&s->data_mem, obj, &pci_host_data_le_ops, s,
                          "pci-conf-data", 4);
}
```
进行 pci 地址空间的 IO 就是只有这个入口，之后在逐步分发:


## pci_setup
- pci_setup
  - pci_probe_host : 检测 PCI 是否存在
    - `outl(0x80000000, PORT_PCI_CMD);`
  - [ ] pci_bios_init_bus : 应该也是遍历所有的设备
  - pci_probe_devices : 枚举所有的设备
  - pci_bios_init_platform : 初始化 platform pci, 也就是 bios 从什么地方开始
  - pci_bios_check_devices
    - pci_region_create_entry
  - pci_bios_map_devices
    - pci_bios_init_root_regions_io : 映射 ioport
    - pci_region_map_entries
      - pci_region_map_one_entry
        - pci_config_writeb : 
  - pci_bios_init_devices
    - `if (pin != 0) pci_config_writeb(bdf, PCI_INTERRUPT_LINE, pci_slot_get_irq(pci, pin));` : 如果可以，那么分配中断给他
  - pci_enable_default_vga

验证 : 在内核的时候，绝对不会发生对于 pci 配置空间的操作, 配置还是会被写:
或者，我们想要的是，bios 的工作，无论是谁来做，其实都是相同的。


## 到底在 pci 配置空间中写入了什么东西

e1000 的三个 bar 空间:
```c
PCI: map device bdf=00:03.0  bar 1, addr 0000c000, size 00000040 [io]
PCI: map device bdf=00:03.0  bar 6, addr feb80000, size 00040000 [mem]
PCI: map device bdf=00:03.0  bar 0, addr febc0000, size 00020000 [mem]
```

pci 设备的 bar 的起始位置映射的配置是 bios 中的代码扫描完成的
```
huxueshi:e1000_write_config 10 ffffffff 4
huxueshi:e1000_write_config 10 0 4
huxueshi:e1000_write_config 14 ffffffff 4
huxueshi:e1000_write_config 14 1 4
huxueshi:e1000_write_config 18 ffffffff 4
huxueshi:e1000_write_config 18 0 4
huxueshi:e1000_write_config 1c ffffffff 4
huxueshi:e1000_write_config 1c 0 4
huxueshi:e1000_write_config 20 ffffffff 4
huxueshi:e1000_write_config 20 0 4
huxueshi:e1000_write_config 24 ffffffff 4
huxueshi:e1000_write_config 24 0 4
huxueshi:e1000_write_config 30 fffff800 4
huxueshi:e1000_write_config 30 0 4
huxueshi:e1000_write_config 14 c000 4
huxueshi:e1000_write_config 30 feb80000 4
huxueshi:e1000_write_config 10 febc0000 4
huxueshi:e1000_write_config 3c b 1
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 30 fffffffe 4
huxueshi:e1000_write_config 30 feb80001 4
huxueshi:e1000_write_config 30 feb80000 4
// 下面是内核日志

huxueshi:e1000_write_config 4 503 2
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 10 ffffffff 4
huxueshi:e1000_write_config 10 febc0000 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 14 ffffffff 4
huxueshi:e1000_write_config 14 c001 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 18 ffffffff 4
huxueshi:e1000_write_config 18 0 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 1c ffffffff 4
huxueshi:e1000_write_config 1c 0 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 20 ffffffff 4
huxueshi:e1000_write_config 20 0 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 24 ffffffff 4
huxueshi:e1000_write_config 24 0 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 100 2
huxueshi:e1000_write_config 30 fffff800 4
huxueshi:e1000_write_config 30 feb80000 4
huxueshi:e1000_write_config 4 103 2
huxueshi:e1000_write_config 4 107 2
huxueshi:e1000_write_config d 40 1
```

- 感觉 e1000 被重新初始化了一次
- 这就是将所有的 bar 空间全部扫描一次  
- 这些在进行 e1000_probe 之前已经完成了。

真相就是在这里:
```c
/*
#0  pci_write_config_word (dev=0xffff8881002d9800, where=4, val=256) at drivers/pci/pci.h:383
#1  0xffffffff8142f3f5 in __pci_read_base (dev=dev@entry=0xffff8881002d9800, type=type@entry=pci_bar_unknown, res=res@entry=0xffff8881002d9c50, pos=pos@entry=28) at dri
vers/pci/probe.c:190
#2  0xffffffff8142f4b2 in pci_read_bases (dev=0xffff8881002d9800, howmany=6, rom=48) at drivers/pci/probe.c:335
#3  0xffffffff8142fa17 in pci_setup_device (dev=dev@entry=0xffff8881002d9800) at drivers/pci/probe.c:1848
#4  0xffffffff8143060d in pci_scan_device (devfn=8, bus=0xffff888100259c00) at drivers/pci/probe.c:2355
#5  pci_scan_single_device (devfn=8, bus=0xffff888100259c00) at drivers/pci/probe.c:2512
#6  pci_scan_single_device (bus=0xffff888100259c00, devfn=8) at drivers/pci/probe.c:2502
#7  0xffffffff8143068d in pci_scan_slot (bus=bus@entry=0xffff888100259c00, devfn=devfn@entry=8) at drivers/pci/probe.c:2591
#8  0xffffffff814317b0 in pci_scan_child_bus_extend (bus=bus@entry=0xffff888100259c00, available_buses=available_buses@entry=0) at drivers/pci/probe.c:2808
#9  0xffffffff81431997 in pci_scan_child_bus (bus=bus@entry=0xffff888100259c00) at drivers/pci/probe.c:2938
#10 0xffffffff81471601 in acpi_pci_root_create (root=root@entry=0xffff888100222a00, ops=ops@entry=0xffffffff826411c0 <acpi_pci_root_ops>, info=info@entry=0xffff88810022
ca20, sysdata=sysdata@entry=0xffff88810022ca58) at drivers/acpi/pci_root.c:925
#11 0xffffffff81b23a9d in pci_acpi_scan_root (root=root@entry=0xffff888100222a00) at arch/x86/pci/acpi.c:368
#12 0xffffffff81b4142c in acpi_pci_root_add (device=0xffff888100217000, not_used=<optimized out>) at drivers/acpi/pci_root.c:597
#13 0xffffffff8146a1e3 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038
#14 acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#15 0xffffffff8146a130 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#16 0xffffffff8146a130 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#17 0xffffffff8146bf3f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#18 0xffffffff82b6f9a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#19 0xffffffff82b6f6d3 in acpi_init () at drivers/acpi/bus.c:1341
#20 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b6f2e1 <acpi_init>) at init/main.c:1249
#21 0xffffffff82b3d26b in do_initcall_level (command_line=0xffff888100123400 "root", level=4) at ./include/linux/compiler.h:234
#22 do_initcalls () at init/main.c:1338
#23 do_basic_setup () at init/main.c:1358
#24 kernel_init_freeable () at init/main.c:1560
#25 0xffffffff81b7c249 in kernel_init (unused=<optimized out>) at init/main.c:1447
#26 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#27 0x0000000000000000 in ?? ()
```
从 pci_read_bases 中可以看的超级清晰，为什么在 Linux kernel 中间依旧存在配置 bar 空间的行为.
