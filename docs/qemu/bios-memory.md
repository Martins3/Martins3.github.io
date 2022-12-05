# QEMU 中的 seabios : 地址空间
<!-- vim-markdown-toc GitLab -->

* [pc.bios](#pcbios)
  * [map pc.bios to guest](#map-pcbios-to-guest)
  * [pc.bios is mapped to two location](#pcbios-is-mapped-to-two-location)
  * [first instructions executed in seabios](#first-instructions-executed-in-seabios)
* [pc.rom](#pcrom)
* [PAM](#pam)
  * [QEMU 侧如何处理 PAM](#qemu-侧如何处理-pam)
  * [Seabios 侧如何处理 PAM](#seabios-侧如何处理-pam)
* [SMM](#smm)
  * [SMM 地址空间的构建](#smm-地址空间的构建)
  * [seabios 如何使用 SMM](#seabios-如何使用-smm)
  * [QEMU 如何响应](#qemu-如何响应)
  * [SMM 的使用](#smm-的使用)
* [问题](#问题)

<!-- vim-markdown-toc -->

seabios 的基础知识可以参考李强的《QEMU/KVM 源码解析与应用》, 下面来分析一下和地址空间相关的几个小问题。

## pc.bios
QEMU 支持很多种类的 bios, seabios 只是其中的一种, bios 加载地址空间中，该 MemoryRegion 的名称为 pc.bios
### map pc.bios to guest

在 x86_bios_rom_init 中会调用 rom_add_file_fixed 将 256k 大小的 bios 正好映射到 4G - 256k 的地址上
同时创建了 MemoryRegion pc.bios，但是两者并没有没有关联起来。实际上等待两者关联起来需要等到整个 QEMU 初始化结束之后
调用 rom_reset 的时候

```c
  memory_region_init_ram(bios, NULL, "pc.bios", bios_size, &error_fatal);

  rom_add_file_fixed(bios_name, (uint32_t)(-bios_size), -1)
```

其实真正将两者关联起来的位置在 rom_reset:

- rom_reset
  - address_space_write_rom
    - address_space_write_rom_internal
      - address_space_translate : 通过 4G - 256k 地址查询到 pc.bios 这个 MemoryRegion，然后将 Rom::data 的数据拷贝到 MemoryRegion::RamBlock::host
      - memcpy

### pc.bios is mapped to two location
实际上，pc.bios 的后 128k 同时被映射到了 0xe0000 ~ 0xfffff 的位置上

seabios 的 src/fw/shadow.c 中存在有一个注释:

x86_bios_rom_init 中通过创建 isa-bios 的 alias 的实现的。

从地址中看，这确实:
```txt
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
```

```c
// On the emulators, the bios at 0xf0000 is also at 0xffff0000
#define BIOS_SRC_OFFSET 0xfff00000
```

之所以需要将 pc.bios 映射两次下面再分析。
### first instructions executed in seabios
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

在 transition32 中，将
```c
// Place CPU into 32bit mode from 16bit mode.
// %edx = return location (in 32bit mode)
// Clobbers: ecx, flags, segment registers, cr0, idt/gdt
        DECLFUNC transition32
        .global transition32_nmi_off
transition32:
        // Disable irqs (and clear direction flag)
        cli
        cld

        // Disable nmi
        movl %eax, %ecx
        movl $CMOS_RESET_CODE|NMI_DISABLE_BIT, %eax
        outb %al, $PORT_CMOS_INDEX
        inb $PORT_CMOS_DATA, %al

        // enable a20
        inb $PORT_A20, %al
        orb $A20_ENABLE_BIT, %al
        outb %al, $PORT_A20
        movl %ecx, %eax

transition32_nmi_off:
        // Set segment descriptors
        lidtw %cs:pmode_IDT_info
        lgdtw %cs:rombios32_gdt_48

        // Enable protected mode
        movl %cr0, %ecx
        andl $~(CR0_PG|CR0_CD|CR0_NW), %ecx
        orl $CR0_PE, %ecx
        movl %ecx, %cr0

        // start 32bit protected mode code
        ljmpl $SEG32_MODE32_CS, $(BUILD_BIOS_ADDR + 1f)

        .code32
        // init data segments
1:      movl $SEG32_MODE32_DS, %ecx
        movw %cx, %ds
        movw %cx, %es
        movw %cx, %ss
        movw %cx, %fs
        movw %cx, %gs

        jmpl *%edx
        .code16
```


## pc.rom
在 pc_memory_init 中初始化:
```c
    option_rom_mr = g_malloc(sizeof(*option_rom_mr));
    memory_region_init_ram(option_rom_mr, NULL, "pc.rom", PC_ROM_SIZE,
                           &error_fatal);
    if (pcmc->pci_enabled) {
        memory_region_set_readonly(option_rom_mr, true);
    }
    memory_region_add_subregion_overlap(rom_memory,
                                        PC_ROM_MIN_VGA,
                                        option_rom_mr,
                                        1);
```

实际上，当 pci enable 的时候，pc.rom 被设置为 readonly 了, 将 option_rom_mr 相关的代码都删除，还是可以正常启动

## PAM
PAM 的作用可以将对于 bios 空间读写转发到 PCI 或者 RAM 中，因为读写 ROM 比较慢, 进一步可以参考:
- https://patchwork.ozlabs.org/project/qemu-devel/patch/1437389593-15297-1-git-send-email-real@ispras.ru/
- https://wiki.qemu.org/Documentation/Platforms/PC

### QEMU 侧如何处理 PAM
在 i440fx_init 初始化的时候, 来初始化所有 `PAMMemoryRegion`, 一共 13 个
- 第一个映射: System BIOS Area Memory Segments, 也即是映射 0xf0000 到 0xfffff
- 后面的 12 个映射 0xc0000 ~ 0xeffff, 每一个映射 0x4000 的大小

```c
/*
 * SMRAM memory area and PAM memory area in Legacy address range for PC.
 * PAM: Programmable Attribute Map registers
 *
 * 0xa0000 - 0xbffff compatible SMRAM
 *
 * 0xc0000 - 0xc3fff Expansion area memory segments
 * 0xc4000 - 0xc7fff
 * 0xc8000 - 0xcbfff
 * 0xcc000 - 0xcffff
 * 0xd0000 - 0xd3fff
 * 0xd4000 - 0xd7fff
 * 0xd8000 - 0xdbfff
 * 0xdc000 - 0xdffff
 * 0xe0000 - 0xe3fff Extended System BIOS Area Memory Segments
 * 0xe4000 - 0xe7fff
 * 0xe8000 - 0xebfff
 * 0xec000 - 0xeffff
 *
 * 0xf0000 - 0xfffff System BIOS Area Memory Segments
 */
```

```c
typedef struct PAMMemoryRegion {
    MemoryRegion alias[4];  /* index = PAM value */
    unsigned current;
} PAMMemoryRegion;
```
- alias : 每一个 PAM 存在四种映射属性, 所以创建出来四个 alias, 分别为:
  - 0 : read / write 到 PCI
  - 1 : read 转发到 PCI , write 到 ROM
  - 2 : 和 1 相反操作
  - 3 : read / write 到 RAM

当然，QEMU 目前的实现和这个存在一点点差异，参考 `init_pam`

当想要修改 PAM 寄存器的属性，最后会调用下面的函数，将原来的映射 disable 掉，启用新的映射
```c
void pam_update(PAMMemoryRegion *pam, int idx, uint8_t val)
{
    assert(0 <= idx && idx < PAM_REGIONS_COUNT);

    memory_region_set_enabled(&pam->alias[pam->current], false);
    pam->current = (val >> ((!(idx & 1)) * 4)) & PAM_ATTR_MASK;
    memory_region_set_enabled(&pam->alias[pam->current], true);
}
```

- 在 PAM 完全没有打开的时候，映射的 PCI
- 然后映射为 RAM
- 最后映射为 ROM

### Seabios 侧如何处理 PAM
guest 通过 PCI bridge i440fx 的 `I440FX_PAM` 的位置来实现更新 PAM 的。

具体代码在 `__make_bios_writable_intel` 中，因为修改了映射之后 `isa-bios` 就不见了，
取代的是没有内容的 RAM 的，这个操作是:
1. 在 `make_bios_writable_intel` : 会跳转的时候会加上 `#define BIOS_SRC_OFFSET 0xfff00000` 的偏移，而 [pc.bios](#pcbios) 中已经提到过，开始的时候会映射两份出去, 一份就是加上 BIOS_SRC_OFFSET 偏移的。
2. 在 `__make_bios_writable_intel` 中，会将代码拷贝一份回去。

```c
// Enable shadowing and copy bios.
static void
__make_bios_writable_intel(u16 bdf, u32 pam0)
{
    // Read in current PAM settings from pci config space
    union pamdata_u pamdata;
    pamdata.data32[0] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4));
    pamdata.data32[1] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4) + 4);
    u8 *pam = &pamdata.data8[pam0 & 0x03];

    // Make ram from 0xc0000-0xf0000 writable
    int i;
    for (i=0; i<6; i++)
        pam[i + 1] = 0x33;

    // Make ram from 0xf0000-0x100000 writable
    int ram_present = pam[0] & 0x10;
    pam[0] = 0x30;

    // Write PAM settings back to pci config space
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4), pamdata.data32[0]);
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4) + 4, pamdata.data32[1]);

    if (!ram_present)
        // Copy bios.
        memcpy(VSYMBOL(code32flat_start)
               , VSYMBOL(code32flat_start) + BIOS_SRC_OFFSET
               , SYMBOL(code32flat_end) - SYMBOL(code32flat_start));
}

static void
make_bios_writable_intel(u16 bdf, u32 pam0)
{
    int reg = pci_config_readb(bdf, pam0);
    if (!(reg & 0x10)) {
        // QEMU doesn't fully implement the piix shadow capabilities -
        // if ram isn't backing the bios segment when shadowing is
        // disabled, the code itself won't be in memory.  So, run the
        // code from the high-memory flash location.
        u32 pos = (u32)__make_bios_writable_intel + BIOS_SRC_OFFSET;
        void (*func)(u16 bdf, u32 pam0) = (void*)pos;
        func(bdf, pam0);
        return;
    }
    // Ram already present - just enable writes
    __make_bios_writable_intel(bdf, pam0);
}
```
当 make bios writable 的时候，原来的 isa-bios 的位置(0xe0000)会被设置为 RAM，其实内容相当于被清空了
所以需要从 pc.ram(4G - 256k) 的位置拷贝过来

当 bios 结束之后，这些 PAM 的位置会再次设置上 `make_bios_readonly_intel`，但是 0xe4000 ~ 0xeffff 的部分会被豁免。

```c
static void
make_bios_readonly_intel(u16 bdf, u32 pam0)
{
    // Flush any pending writes before locking memory.
    wbinvd();

    // Read in current PAM settings from pci config space
    union pamdata_u pamdata;
    pamdata.data32[0] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4));
    pamdata.data32[1] = pci_config_readl(bdf, ALIGN_DOWN(pam0, 4) + 4);
    u8 *pam = &pamdata.data8[pam0 & 0x03];

    // Write protect roms from 0xc0000-0xf0000
    u32 romlast = BUILD_BIOS_ADDR, rommax = BUILD_BIOS_ADDR;
    if (CONFIG_WRITABLE_UPPERMEMORY)
        romlast = rom_get_last();
    if (CONFIG_MALLOC_UPPERMEMORY)
        rommax = rom_get_max();
    int i;
    for (i=0; i<6; i++) {
        u32 mem = BUILD_ROM_START + i * 32*1024;
        if (romlast < mem + 16*1024 || rommax < mem + 32*1024) {
            if (romlast >= mem && rommax >= mem + 16*1024)
                pam[i + 1] = 0x31;
            break;
        }
        pam[i + 1] = 0x11;
    }

    // Write protect 0xf0000-0x100000
    pam[0] = 0x10;

    // Write PAM settings back to pci config space
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4), pamdata.data32[0]);
    pci_config_writel(bdf, ALIGN_DOWN(pam0, 4) + 4, pamdata.data32[1]);
}
```

添加 kvmvapic 的影响，最后得到 flatview 就是在 0xc0000 ~ 0xfffff 中有两个 RAM 的区间了
```txt
    00000000000cb000-00000000000cdfff (prio 1000, i/o): alias kvmvapic-rom @pc.ram 00000000000cb000-00000000000cdfff
```

```txt
  00000000000c0000-00000000000cafff (prio 0, rom): pc.ram @00000000000c0000
  00000000000cb000-00000000000cdfff (prio 0, ram): pc.ram @00000000000cb000 // kvmvapic-rom
  00000000000ce000-00000000000e3fff (prio 0, rom): pc.ram @00000000000ce000
  00000000000e4000-00000000000effff (prio 0, ram): pc.ram @00000000000e4000 // 被豁免的
  00000000000f0000-00000000000fffff (prio 0, rom): pc.ram @00000000000f0000
```

总结一些，本来 0xc0000 ~ 0xfffff 的区间是映射到 PCI 上的，之后修改为 RAM 的，最后修改为 ROM 的。

## SMM
SMM 实际上是给 firmware 使用的, 其具体作用可以进一步参考 https://www.ssi.gouv.fr/uploads/IMG/pdf/Cansec_final.pdf

> The execution environment after entering SMM is in real address mode with paging disabled (CR0.PE = CR0.PG = 0). In this initial execution environment, the SMI handler
can address up to 4 GBytes of memory and can execute all I/O and system instructions. (Intel SDM vol 3 chapter 34)[^1]

在 x86_cpu_reset 中间将 CPUX86State::smbase 初始化 0x30000, 而 helper_rsm 会将这数值重置为 0xa0000

初始化数值是 0x30000 从 seabios 的 src/config.h 中 `#define BUILD_SMM_INIT_ADDR       0x30000` 可以得到验证。

### SMM 地址空间的构建
而在 i440fx_init 中，创建出来了 smram_region 和 smram
```plain
memory-region: smram
  0000000000000000-00000000ffffffff (prio 0, i/o): smram
    00000000000a0000-00000000000bffff (prio 0, ram): alias smram-low @pc.ram 00000000000a0000-00000000000bffff
```

```plain
address-space: cpu-memory-0
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-00000000bfffffff (prio 0, ram): alias ram-below-4g @pc.ram 0000000000000000-00000000bfffffff
    0000000000000000-ffffffffffffffff (prio -1, i/o): pci
      00000000000a0000-00000000000bffff (prio 1, i/o): vga-lowmem
      // ....
    00000000000a0000-00000000000bffff (prio 1, i/o): alias smram-region @pci 00000000000a0000-00000000000bffff
```

在 tcg_cpu_realizefn 和 tcg_cpu_machine_done 中构建 cpu-smm

```plain
address-space: cpu-smm-0
  0000000000000000-ffffffffffffffff (prio 0, i/o): memory
    0000000000000000-00000000ffffffff (prio 1, i/o): alias smram @smram 0000000000000000-00000000ffffffff
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias memory @system 0000000000000000-ffffffffffffffff
```

- smram_region :  因为 MemoryRegion pci 的优先级比 MemoryRegion pc.ram 的优先级低，使用 smram_region 可以将本来会被 ram 覆盖的 vga-lowmem 重新显示出来。
- smram : 当 CPU 在 SMM 模式下，其选择的 address-space 是 cpu-smm-0，其实最后的效果就是将 system_memory 上，将原来 0xa0000 ~ 0xbffff 的位置上放上 ram
而 0xa0000 ~ 0xbffff 上恰好放置的是 vga-lowmem, 也就是在 SMM 模式下，会将 vga-lowmem 用 ram 覆盖上。

### seabios 如何使用 SMM
写 I440FX 的配置空间

```c
    int smram_region = !(pd->config[I440FX_SMRAM] & SMRAM_D_OPEN);
    int smram = pd->config[I440FX_SMRAM] & SMRAM_G_SMRAME;
    printf("huxueshi:%s %d %d\n", __FUNCTION__, smram_region, smram);
```

- (smram_region, smram) = (1 0) ==> 因为 smram 关闭，那么 CPU 处于 SMM 模式下和普通模式没有区别, 所有的 CPU 都是无法访问到 SMM 空间的
- (smram_region, smram) = (0 8) ==> 因为 smram_region 关闭，所有的 CPU 访问到的都是 smm 模式，也即是访问到的是 pc.ram
- (smram_region, smram) = (1 8) ==> 正常的 smm 空间，smm 模式下的 CPU 和正常的 CPU 走不同的位置

seabios 中代码在这里
```c
// This code is hardcoded for PIIX4 Power Management device.
static void piix4_apmc_smm_setup(int isabdf, int i440_bdf)
{
    /* check if SMM init is already done */
    u32 value = pci_config_readl(isabdf, PIIX_DEVACTB);
    if (value & PIIX_DEVACTB_APMC_EN)
        return;

    /* enable the SMM memory window */
    pci_config_writeb(i440_bdf, I440FX_SMRAM, 0x02 | 0x48);

    smm_save_and_copy();

    /* enable SMI generation when writing to the APMC register */
    pci_config_writel(isabdf, PIIX_DEVACTB, value | PIIX_DEVACTB_APMC_EN);

    /* enable SMI generation */
    value = inl(acpi_pm_base + PIIX_PMIO_GLBCTL);
    outl(value | PIIX_PMIO_GLBCTL_SMI_EN, acpi_pm_base + PIIX_PMIO_GLBCTL);

    smm_relocate_and_restore();

    /* close the SMM memory window and enable normal SMM */
    pci_config_writeb(i440_bdf, I440FX_SMRAM, 0x02 | 0x08);
}
```

### QEMU 如何响应
其实 PAM 和 SMM 的响应都是在 i440fx_update_memory_mappings 中，而且操作非常类似，就是 enable / disable MemoryRegion
只是 SMM 处理的是 smram 和 smram_region 两个 MemoryRegion

### SMM 的使用
不考虑 pflash 的使用情况下，`cpu_get_mem_attrs` 唯一插入使用 `.secure` 的位置
```c
static inline MemTxAttrs cpu_get_mem_attrs(CPUX86State *env)
{
    return ((MemTxAttrs) { .secure = (env->hflags & HF_SMM_MASK) != 0 });
}
```
而 HF_SMM_MASK 在 `env->hflags` 的插入和删除位置 smm_helper 中间。

而 cpu_get_mem_attrs 的位置在各个 helper 以及 handle_mmu_fault 中。
这些组装的出来的 MemTxAttrs 的最终使用位置是: cpu_asidx_from_attrs。
如此，如果是 SMM 的地址空间, 使用相同的地址访问，最后就会访问到 ram 上而不是 vga-lowmem。


## 问题
- 虽然从代码中可以知道 pc Machine 的 0 ~ 1M 的物理地址空间内的各种布局，但是其规定在哪里，并不知道是来自于哪一个手册?
- 为什么需要将 pc.bios 映射到 4G pci 空间的最上方，这又是哪里的规定?

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

[^1]: https://en.wikipedia.org/wiki/System_Management_Mode
