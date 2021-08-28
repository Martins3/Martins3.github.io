# QEMU softmmu 访存函数集整理

<!-- vim-markdown-toc GitLab -->

- [TODO](#todo)
- [CPU 访存](#cpu-访存)
- [设备访存](#设备访存)
- [load_helper/store_helper 的封装函数](#load_helperstore_helper-的封装函数)
- [DMA](#dma)

<!-- vim-markdown-toc -->
QEMU 为了处理大端小端(le/be), 不同大小(size = 1/2/4/8), 以及访存方向(load/store)，定义了一堆类似的函数

## TODO
- [ ] cpu_physical_memory_rw

- [ ] 找到那些到达 flatview_read_continue 的函数

- [ ] 总结一些访存的几条标准路径
    - 上面的一堆接口
    - TLB fast path and slow path

## CPU 访存

`x86_*_phys` => `address_space_*` => (st/ld)(w/l/q/uw/sw)_(le/be)_p

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
而 `address_space_*` 之类都是在 memory_ldst.c.inc 中间生成的。

在 address_space_stw 

- [ ] 在 x86_stw_phys 应该都是各种 helper 访问的，也就是其中 CPU 访存的模拟而已，为什么在 address_space_stw 中非要处理 device 的大端小端，除非从 io_readx 中同样也是处理了的。
- memory_ldst.c 中，最后的 dirty 只是剩下一个 x86_stl_phys_notdirty 了
  - [x] 这并不是移植的时候，特地的删除的

- [ ] 在我的设想中，如果 store_helper 访存 和 helper 中访存储的唯一区别在于前者需要查询 TLB 进行虚实地址装换，后者是已经获取了物理地址的，所以后者是前者的一个简化版的。


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

## load_helper/store_helper 的封装函数

helper_ret_stb_mmu 
helper_le_stw_mmu  
helper_le_stl_mmu  
helper_le_stq_mmu  
helper_ret_ldsb_mmu
helper_ret_ldub_mmu
helper_le_ldsw_mmu 
helper_le_lduw_mmu 
helper_le_ldsl_mmu 
helper_le_ldul_mmu 
helper_le_ldq_mmu  

## DMA
- [ ] 无法理解 DMA 和 stl_le_phys 的差别啊
- [ ] 是不是这些 DMA 函数都是给设备使用的 ?
    - 如果可以使用 DMA，那么一定会使用的 DMA 的

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


