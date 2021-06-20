# Qemu 内存虚拟化

## [kernel doc](https://qemu.readthedocs.io/en/latest/devel/memory.html)
In addition to MemoryRegion objects, the memory API provides AddressSpace objects for every root and possibly for intermediate MemoryRegions too. These represent memory as seen from the CPU or a device’s viewpoint.
- [ ] 一个 bus 为什么需要自己的视角啊

> For example, a PCI BAR may be composed of a RAM region and an MMIO region.
- [ ] 什么意思 ?

alias: a subsection of another region. 

## TODO
- 既然 flatview 计算好了，那么按照道理来说，就可以直接注册，结果每次 mmio，路径那么深
  - [ ] 一种可能，那就是，这空间是动态分配的
    - [ ] 似乎不是这个原因

1. info mtree 的时候:
    1. address-space
    2. memory-region

- [ ] container 是一个啥概念啊 ?
```c
static hwaddr memory_region_to_absolute_addr(MemoryRegion *mr, hwaddr offset)
{
    MemoryRegion *root;
    hwaddr abs_addr = offset;

    abs_addr += mr->addr;
    for (root = mr; root->container; ) {
        root = root->container;
        abs_addr += root->addr;
    }

    return abs_addr;
}
```

- [ ] memory_region_add_subregion



举个例子:
```
0000000000000600-000000000000063f (prio 0, i/o): piix4-pm
```

```
>>> p mr->ops->write
$4 = (void (*)(void *, hwaddr, uint64_t, unsigned int)) 0x555555880a20 <acpi_pm_cnt_write>
>>> p mr->container->name
$5 = 0x5555569b4040 "piix4-pm"
>>> p mr->name
$6 = 0x555556dd37d0 "acpi-cnt"
>>> p/x mr->container->addr
$8 = 0x600
```
