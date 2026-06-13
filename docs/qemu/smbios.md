# smbios
- [ ] 似乎是 fw_cfg_build_smbios 制作出来一个文件 然后通过 fw_cfg 传递给 seabios 使用的吧

https://gist.github.com/smoser/290f74c256c89cb3f3bd434a27b9f64c

- fw_cfg_build_smbios
  - 然后就是各种构建 smbios 了
  - [ ] 无法理解的是，为什么需要 anchor 啊
    - [ ] smbios 也是有 anchor 的吗?

## seabios
在 seabios 的那一侧:
```c
#define QEMU_CFG_SMBIOS_ENTRIES         (QEMU_CFG_ARCH_LOCAL + 1)
```
1. qemu_cfg_init : smbios 是其中的一个文件
  - qemu_cfg_read_entry(&count, QEMU_CFG_FILE_DIR, sizeof(count));

日志:
```plain
Add romfile: etc/smbios/smbios-anchor (size=31)
Add romfile: etc/smbios/smbios-tables (size=354)
```
2. smbios_setup
  - smbios_romfile_setup
    - `f_anchor->copy(f_anchor, &ep, f_anchor->size);` : 从 QEMU 中将制作的表格读取出来
    - `f_tables->copy(f_tables, qtables, f_tables->size);`
    - smbios_new_type_0 : 填充 type 0 的表格
    - copy_smbios : 从 stack 上拷贝到一个确定的区域
  - smbios_legacy_setup : 并不会 fallback 到这里

## qemu
表格的创建 : /hw/smbios/smbios.c

- fw_cfg_build_smbios
  - smbios_get_tables : 一系列的表格制作，需要指出的是 smbios_build_type_0_table 实际上并不会执行，因为这里记录的 SeaBIOS 的信息，是让 bios 制作的
    - smbios_build_type_0_table();
    - smbios_build_type_1_table();
    - smbios_build_type_2_table();
    - smbios_build_type_3_table();
  - fw_cfg_add_file(fw_cfg, "etc/smbios/smbios-tables", smbios_tables, smbios_tables_len);
  - fw_cfg_add_file(fw_cfg, "etc/smbios/smbios-anchor", smbios_anchor, smbios_anchor_len);

存在两种格式的 smbios:
```c
/* SMBIOS Entry Point
 * There are two types of entry points defined in the SMBIOS specification
 * (see below). BIOS must place the entry point(s) at a 16-byte-aligned
 * address between 0xf0000 and 0xfffff. Note that either entry point type
 * can be used in a 64-bit target system, except that SMBIOS 2.1 entry point
 * only allows the SMBIOS struct table to reside below 4GB address space.
 */

/* SMBIOS 2.1 (32-bit) Entry Point
 *  - introduced since SMBIOS 2.1
 *  - supports structure table below 4GB only
 */
struct smbios_21_entry_point {
    uint8_t anchor_string[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t smbios_major_version;
    uint8_t smbios_minor_version;
    uint16_t max_structure_size;
    uint8_t entry_point_revision;
    uint8_t formatted_area[5];
    uint8_t intermediate_anchor_string[5];
    uint8_t intermediate_checksum;
    uint16_t structure_table_length;
    uint32_t structure_table_address;
    uint16_t number_of_structures;
    uint8_t smbios_bcd_revision;
} QEMU_PACKED;

/* SMBIOS 3.0 (64-bit) Entry Point
 *  - introduced since SMBIOS 3.0
 *  - supports structure table at 64-bit address space
 */
struct smbios_30_entry_point {
    uint8_t anchor_string[5];
    uint8_t checksum;
    uint8_t length;
    uint8_t smbios_major_version;
    uint8_t smbios_minor_version;
    uint8_t smbios_doc_rev;
    uint8_t entry_point_revision;
    uint8_t reserved;
    uint32_t structure_table_max_size;
    uint64_t structure_table_address;
} QEMU_PACKED;
```

## kernel
- [ ] 内核是如何使用这些内容的 ?

## 问题

在 pc_init1 中
```c
    if (pcmc->smbios_defaults) {
        MachineClass *mc = MACHINE_GET_CLASS(machine);
        /* These values are guest ABI, do not change */
        smbios_set_defaults("QEMU", "Standard PC (i440FX + PIIX, 1996)",
                            mc->name, pcmc->smbios_legacy_mode,
                            pcmc->smbios_uuid_encoded,
                            SMBIOS_ENTRY_POINT_21);
    }
```
其最后体现出来的效果是什么啊?

- 在 seabios 中间如何使用不知道，但是


## dmidecode(8)
dmidecode is a tool for dumping a computer's DMI (some say SMBIOS ) table contents in a human-readable format. This table contains a description of the system's hardware components, as well as other useful pieces of information such as serial numbers and BIOS revision.

这个工具的 backtrace 是什么样子的，从 Host QEMU 到 guest 的输出。


## SMBIOS 可以用来汇报 Guest 的 CPU topo ?
- smbios_get_tables 吗?

guest 内核中:
socket 应该是通过这个数值获取的:
```sh
for((i=0; i < 8; i++)); do
    cat /sys/devices/system/cpu/cpu$i/topology/physical_package_id
done
```

最后检查，是出现这个位置来设置，通过 cpuid
- detect_extended_topology

而 QEMU 中，对应的生成位置是: cpu_x86_cpuid

```c
            if (*eax & 31) {
                int host_vcpus_per_cache = 1 + ((*eax & 0x3FFC000) >> 14);
                int vcpus_per_socket = env->nr_dies * cs->nr_cores *
                                       cs->nr_threads;
                if (cs->nr_cores > 1) {
                    *eax &= ~0xFC000000;
                    *eax |= (pow2ceil(cs->nr_cores) - 1) << 26;
                }
                if (host_vcpus_per_cache > vcpus_per_socket) {
                    *eax &= ~0x3FFC000;
                    *eax |= (pow2ceil(vcpus_per_socket) - 1) << 14;
                }
            }
```

## 物理上的实现

```txt
16:46:41 openat(AT_FDCWD, "/sys/firmware/dmi/tables/smbios_entry_point", O_RDONLY) = 3
16:46:41 newfstatat(3, "", {st_mode=S_IFREG|0400, st_size=24, ...}, AT_EMPTY_PATH) = 0
16:46:41 openat(AT_FDCWD, "/sys/firmware/dmi/tables/DMI", O_RDONLY) = 3
```

```txt
└── id
    ├── bios_date
    ├── bios_release
    ├── bios_vendor
    ├── bios_version
    ├── board_asset_tag
    ├── board_name
    ├── board_serial
    ├── board_vendor
    ├── board_version
    ├── chassis_asset_tag
    ├── chassis_serial
    ├── chassis_type
    ├── chassis_vendor
    ├── chassis_version
    ├── modalias
    ├── power
    │   ├── async
    │   ├── autosuspend_delay_ms
    │   ├── control
    │   ├── runtime_active_kids
    │   ├── runtime_active_time
    │   ├── runtime_enabled
    │   ├── runtime_status
    │   ├── runtime_suspended_time
    │   └── runtime_usage
    ├── product_family
    ├── product_name
    ├── product_serial
    ├── product_sku
    ├── product_uuid
    ├── product_version
    ├── subsystem -> ../../../../class/dmi
    ├── sys_vendor
    └── uevent
```

操作系统的实现: linux/drivers/firmware/dmi_scan.c
