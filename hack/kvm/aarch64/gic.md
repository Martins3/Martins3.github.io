```diff
History:        #0
Commit:         0aa1de57319c4e023187aca0d59dd593a96459a8
Author:         Andre Przywara <andre.przywara@arm.com>
Committer:      Marc Zyngier <maz@kernel.org>
Author Date:    Fri Jul 15 19:43:29 2016
Committer Date: Tue Jul 19 01:14:35 2016

KVM: arm64: vgic: Handle ITS related GICv3 redistributor registers

In the GICv3 redistributor there are the PENDBASER and PROPBASER
registers which we did not emulate so far, as they only make sense
when having an ITS. In preparation for that emulate those MMIO
accesses by storing the 64-bit data written into it into a variable
which we later read in the ITS emulation.
We also sanitise the registers, making sure RES0 regions are respected
and checking for valid memory attributes.

Signed-off-by: Andre Przywara <andre.przywara@arm.com>
Reviewed-by: Marc Zyngier <marc.zyngier@arm.com>
Tested-by: Eric Auger <eric.auger@redhat.com>
Signed-off-by: Marc Zyngier <marc.zyngier@arm.com>
```

```dif
History:        #0
Commit:         731532176716e2775a5d21115bb9c5c61e0cb704
Author:         Alexander Graf <graf@amazon.com>
Committer:      Marc Zyngier <maz@kernel.org>
Author Date:    Wed Jul  1 22:02:06 2020
Committer Date: Mon Jul  6 02:15:34 2020

KVM: arm64: vgic-its: Change default outer cacheability for {PEND, PROP}BASER

PENDBASER and PROPBASER define the outer caching mode for LPI tables.
The memory backing them may not be outer sharable, so we mark them as nC
by default. This however, breaks Windows on ARM which only accepts
SameAsInner or RaWaWb as values for outer cachability.

We do today already allow the outer mode to be set to SameAsInner
explicitly, so the easy fix is to default to that instead of nC for
situations when an OS asks for a not fulfillable cachability request.

This fixes booting Windows in KVM with vgicv3 and ITS enabled for me.

Signed-off-by: Alexander Graf <graf@amazon.com>
Signed-off-by: Marc Zyngier <maz@kernel.org>
Link: https://lore.kernel.org/r/20200701140206.8664-1-graf@amazon.com
```

分析下两个寄存器:
- [`PENDBASER`](https://developer.arm.com/documentation/ddi0595/2021-06/External-Registers/GICR-PENDBASER--Redistributor-LPI-Pending-Table-Base-Address-Register)
  - Specifies the base address of the LPI Pending table, and the Shareability and Cacheability of accesses to the LPI Pending table.
- [`PROPBASER`](https://developer.arm.com/documentation/ddi0601/2022-03/External-Registers/GICR-PROPBASER--Redistributor-Properties-Base-Address-Register)


- [Cacheable and shareable memory attributes](https://developer.arm.com/documentation/den0024/a/Memory-Ordering/Memory-attributes/Cacheable-and-shareable-memory-attributes)


## 简单的代码分析
```c
const struct kvm_io_device_ops kvm_io_gic_ops = {
	.read = dispatch_mmio_read,
	.write = dispatch_mmio_write,
};
```

```txt
=> vgic_sanitise_outer_cacheability
=> vgic_uaccess
=> vgic_v3_redist_uaccess
=> vgic_v3_attr_regs_access
=> vgic_v3_set_attr
=> kvm_device_ioctl_attr
=> kvm_device_ioctl
=> do_vfs_ioctl
=> ksys_ioctl
=> __arm64_sys_ioctl
=> el0_svc_common
=> el0_svc_handler
=> el0_svc
```
或者这种:
```txt
=> vgic_sanitise_outer_cacheability
=> dispatch_mmio_write
=> __kvm_io_bus_write.isra.23
=> kvm_io_bus_write
=> io_mem_abort
=> kvm_handle_guest_abort
=> handle_exit
=> kvm_arch_vcpu_ioctl_run
=> kvm_vcpu_ioctl
=> do_vfs_ioctl
=> ksys_ioctl
=> __arm64_sys_ioctl
=> el0_svc_common
=> el0_svc_handler
=> el0_svc
```

- [x] 似乎只有启动的时候调用?
  - 对应在 QEMU 中的这个位置: hw/intc/arm_gicv3_kvm.c

定义和 its 相关的寄存器:
```c
static struct vgic_register_region its_registers[] = {
  // ...
}


static const struct vgic_register_region vgic_v3_rd_registers[] = {
  // ...
}
```


- vgic_mmio_write_its_baser
  - vgic_sanitise_its_baser : 对于写入的寄存器数值进行检


- vgic_v3_set_attr
  - vgic_v3_attr_regs_access
    - vgic_v3_redist_uaccess


设备的定义:
```c
struct kvm_device_ops kvm_arm_vgic_v3_ops = {
	.name = "kvm-arm-vgic-v3",
	.create = vgic_create,
	.destroy = vgic_destroy,
	.set_attr = vgic_v3_set_attr,
	.get_attr = vgic_v3_get_attr,
	.has_attr = vgic_v3_has_attr,
};
```

- kvm_arch_init
  - init_subsystems
    - kvm_vgic_hyp_init
      - vgic_v2_probe
      - vgic_v3_probe
        - kvm_registr_vgic_device


## [ARM® Interrupt Virtualization](http://events17.linuxfoundation.org/sites/events/files/slides/ARM_Interrupt_Virtualization_Przywara.pdf)

## [ARM Virtual Generic Interrupt Controller v2 (VGIC)](https://www.kernel.org/doc/html/latest/virt/kvm/devices/arm-vgic.html)

## [ARM Virtual Interrupt Translation Service (ITS)](https://www.kernel.org/doc/html/latest/virt/kvm/devices/arm-vgic-its.html)

## [骏的世界](http://www.lujun.org.cn/?tag=gic)

## [ARM GICv3 ITS介绍及代码分析](https://blog.csdn.net/yhb1047818384/article/details/89061672)

## [ARM GICv3中断控制器](https://blog.csdn.net/yhb1047818384/article/details/86708769)

## [GIC 中断虚拟化](https://zhuanlan.zhihu.com/p/535997324)
