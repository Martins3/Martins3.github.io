## intremap

## 到底是否 enable 了？

```c
static const struct irq_domain_ops amd_ir_domain_ops = {
	.select = irq_remapping_select,
	.alloc = irq_remapping_alloc,
	.free = irq_remapping_free,
	.activate = irq_remapping_activate,
	.deactivate = irq_remapping_deactivate,
};
```

- `__alloc_irq_table`


检查 /proc/interrupts
```txt
 PIN:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Posted-interrupt notification event
 NPI:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Nested posted-interrupt event
 PIW:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Posted-interrupt wakeup event
```

## 这个是做啥的哇
```c
static int iommu_setup_msi(struct amd_iommu *iommu)
{
	int r;

	r = pci_enable_msi(iommu->dev);
	if (r)
		return r;

	r = request_threaded_irq(iommu->dev->irq,
				 amd_iommu_int_handler,
				 amd_iommu_int_thread,
				 0, "AMD-Vi",
				 iommu);

	if (r) {
		pci_disable_msi(iommu->dev);
		return r;
	}

	return 0;
}
```

cat /sys/module/kvm_amd/parameters/avic


## 似乎 interrupt remapping 是默认启动的

```txt
🤒  dmesg | grep AMD-Vi
[    0.129050] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR0, rdevid:160
[    0.129051] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR1, rdevid:160
[    0.129052] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR2, rdevid:160
[    0.129052] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR3, rdevid:160
[    0.129053] AMD-Vi: Using global IVHD EFR:0x246577efa2254afa, EFR2:0x0
[    0.514728] pci 0000:00:00.2: AMD-Vi: IOMMU performance counters supported
[    0.515027] pci 0000:00:00.2: AMD-Vi: Found IOMMU cap 0x40
[    0.515028] AMD-Vi: Extended features (0x246577efa2254afa, 0x0): PPR NX GT [5] IA GA PC GA_vAPIC
[    0.515031] AMD-Vi: Interrupt remapping enabled
[    0.515270] AMD-Vi: Virtual APIC enabled
```

- amd_iommu_xt_mode 这是什么东西?

## intel 中测试到的

```txt
PIN:      15536          1       3904          1      19275          3       1119          0        120          2        158          0         16          5          3          3          4          2         44          9         20          9          8          3          7         16         15         20         13          4          4          2   Posted-interrupt notification event
NPI:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Nested posted-interrupt event
PIW:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Posted-interrupt wakeup event
```

## 的确，普通设备也是会使用 posted-interrupt 的

```txt
PIN:        106          3         34          2         22          1         24          2         10          4         24          3         29         11         16          4         24         26         33        400         11         22         11         19        133         18         14         19         13          8          8          4   Posted-interrupt notification event
NPI:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Nested posted-interrupt event
PIW:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0   Posted-interrupt wakeup event
```
