# qboot

## 问题
- [ ] 据说，PCI 会将所有的设备需要映射的空间告诉操作系统，是这样的吗 ?

## 分析
- [x] 忽然意识到 : 难道 bios 和 主板不是一个东西啊
  - 是的啊, BIOS 根据主板提供的接口进行初始化


- [x] 所以，qemu 的那些代码 int 代码，依赖于 bios 的中断向量的东西，到底是怎么处理的
  - 参考 : qboot/entry.S

- [x] 这种显示字符的操作，这些地址的规定到底是怎么回事 ?
```c
static inline void int10_putchar(struct biosregs *args)
{
	uint8_t al = args->eax & 0xFF;

	outb(0x3f8, al);
}
```
靠 Intel Desktop Board D845EBG2 之类的手册了


分析一下 Qemu 的加载过程:
```plain
#0  x86_bios_rom_init (rom_memory=0x5555564456b0, isapc_ram_fw=false) at /home/maritns3/core/qemu/hw/i386/x86.c:657
#1  0x00005555558e4294 in pc_system_firmware_init (pcms=0x555556306e30, rom_memory=0x5555564456b0) at /home/maritns3/core/qemu/hw/i386/pc_sysfw.c:242
#2  0x00005555558dd98a in pc_memory_init (pcms=pcms@entry=0x555556306e30, system_memory=system_memory@entry=0x555556375800, rom_memory=rom_memory@entry=0x5555564456b0,
ram_memory=ram_memory@entry=0x7fffffffd4d0) at /home/maritns3/core/qemu/hw/i386/pc.c:1223
#3  0x00005555558e0c1b in pc_init1 (machine=0x555556306e30, pci_type=0x555555cce08c "i440FX", host_type=0x555555ccd15b "i440FX-pcihost") at /home/maritns3/core/qemu/hw/
i386/pc_piix.c:184
#4  0x00005555559f9263 in machine_run_board_init (machine=0x555556306e30) at /home/maritns3/core/qemu/hw/core/machine.c:1143
#5  0x00005555557f7c88 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/qemu/vl.c:4348
```

machine_run_board_init

```c
struct ObjectClass // 类似于 MachineClass 的 base class
struct Object // 所有对象的 base class
```

```c
    MachineClass *machine_class = MACHINE_GET_CLASS(machine);
```
将 machine 转化为 Object 类型，Object 类型中间持有了其所属的 Class 指针。

```c
DEFINE_I440FX_MACHINE(v4_2, "pc-i440fx-4.2", NULL,
                      pc_i440fx_4_2_machine_options);

static void pc_i440fx_4_2_machine_options(MachineClass *m)
{
    PCMachineClass *pcmc = PC_MACHINE_CLASS(m);
    pc_i440fx_machine_options(m);
    m->alias = "pc";
    m->is_default = 1; // 这个字段导致默认选择的机器是这个
    pcmc->default_cpu_version = 1;
}

// lv.c:main

    machine_class = select_machine();
    current_machine = MACHINE(object_new(object_class_get_name(
                          OBJECT_CLASS(machine_class))));

```

machine_class 感觉已经持有了足够多的信息了, 保持初始化的函数之类的:


- qboot/cstart.S 是入口，然后跳转到, 这个位置是 reset vector 设置的
  - main.c:main

## PCI 和主板
让我们看看 PCI
```c
/*
 * PCI Configuration Mechanism #1 I/O ports. See Section 3.7.4.1.
 * ("Configuration Mechanism #1") of the PCI Local Bus Specification 2.1 for
 * details.
 */
#define PCI_CONFIG_ADDRESS	0xcf8
#define PCI_CONFIG_DATA		0xcfc
#define PCI_CONFIG_BUS_FORWARD	0xcfa
#define PCI_IO_SIZE		0x100
#define PCI_IOPORT_START	0x6200
#define PCI_CFG_SIZE		(1ULL << 24)
```
在狮子书的设备模拟上，配置空间都是需要利用 PCI_CONFIG_ADDRESS 和 PCI_CONFIG_DATA 访问，而
BAR 空间配置之后，对于 BAR 空间可以直接内容直接访问。

## SMBIOS

最开始 fw_cfg 用于加载固件的

fw_cfg 使用 IO 端口 0x510 开始的若干干端口

其实是用于端口，规定数据组织形式, 用户可以创建

- [ ] 在虚拟机中 /sys/firmware/ 下居然可以看到 qemu 配置的 cfg
