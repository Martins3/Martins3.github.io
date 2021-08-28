# QEMU softmmu 访存函数集整理

<!-- vim-markdown-toc GitLab -->

    - [access_size](#access_size)
    - [endianness](#endianness)
- [TODO](#todo)
- [softmmu 慢速路径访存](#softmmu-慢速路径访存)
- [CPU 访存](#cpu-访存)
- [设备访存](#设备访存)
- [PCI 设备访存](#pci-设备访存)

<!-- vim-markdown-toc -->
QEMU 为了处理大端小端(le/be), 不同大小(size = 1/2/4/8), 以及访存方向(load/store)，定义了一堆类似的函数

#### access_size
access_with_adjusted_size 会计算调用的大小，实际上，最终将大小约束到 1 - 4 之间, 如果需要进行的 io 的大小超过这个 4, 那么就使用循环反复调用 MemoryRegionOps::read

所以，真的会出现 pio/mmio 的 size > 4 的情况吗, 实际测试显示，只有 vga-lowmem 会是如此。

MemoryRegionOps::read 的参数是有 size 的



#### endianness
memory_region_dispatch_read 在最后会调用 adjust_endianness
而 memory_region_dispatch_write 会在开始的时候调用 adjust_endianness

> 如果 guest 架构和 host 架构的 endianness 不相同的话，写入 RAM 的时候需要注意。
> 但是如果设备的 endian 的不同和 host 不同，为什么


目前只有一个 device 是 big endianness 的，那就是 fwcfg.dma
- hw/nvram/fw_cfg.c 中 fw_cfg_dma_mem_ops 和 fw_cfg_comb_mem_ops 的确如此定义
- 从 qemu_cfg_dma_transfer 中也可以找到证据

```c
/*
0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
```

实际上，qemu_cfg_dma_transfer 的处理和 adjust_endianness 的操作不是重合了吗?

```c
static void
qemu_cfg_dma_transfer(void *address, u32 length, u32 control)
{
    QemuCfgDmaAccess access;

    access.address = cpu_to_be64((u64)(u32)address);
    access.length = cpu_to_be32(length);
    access.control = cpu_to_be32(control);

    barrier();

    outl(cpu_to_be32((u32)&access), PORT_QEMU_CFG_DMA_ADDR_LOW);

    while(be32_to_cpu(access.control) & ~QEMU_CFG_DMA_CTL_ERROR) {
        yield();
    }
}
```

```c
static void adjust_endianness(MemoryRegion *mr, uint64_t *data, MemOp op)
{
    if ((op & MO_BSWAP) != devend_memop(mr->ops->endianness)) {
        switch (op & MO_SIZE) {
        case MO_8:
            break;
        case MO_16:
            *data = bswap16(*data);
            break;
        case MO_32:
            *data = bswap32(*data);
            break;
        case MO_64:
            *data = bswap64(*data);
            break;
        default:
            g_assert_not_reached();
        }
    }
}
```

- [ ] 就分析一下 fw_cfg 吧

## TODO
- [ ] 找到那些到达 flatview_read_continue 的函数

- [ ] 为什么需要处理 endian 的情况，如果一个设备是 bigendian 的，那么其放入到内存的就是 bigendian 的，那么模拟设备的代码为什么需要考虑这个事情啊，让 CPU 自己处理啊!

- cpu_physical_memory_rw : 对于 address_space_rw 的一个封装而已
    - 走了 

## softmmu 慢速路径访存 
当 soft tlb 没有命中之后，会切入到此处，
`helper_(ret/le)_(ld/st)(sb/ub/sw/uw/l/q/sl/ul)_mmu`

简单的组装出参数之后，调用 `load_helper`/`store_helper`

## CPU 访存
`x86_*_phys` => `address_space_(ld/st)(w/l/q)_(le/be)` => (st/ld)(w/l/q/uw/sw)_(le/be)_p

在 target/i386/helper.c 定义了一堆类似下面的函数:

```c
void x86_stw_phys(CPUState *cs, hwaddr addr, uint32_t val)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    MemTxAttrs attrs = cpu_get_mem_attrs(env);
    AddressSpace *as = cpu_addressspace(cs, attrs);

    address_space_stw(as, addr, val, attrs, NULL);
}
```

而 `address_space_(ld/st)(w/l/q)_(le/be)`  之类都是在 memory_ldst.c.inc 中间生成的。

调用 address_space_stl_internal 的 endian 是确定的，就是 DEVICE_NATIVE_ENDIAN 的。

- 在 x86_stw_phys 应该都是各种 helper 访问的，也就是其中 CPU 访存的模拟而已，为什么在 address_space_stw 中非要处理 device 的大端小端，从 io_readx 中同样也是处理了的。
- 如果访问是 ram, 那么就不存在这个装换的需求了，因为 host 和 guest 的相同的。但是如果是访问的是设备，和 io_readx 相同，走的路径都是 `memory_region_dispatch_read` 的，在哪里进行


- memory_ldst.c 中，最后的 dirty 只是剩下一个 x86_stl_phys_notdirty 了
  - [x] 这并不是移植的时候，特地的删除的


## 设备访存
设备模拟模块调用的, 比如 ioapic 想要发送中断的时候，就会调用 `ioapic_service` => `stl_le_phys`

只是设备访存的时候进行的操作, 设备的 AddressSpace 和 MemTxAttrs 是确定的，所以直接调用到 `address_space_*` 就可以了。

```c
static inline void glue(stl_le_phys, SUFFIX)(ARG1_DECL, hwaddr addr, uint32_t val)
{
    glue(address_space_stl_le, SUFFIX)(ARG1, addr, val,
                                       MEMTXATTRS_UNSPECIFIED, NULL);
}
```


## PCI 设备访存
- [ ] 我总是设想，DMA 和 stl_le_phys 的区别在于增加了一个 IOMMU, 也许不是如此，可以检测一下。


- [ ] 无法理解 DMA 和 stl_le_phys 的差别啊
- [ ] 是不是这些 DMA 函数都是给设备使用的 ?
    - 如果可以使用 DMA，那么一定会使用的 DMA 的

如果可以确定，这个
```c
#define DEFINE_LDST_DMA(_lname, _sname, _bits, _end) \
    static inline uint##_bits##_t ld##_lname##_##_end##_dma(AddressSpace *as, \
                                                            dma_addr_t addr) \
    {                                                                   \
        uint##_bits##_t val;                                            \
        dma_memory_read(as, addr, &val, (_bits) / 8);                   \
        return _end##_bits##_to_cpu(val);                               \
    }                                                                   \
    static inline void st##_sname##_##_end##_dma(AddressSpace *as,      \
                                                 dma_addr_t addr,       \
                                                 uint##_bits##_t val)   \
    {                                                                   \
        val = cpu_to_##_end##_bits(val);                                \
        dma_memory_write(as, addr, &val, (_bits) / 8);                  \
    }

static inline uint8_t ldub_dma(AddressSpace *as, dma_addr_t addr)
{
    uint8_t val;

    dma_memory_read(as, addr, &val, 1);
    return val;
}

static inline void stb_dma(AddressSpace *as, dma_addr_t addr, uint8_t val)
{
    dma_memory_write(as, addr, &val, 1);
}

DEFINE_LDST_DMA(uw, w, 16, le);
DEFINE_LDST_DMA(l, l, 32, le);
DEFINE_LDST_DMA(q, q, 64, le);
DEFINE_LDST_DMA(uw, w, 16, be);
DEFINE_LDST_DMA(l, l, 32, be);
DEFINE_LDST_DMA(q, q, 64, be);

#undef DEFINE_LDST_DMA
```


```c
#define PCI_DMA_DEFINE_LDST(_l, _s, _bits)                              \
    static inline uint##_bits##_t ld##_l##_pci_dma(PCIDevice *dev,      \
                                                   dma_addr_t addr)     \
    {                                                                   \
        return ld##_l##_dma(pci_get_address_space(dev), addr);          \
    }                                                                   \
    static inline void st##_s##_pci_dma(PCIDevice *dev,                 \
                                        dma_addr_t addr, uint##_bits##_t val) \
    {                                                                   \
        st##_s##_dma(pci_get_address_space(dev), addr, val);            \
    }

PCI_DMA_DEFINE_LDST(ub, b, 8);
PCI_DMA_DEFINE_LDST(uw_le, w_le, 16)
PCI_DMA_DEFINE_LDST(l_le, l_le, 32);
PCI_DMA_DEFINE_LDST(q_le, q_le, 64);
PCI_DMA_DEFINE_LDST(uw_be, w_be, 16)
PCI_DMA_DEFINE_LDST(l_be, l_be, 32);
PCI_DMA_DEFINE_LDST(q_be, q_be, 64);
```


