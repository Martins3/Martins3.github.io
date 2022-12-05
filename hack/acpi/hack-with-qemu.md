# hack with qemu
- `-acpitable`
- [ ] https://gist.github.com/mcastelino/47c1fcec1b364d5e82d42e6f341eba78 : 调试的方法

info mtree
```
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr

    000000000000ae00-000000000000ae17 (prio 0, i/o): acpi-pci-hotplug
    000000000000af00-000000000000af0b (prio 0, i/o): acpi-cpu-hotplug
    000000000000afe0-000000000000afe3 (prio 0, i/o): acpi-gpe0
```

- 尝试使用 QEMU 去理解，acpi 到底会需要什么硬件支持

# How acpi works in Linux Kernel and Simulated in QEMU

## ACPI 基础教程
1. [Linux kernel doc](https://www.kernel.org/doc/html/latest/firmware-guide/acpi/index.html#)
2. [acpica : acpi introduction](https://acpica.org/sites/acpica/files/ACPI-Introduction.pdf) :star:
3. [ACPI Source Language (ASL) Tutorial](https://acpica.org/sites/acpica/files/asl_tutorial_v20190625.pdf)
4. [ACPI: Design Principles and Concerns](https://www.ssi.gouv.fr/uploads/IMG/pdf/article_acpi.pdf)
5. [Getting started with ACPI](https://dortania.github.io/Getting-Started-With-ACPI/#a-quick-explainer-on-acpi)

## 反汇编 acpi table
- /sys/firmware/acpi/tables 中用 iasl -i
- 如果在 Loongarch 上没有 isal ，可以 scp 到 x86 机器上拷贝

## 问题
- [ ] 需要动态构建 ACPI 表吗?


## acpi setup

#### FDC0 / MOU / KBD 的 ACPI 描述是如何添加进去的
```txt
#0  i8042_build_aml (isadev=0x5555566eb400, scope=0x55555699a990) at ../hw/input/pckbd.c:564
#1  0x000055555590c474 in isa_build_aml (bus=<optimized out>, scope=scope@entry=0x55555699a990) at ../hw/isa/isa-bus.c:214
#2  0x0000555555a6e08d in build_isa_devices_aml (table=table@entry=0x555556909540) at /home/maritns3/core/kvmqemu/include/hw/isa/isa.h:17
#3  0x0000555555a71281 in build_dsdt (machine=0x5555566c0400, pci_hole64=<synthetic pointer>, pci_hole=<synthetic pointer>, misc=<synthetic pointer>, pm=0x7fffffffd450, linker=0x5555568d6bc0, table_data=0x555556aae0a0) at ../hw/i386/acpi-build.c:1403
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

应该是，acpi 必须在运行时才可以构建好, 而且是通过 copy of table in RAM 来 patched
```c
typedef struct AcpiBuildState {
    /* Copy of table in RAM (for patching). */
    MemoryRegion *table_mr;
    /* Is table patched? */
    uint8_t patched;
    void *rsdp;
    MemoryRegion *rsdp_mr;
    MemoryRegion *linker_mr;
} AcpiBuildState;
```


## 从 NVDIMM 到 Bios Linker
https://richardweiyang-2.gitbook.io/understanding_qemu/00-qmeu_bios_guest/03-seabios

https://richardweiyang-2.gitbook.io/understanding_qemu/00-devices/00-an_example/05-nvdimm
> 似乎，连 acpi 的函数和构建地址空间

从 romfile_loader_execute 看，etc/table-loader 中就是装载各种 table 的东西

etc/table-loader

- build_rsdt : 指向其他的 table 的，之所以需要 linker，好像是因为将 table 放到哪里，只是知道相对偏移，而不知道绝对偏移，
所以需要 linker 将绝对值计算出来。

- checksum 需要让 guest 计算的原因:
  - 因为 checksum 中间包含了 linker 正确计算出来的指针，只有被修正之后的指针才能计算出来正确的 checksum

DSDT address to be filled by Guest linker at runtime

- [x] 为什么 microvm 的 table 就不会动态修改? (猜测是一些东西写死了吧, 不需要 linker 吧)

除了 TMPLOG ，其余的三个都是和 acpi_build_update 关联起来的:
```c
#define ACPI_BUILD_TABLE_FILE "etc/acpi/tables"
#define ACPI_BUILD_RSDP_FILE "etc/acpi/rsdp"
#define ACPI_BUILD_TPMLOG_FILE "etc/tpm/log"
#define ACPI_BUILD_LOADER_FILE "etc/table-loader"
```

- bios_linker_loader_alloc : ask guest to load file into guest memory.
  - romfile_loader_allocate 实际上加载的两个文件为 etc/acpi/rsdp 和 etc/acpi/tables
  - 应该是首先传递进去的是  etc/table-loader, 然后靠这个将 etc/acpi/rsdp 和 etc/acpi/tables 传递进去


# la 中如何处理 acpi
- [ ] 似乎现在 QEMU 中都是支持的了，还是可以看看的

感觉 ramooflax 的处理是相当的简单，其实他甚至可以在 acpi 解析之前就可以来初始化 e1000

ramooflax 不需要模拟 acpi 设备空间，因为设备本身就是相同的

- [ ] 是不是， acpi 探测出来了 pcie，那么剩下的事情应该 pcie 自己做
- [ ] 探测，到底指的是什么
```txt
[    0.000000] efi:  ACPI 2.0=0xfd2f4000  SMBIOS=0x900000000fffe000  SMBIOS 3.0=0x90000000fd132000
[    0.000000] ACPI: Early table checksum verification disabled
[    0.000000] ACPI: RSDP 0x00000000FD2F4000 000024 (v02 LOONGS)
[    0.000000] ACPI: XSDT 0x00000000FD2F3000 000044 (v01 LOONGS TP-R00   00000004      01000013)
[    0.000000] ACPI: FACP 0x00000000FD2F0000 0000F4 (v03 LOONGS TP-R00   00000004 PTEC 20160527)
[    0.000000] ACPI: DSDT 0x00000000FD2EE000 001A9C (v02 LGSON  TP-R00   00000476 INTL 20160527)
[    0.000000] ACPI: FACS 0x00000000FD2F1000 000040
[    0.000000] ACPI: APIC 0x00000000FD2F2000 00005E (v01 LOONGS LOONGSON 00000001 LIUX 00000000)
[    0.000000] ACPI: SRAT 0x00000000FD2ED000 0000C0 (v02 LOONGS LOONGSON 00000002 LIUX 01000013)
[    0.000000] ACPI: SLIT 0x00000000FD2EC000 00002D (v01 LOONGS LOONGSON 00000002 LIUX 01000013)
```

```txt
[    0.888502] [<900000000020864c>] show_stack+0x2c/0x100
[    0.888504] [<9000000000ec3988>] dump_stack+0x90/0xc0
[    0.888507] [<90000000009a605c>] really_probe+0x20c/0x2b8
[    0.888509] [<90000000009a62b4>] driver_probe_device+0x64/0x100
[    0.888511] [<90000000009a3e18>] bus_for_each_drv+0x68/0xa8
[    0.888513] [<90000000009a5dcc>] __device_attach+0x124/0x1a0
[    0.888515] [<90000000009a504c>] bus_probe_device+0x9c/0xc0
[    0.888517] [<90000000009a14b0>] device_add+0x350/0x608
[    0.888519] [<90000000009a7fc0>] platform_device_add+0x128/0x288
[    0.888521] [<90000000009a8dd0>] platform_device_register_full+0xb8/0x130
[    0.888523] [<90000000008a259c>] acpi_create_platform_device+0x28c/0x300
[    0.888525] [<9000000000898368>] acpi_default_enumeration+0x28/0x58
[    0.888526] [<9000000000898598>] acpi_bus_attach+0x1d0/0x210
[    0.888528] [<9000000000898420>] acpi_bus_attach+0x58/0x210
[    0.888530] [<9000000000898420>] acpi_bus_attach+0x58/0x210
[    0.888532] [<900000000089a56c>] acpi_bus_scan+0x34/0x78
[    0.888534] [<90000000012f8060>] acpi_scan_init+0x170/0x2b8
[    0.888536] [<90000000012f7c68>] acpi_init+0x2e4/0x338
[    0.888537] [<90000000002004fc>] do_one_initcall+0x6c/0x170
[    0.888540] [<90000000012d4ce0>] kernel_init_freeable+0x1f8/0x2b8
[    0.888542] [<9000000000eda794>] kernel_init+0x10/0xf4
[    0.888544] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
```
- [ ] 那么 e1000 需要加入到 acpi 的空间中吗 ?


- `acpi_init`
  - [ ] `init_acpi_device_notify`
  - `acpi_bus_init`
    - `acpi_bus_init_irq`
    - `bus_register(&acpi_bus_type);`
  - [ ] `pci_mmcfg_late_init` : 龙芯自定义的一些的东西
  - `acpi_iort_init`
  - `acpi_scan_init`
    - `acpi_bus_scan` : 进一步调用 `acpi_bus_check_add` 来探测设备，将探测到的设备初始化为 `acpi_device`
      - `device_attach`

  - [ ] `acpi_bus_init`
  - [ ] `acpi_ec_init`


# acpica 的移植

1. 代码拷贝
```sh
git clone ...
cd generate/linux
./gen-repo.sh
```

2. 修改头文件
3. 修改 Makefile

我找到了好几个 mini kernel 的存在，目前来说，几乎没有人会
将整个 acpica 包含进去，我们应该好好调查一下，真的有必要吗?


[^1]: https://stackoverflow.com/questions/60137506/qemu-support-for-acpi-2-0
