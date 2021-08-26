# QEMU 的 memory model 和 softmmu 设计分析


## memory model

### 概述
原图来自于 kernelgo.org, 这里进行一些小小的修改。
![](./img/qemu-address-space.svg)

首先感受一下 memory model 是什么, 下面是一个例子，guest 机器的配置使用[这个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/hack/qemu/x64-e1000/alpine.sh)生成的。
```txt
 ┌──────────► 这是一个 AddressSpace，AddressSpace 用于描述整个地址空间的映射关系。
 │                                        ┌───────────────────────► MemoryRegion 的优先级，如果一个范围两个 MemoryRegion 出现重叠，优先级高的压制优先级低的 
 │                                        │
 │                                        │   ┌───────────────────► 表示这个空间的类型，一般划分为 io 和 RAM
 │                                        │   │    ┌──────────────► 这是一个 MemoryRegion，这是 Address Space 中最核心的概念，MemoryRegion 用于描述一个范围内的映射规则
address-space: memory                     │   │    │
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-00000000bfffffff (prio 0, ram): alias ram-below-4g @pc.ram 0000000000000000-00000000bfffffff──────────────┐
    0000000000000000-ffffffffffffffff (prio -1, i/o): pci                                                                      │
      00000000000a0000-00000000000bffff (prio 1, i/o): vga-lowmem                                                              │
      00000000000c0000-00000000000dffff (prio 1, rom): pc.rom                                                                  │
      00000000000e0000-00000000000fffff (prio 1, rom): alias isa-bios @pc.bios 0000000000020000-000000000003ffff               │
      00000000fe000000-00000000fe7fffff (prio 1, ram): vga.vram                                                                │
      00000000fe800000-00000000fe803fff (prio 1, i/o): virtio-pci                                                              │
        00000000fe800400-00000000fe80041f (prio 0, i/o): vga ioports remapped                                                  │
        00000000fe800500-00000000fe800515 (prio 0, i/o): bochs dispi interface                                                 │
        00000000fe800600-00000000fe800607 (prio 0, i/o): qemu extended regs                                                    │
        00000000fe801000-00000000fe8017ff (prio 0, i/o): virtio-pci-common-virtio-gpu                                          │
        00000000fe801800-00000000fe801fff (prio 0, i/o): virtio-pci-isr-virtio-gpu                                             │
        00000000fe802000-00000000fe802fff (prio 0, i/o): virtio-pci-device-virtio-gpu                                          │
        00000000fe803000-00000000fe803fff (prio 0, i/o): virtio-pci-notify-virtio-gpu                                          │
      00000000fe804000-00000000fe807fff (prio 1, i/o): virtio-pci                                                              │
        00000000fe804000-00000000fe804fff (prio 0, i/o): virtio-pci-common-virtio-9p                                           │
        00000000fe805000-00000000fe805fff (prio 0, i/o): virtio-pci-isr-virtio-9p                                              │
        00000000fe806000-00000000fe806fff (prio 0, i/o): virtio-pci-device-virtio-9p                                           │
        00000000fe807000-00000000fe807fff (prio 0, i/o): virtio-pci-notify-virtio-9p                                           │
      00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio                                                              │
      00000000febf0000-00000000febf3fff (prio 1, i/o): nvme-bar0                                                               │
        00000000febf0000-00000000febf1fff (prio 0, i/o): nvme                                                                  └── ram-below-4g 是 pc.ram 的一个 alias
        00000000febf2000-00000000febf240f (prio 0, i/o): msix-table
        00000000febf3000-00000000febf300f (prio 0, i/o): msix-pba                                                              ┌── ram-above-4g 也是 pc.ram 的一个 alias, 两者都被放到 system 这个 MemoryRegion 上
      00000000febf4000-00000000febf4fff (prio 1, i/o): virtio-vga-msix                                                         │
        00000000febf4000-00000000febf402f (prio 0, i/o): msix-table                                                            │
        00000000febf4800-00000000febf4807 (prio 0, i/o): msix-pba                                                              │
      00000000febf5000-00000000febf5fff (prio 1, i/o): virtio-9p-pci-msix                                                      │
        00000000febf5000-00000000febf501f (prio 0, i/o): msix-table                                                            │
        00000000febf5800-00000000febf5807 (prio 0, i/o): msix-pba                                                              │
      00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios                                                                 │
    00000000000a0000-00000000000bffff (prio 1, i/o): alias smram-region @pci 00000000000a0000-00000000000bffff                 │
    00000000000c0000-00000000000c3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000c0000-00000000000c3fff                   │
    00000000000c4000-00000000000c7fff (prio 1, ram): alias pam-rom @pc.ram 00000000000c4000-00000000000c7fff                   │
    00000000000c8000-00000000000cbfff (prio 1, ram): alias pam-rom @pc.ram 00000000000c8000-00000000000cbfff                   │
    00000000000cb000-00000000000cdfff (prio 1000, ram): alias kvmvapic-rom @pc.ram 00000000000cb000-00000000000cdfff           │
    00000000000cc000-00000000000cffff (prio 1, ram): alias pam-rom @pc.ram 00000000000cc000-00000000000cffff                   │
    00000000000d0000-00000000000d3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000d0000-00000000000d3fff                   │
    00000000000d4000-00000000000d7fff (prio 1, ram): alias pam-rom @pc.ram 00000000000d4000-00000000000d7fff                   │
    00000000000d8000-00000000000dbfff (prio 1, ram): alias pam-rom @pc.ram 00000000000d8000-00000000000dbfff                   │
    00000000000dc000-00000000000dffff (prio 1, ram): alias pam-rom @pc.ram 00000000000dc000-00000000000dffff                   │
    00000000000e0000-00000000000e3fff (prio 1, ram): alias pam-rom @pc.ram 00000000000e0000-00000000000e3fff                   │
    00000000000e4000-00000000000e7fff (prio 1, ram): alias pam-ram @pc.ram 00000000000e4000-00000000000e7fff                   │
    00000000000e8000-00000000000ebfff (prio 1, ram): alias pam-ram @pc.ram 00000000000e8000-00000000000ebfff                   │
    00000000000ec000-00000000000effff (prio 1, ram): alias pam-ram @pc.ram 00000000000ec000-00000000000effff                   │
    00000000000f0000-00000000000fffff (prio 1, ram): alias pam-rom @pc.ram 00000000000f0000-00000000000fffff                   │
    00000000fec00000-00000000fec00fff (prio 0, i/o): kvm-ioapic                                                                │
    00000000fed00000-00000000fed003ff (prio 0, i/o): hpet                                                                      │
    00000000fee00000-00000000feefffff (prio 4096, i/o): kvm-apic-msi                                                           │
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff──────────────┘
```

总体来说，MemoryRegion 用于描述一个范围内的映射规则, AddressSpace 用于描述整个地址空间的映射关系。有了映射关系，当 guest 访问到这些地址的时候，就可以知道具体应该进行什么操作了。

下面是 kvm 处理 io 端口的操作的一个调用流程图，只要给出 AddressSpace 以及 地址，最后就可以找到最后的 handler 为 `kbd_read_data`

```c
/*
#2  kbd_read_data (opaque=0x555556844d98, addr=<optimized out>, size=<optimized out>) at ../hw/input/pckbd.c:387
#3  0x0000555555cd2092 in memory_region_read_accessor (mr=mr@entry=0x555556844df0, addr=0, value=value@entry=0x7fffd9ff9130, size=size@entry=1, shift=0, mask=mask@entry=255, attrs=...) at ../softmmu/memory.c:440
#4  0x0000555555cceb1e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffd9ff9130, size=size@entry=1, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555cd2050 <memory_region_read_accessor>, mr=0x555556844df0, attrs=...) at ../softmmu/memory.c:554
#5  0x0000555555cd1ac1 in memory_region_dispatch_read1 (attrs=..., size=<optimized out>, pval=0x7fffd9ff9130, addr=0, mr=0x555556844df0) at ../softmmu/memory.c:1424
#6  memory_region_dispatch_read (mr=mr@entry=0x555556844df0, addr=0, pval=pval@entry=0x7fffd9ff9130, op=MO_8, attrs=attrs@entry=...) at ../softmmu/memory.c:1452
#7  0x0000555555c9eb89 in flatview_read_continue (fv=fv@entry=0x7ffe4402d230, addr=addr@entry=96, attrs=..., ptr=ptr@entry=0x7fffeb17d000, len=len@entry=1, addr1=<optimized out>, l=<optimized out>, mr=0x555556844df0) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:165
#8  0x0000555555c9ed43 in flatview_read (fv=0x7ffe4402d230, addr=addr@entry=96, attrs=attrs@entry=..., buf=buf@entry=0x7fffeb17d000, len=len@entry=1) at ../softmmu/physmem.c:2881
#9  0x0000555555c9ee96 in address_space_read_full (as=0x555556606880 <address_space_io>, addr=96, attrs=..., buf=0x7fffeb17d000, len=1) at ../softmmu/physmem.c:2894
#10 0x0000555555c9f015 in address_space_rw (as=<optimized out>, addr=addr@entry=96, attrs=..., attrs@entry=..., buf=<optimized out>, len=len@entry=1, is_write=is_write@entry=false) at ../softmmu/physmem.c:2922
#11 0x0000555555c8ece9 in kvm_handle_io (count=1, size=1, direction=<optimized out>, data=<optimized out>, attrs=..., port=96) at ../accel/kvm/kvm-all.c:2635
#12 kvm_cpu_exec (cpu=cpu@entry=0x555556af4410) at ../accel/kvm/kvm-all.c:2886
#13 0x0000555555cf1825 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556af4410) at ../accel/kvm/kvm-accel-ops.c:49
#14 0x0000555555e55983 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:541
#15 0x00007ffff628d609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#16 0x00007ffff61b4293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
*/
```
### AddressSpace
AddressSpace 用于描述整个地址空间的映射关系, 不同的地址空间的映射关系不同。guest 写相同的地址，在 io 的空间和 memory 空间的效果不同的。
在 kvm 模式下，主要是两个 AddressSpace，一个是 `address_space_memory` 另一个是 `address_space_io`, 定义在 `softmmu/physmem.c` 中间。

```c
        case KVM_EXIT_IO:
            DPRINTF("handle_io\n");
            /* Called outside BQL */
            kvm_handle_io(run->io.port, attrs,
                          (uint8_t *)run + run->io.data_offset,
                          run->io.direction,
                          run->io.size,
                          run->io.count);
            ret = 0;
            break;
        case KVM_EXIT_MMIO:
            DPRINTF("handle_mmio\n");
            /* Called outside BQL */
            address_space_rw(&address_space_memory,
                             run->mmio.phys_addr, attrs,
                             run->mmio.data,
                             run->mmio.len,
                             run->mmio.is_write);
```

### MemoryRegion
MemoryRegion 用于描述一个范围内的映射规则。

例如下面的范围中描述的是，e1000-mmio 空间，对于该范围的读写最后不是在读写内存，而是在在操作设备，这会触发设备的模拟工作。
```txt
      00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio                                                              
```
而这个范围访问 ram 了。
```txt
    0000000100000000-00000001bfffffff (prio 0, ram): alias ram-above-4g @pc.ram 00000000c0000000-000000017fffffff
```

刚开始解除可能比较奇怪，为什么 memory region 重叠，为什么需要制作出现 alias 的概念。
其实，很简单，这样写代码比较方便，
- 如果不让 memory region 可以 overlap，那么一个模块在构建 memory region 的时候需要考虑另一个另一个模块的 memory region 的大小。
    - pci 空间是覆盖了 64 bit 的空间，和 ram-below-4g 和 ram-above-4g 是 overlap 的, 而 ram 的大小是动态确定，如果禁止 overlap, 那么 pci 的范围也需要动态调整
- 使用 alias 可以将一个 memory region 的一部分放到另一个 memory region 上，就 ram-below-4g 和 ram-above-4g 的例子而言，就是将创建 ram-below-4g 是 pc.ram 的一部分，然后放到 MemoryRegion `system` 上。
    - 如果不采用这种方法，而是创建两个分裂的 pc.ram, 那么就要 mmap 两次，之后的种种操作也都需要进行两次。

### FlatView
给出一个地址，显然需要尽快知道该地址映射到什么位置, 但是
MemoryRegion 存在 overlap，alias 的，获取这个地址上的真正的 memory region 需要进行计算，
不可能每次访存都将 memory region 的关系计算一次，而且要保存下来。这个保存下来的结果就是 FlatView 了。

FlatView 由一个个 FlatRange 组成，互相不重合，描述了最终的地址空间的样子，从 MemoryRegion 生成 FlatRange 的过程在 `render_memory_region`

```txt
  0000000000000000-00000000000bffff (prio 0, ram): pc.ram
  00000000000c0000-00000000000cafff (prio 0, rom): pc.ram @00000000000c0000
  00000000000cb000-00000000000cdfff (prio 0, ram): pc.ram @00000000000cb000
  00000000000ce000-00000000000e3fff (prio 0, rom): pc.ram @00000000000ce000
  00000000000e4000-00000000000effff (prio 0, ram): pc.ram @00000000000e4000
  00000000000f0000-00000000000fffff (prio 0, rom): pc.ram @00000000000f0000
  0000000000100000-00000000bfffffff (prio 0, ram): pc.ram @0000000000100000
  00000000fd000000-00000000fdffffff (prio 1, ram): vga.vram
  00000000fe000000-00000000fe000fff (prio 0, i/o): virtio-pci-common-virtio-9p
  00000000fe001000-00000000fe001fff (prio 0, i/o): virtio-pci-isr-virtio-9p
  00000000fe002000-00000000fe002fff (prio 0, i/o): virtio-pci-device-virtio-9p
  00000000fe003000-00000000fe003fff (prio 0, i/o): virtio-pci-notify-virtio-9p
  00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio
  00000000febf0000-00000000febf1fff (prio 0, i/o): nvme
  00000000febf2000-00000000febf240f (prio 0, i/o): msix-table
  00000000febf3000-00000000febf300f (prio 0, i/o): msix-pba
  00000000febf4000-00000000febf5fff (prio 0, i/o): nvme
  00000000febf6000-00000000febf640f (prio 0, i/o): msix-table
  00000000febf7000-00000000febf700f (prio 0, i/o): msix-pba
  00000000febf8000-00000000febf817f (prio 0, i/o): edid
  00000000febf8180-00000000febf83ff (prio 1, i/o): vga.mmio @0000000000000180
  00000000febf8400-00000000febf841f (prio 0, i/o): vga ioports remapped
  00000000febf8420-00000000febf84ff (prio 1, i/o): vga.mmio @0000000000000420
  00000000febf8500-00000000febf8515 (prio 0, i/o): bochs dispi interface
  00000000febf8516-00000000febf85ff (prio 1, i/o): vga.mmio @0000000000000516
  00000000febf8600-00000000febf8607 (prio 0, i/o): qemu extended regs
  00000000febf8608-00000000febf8fff (prio 1, i/o): vga.mmio @0000000000000608
  00000000febf9000-00000000febf901f (prio 0, i/o): msix-table
  00000000febf9800-00000000febf9807 (prio 0, i/o): msix-pba
  00000000fec00000-00000000fec00fff (prio 0, i/o): ioapic
  00000000fed00000-00000000fed003ff (prio 0, i/o): hpet
  00000000fee00000-00000000feefffff (prio 4096, i/o): apic-msi
  00000000fffc0000-00000000ffffffff (prio 0, rom): pc.bios
  0000000100000000-00000001bfffffff (prio 0, ram): pc.ram @00000000c0000000
```

### AddressSpaceDispatch
FlatView 是一个数组形式，为了加快访问，显然需要使用构成一个红黑树之类的，可以在 log(n) 的时间内访问的, QEMU 实现的
这个就是 AddressSpaceDispatch 了。

将 FlatRange 逐个调用 `flatview_add_to_dispatch` 创建出来的。

### MemoryListener
当修改地址空间的映射规则的时候，有时候需要执行一下 hook 函数，最典型的就是 kvm 添加了一个 ram 的时候，这个时候是需要通知内核的 kvm 模块的，
而 tcg 的地址空间是纯粹软件模拟，就无需注册这个 hook

MemoryListener 的还有一个主要功能是 dirty memory 的记录, kvm 依赖内核模块，所以总是需要执行一下对应的通知内核操作。

### CPUAddressSpace
tcg 因为使用 softmmu 的原因，cpu 的访问 ram 也是走软件的，

CPUAddressSpace 并不是单独给 CPU 创建一个新的映射，只是将 CPUState 和 AddressSpace 相关的放到一起而已。
具体例子可以看， address_space_translate_for_iotlb 和 iotlb_to_section 。

### SMM
tcg 处理 AddressSpace 最大的不同在于为 SMM 创建一个新的地址空间。

在 do_smm_enter 中，会

```c
    env->hflags |= HF_SMM_MASK;
```

导致制作出来的 MemTxAttrs 的不同
```c
static inline MemTxAttrs cpu_get_mem_attrs(CPUX86State *env)
{
    return ((MemTxAttrs) { .secure = (env->hflags & HF_SMM_MASK) != 0 });
}
```

最后在 tlb_set_page_with_attrs 中选择不同的地址空间的。

### IOMMU
IOMMU 简单来说是

### QA
- 为什么 ram 不是一整块，而是拆分出来了 ram-below-4g 和 ram-above-4g 两个部分?
    - 因为中间需要留出 mmio 空洞
