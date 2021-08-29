# QEMU 中的 seabios : 地址空间
<!-- vim-markdown-toc GitLab -->

- [seabios 执行的第一行代码](#seabios-执行的第一行代码)
- [pc.bios](#pcbios)
- [PAM](#pam)
    - [QEMU 侧如何处理 PAM](#qemu-侧如何处理-pam)
    - [Seabios 侧如何处理 PAM](#seabios-侧如何处理-pam)
- [SMM](#smm)

<!-- vim-markdown-toc -->

seabios 的基础知识可以参考李强的《QEMU/KVM 源码解析与应用》, 下面来分析一下和地址空间相关的几个小问题。

## seabios 执行的第一行代码

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

## pc.bios
QEMU 支持很多种类的 bios, seabios 只是其中的一种, bios 加载地址空间中，该 MemoryRegion 的名称为 pc.bios

seabios 的 src/fw/shadow.c 中存在有一个注释:

```c
// On the emulators, the bios at 0xf0000 is also at 0xffff0000
#define BIOS_SRC_OFFSET 0xfff00000
```
是的，seabios 被同时映射到两个位置。

从地址中看，这确实:
```txt
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
```
也就是一方面，pc.bios 被映射到 4G 的顶端，一方面其后 0x20000 的部分被放到了 0xe0000 的位置
这就是 seabios 启动的时候，可以直接跳转到这里。

## PAM
PAM 的作用可以将对于 bios 空间读写转发到 PCI 或者 RAM 中，因为读写 ROM 比较慢, 进一步可以参考:
- https://patchwork.ozlabs.org/project/qemu-devel/patch/1437389593-15297-1-git-send-email-real@ispras.ru/
- https://wiki.qemu.org/Documentation/Platforms/PC


#### QEMU 侧如何处理 PAM

在 i440fx_init 初始化的时候, 来初始化所有 `PAMMemoryRegion`, 一共 13 个
- 第一个映射: System BIOS Area Memory Segments, 映射 0x10000 到 0xfffff
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

#### Seabios 侧如何处理 PAM
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
当 bios 结束之后，这些 PAM 的位置会再次设置上 `make_bios_readonly_intel`，但是 0xe4000 ~ 0xeffff 的部分会被豁免。

总结一些，本来 0xc0000 ~ 0xfffff 的区间是映射到 PCI 上的，之后修改为 RAM 的，最后修改为 ROM 的。

## SMM
i440fx_update_memory_mappings 中初始化了 smram_region
```c
    00000000000a0000-00000000000bffff (prio 1, i/o): alias smram-region @pci 00000000000a0000-00000000000bffff
```
其作用是将本来会被 ram 覆盖的 vga-lowmem 重新显示出来。


当 CPU 在 SMM 模式下，其空间如下，其实最后的效果就是将 system_memory 上，将原来 0xa0000 ~ 0xbffff 的位置上放上 ram

```
address-space: cpu-smm-0
  0000000000000000-ffffffffffffffff (prio 0, i/o): memory
    0000000000000000-00000000ffffffff (prio 1, i/o): alias smram @smram 0000000000000000-00000000ffffffff
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias memory @system 0000000000000000-ffffffffffffffff

memory-region: smram
  0000000000000000-00000000ffffffff (prio 0, i/o): smram
    00000000000a0000-00000000000bffff (prio 0, ram): alias smram-low @pc.ram 00000000000a0000-00000000000bffff
```

而 0xa0000 ~ 0xbffff 上恰好放置的是 vga-lowmem, 也就是在 SMM 模式下，会将 vga-lowmem 用 ram 覆盖上。
