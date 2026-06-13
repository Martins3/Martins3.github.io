## 似乎，中断经过 IOMMU 似乎是不受内核参数 iommu=off 控制的

似乎是只要检测到了 IOMMU，就会使用，也许这个只是 intel 上的问题，AMD 也是可以测试下。

## 为什么关闭 iommu 之后，domain 的名称还是 INTEL-IR-0-13

至少在虚拟机中，就是只有两个 domain 的
```txt
➜  irqs cat 58
handler:  handle_edge_irq
device:   0000:00:0b.0
status:   0x00000000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x19600200
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_MANAGED
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 7
effectiv: 7
pending:
domain:  PCI-MSIX-0000:00:0b.0-12
 hwirq:   0x8
 chip:    PCI-MSIX-0000:00:0b.0
  flags:   0x430
             IRQCHIP_SKIP_SET_WAKE
             IRQCHIP_ONESHOT_SAFE
 parent:
    domain:  VECTOR
     hwirq:   0x3a
     chip:    APIC
      flags:   0x0
     Vector:    34
     Target:     7
     move_in_progress: 0
     is_managed:       1
     can_reserve:      0
     has_reserved:     0
     cleanup_pending:  0
```

## cat /proc/interrupts 在 vfio 绑定前后的变化
之前是 8 个中断，之后是 8 个 msi

```txt
nvme1q1
nvme1q2
nvme1q3
nvme1q4
nvme1q5
nvme1q6
nvme1q7
nvme1q8
```

之后

```txt
vfio-msix[0](0000:03:00.0)
nvme2q0
nvme1q0
vfio-msix[1](0000:03:00.0)
vfio-msix[2](0000:03:00.0)
vfio-msix[3](0000:03:00.0)
vfio-msix[4](0000:03:00.0)
vfio-msix[5](0000:03:00.0)
vfio-msix[6](0000:03:00.0)
vfio-msix[7](0000:03:00.0)
vfio-msix[8](0000:03:00.0)
```

为什么 irq number 不是连续的，有趣的东西.

## /sys/kernel/debug/irq/domains

```txt
 default
 DMAR-MSI
 INTEL-IR-0-13
 IO-APIC-2
 IR-PCI-MSI-0000:00:01.0-11
 IR-PCI-MSI-0000:00:06.0-11
 IR-PCI-MSI-0000:00:14.0-11
 IR-PCI-MSI-0000:00:16.0-11
 IR-PCI-MSI-0000:00:17.0-11
 IR-PCI-MSI-0000:00:1a.0-11
 IR-PCI-MSI-0000:00:1c.0-11
 IR-PCI-MSI-0000:00:1c.2-11
 IR-PCI-MSI-0000:00:1d.0-11
 IR-PCI-MSI-0000:00:1f.3-11
 IR-PCI-MSI-0000:01:00.0-11
 IR-PCI-MSIX-0000:00:14.3-12
 IR-PCI-MSIX-0000:02:00.0-12
 IR-PCI-MSIX-0000:03:00.0-12
 IR-PCI-MSIX-0000:05:00.0-12
 IR-PCI-MSIX-0000:06:00.0-12
'\_SB.GPI0'
'\_SB.PC00.SBUS'
 VECTOR
```


## 138 : nvme2q1

```txt
handler:  handle_edge_irq
device:   0000:06:00.0
status:   0x00004000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x31608200
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_MOVE_PCNTXT
            IRQD_AFFINITY_MANAGED
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 0-1
effectiv: 1
pending:
domain:  IR-PCI-MSIX-0000:06:00.0-12
 hwirq:   0x1
 chip:    IR-PCI-MSIX-0000:06:00.0
  flags:   0x430
             IRQCHIP_SKIP_SET_WAKE
             IRQCHIP_ONESHOT_SAFE
 parent:
    domain:  INTEL-IR-0-13
     hwirq:   0x230000
     chip:    INTEL-IR
      flags:   0x0
     parent:
        domain:  VECTOR
         hwirq:   0x8a
         chip:    APIC
          flags:   0x0
         Vector:    34
         Target:     1
         move_in_progress: 0
         is_managed:       1
         can_reserve:      0
         has_reserved:     0
         cleanup_pending:  0
```

- 这里的三个 domain 居然完全相同

### domains

关闭 iommu :
```txt
-r--r--r-- 1 root root 0 Nov 23 11:25 default
-r--r--r-- 1 root root 0 Nov 23 11:25 IO-APIC-0
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:02.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:03.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:04.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:05.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:06.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:07.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:09.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:0a.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 PCI-MSIX-0000:00:0b.0-12
-r--r--r-- 1 root root 0 Nov 23 11:25 VECTOR
```

打开 iommu :
```txt
drwxr-xr-x 2 root root 0 Nov 23 23:35  .
drwxr-xr-x 4 root root 0 Nov 23 23:35  ..
-r--r--r-- 1 root root 0 Nov 23 23:35  default
-r--r--r-- 1 root root 0 Nov 23 23:35  DMAR-MSI
-r--r--r-- 1 root root 0 Nov 23 23:35  INTEL-IR-0-13
-r--r--r-- 1 root root 0 Nov 23 23:35  IO-APIC-0
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSI-0000:00:1f.2-11
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:01.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:02.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:03.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:04.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:05.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:06.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:08.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:09.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35  IR-PCI-MSIX-0000:00:0a.0-12
-r--r--r-- 1 root root 0 Nov 23 23:35 '\_SB.PCI0.SFB'
-r--r--r-- 1 root root 0 Nov 23 23:35  VECTOR
```

# 回忆下 iommu 中断是如何进行的!

## amd
```c
static void iommu_enable_ga(struct amd_iommu *iommu)
{
#ifdef CONFIG_IRQ_REMAP
	switch (amd_iommu_guest_ir) {
	case AMD_IOMMU_GUEST_IR_VAPIC:
	case AMD_IOMMU_GUEST_IR_LEGACY_GA:
		iommu_feature_enable(iommu, CONTROL_GA_EN);
		iommu->irte_ops = &irte_128_ops;
		break;
	default:
		iommu->irte_ops = &irte_32_ops;
		break;
	}
#endif
}
```

```txt
[    3.788151] Call Trace:
[    3.788151]  dump_stack+0x66/0x8b
[    3.788151]  irte_ga_set_affinity+0x64/0x80
[    3.788151]  irq_remapping_activate+0x6e/0x80
[    3.788151]  __irq_domain_activate_irq+0x46/0x90
[    3.788151]  ? __irq_get_desc_lock+0x51/0x80
[    3.788151]  __irq_domain_activate_irq+0x7d/0x90
[    3.788151]  ? irq_set_msi_desc_off+0x5a/0x90
[    3.788151]  irq_domain_activate_irq+0x25/0x40
[    3.788151]  msi_domain_alloc_irqs+0x25a/0x300
[    3.788151]  native_setup_msi_irqs+0x54/0x90
[    3.788151]  __pci_enable_msix_range+0x3ed/0x5e0
[    3.788151]  pci_alloc_irq_vectors_affinity+0x91/0xf0
[    3.788151]  xhci_run+0x11a/0x5a0
[    3.788151]  usb_add_hcd+0x396/0x8a0
[    3.788151]  usb_hcd_pci_probe+0x271/0x410
[    3.788151]  ? __switch_to_asm+0x41/0x70
[    3.788151]  xhci_pci_probe+0x27/0x1a0
[    3.788151]  local_pci_probe+0x42/0xa0
[    3.788151]  work_for_cpu_fn+0x16/0x20
[    3.788151]  process_one_work+0x195/0x3d0
[    3.788151]  worker_thread+0x1cf/0x390
[    3.788151]  ? process_one_work+0x3d0/0x3d0
[    3.788151]  kthread+0x113/0x130
[    3.788151]  ? kthread_create_worker_on_cpu+0x70/0x70
[    3.788151]  ret_from_fork+0x22/0x40
[    3.874765] CPU: 0 PID: 177 Comm: kworker/0:2 Not tainted 4.19.90 #1
```

## qemu 的分析角度理解
```c
    if (iommu->ga_enabled) {
        ret = amdvi_int_remap_ga(iommu, origin, translated, dte, irq, sid);
    } else {
        ret = amdvi_int_remap_legacy(iommu, origin, translated, dte, irq, sid);
    }
```

```txt
	amd_iommu_intr=	[HW,X86-64]
			Specifies one of the following AMD IOMMU interrupt
			remapping modes:
			legacy     - Use legacy interrupt remapping mode.
			vapic      - Use virtual APIC mode, which allows IOMMU
			             to inject interrupts directly into guest.
			             This mode requires kvm-amd.avic=1.
			             (Default when IOMMU HW support is present.)

```

## [ ] amd_iommu_intr=legacy 可以解决吗?

## [ ] itre 的物理地址在哪里
不可以，还可以修改

## [ ] 如果使用了 iommu ，但是中断来源是 msi ，会出现问题吗？

## 尝试一下 deactive 试试吧

- irq_domain_deactivate_irq

- __irq_domain_deactivate_irq --> irte_ga_deactivate

- 一个 irte

## 找到 ITRE table ，并且遍历

这里只是 basic mode, 后面还存在升级模式，
关于他们的差别，可以参考 QEMU 的函数 `__amdvi_int_remap_msi`，也就是 GAEn

## 思考一个简单问题，直通设备为什么需要 interrupt remapping

但是 intel 可以 disable 掉这个
```txt
	intremap=	[X86-64,Intel-IOMMU,EARLY]
			on	enable Interrupt Remapping (default)
			off	disable Interrupt Remapping
			nosid	disable Source ID checking
			no_x2apic_optout
				BIOS x2APIC opt-out request will be ignored
			nopost	disable Interrupt Posting
			posted_msi
				enable MSIs delivered as posted interrupts
```

## https://blog.kernel.love/interrupt-remapping.html

msi 的中断

- https://zhuanlan.zhihu.com/p/372385232 ：分析了初始化的过程

- 在 guest 中以为持有了某一个设备，那么如何才可以真正的使用

这两个函数正好描述了 MSI 的样子:
- `__irq_msi_compose_msg` 是普通的 irq 组装的样子
- linux/drivers/iommu/intel/irq_remapping.c 中的 fill_msi_msg

- [ ] 是不是只有在直通的时候，interrupt remapping 才需要，似乎打开 IOMMU 和使用 interrupt remapping 不是总是绑定的?

```c
static const struct irq_domain_ops intel_ir_domain_ops = {
    .select = intel_irq_remapping_select,
    .alloc = intel_irq_remapping_alloc,
    .free = intel_irq_remapping_free,
    .activate = intel_irq_remapping_activate,
    .deactivate = intel_irq_remapping_deactivate,
};
```

在其中 intel_irq_remapping_alloc 的位置将会来创建 IRTE


在 intel_setup_irq_remapping 中，调用 arch_get_ir_parent_domain
```c
/* Get parent irqdomain for interrupt remapping irqdomain */
static inline struct irq_domain *arch_get_ir_parent_domain(void)
{
    return x86_vector_domain;
}
```

## https://blog.kernel.love/posted-interrupt.html

## 有趣

intremap=nosid

- https://www.reddit.com/r/linuxquestions/comments/8te134/what_do_nomodeset_intremapnosid_and_other_grub/
- https://www.greenbone.net/finder/vt/results/1.3.6.1.4.1.25623.1.0.870999
- https://serverfault.com/questions/1077297/ilo4-and-almalinux-centos8-do-not-work-properly

## 是否打开 iommu ，负责 msi 的 irq_chip 内容会不同的

```c
static struct irq_chip dmar_msi_controller = {
	.name			= "DMAR-MSI",
	.irq_unmask		= dmar_msi_unmask,
	.irq_mask		= dmar_msi_mask,
	.irq_ack		= irq_chip_ack_parent,
	.irq_set_affinity	= msi_domain_set_affinity,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_compose_msi_msg	= dmar_msi_compose_msg,
	.irq_write_msi_msg	= dmar_msi_write_msg,
	.flags			= IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_AFFINITY_PRE_STARTUP,
};
```

```c
static const struct msi_domain_template pci_msix_template = {
	.chip = {
		.name			= "PCI-MSIX",
		.irq_mask		= pci_irq_mask_msix,
		.irq_unmask		= pci_irq_unmask_msix,
		.irq_write_msi_msg	= pci_msi_domain_write_msg,
		.flags			= IRQCHIP_ONESHOT_SAFE,
	},
  // ... 省掉不太相关的定义
};
```

是不是在 intel 平台上，当启用 iommu 的时候，用 "DMAR-MSI" ，
而没有启用的时候，结果为 "PCI-MSIX"


## [ ] iommu 也会影响 ioapic ，我实在是没有想到这些的
```c
static struct irq_chip ioapic_chip __read_mostly = {
	.name			= "IO-APIC",
	.irq_startup		= startup_ioapic_irq,
	.irq_mask		= mask_ioapic_irq,
	.irq_unmask		= unmask_ioapic_irq,
	.irq_ack		= irq_chip_ack_parent,
	.irq_eoi		= ioapic_ack_level,
	.irq_set_affinity	= ioapic_set_affinity,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_get_irqchip_state	= ioapic_irq_get_chip_state,
	.flags			= IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_AFFINITY_PRE_STARTUP,
};

static struct irq_chip ioapic_ir_chip __read_mostly = {
	.name			= "IR-IO-APIC",
	.irq_startup		= startup_ioapic_irq,
	.irq_mask		= mask_ioapic_irq,
	.irq_unmask		= unmask_ioapic_irq,
	.irq_ack		= irq_chip_ack_parent,
	.irq_eoi		= ioapic_ir_ack_level,
	.irq_set_affinity	= ioapic_set_affinity,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_get_irqchip_state	= ioapic_irq_get_chip_state,
	.flags			= IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_AFFINITY_PRE_STARTUP,
};
```
差别只有
```c
	.irq_eoi		= ioapic_ir_ack_level,
```

到底使用哪一个的差别居然是，但是居然也是合理的:
```c
	irq_data->chip = (domain->parent == x86_vector_domain) ?
			  &ioapic_chip : &ioapic_ir_chip;
```

这里可以看到 IO-APIC 的 parent 是 INTEL-IR-1-13 ，可以找到证据吗?

```txt
[root@nixos:/sys/kernel/debug/irq/irqs]# cat 8
handler:  handle_edge_irq
device:   (null)
status:   0x00004000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x1d408200
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_MOVE_PCNTXT
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_CAN_RESERVE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 0-31
effectiv: 10
pending:
domain:  IO-APIC-2
 hwirq:   0x8
 chip:    IR-IO-APIC
  flags:   0x410
             IRQCHIP_SKIP_SET_WAKE
 parent:
    domain:  INTEL-IR-1-13
     hwirq:   0x70000
     chip:    INTEL-IR
      flags:   0x0
     parent:
        domain:  VECTOR
         hwirq:   0x8
         chip:    APIC
          flags:   0x0
         Vector:    32
         Target:    10
         move_in_progress: 0
         is_managed:       0
         can_reserve:      1
         has_reserved:     0
         cleanup_pending:  0
```

如果对于 io-apic 设置中断 affinity ，那么会去修改 iommu 而不是 iommu 。
这都是完全合理的。

## 配置中断总是会通过 iommu 的
```txt
[    5.022341] Call Trace:
[    5.022343]  <TASK>
[    5.022799]  dump_stack_lvl+0x4a/0x80
[    5.022949]  intel_ir_set_affinity+0x2a/0xb0
[    5.023122]  ioapic_set_affinity+0x21/0x70
[    5.023286]  irq_do_set_affinity+0x154/0x180
[    5.023289]  irq_setup_affinity+0x8f/0xd0
[    5.023290]  irq_startup+0x120/0x140
[    5.023764]  __setup_irq+0x3dc/0x6a0
[    5.023765]  request_threaded_irq+0x10d/0x180
[    5.024079]  univ8250_setup_irq+0x152/0x190
[    5.024081]  serial8250_do_startup+0x2ab/0x880
[    5.024082]  uart_port_startup+0x120/0x280
[    5.024085]  uart_port_activate+0x47/0x70
[    5.024086]  tty_port_open+0x7f/0xd0
[    5.024087]  ? _raw_spin_unlock+0x23/0x40
[    5.024089]  uart_open+0x1e/0x30
[    5.024090]  tty_open+0x14c/0x710
[    5.024092]  chrdev_open+0xcc/0x230
[    5.024094]  ? __pfx_chrdev_open+0x10/0x10
[    5.024095]  do_dentry_open+0x202/0x550
[    5.024097]  path_openat+0xcfa/0x11f0
[    5.024098]  ? psi_group_change+0x168/0x400
[    5.024099]  do_filp_open+0xb3/0x160
[    5.024101]  do_sys_openat2+0xab/0xe0
[    5.024102]  __x64_sys_openat+0x6e/0xa0
[    5.024104]  do_syscall_64+0x43/0xf0
[    5.024110]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
[    5.024118] RIP: 0033:0x7f15926c99d8
```

```txt
[    0.957782] Call Trace:
[    0.957782]  <TASK>
[    0.957782]  dump_stack_lvl+0x4a/0x80
[    0.957782]  intel_ir_set_affinity+0x2a/0xb0
[    0.957782]  msi_domain_set_affinity+0x4d/0xc0
[    0.957782]  irq_do_set_affinity+0x154/0x180
[    0.957782]  irq_startup+0xd0/0x140
[    0.957782]  __setup_irq+0x3dc/0x6a0
[    0.957782]  request_threaded_irq+0x10d/0x180
[    0.957782]  ? __pfx_nvme_irq+0x10/0x10
[    0.957782]  pci_request_irq+0xb0/0x100
[    0.957782]  ? blk_queue_exit+0x12/0x50
[    0.957782]  ? __nvme_submit_sync_cmd+0xe3/0x170
[    0.957782]  queue_request_irq+0x6f/0x80
[    0.957782]  nvme_setup_io_queues+0x5c9/0x780
[    0.957782]  nvme_probe+0x61a/0x770
[    0.957782]  local_pci_probe+0x3f/0x90
[    0.957782]  pci_device_probe+0xc1/0x1e0
[    0.957782]  really_probe+0xbc/0x2c0
[    0.957782]  __driver_probe_device+0x73/0x120
[    0.957782]  driver_probe_device+0x1f/0xe0
[    0.957782]  __driver_attach_async_helper+0x53/0xb0
[    0.957782]  async_run_entry_fn+0x21/0xa0
[    0.957782]  process_one_work+0x138/0x2f0
[    0.957782]  worker_thread+0x2f5/0x420
[    0.957782]  ? __pfx_worker_thread+0x10/0x10
[    0.957782]  kthread+0xe3/0x110
[    0.957782]  ? __pfx_kthread+0x10/0x10
[    0.957782]  ret_from_fork+0x31/0x50
[    0.957782]  ? __pfx_kthread+0x10/0x10
[    0.957782]  ret_from_fork_asm+0x1b/0x30
[    0.957782]  </TASK>
```

```txt
[    7.057610] Call Trace:
[    7.057674]  <TASK>
[    7.057732]  dump_stack_lvl+0x4a/0x80
[    7.057826]  intel_ir_set_affinity+0x2a/0xb0
[    7.057935]  msi_domain_set_affinity+0x4d/0xc0
[    7.058048]  irq_do_set_affinity+0x154/0x180
[    7.058158]  irq_set_affinity_locked+0x10c/0x1b0
[    7.058278]  irq_set_affinity+0x3f/0x60
[    7.058377]  irq_affinity_proc_write+0xaf/0xd0
[    7.058492]  proc_reg_write+0x59/0xa0
[    7.058589]  vfs_write+0xef/0x420
[    7.058676]  ? __do_sys_newfstatat+0x4e/0x80
[    7.058788]  ? __fget_light+0x85/0x100
[    7.058886]  ksys_write+0x6f/0xf0
[    7.058972]  do_syscall_64+0x43/0xf0
[    7.059066]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
[    7.059195] RIP: 0033:0x7f3e71a79d5f
```

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
