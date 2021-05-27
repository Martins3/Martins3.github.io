# Qemu 内存虚拟化

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
