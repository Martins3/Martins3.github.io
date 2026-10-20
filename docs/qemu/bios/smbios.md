# smbios / dmi
<!-- 576ceb91-74a0-4feb-b1c9-663fe951215e -->

## DMI 和 SMBIOS 是什么关系?
DMI（Desktop Management Interface)

其实 DMI 和 SMBIOS 就是一个东西

> Man dmidecode(8) 可以看到
>
> dmidecode is a tool for dumping a computer's DMI (some say SMBIOS ) table
> contents in a human-readable format. This table contains a description of the
> system's hardware components, as well as other useful pieces of information such
> as serial numbers and BIOS revision.

## 工作原理

固件提供 SMBIOS 表
    ↓
Linux 的 dmi_scan.c 识别
    ↓
/sys/firmware/dmi/tables/DMI 暴露
    ↓
dmidecode 按 SMBIOS 规范解析

## 虚拟机中的工作原理

dmidecode 解析的是 qemu 填充的 smbios 的内容

- [ ] 似乎是 fw_cfg_build_smbios 制作出来一个文件 然后通过 fw_cfg 传递给 seabios 使用的吧

https://gist.github.com/smoser/290f74c256c89cb3f3bd434a27b9f64c

- fw_cfg_build_smbios
  - 然后就是各种构建 smbios 了
  - [ ] 无法理解的是，为什么需要 anchor 啊
    - [ ] smbios 也是有 anchor 的吗?

## 虚拟化下的基本流程

### qemu
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


### seabios
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

## kernel

### 观察
13900k
```txt
Apr 23 12:23:19 nixos kernel: DMI: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
Apr 23 12:23:19 nixos kernel: DMI: Memory slots populated: 2/4
Apr 23 12:23:19 nixos kernel: tsc: Detected 3000.000 MHz processor
Apr 23 12:23:19 nixos kernel: tsc: Detected 2995.200 MHz TSC
```
```txt
bios_date:08/12/2022
bios_release:16.20
bios_vendor:American Megatrends Inc.
bios_version:1620
board_asset_tag:Default string
board_name:TUF GAMING B660-PLUS WIFI D4
```

qemu 中启动:
```txt
 sudo dmesg | grep DMI
[sudo] password for martins3:
[    0.000000] DMI: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[    0.000000] DMI: Memory slots populated: 1/1
```
```txt
martins3@linux:/sys/class/dmi/id$ grep . *
bios_date:04/01/2014
bios_release:0.0
bios_vendor:SeaBIOS
bios_version:rel-1.17.0-4-g0026c353eb4e
```

这个对应的实现，就是在 pc_init1 中:
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

Hyper-V 虚拟机
```txt
🧀  sudo dmesg | grep DMI
[sudo] password for martins3:
[    0.000000] DMI: Microsoft Corporation Virtual Machine/Virtual Machine, BIOS Hyper-V UEFI Release v4.1 09/04/2024
[    0.000000] DMI: Memory slots populated: 2/2
```

n100 的机器:
```txt
Apr 17 00:13:01 localhost kernel: DMI: Micro Computer (HK) Tech Limited Venus Series/DNBID, BIOS 0.15 01/10/2024
Apr 17 00:13:01 localhost kernel: DMI: Memory slots populated: 4/8
Apr 17 00:13:01 localhost kernel: tsc: Detected 800.000 MHz processor
Apr 17 00:13:01 localhost kernel: tsc: Detected 806.400 MHz TSC
```

### 实现
```txt
DMI/SMBIOS SUPPORT
M:	Jean Delvare <jdelvare@suse.com>
S:	Maintained
T:	git git://git.kernel.org/pub/scm/linux/kernel/git/jdelvare/staging.git dmi-for-next
F:	Documentation/ABI/testing/sysfs-firmware-dmi-tables
F:	drivers/firmware/dmi-id.c
F:	drivers/firmware/dmi_scan.c
F:	include/linux/dmi.h
```

此外，还有这些东西:
- /sys/class/dmi/id
- /sys/firmware/dmi/entries : 具体 entry
- /sys/firmware/dmi/tables
	- /sys/firmware/dmi/tables/smbios_entry_point
	- /sys/firmware/dmi/tables/DMI


找到 dmi_scan_machine 中，其具体的逻辑为:

> [!NOTE]
> 参考神奇海螺的意见，有待验证

  Method 1: EFI Configuration Tables
   - When EFI is available, the kernel checks efi.smbios3 (64-bit entry point) and efi.smbios (32-bit entry point)
   - These addresses are provided by the EFI firmware and stored in the EFI configuration tables

  Method 2: Memory Scanning (Non-EFI fallback)
   - Scans physical memory from 0xF0000 to 0x100000 (64KB range)
   - Looks for SMBIOS signatures in memory: _SM3_, `_SM_`, and `_DMI_`


那么如何获取的 efi.smbios3 ，大致过程因为是
1. GUID Definition: SMBIOS 3.0 has a specific GUID defined as:

#define SMBIOS3_TABLE_GUID EFI_GUID(0xf2fd1544, 0x9794, 0x4a2c, 0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94)

2. Table Mapping: The system defines a mapping between the SMBIOS3 GUID and the efi.smbios3 field:

```txt
   1    static const efi_config_table_type_t common_tables[] __initconst = {
   2        // ...
   3        {SMBIOS3_TABLE_GUID, &efi.smbios3, "SMBIOS 3.0"},
   4        // ...
   5    };
```

3. Runtime Assignment: During early boot, the efi_config_parse_tables() function:
   - Iterates through the EFI configuration tables provided by the firmware
   - For each table, it calls match_config_table() to check if the table's GUID matches any of the expected GUIDs in the common_tables array
   - When it finds a match between a firmware table's GUID and SMBIOS3_TABLE_GUID, it assigns the table's physical
     address to efi.smbios3 via *(table_types[i].ptr) = table;


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

## dmidecode

### 可以展示更加具体的 CPU 信息

920-4826
```txt
andle 0x002F, DMI type 4, 48 bytes
Processor Information
        Socket Designation: CPU02
        Type: Central Processor
        Family: ARM
        Manufacturer: HiSilicon
        ID: 10 D0 1F 48 00 00 00 00
        Signature: Implementor 0x48, Variant 0x1, Architecture 15, Part 0xd01, Revision 0
        Version: Kunpeng 920-4826
        Voltage: 0.9 V
```
```txt
processor       : 95
BogoMIPS        : 200.00
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm jscvt fcma dcpop asimddp asimdfhm
CPU implementer : 0x48
CPU architecture: 8
CPU variant     : 0x1
CPU part        : 0xd01
CPU revision    : 0
```

在另外一个机器上:
```txt
Handle 0x002F, DMI type 4, 48 bytes
Processor Information
        Socket Designation: CPU02
        Type: Central Processor
        Family: ARM
        Manufacturer: HiSilicon
        ID: 10 D0 1F 48 00 00 00 00
        Signature: Implementor 0x48, Variant 0x1, Architecture 15, Part 0xd01, Revision 0
        Version: HUAWEI Kunpeng 920 5250
        Voltage: 0.9 V
```
```txt
processor       : 95
BogoMIPS        : 200.00
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm jscvt fcma dcpop asimddp asimdfhm
CPU implementer : 0x48
CPU architecture: 8
CPU variant     : 0x1
CPU part        : 0xd01
CPU revision    : 0
```
可以发现两个机器的 /proc/cpuinfo 完全相同，但是第二个机器的 Version 是 5250

### 用来汇报 Guest 的 CPU topo
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

### 报告 memory 的状态

例如可以获取到

 项目                   当前配置
━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 CPU                    Intel Core i9-13900K
─────────────────────  ───────────────────────────────────
 主板                   ASUS TUF GAMING B660-PLUS WIFI D4
─────────────────────  ───────────────────────────────────
 内存                   光威 DDR4，4 × 32 GiB
─────────────────────  ───────────────────────────────────
 每条数据宽度           64 bit
─────────────────────  ───────────────────────────────────
 每条 Rank 数           2
─────────────────────  ───────────────────────────────────
 BIOS 报告的运行速率    全部为 2400 MT/s
─────────────────────  ───────────────────────────────────
 通道分布               两个通道各 64 GiB，每通道两条

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
