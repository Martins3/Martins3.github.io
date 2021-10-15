# QEMU 执行的 guest 第一行代码

## seabios 一侧
> On emulators, this phase starts when the CPU starts execution in 16bit
> mode at 0xFFFF0000:FFF0. The emulators map the SeaBIOS binary to this
> address, and SeaBIOS arranges for romlayout.S:reset_vector() to be
> present there. This code calls romlayout.S:entry_post() which then
> calls post.c:handle_post() in 32bit mode.

以上是 seabios 的文档，意思很简单: reset_vector => entry_post => handle_post

从 seabios 的源码中也可以验证:
```asm
        // Reset stack, transition to 32bit mode, and call a C function.
        .macro ENTRY_INTO32 cfunc
        xorw %dx, %dx
        movw %dx, %ss
        movl $ BUILD_STACK_ADDR , %esp
        movl $ \cfunc , %edx
        jmp transition32
        .endm
```

```asm
entry_post:
        cmpl $0, %cs:HaveRunPost                // Check for resume/reboot
        jnz entry_resume
        ENTRY_INTO32 _cfunc32flat_handle_post   // Normal entry point

        ORG 0xe2c3
```

```asm
reset_vector:
        ljmpw $SEG_BIOS, $entry_post

        // 0xfff5 - BiosDate in misc.c

        // 0xfffe - BiosModelId in misc.c

        // 0xffff - BiosChecksum in misc.c

        .end
```
从上面的代码还可以知道，stack 的顶是 0x7000
```c
#define BUILD_STACK_ADDR          0x7000
```


在 seabios 添加一个调试语句
```c
dprintf(1, "%p\n", VSYMBOL(entry_post));
```
可以很容易得到 entry_post 的地址为: `0x000fe05b`

也就是 seabios 执行的第一行代码就是从 0xfffffff0 跳转到 0x000fe05b 上

## QEMU 一侧
主要发生在 loader.c 中

### 构建 pc.bios 地址空间
在 x86_bios_rom_init 中初始化，这个没有什么好分析的:

```c
void x86_bios_rom_init(MemoryRegion *rom_memory, bool isapc_ram_fw)
{
    bios = g_malloc(sizeof(*bios));
    memory_region_init_ram(bios, NULL, "pc.bios", bios_size, &error_fatal);
    if (!isapc_ram_fw) {
        memory_region_set_readonly(bios, true);
    }
    ret = rom_add_file_fixed(bios_name, (uint32_t)(-bios_size), -1);
```

### 关联 Rom image
创建 MemoryRegion pc.bios 但是如何将 Rom image 关联上去值得分析:


## loader
- isa-bios 是 pc.bios 的后 128k 的部分，而且放到 1M 的后面的空间中

总体来说，loader 在处理 elf, ramdisk 和 rom 的事情，但是暂时需要的并不多。

## struct Rom

```c
int rom_add_option(const char *file, int32_t bootindex)
{
    return rom_add_file(file, "genroms", 0, bootindex, true, NULL, NULL);
}
```
实际上，变成了只有 rom_add_option 位置。

在 pc_memory_init 中:
```c
    for (i = 0; i < nb_option_roms; i++) {
        rom_add_option(option_rom[i].name, option_rom[i].bootindex);
    }
```
其中 nb_option_roms 和 option_rom 是两个全局变量

**分析其作用**

体现在 rom_add_file 中, 会调用 fw_cfg_add_file, 将文件添加进去


## rom_reset 加载到内存中

为什么可以将 pc.bios 和文件联系起来的 ?
在 x86_bios_rom_init 中会调用 rom_add_file_fixed 设置 bios 的内容在 4G-256k 的位置，
从常规的思考，应该是在创建 MemoryRegion bios 的基础上，让 bios 持有的 RamBlock 最后指向
mmap 的这个文件，但是实际上，并不是，而是简单的通过 rom_add_file_fixed 就可以做到。
```c
  rom_add_file_fixed(bios_name, (uint32_t)(-bios_size), -1)
```

- x86_bios_rom_init 只是负责将文件添加进去，而 rom_reset 负责将内容拷贝过去

调用路径:
- address_space_write_rom
  - address_space_write_rom_internal
    - address_space_translate : 找到对应的 MemoryRegion 也就是 MemoryRegion mr 了
    - memcpy

下面几个内容应该是 rom 直接加载
```c
huxueshi:rom_reset /home/maritns3/core/seabios/out/bios.bin
huxueshi:rom_reset etc/acpi/tables
huxueshi:rom_reset etc/table-loader
huxueshi:rom_reset etc/acpi/rsdp
```

```c
huxueshi:rom_insert /home/maritns3/core/seabios/out/bios.bin
huxueshi:rom_insert kvmvapic.bin
huxueshi:rom_insert linuxboot_dma.bin
huxueshi:rom_insert etc/acpi/tables
huxueshi:rom_insert etc/table-loader
huxueshi:rom_insert etc/acpi/rsdp
```

- insert 进去的是 6 个，但是 rom_reset 只有 4 个，因为那些 `rom->fw_file != NULL` 使用 fw_file 加载就好了

实际上，这些数据都是通过 fw_cfg 传递的
```c
huxueshi:fw_cfg_add_file_callback etc/e820
huxueshi:fw_cfg_add_file_callback genroms/kvmvapic.bin
huxueshi:fw_cfg_add_file_callback genroms/linuxboot_dma.bin
huxueshi:fw_cfg_add_file_callback etc/system-states
huxueshi:fw_cfg_add_file_callback etc/acpi/tables
huxueshi:fw_cfg_add_file_callback etc/table-loader
huxueshi:fw_cfg_add_file_callback etc/tpm/log
huxueshi:fw_cfg_add_file_callback etc/acpi/rsdp
huxueshi:fw_cfg_add_file_callback etc/smbios/smbios-tables
huxueshi:fw_cfg_add_file_callback etc/smbios/smbios-anchor
huxueshi:fw_cfg_add_file_callback bootorder
huxueshi:fw_cfg_add_file_callback bios-geometry
```

为什么就是下面三个是关联的 memory_region
```c
huxueshi:rom_insert etc/acpi/tables
huxueshi:rom_insert etc/table-loader
huxueshi:rom_insert etc/acpi/rsdp
```

具体原因可以参考:
90921644ff0d58e6e165cc439321328e5d771256
```diff
tree 90921644ff0d58e6e165cc439321328e5d771256
parent 0851c9f75ccb0baf28f5bf901b9ffe3c91fcf969
author Michael S. Tsirkin <mst@redhat.com> Mon Aug 19 17:26:55 2013 +0300
committer Michael S. Tsirkin <mst@redhat.com> Wed Aug 21 00:18:39 2013 +0300

loader: store FW CFG ROM files in RAM

ROM files that are put in FW CFG are copied to guest ram, by BIOS, but
they are not backed by RAM so they don't get migrated.

Each time we change two bytes in such a ROM this breaks cross-version
migration: since we can migrate after BIOS has read the first byte but
before it has read the second one, getting an inconsistent state.

Future-proof this by creating, for each such ROM,
an MR serving as the backing store.
This MR is never mapped into guest memory, but it's registered
as RAM so it's migrated with the guest.

Naturally, this only helps for -M 1.7 and up, older machine types
will still have the cross-version migration bug.
Luckily the race window for the problem to trigger is very small,
which is also likely why we didn't notice the cross-version
migration bug in testing yet.

Signed-off-by: Michael S. Tsirkin <mst@redhat.com>
Reviewed-by: Laszlo Ersek <lersek@redhat.com>
```

让 rom 和 mr 关联的原因: 因为 bios 无法自动同步，所以使用 MemoryRegion 保存 bios 从而可以自动 migration
解决方法:
1. 创建 rom_set_mr : 将 rom 关联一个 mr, 并且将 rom 中的数据拷贝到 mr 的空间中
2. 修改 rom_add_file  : fw_cfg 提供数据给 guest 注册的时候只是需要一个指针，如果配置了 option_rom_has_mr 的话，那么这个指针来自于 memory_region_get_ram_ptr

## rom_insert 的调用者
- rom_add_blob
- rom_add_file
- rom_add_elf_program : 暂时没有使用

- rom_add_file 和 rom_add_blob 的区别
    - rom_add_file 提供的是文件名，其中需要打开文件，将文件内容放到其中
    - rom_add_blob 的参数冲 blob 需要拷贝过去即可
    - 其实，从三者调用者这个就已经很清楚了

#### rom_add_file
1. 将文件中内容拷贝到 Rom::data

似乎只是看到下面三个调用者
```c
huxueshi:rom_add_file /home/maritns3/core/seabios/out/bios.bin
huxueshi:rom_add_file /home/maritns3/core/kvmqemu/build/pc-bios/kvmvapic.bin
huxueshi:rom_add_file /home/maritns3/core/kvmqemu/build/pc-bios/linuxboot_dma.bin
```

观看一个最经典的:
```c
/*
#0  rom_add_file (file=file@entry=0x555556fe3010 "/home/maritns3/core/seabios/out/bios.bin", fw_dir=fw_dir@entry=0x0, addr=addr@entry=4294705152, bootindex=bootindex@entry=-1, option_rom=option_rom@entry=false, mr=mr@entry=0x0, as=0x0) at ../hw/core/loader.c:944
#1  0x0000555555b93560 in x86_bios_rom_init (ms=<optimized out>, default_firmware=<optimized out>, rom_memory=0x555556a132d0, isapc_ram_fw=<optimized out>) at ../hw/i386/x86.c:1110
#2  0x0000555555b9b988 in pc_system_firmware_init (pcms=0x5555568a65e0, rom_memory=0x555556a132d0) at /home/maritns3/core/kvmqemu/include/hw/boards.h:24
#3  0x0000555555b8baa0 in pc_memory_init (pcms=pcms@entry=0x5555568a65e0, system_memory=system_memory@entry=0x5555566a9460, rom_memory=rom_memory@entry=0x555556a132d0,ram_memory=ram_memory@entry=0x7fffffffd290) at ../hw/i386/pc.c:945
#4  0x0000555555b9e281 in pc_init1 (machine=0x5555568a65e0, pci_type=0x555555f08869 "i440FX", host_type=0x555555f3773c "i440FX-pcihost") at ../hw/i386/pc_piix.c:185
#5  0x0000555555a9b934 in machine_run_board_init (machine=0x5555568a65e0) at ../hw/core/machine.c:1272
#6  0x0000555555d09ef4 in qemu_init_board () at ../softmmu/vl.c:2618
#7  qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2692
#8  0x0000555555d0d6b0 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3714
#9  0x0000555555940c8d in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```

#### rom_add_blob
下面三个通过 rom_add_blob 来添加的, 而且其调用者都是 acpi_add_rom_blob
1. etc/acpi/tables
2. etc/table-loader
3. etc/acpi/rsdp

- 创建的 mr 实际上根本没有挂载到任何 mr 其作用更像是分配空间而已, seabios 读去空间还是靠 fw_cfg 的

在下面三个位置都会进行从 Rom::data 到 mr 的空间的拷贝，但是都没啥意, 因为在 acpi_build_update 中进行 memcpy 的
- rom_set_mr
- rom_reset
- acpi_ram_update

```c
static void acpi_ram_update(MemoryRegion *mr, GArray *data)
{
    uint32_t size = acpi_data_len(data);

    /* Make sure RAM size is correct - in case it got changed e.g. by migration */
    memory_region_ram_resize(mr, size, &error_abort);

    memcpy(memory_region_get_ram_ptr(mr), data->data, size);
    memory_region_set_dirty(mr, 0, size);
}
```


```c
/*
#0  rom_add_blob (name=name@entry=0x555555eeba65 "etc/acpi/tables", blob=0x555557de3890, len=131072, max_len=max_len@entry=2097152, addr=addr@entry=18446744073709551615, fw_file_name=fw_file_name@entry=0x555555eeba65 "etc/acpi/tables", fw_callback=0x555555bb2300 <acpi_build_update>, callback_opaque=0x555556b4ef90, as=0x0, read_only=true) at ../hw/core/loader.c:1044
#1  0x0000555555998607 in acpi_add_rom_blob (update=update@entry=0x555555bb2300 <acpi_build_update>, opaque=opaque@entry=0x555556b4ef90, blob=0x555556acc030, name=<optimized out>, name@entry=0x555555eeba65 "etc/acpi/tables") at ../hw/acpi/utils.c:46
#2  0x0000555555bb2568 in acpi_setup () at ../hw/i386/acpi-build.c:2733
*/

/*
#0  rom_add_blob (name=name@entry=0x555555eeba75 "etc/table-loader", blob=0x555556ae8950, len=4096, max_len=max_len@entry=65536, addr=addr@entry=18446744073709551615, f
w_file_name=fw_file_name@entry=0x555555eeba75 "etc/table-loader", fw_callback=0x555555bb2300 <acpi_build_update>, callback_opaque=0x555556b4ef90, as=0x0, read_only=true
) at ../hw/core/loader.c:1044
#1  0x0000555555998607 in acpi_add_rom_blob (update=update@entry=0x555555bb2300 <acpi_build_update>, opaque=opaque@entry=0x555556b4ef90, blob=0x555556acc0f0, name=<opti
mized out>, name@entry=0x555555eeba75 "etc/table-loader") at ../hw/acpi/utils.c:46
#2  0x0000555555bb2593 in acpi_setup () at ../hw/i386/acpi-build.c:2738
*/

/*
#0  rom_add_blob (name=name@entry=0x555555eeba86 "etc/acpi/rsdp", blob=0x555556eb87d0, len=20, max_len=max_len@entry=4096, addr=addr@entry=18446744073709551615, fw_file_name=fw_file_name@entry=0x555555eeba86 "etc/acpi/rsdp", fw_callback=0x555555bb2300 <acpi_build_update>, callback_opaque=0x555556b4ef90, as=0x0, read_only=true) at ../hw/core/loader.c:1044
#1  0x0000555555998607 in acpi_add_rom_blob (update=update@entry=0x555555bb2300 <acpi_build_update>, opaque=opaque@entry=0x555556b4ef90, blob=0x555556acc000, name=<optimized out>, name@entry=0x555555eeba86 "etc/acpi/rsdp") at ../hw/acpi/utils.c:46
#2  0x0000555555bb273f in acpi_setup () at ../hw/i386/acpi-build.c:2779
*/
```


## boot device
关联文件 softmmu/bootdevice.c

在 seabios 的 loadBootOrder 中需要读读去 fw_cfg 的 bootorder,
seabios 的 boot order 是受到 fw_cfg 制作的 bootorder 控制的, 此处就是在制作 bootorder

```c
typedef struct FWBootEntry FWBootEntry;

static QTAILQ_HEAD(, FWBootEntry) fw_boot_order =
    QTAILQ_HEAD_INITIALIZER(fw_boot_order);
```
实际上，对于 add_boot_device_path 只有 linuxboot_dma.bin 有意义
huxueshi:add_boot_device_path bootindex=0 dev=(nil) suffix=/rom@genroms/linuxboot_dma.bin

应该是为了向 fw_cfg 提供: get_boot_devices_list

- fw_cfg_machine_reset
  - get_boot_devices_list : 返回内容 /rom@genroms/linuxboot_dma.bin
  - `ptr = fw_cfg_modify_file(s, "bootorder", (uint8_t *)buf, len);` : 提供给 seabios 使用
  - get_boot_devices_lchs_list
  - [x] `ptr = fw_cfg_modify_file(s, "bios-geometry", (uint8_t *)buf, len);` : 如果其中的内容是空的，fw_cfg 如何处理的
      - seabios 的 loadBiosGeometry 中，当调用 romfile_loadfile 可以获取一个空, 具体 fw_cfg 的细节再说吧

结论，传递给 seabios 的
| bootorder                      | bios-geometry |
|--------------------------------|---------------|
| /rom@genroms/linuxboot_dma.bin | -             |

#### lchs
lchs : logical cylinder head sector[^1]

```c
static QTAILQ_HEAD(, FWLCHSEntry) fw_lchs =
    QTAILQ_HEAD_INITIALIZER(fw_lchs);
```

- add_boot_device_lchs : 的调用者存在 scsi 和 virtio-blk 暂时不用管理这个吧

## 问题
- 虽然从代码中可以知道 pc Machine 的 0 ~ 1M 的物理地址空间内的各种布局，但是其规定在哪里，并不知道是来自于哪一个手册?
- 为什么需要将 pc.bios 映射到 4G pci 空间的最上方，这又是哪里的规定?

[^1]: https://en.wikipedia.org/wiki/Cylinder-head-sector
