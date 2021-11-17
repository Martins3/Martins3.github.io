# seabios
- 使用 KVM 和 tcg 执行 seabios 会出现区别吗, 同样是 tcg，在 LA 上执行和在 x86 上执行存在区别吗? 没有区别，所以在 x86 上使用 kvm 测试就可以了

现在的设想是，如果知道只是一些 ioport 来这些设备，一个个的填补也不错。
对于 pci 只需要一个很小的支持，而对于 acpi 的支持几乎为 0

## 问题
- [ ] 如果通过 boot_disk 中加载的 MBR，那么 MBR 中是什么

- [ ] pci_probe_host 中间说明实际上可以暂时 disable 掉这些 PCI
但是 qemu_detect 说明似乎至少需要提供一个关于 PCI host 的 IO 操作, 所以最小的需要 PCI 支持是什么 ？

- [ ] enable_hwirq
  - [ ] ivt_init

- [ ] 分析 seabios 的 loader script, 有两个需求
    - [ ] QEMU 同时将 bios.bin 映射到 1M 和 4G 的最后位置，reset vector 首先从 4G 的位置跳转到开始的位置, 那岂不是 bios.bin 的前 128k 永远都是没有作用的
        - 可以找到其 loader script
    - [ ] 需要有一个标准的方法来分析给定一个 bios 地址，然后找到对应 seabios 源码，方便之后的调试

- pci_device_tbl : 中间还有更多的设备，注册了各种 dvices 的初始化 hook 函数

- [ ] 初始化路径图从 /home/maritns3/core/vn/docs/qemu/fw_cfg.md 的 seabios 哪一个 section 更新一下
## 决策
虽然具体内容不是也很清楚，但是没有一个 MMIO，找到对应的位置就差不多了

| device | seabios                             | QEMU |
|--------|-------------------------------------|------|
| pci    | pci_probe_host, pci_config_readw    |      |
| acpi   |                                     |      |
| serial | serial_setup (PORT_SERIAL1 0x3f8)   |      |
| timer  | tsctimer_setup (PORT_PIT_MODE 0x43) |      |
| rtc    | rtc_write / rtc_read                |      |

## vga
- maininit
  - vgarom_setup
  - sercon_setup
  - enable_vga_console

取决于启动的方式，如果 -nographic 的话这个这三个函数被注释掉似乎没有任何影响。

但是如果是在 graphic mode, 那么会导致 gtk 的界面永远都是
Guest has not initialized the diaplay(yet)

## MTRR
MTRR use is replaced on modern x86 hardware with PAT. [^2]

Typically, the BIOS (basic input/output system) software configures the MTRRs.[^3]

section 11.3 "Methods of Caching Available"

The MTRRs are useful for statically
describing memory types for physical ranges, and are typically set up by the system BIOS. The PAT extends the
functions of the PCD and PWT bits in page tables to allow all five of the memory types that can be assigned with the
MTRRs (plus one additional memory type) to also be assigned dynamically to pages of the linear address space.[^4]

## threads
The goal of these threads is to reduce overall boot time by parallelizing hardware delays. (For example, by allowing the wait for an ATA hard drive to spin-up and respond to commands to occur in parallel with the wait for a PS/2 keyboard to respond to a setup command.) These hardware setup threads are only available during the "setup" sub-phase of the POST phase.[^1]

## 基本调用路径
The emulators map the SeaBIOS binary to this address,
and SeaBIOS arranges for romlayout.S:reset_vector() to be present there.
This code calls romlayout.S:entry_post() which then calls post.c:handle_post() in 32bit mode.[^1]

- [ ] 之前启动的部分直接放到这里吧

- handle_post
  - serial_debug_preinit :apple: 串口调试，使用 CONFIG_DEBUG_SERIAL 可以关闭
  - make_bios_writable :apple:
    - code_mutable_preinit
      - rtc_write (Setup reset-vector entry point (controls legacy reboots))
  - dopost
    - qemu_preinit
      - qemu_detect
        - qemu_debug_preinit :apple: 关注一下 DebugOutputPort
        - pci_config_readw : 通过读去 PCI 设备的 vendor id 来确定到底需要什么东西 :x: 虽然似乎可以不支持 pci，但是 host bridge 还是需要好好模拟的呀
      - kvm_detect : 空函数, 但是表示需要支持 cpuid
      - qemu_early_e820
        - qemu_cfg_detect : fw_cfg 是现在唯一理解的方法，而且很好的处理了 kernel 的加载问题，所以 fw_cfg 方案是必选的
        - 进行 qemu_cfg_read 之类的事情，将 "etc/e820" 从 host 中读取过来
      - e820_add
      - coreboot_preinit : 为了支持 coreboot, 所以这个是空函数
    - reloc_preinit [^11]
      - maininit : 这就是所有的位置了
        - interface_init
          - malloc_init
          - qemu_cfg_init : 为了支持 fw_cfg，尤其是其中的 file, 需要在 seabios 这边进行支持
            - qemu_cfg_read_entry
            - qemu_cfg_e820
          - ivt_init
          - bda_init
          - boot_init
            - romfile_loadint("etc/boot-fail-wait", 60*1000) : 在 fw_cfg_common_realize => fw_cfg_reboot, 从而在 QEMU 的命令行参数可以控制 seabios 的运行
            - loadBootOrder : 初始化 Bootorder
            - loadBiosGeometry
          - bios32_init : 和 pcibios 相关的
          - pmm_init : 和 bios32_init 类似
          - pnp_init : 和 bios32_init 类似
          - kbd_init :pear: 初始化的是一些结构体
          - mouse_init : mouse 是一个可以配置的系统
        - platform_hardware_setup
          - dma_setup
          - pic_setup :x:
          - thread_setup
          - [ ] mathcp_setup : 在 /proc/ioports 中可以看到 fpu, 也许有关吧
          - qemu_platform_setup
            - pci_setup
              - pci_probe_host : 检测 PCI 是否存在, 从这里进行拦截掉吧
                - `outl(0x80000000, PORT_PCI_CMD);`
              - pci_bios_init_bus :
              - [ ] pci_probe_devices : 枚举所有的设备
              - [ ] pci_bios_init_platform : platform pci, 也就是 bios 从什么地方开始
              - [ ] pci_bios_check_devices
                - pci_region_create_entry
              - pci_bios_map_devices
                - pci_bios_init_root_regions_io : 映射 ioport
                - pci_region_map_entries
                  - pci_region_map_one_entry
                    - pci_config_writeb :
              - pci_bios_init_devices
                - `if (pin != 0) pci_config_writeb(bdf, PCI_INTERRUPT_LINE, pci_slot_get_irq(pci, pin));` : 如果可以，那么分配中断给他
              - pci_enable_default_vga
            - smm_device_setup : System Management Mode[^5] 探测 isapci 和 pmpci
            - smm_setup
            - mtrr_setup
            - msr_feature_control_setup :pear: 应该是空的
            - smp_setup
              - smp_scan :apple: 似乎需要通过 APIC_SVR 和 APIC_LINT1 之类的蛇皮玩意儿来实现操作
            - pirtable_setup :pear:
            - smbios_setup :pear:
              - smbios_romfile_setup
              - smbios_legacy_setup
            - romfile_loader_execute("etc/table-loader") : fw_cfg linker 相关
            - find_acpi_rsdp
            - acpi_dsdt_parse
            - virtio_mmio_setup_acpi
            - find_acpi_rsdp
            - acpi_setup
          - timer_setup
          - clock_setup
            - pit_setup
            - rtc_setup
            - rtc_updating
          - tpm_setup : 似乎是和可信计算相关的 [^6]
        - threads_during_optionroms : 难道在 bios 也是支持多线程吗 ?
        - device_hardware_setup : 这些设备按道理应该都其实是可选的
          - usb_setup
          - ps2port_setup
          - block_setup
          - lpt_setup :apple:
          - serial_setup
          - cbfs_payload_setup : CONFIG_COREBOOT_FLASH
        - vga 相关的配置
          - vgarom_setup
          - sercon_setup
          - enable_vga_console
        - optionrom_setup : Non-VGA option rom init
          - run_file_roms : Run all roms in a given CBFS directory. 其实也就是 genroms/linuxboot_dma.bin 和 genroms/kvmvapic.bin
            - init_optionrom
              - callrom
                - `__callrom`
        - [ ] interactive_bootmenu
        - wait_threads
        - make_bios_readonly
          - startBoot
            - call16_int(0x19, &br)
              - handle_19
                - do_boot
                  - boot_rom
                    - call_boot_entry

## 结论

kvm_detect 中使用一条 cpuid 指令，来知道当前在 KVM 中间运行

- [x] 如何实现 dma 的，如何实现 region 的操作的
```plain
0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
```

P69 : 在 fw_cfg_io_realize 调用 memory_region_init_io 来实现分配 FW_CFG_CTL_SIZE 的

总结下，fw_cfg : 存在一些数值是常规定义的，没有定义过的为 file，fw_cfg_add_file_callback 会计算出其偏移 file 的偏移，并且将文件信息放到 `FWCfgFiles *files;` 中间:


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

```plain
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
  - qemu: build_append_pci_bus_devices 中定义的, 具体作用未知
- [x] 那些 LNK 之类的是搞什么的 ?
  - qemu :  build_piix4_pci0_int, 似乎是 routing table 之类的东西

在 seabios/src/fw/dsdt_parser.c: parse_resource 将资源划分为 small resource, 所以打印分为两种。

#### qemu 是如何制作这个东西的
参考 ./hack/acpi/acpi.md 中 ACPI considerations for PCI host bridges, 大概可以知道 acpi 的 crs 就是用于记录 io 空间和 irq 的

在 https://github.com/disdi/ACPI/blob/master/debian/ssdt.dsl 似乎找到了对应的 QEMU 对应的 dsl

- [ ] 试图拦截一下 GPE 的内容

总体来说，是从 build_dsdt 的位置触发的。

#### [ ] 为什么在 seabios 中间会调用 `acpi_build_update`
调用位置在 `romfile_loader_execute` 中间的

```plain
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
## 到底在 pci 配置空间中写入了什么东西

e1000 的三个 bar 空间:
```c
PCI: map device bdf=00:03.0  bar 1, addr 0000c000, size 00000040 [io]
PCI: map device bdf=00:03.0  bar 6, addr feb80000, size 00040000 [mem]
PCI: map device bdf=00:03.0  bar 0, addr febc0000, size 00020000 [mem]
```

pci 设备的 bar 的起始位置映射的配置是 bios 中的代码扫描完成的
```plain
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

## cat /proc/ioports
```plain
0000-0cf7 : PCI Bus 0000:00
  0000-001f : dma1
  0020-0021 : pic1
  0040-0043 : timer0
  0050-0053 : timer1
  0060-0060 : keyboard
  0064-0064 : keyboard
  0070-0077 : rtc0
  0080-008f : dma page reg
  00a0-00a1 : pic2
  00c0-00df : dma2
  00f0-00ff : fpu
  0170-0177 : 0000:00:01.1
    0170-0177 : ata_piix
  01f0-01f7 : 0000:00:01.1
    01f0-01f7 : ata_piix
  0376-0376 : 0000:00:01.1
    0376-0376 : ata_piix
  03c0-03df : vga+
  03f2-03f2 : floppy
  03f4-03f5 : floppy
  03f6-03f6 : 0000:00:01.1
    03f6-03f6 : ata_piix
  03f7-03f7 : floppy
  03f8-03ff : serial
  0510-051b : QEMU0002:00
  0600-063f : 0000:00:01.3
    0600-0603 : ACPI PM1a_EVT_BLK
    0604-0605 : ACPI PM1a_CNT_BLK
    0608-060b : ACPI PM_TMR
  0700-070f : 0000:00:01.3
0cf8-0cff : PCI conf1
0d00-ffff : PCI Bus 0000:00
  afe0-afe3 : ACPI GPE0_BLK
  c000-c03f : 0000:00:03.0
    c000-c03f : e1000
  c040-c05f : 0000:00:05.0
  c060-c06f : 0000:00:01.1
    c060-c06f : ata_piix
```


[^1]: https://www.seabios.org/Execution_and_code_flow
[^2]: https://www.kernel.org/doc/html/latest/x86/mtrr.html
[^3]: intel manual volume 11.11
[^4]: intel manual volume 11.12
[^5]: https://en.wikipedia.org/wiki/System_Management_Mode
[^6]: https://docs.microsoft.com/en-us/windows-hardware/test/hlk/testref/trusted-execution-environment-acpi-profile

[^11]: reloc_preinit 将 maininit 执行的代码再次进行一次拷贝，但是不知道为什么如此操作
