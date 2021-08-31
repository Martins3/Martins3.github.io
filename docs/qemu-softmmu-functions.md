# QEMU softmmu 访存 helper 整理

<!-- vim-markdown-toc GitLab -->

- [背景介绍](#背景介绍)
  - [access size](#access-size)
  - [endianness](#endianness)
- [softmmu 慢速路径访存](#softmmu-慢速路径访存)
- [CPU 访存](#cpu-访存)
- [CPU 访问物理内存](#cpu-访问物理内存)
- [CPU 访问虚拟内存](#cpu-访问虚拟内存)
- [CPU 访问 IO](#cpu-访问-io)
- [设备访存](#设备访存)
- [PCI 设备访存](#pci-设备访存)
- [总结](#总结)

<!-- vim-markdown-toc -->
QEMU 为了处理大端小端(le/be), 不同大小(size = 1/2/4/8), 以及访存方向(load/store)，定义了一堆类似的 helper。

## 背景介绍

### access size
实际上，这些函数集合处理的大小都是 b w l q 四种，**因为这就是访问 IO 空间的所有长度可能性**，为了方便，创建出来一堆 helper 。

IO 访问的，最后总是走到 `memory_region_dispatch_(read/write)` 上面来的。

- memory_region_dispatch_read
  - memory_region_dispatch_read1
      - access_with_adjusted_size
          - memory_region_read_accessor
              - `mr->ops->read`

`MemoryRegionOps::impl::min_access_size` 和 `MemoryRegionOps::impl::max_access_size` 可以描述设备进行一次 IO 最多和最少传输的 

如果一次 IO 的数量越过了 min_access_size / max_access_size，那么可以循环反复调用 `MemoryRegionOps::read`

### endianness
endianness 为什么会产生问题，先从一个简单的情况考虑，使用 load_helper 可以读取到 guest 一个地址上的数值，之后这个地址被加载到 host 的模拟寄存器，并且进行运算。
显然，为了让 host CPU 正确理解这个数值，需要在 load_helper 返回数值的时候，进行装换一下。

到底是调用下面哪一个函数，取决于 guest 是大端还是小端

```c
tcg_target_ulong helper_le_lduw_mmu(CPUArchState *env, target_ulong addr,
                                    TCGMemOpIdx oi, uintptr_t retaddr)
{
    return full_le_lduw_mmu(env, addr, oi, retaddr);
}

static uint64_t full_be_lduw_mmu(CPUArchState *env, target_ulong addr,
                                 TCGMemOpIdx oi, uintptr_t retaddr)
{
    return load_helper(env, addr, oi, retaddr, MO_BEUW, false,
                       full_be_lduw_mmu);
}
```

目前只有一个 device 是 big endianness 的，那就是 fwcfg.dma
```c
/*
0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
```


这是 fw_cfg_dma_transfer 中进行的调整，adjust_endianness 也是进行了调整的。
```c
static void fw_cfg_dma_transfer(FWCfgState *s)
{
    dma_addr_t len;
    FWCfgDmaAccess dma;
    int arch;
    FWCfgEntry *e;
    int read = 0, write = 0;
    dma_addr_t dma_addr;

    /* Reset the address before the next access */
    dma_addr = s->dma_addr;
    s->dma_addr = 0;

    if (dma_memory_read(s->dma_as, dma_addr, &dma, sizeof(dma))) {
        stl_be_dma(s->dma_as, dma_addr + offsetof(FWCfgDmaAccess, control),
                   FW_CFG_DMA_CTL_ERROR);
        return;
    }

    dma.address = be64_to_cpu(dma.address);
    dma.length = be32_to_cpu(dma.length);
    dma.control = be32_to_cpu(dma.control);

```

而下面是 seabios 的代码证明了操作
```c
static void
fw_cfg_dma_transfer(void *address, u32 length, u32 control)
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

## softmmu 慢速路径访存 
当 soft tlb 没有命中之后，会切入到此处，
`helper_(ret/le)_(ld/st)(sb/ub/sw/uw/l/q/sl/ul)_mmu`

简单的组装出参数之后，调用 `load_helper`/`store_helper`

## CPU 访存
target/i386/ 下的各种 helper 在模拟 CPU 访存的过程, 
- 如果是模拟 page walk 之类的，那就是直接访问物理内存
- 如果是模拟 FPU 之类的，那就是访问虚拟存储
## CPU 访问物理内存
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

## CPU 访问虚拟内存
因为是访问虚拟存储，最终必然要调用到 store_helper / load_helper 的位置，
这些 helper 的作用就是组装这两个 helper 的函数了。

cpu_ldst.h 中的描述应该是相当清晰了。
```c
/*
 * Generate inline load/store functions for all MMU modes (typically
 * at least _user and _kernel) as well as _data versions, for all data
 * sizes.
 *
 * Used by target op helpers.
 *
 * The syntax for the accessors is:
 *
 * load:  cpu_ld{sign}{size}{end}_{mmusuffix}(env, ptr)
 *        cpu_ld{sign}{size}{end}_{mmusuffix}_ra(env, ptr, retaddr)
 *        cpu_ld{sign}{size}{end}_mmuidx_ra(env, ptr, mmu_idx, retaddr)
 *
 * store: cpu_st{size}{end}_{mmusuffix}(env, ptr, val)
 *        cpu_st{size}{end}_{mmusuffix}_ra(env, ptr, val, retaddr)
 *        cpu_st{size}{end}_mmuidx_ra(env, ptr, val, mmu_idx, retaddr)
 *
 * sign is:
 * (empty): for 32 and 64 bit sizes
 *   u    : unsigned
 *   s    : signed
 *
 * size is:
 *   b: 8 bits
 *   w: 16 bits
 *   l: 32 bits
 *   q: 64 bits
 *
 * end is:
 * (empty): for target native endian, or for 8 bit access
 *     _be: for forced big endian
 *     _le: for forced little endian
 *
 * mmusuffix is one of the generic suffixes "data" or "code", or "mmuidx".
 * The "mmuidx" suffix carries an extra mmu_idx argument that specifies
 * the index to use; the "data" and "code" suffixes take the index from
 * cpu_mmu_index().
 */
```


## CPU 访问 IO 

在 misc_helper.c 中存在下面的一系列封装，其 x86_stw_phys 的效果非常类似，只是其 address space 是 `address_space_io`
```c
void helper_outb(CPUX86State *env, uint32_t port, uint32_t data)
{
    address_space_stb(&address_space_io, port, data,
                      cpu_get_mem_attrs(env), NULL);
}
```

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
如果是 PCI 设备，更多的使用下面 dma 方式。

## PCI 设备访存
过下面两组宏，制作出来大量的 dma 相关的辅助函数，这些函数都是主要是给 PCI 设备用的，

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

## 总结
进行访存最后总是百川如海，

- store_helper => 此处区分访问的对象是是否为 RAM, 如果是 RAM，直接 memcpy, 如果是 IO，那么 memory_region_dispatch_write
- address_space_ldl_internal => 此处区分, 如果是 IO ，那么 memory_region_dispatch_write
- address_space_rw => flatview_write_continue => 此处区分，如果是 IO，那么 memory_region_dispatch_write

而在 memory_region_dispatch_write 将会处理 endianness 和 size 的大小。

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
