# 求求了，彻底搞清楚这个问题

https://www.intel.com/content/www/us/en/docs/programmable/683140/22-4-8-0-0/msi-x.html
https://www.intel.com/content/www/us/en/docs/programmable/683686/20-4/msi-x-capability-structure.html

rg msix 一下，将 msix 的全部都整理到这里

## MSI-X 基本结构
<!-- 7b6d4987-2f13-4a54-938e-e40b2208d37f -->

在 bar 的中存储两个东西的地址:
1. MSI-X table : table 中每一个项目就是 msi address 和 msi data ，CPU 通过写这个来控制中断亲和性
2. PBA (pending bit array)

设备在需要触发某个向量时：
- 从 MSI-X table 取出该条目的 Address/Data
- 向该地址执行一次 PCIe Memory Write
- LAPIC（或 IOMMU/Posted-Interrupt 逻辑）接收并投递中断

如果直通一个 nvme 到虚拟机中， info mtree 可以观察到

```txt
      00000000c0347000-00000000c0347fff (prio 1, i/o): virtio-scsi-pci-msix
        00000000c0347000-00000000c034706f (prio 0, i/o): msix-table
        00000000c0347800-00000000c0347807 (prio 0, i/o): msix-pba

      0000004800040000-0000004800047fff (prio 1, i/o): 0000:c8:00.0 base BAR 0
        0000004800040000-0000004800047fff (prio 0, i/o): 0000:c8:00.0 BAR 0
          0000004800040000-0000004800047fff (prio 0, ramd): 0000:c8:00.0 BAR 0 mmaps[0]
        0000004800043000-0000004800043fff (prio 0, i/o): msix-table
```
无论是否开启了 posted interrupt 都是没有 msix-pba 的，这非常合理的:
```txt
    0000004800068000-000000480006bfff (prio 1, i/o): 0000:03:00.0 base BAR 0
      0000004800068000-000000480006bfff (prio 0, i/o): 0000:03:00.0 BAR 0
        0000004800068000-000000480006bfff (prio 0, ramd): 0000:03:00.0 BAR 0 mmaps[0]
      000000480006b000-000000480006b08f (prio 0, i/o): msix-table
```


参考 qemu
hw/pci/msix.c
```c
/*
 * Make PCI device @dev MSI-X capable
 * @nentries is the max number of MSI-X vectors that the device support.
 * @table_bar is the MemoryRegion that MSI-X table structure resides.
 * @table_bar_nr is number of base address register corresponding to @table_bar.
 * @table_offset indicates the offset that the MSI-X table structure starts with
 * in @table_bar.
 * @pba_bar is the MemoryRegion that the Pending Bit Array structure resides.
 * @pba_bar_nr is number of base address register corresponding to @pba_bar.
 * @pba_offset indicates the offset that the Pending Bit Array structure
 * starts with in @pba_bar.
 * Non-zero @cap_pos puts capability MSI-X at that offset in PCI config space.
 * @errp is for returning errors.
 *
 * Return 0 on success; set @errp and return -errno on error:
 * -ENOTSUP means lacking msi support for a msi-capable platform.
 * -EINVAL means capability overlap, happens when @cap_pos is non-zero,
 * also means a programming error, except device assignment, which can check
 * if a real HW is broken.
 */
int msix_init(struct PCIDevice *dev, unsigned short nentries,
              MemoryRegion *table_bar, uint8_t table_bar_nr,
              unsigned table_offset, MemoryRegion *pba_bar,
              uint8_t pba_bar_nr, unsigned pba_offset, uint8_t cap_pos,
              Error **errp)
```

## 调试一个小问题

为什么 msi 需要做这个验证:

dev->no_64bit_msi : 当 pci config 中的 PCI_MSI_FLAGS 没有 `PCI_MSI_FLAGS_64BIT` 的时候设置上，
表示这个设备仅仅支持 32bit 的 msi ( msi 也是分 32 bit 和 64bit 的，真奇怪啊)
```c
static int msi_verify_entries(struct pci_dev *dev)
{
	struct msi_desc *entry;

	if (!dev->no_64bit_msi)
		return 0;

  // 走到这里，说明仅仅支持 32bit msi ，那么 entry 就不可以配置 64bit
	msi_for_each_desc(entry, &dev->dev, MSI_DESC_ALL) {
		if (entry->msg.address_hi) {
			pci_err(dev, "arch assigned 64-bit MSI address %#x%08x but device only supports 32 bits\n",
				entry->msg.address_hi, entry->msg.address_lo);
			break;
		}
	}
	return !entry ? 0 : -EIO;
}
```
问题是，msi entry 是谁来配置的来着?

## msi
根据 https://habr.com/ru/post/501660/ 告诉如何关闭 pci 的 msi，默认情况下，nvme 就是使用 msi 的
https://www.kernel.org/doc/html/latest/PCI/msi-howto.html : 最详细的文档了
- [ ] 应该 msi 应该是可以直接绕过 ioapic 的
- [ ] pci_alloc_irq_vectors 申请 pci 的入口，为什么最后到达了 msi 的，看漏了其他的入口

- [ ] 键盘等各种 virq 其实是 acpi 决定的，那么，当 nvme 分配的 virq=24 是靠什么让
CPU 知道当 24 号引脚拉高的时候是 msi 中断啊, 从原则上，这是物理决定的才对啊
  - 应该是就是 cpu 了
  - [ ] 感觉已经话费了很长时间了，但是 `__irq_msi_compose_msg` 写入的数据实际上和想想的不一样
- [ ] 当 msi disable 掉，那么 nvme 设备如何通过 pci bridge 实现中断通知


- msix_capability_init : 狮子书 P130
  - msix_map_region : 获取 table 的位置
  - msix_setup_entries : 初始化 msix_entry
    - `list_add_tail(&entry->list, dev_to_msi_list(&dev->dev));` : 将新创建的 msix_entry 加入到 entry 中间
  - pci_msi_setup_msi_irqs
  - msi_domain_alloc_irqs
    - `ops->domain_alloc_irqs` => `__msi_domain_alloc_irqs`
      - irq_domain_alloc_descs : 分配 desc，同时分配 virq 也就是 linux irq
      - irq_domain_activate_irq
        - `__irq_domain_activate_irq`
          - `__irq_domain_activate_irq(irqd->parent_data, reserve)` : 会将 parent 调用一下，也就是 vector
            - x86_vector_activate
          - msi_domain_activate
            - `data->chip->irq_write_msi_msg` 的注册者真不少，但是测试显示只有从 nvme 的路径下 pci_msi_domain_write_msg 被调用而已
              - irq_chip_compose_msi_msg
                - irq_chip_compose_msi_msg
                  - `__irq_msi_compose_msg`
              - pci_msi_domain_write_msg
      - arch_setup_msi_irqs : 这是一个空函数
  - msix_program_entries

从 /sys/devices/pci0000:00/0000.00.04.0/msi_irqs 看，分配了 24 25 26 三个


pci_irq_vector : 获取 virq
device::msi_list 中间获取 `msi_desc->irq`
msix_setup_entries 创建了 msi_desc

- nvme_reset_work
  - nvme_pci_enable => x86_vector_alloc_irqs : 分配 irq_desc
  - nvme_pci_configure_admin_queue => apic_update_vector : 设置 irq_desc

- [ ] msi 还是可以设置 cpu affinity 的
  - [ ] 显示是必须的，因为是直接注入到具体的 cpu 的 lapic 中间的，但是
  - [ ] ioapic 应该也是存在这个问题



```c
/**
 * msi_msg - Representation of a MSI message
 * @address_lo:   Low 32 bits of msi message address
 * @arch_addrlo:  Architecture specific shadow of @address_lo
 * @address_hi:   High 32 bits of msi message address
 *      (only used when device supports it)
 * @arch_addrhi:  Architecture specific shadow of @address_hi
 * @data:   MSI message data (usually 16 bits)
 * @arch_data:    Architecture specific shadow of @data
 */
struct msi_msg {
  union {
    u32     address_lo;
    arch_msi_msg_addr_lo_t  arch_addr_lo;
  };
  union {
    u32     address_hi;
    arch_msi_msg_addr_hi_t  arch_addr_hi;
  };
  union {
    u32     data;
    arch_msi_msg_data_t arch_data;
  };
};
```

[MSI 重点参考](https://www.programmersought.com/article/61643546638/)

```txt
➜  vn git:(master) ✗ sudo cat /proc/iomem | grep apic -i
[sudo] password for maritns3:
  fec00000-fec003ff : IOAPIC 0
fee00000-fee00fff : Local APIC
```
`__irq_msi_compose_msg` 中间组装的 Message Address 就正好是 fee00000

## 可以直接操作 msi 的地址
- __pci_write_msi_msg
    - 看看在存在 IOMMU 的时候和不存在 IOMMU 的时候，填充的内容和填充的路径的差别
    - 太酷了
    - 原来是需要特殊的支持才可以啊
    - https://stackoverflow.com/questions/71352710/how-to-use-kallsyms-lookup-name-in-latest-kernel-versions-for-building-modules
    - https://stackoverflow.com/questions/75950455/modpost-kallsyms-lookup-name-is-undefined
    - https://lore.kernel.org/all/20200221114404.14641-4-will@kernel.org/T/#rc4d887137e35a054df4bc62a710467920ad48c20

## 仔细看看这个，简直不要太简单
https://stackoverflow.com/questions/57804511/question-about-message-signaled-interrupts-msi-on-x86-lapic-system

## MSI
Generic MSIs : https://en.wikipedia.org/wiki/Message_Signaled_Interrupts

linux/drivers/pci/msi.c

use nvme as example:
- nvme_pci_enable
  - pci_alloc_irq_vectors
    - pci_alloc_irq_vectors_affinity
      - `__pci_enable_msix_range`
        - `__pci_enable_msix`
          - **msix_capability_init** : [^3] page 130

[ARM](https://elinux.org/images/8/8c/Zyngier.pdf)

## todo
1. 以 x86 为例，想知道从 architecture 的 entry.S 触发 到指向对应的 handler 的过程是怎样的 ?
2. 中断控制器很简单，其大概是怎么实现的 ?
    1. CPU 提供给 interrupt controller 的接口是什么 ?
        1. 我(CPU) TM 怎么知道这一个信号是来自于 interrupt controller 的 ?
        2. 是不是 CPU 专门为其提供了引脚，各种 CPU 的引脚的数量是什么 ?
        3. IC 的输入输出的引脚的数量是多少 ?
    2. 当 IC 可以进行层次架构之后，

3. 为了多核，是不是也是需要进一步修改 IC 来支持。
    1. affinity 提供硬件支持

## wowotech 相关的内容

1. 中断描述符中应该会包括底层 irq chip 相关的数据结构，linux kernel 中把这些数据组织在一起，形成`struct irq_data`
2. 中断有两种形态，一种就是直接通过 signal 相连，用电平或者边缘触发。另外一种是基于消息的，被称为 MSI (Message Signaled Interrupts)。
3. Interrupt controller 描述符（struct irq_chip）包括了若干和具体 Interrupt controller 相关的`callback`函数

## Documentation/PCI/msi-howto.rst

当 nvme_setup_irqs 正式注册的时候，是的，没有看错，是根据 nvme 的队列数量来确定的:
```c
	return pci_alloc_irq_vectors_affinity(pdev, 1, irq_queues,
			      PCI_IRQ_ALL_TYPES | PCI_IRQ_AFFINITY, &affd);
```

irq_domain_deactivate_irq

应该在 pci emulator 中实现一个 qemu 的，现在似乎不太行。


## 看中断路由表的填充，直接看 irq affinity 的配置就可以了

```c
int msi_domain_set_affinity(struct irq_data *irq_data,
			    const struct cpumask *mask, bool force)
{
	struct irq_data *parent = irq_data->parent_data;
	struct msi_msg msg[2] = { [1] = { }, };
	int ret;

	ret = parent->chip->irq_set_affinity(parent, mask, force);
	if (ret >= 0 && ret != IRQ_SET_MASK_OK_DONE) {
		BUG_ON(irq_chip_compose_msi_msg(irq_data, msg));
		msi_check_level(irq_data->domain, msg);
		irq_chip_write_msi_msg(irq_data, msg);
	}

	return ret;
}
```

看 msi_set_affinity 就可以了

irq_msi_update_msg(irqd, cfg);

```c
static void irq_msi_update_msg(struct irq_data *irqd, struct irq_cfg *cfg)
{
	struct msi_msg msg[2] = { [1] = { }, };

	__irq_msi_compose_msg(cfg, msg);
	irq_data_get_irq_chip(irqd)->irq_write_msi_msg(irqd, msg);
}
```
当存在 iommu 的时候，
msi_set_affinity 就不会被调用了

### amd iommu
```c
static void irte_ga_set_affinity(struct amd_iommu *iommu, void *entry, u16 devid, u16 index,
				 u8 vector, u32 dest_apicid)
{
	struct irte_ga *irte = (struct irte_ga *) entry;

	if (!irte->lo.fields_remap.guest_mode) {
		irte->hi.fields.vector = vector;
		irte->lo.fields_remap.destination =
					APICID_TO_IRTE_DEST_LO(dest_apicid);
		irte->hi.fields.destination =
					APICID_TO_IRTE_DEST_HI(dest_apicid);
		modify_irte_ga(iommu, devid, index, irte);
	}
}
```

### intel iommu

```txt
@[
    intel_ir_set_affinity+5
    msi_domain_set_affinity+77
    irq_do_set_affinity+210
    irq_set_affinity_locked+312
    irq_set_affinity+63
    write_irq_affinity.isra.0+264
    proc_reg_write+87
    vfs_write+267
    ksys_write+110
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

sudo bpftrace -e 'kfunc:vmlinux:intel_ir_set_affinity { printf("%s\n", str(args->data->parent_data->chip->name)); }'
可以捕获到为 APIC ，APIC 需要处理 vector 预留相关的事情
```c
static int
intel_ir_set_affinity(struct irq_data *data, const struct cpumask *mask,
		      bool force)
{
	struct irq_data *parent = data->parent_data;
	struct irq_cfg *cfg = irqd_cfg(data);
	int ret;

	ret = parent->chip->irq_set_affinity(parent, mask, force);
	if (ret < 0 || ret == IRQ_SET_MASK_OK_DONE)
		return ret;

	intel_ir_reconfigure_irte(data, false);
	/*
	 * After this point, all the interrupts will start arriving
	 * at the new destination. So, time to cleanup the previous
	 * vector allocation.
	 */
	vector_schedule_cleanup(cfg);

	return IRQ_SET_MASK_OK_DONE;
}
```

# 将 msi 禁用之后

添加参数 pci=nomsi

```txt
➜  irqs cat /proc/interrupts
           CPU0       CPU1
  0:         23          0   IO-APIC   2-edge      timer
  1:          9          0   IO-APIC   1-edge      i8042
  4:         20          0   IO-APIC   4-edge      ttyS0
  6:          3          0   IO-APIC   6-edge      floppy
  8:          0          1   IO-APIC   8-edge      rtc0
  9:          0          0   IO-APIC   9-fasteoi   acpi
 10:          0       8029   IO-APIC  10-fasteoi   virtio10, virtio7, virtio0, virtio9, virtio2, nvme1q0, nvme1q1, virtio6, virtio4
 11:      58605          0   IO-APIC  11-fasteoi   virtio5, virtio8, virtio1, virtio3, nvme2q0, nvme2q1, nvme0q0, nvme0q1, xhci-hcd:usb1
 12:          0        125   IO-APIC  12-edge      i8042
 14:          0          0   IO-APIC  14-edge      ata_piix
 15:          0          0   IO-APIC  15-edge      ata_piix
NMI:          0          0   Non-maskable interrupts
LOC:      91428      88979   Local timer interrupts
SPU:          0          0   Spurious interrupts
PMI:          0          0   Performance monitoring interrupts
IWI:          0          0   IRQ work interrupts
RTR:          0          0   APIC ICR read retries
RES:       5626       4092   Rescheduling interrupts
CAL:      15916      31525   Function call interrupts
TLB:         15         25   TLB shootdowns
TRM:          0          0   Thermal event interrupts
THR:          0          0   Threshold APIC interrupts
DFR:          0          0   Deferred Error APIC interrupts
MCE:          0          0   Machine check exceptions
MCP:          8          8   Machine check polls
HYP:          1          1   Hypervisor callback interrupts
ERR:          0
MIS:          0
PIN:          0          0   Posted-interrupt notification event
NPI:          0          0   Nested posted-interrupt event
PIW:          0          0   Posted-interrupt wakeup event
```

是的，所有的 PCI 的中断都压缩到一起了。

/sys/kernel/debug/irq/irqs/10
```txt
handler:  handle_fasteoi_irq
device:   (null)
status:   0x00000100
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x1f403200
            IRQD_LEVEL
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_SET
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_CAN_RESERVE
            IRQD_DEFAULT_TRIGGER_SET
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 1
effectiv: 1
pending:
domain:  IO-APIC-0
 hwirq:   0xa
 chip:    IO-APIC
  flags:   0x410
             IRQCHIP_SKIP_SET_WAKE
 parent:
    domain:  VECTOR
     hwirq:   0xa
     chip:    APIC
      flags:   0x0
     Vector:    33
     Target:     1
     move_in_progress: 0
     is_managed:       0
     can_reserve:      1
     has_reserved:     0
     cleanup_pending:  0
```

```txt
➜  irqs cat 11
handler:  handle_fasteoi_irq
device:   (null)
status:   0x00000100
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x1f402200
            IRQD_LEVEL
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_CAN_RESERVE
            IRQD_DEFAULT_TRIGGER_SET
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 0-1
effectiv: 0
pending:
domain:  IO-APIC-0
 hwirq:   0xb
 chip:    IO-APIC
  flags:   0x410
             IRQCHIP_SKIP_SET_WAKE
 parent:
    domain:  VECTOR
     hwirq:   0xb
     chip:    APIC
      flags:   0x0
     Vector:    32
     Target:     0
     move_in_progress: 0
     is_managed:       0
     can_reserve:      1
     has_reserved:     0
     cleanup_pending:  0
```

真的非常惨，两个 irq ，分别指向一个 CPU 。

## kvm 的观测

```txt
Samples: 137K of event 'kvm:kvm_apicv_accept_irq', 1 Hz, Event count (approx.): 77050 lost: 0/0 drop: 0/0
Overhead  Trace output
  80.18%  apicid 0 vec 32 (Fixed|level)
  16.48%  apicid 1 vec 251 (Fixed|edge)
   1.52%  apicid 1 vec 236 (Fixed|edge)
   0.99%  apicid 0 vec 236 (Fixed|edge)
   0.74%  apicid 0 vec 251 (Fixed|edge)
   0.05%  apicid 0 vec 253 (Fixed|edge)
   0.02%  apicid 1 vec 253 (Fixed|edge)
   0.00%  apicid 1 vec 33 (Fixed|level)
```

```txt
Samples: 63K of event 'kvm:kvm_msr', 1 Hz, Event count (approx.): 18059 lost: 0/0 drop: 0/0
Overhead  Trace output
  60.78%  msr_write 830 = 0x1000000fb
   3.06%  msr_write 830 = 0xfb
   0.18%  msr_write 830 = 0xfd
   0.11%  msr_read 3b = 0x0
   0.07%  msr_write 830 = 0x1000000fd
   0.02%  msr_write 6e0 = 0x94ba753656c
   0.02%  msr_write 6e0 = 0x94c00b21576
   0.02%  msr_write 6e0 = 0x94c5a108cdc
```

```txt
Samples: 1M of event 'kvm:kvm_mmio', 1 Hz, Event count (approx.): 667112 lost: 0/0 drop: 0/0
Overhead  Trace output
  10.00%  mmio unsatisfied-read len 1 gpa 0x380040015000 val 0x0
  10.00%  mmio read len 1 gpa 0x380040015000 val 0x0
  10.00%  mmio read len 1 gpa 0x380040021000 val 0x0
  10.00%  mmio read len 4 gpa 0xfebd0044 val 0x0
  10.00%  mmio unsatisfied-read len 1 gpa 0x380040005000 val 0x0
  10.00%  mmio unsatisfied-read len 1 gpa 0x38004000d000 val 0x0
  10.00%  mmio unsatisfied-read len 1 gpa 0x380040021000 val 0x0
  10.00%  mmio unsatisfied-read len 4 gpa 0xfebd0044 val 0x0
```

```txt
Samples: 266K of event 'kvm:kvm_set_irq', 1 Hz, Event count (approx.): 196651 lost: 0/0 drop: 0/0
Overhead  Trace output
  45.54%  gsi 11 level 1 source 0
  45.53%  gsi 11 level 0 source 0
   8.93%  gsi 0 level 0 source 0
   0.00%  gsi 10 level 0 source 0
   0.00%  gsi 10 level 1 source 0
```

kvm:kvm_msi_set_irq 这个完全没有。

- [ ] mmio 的频率极高

## msi 基本数据结构
<!-- 3830478d-54f7-4a3b-81aa-07764b85f6bd -->

```c
struct kvm_irq_routing_msi {
	__u32 address_lo;
	__u32 address_hi;
	__u32 data;
	union {
		__u32 pad;
		__u32 devid;
	};
};
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

address 的内容:
```txt
63                         32 31        20 19  12 11        4 3   2 1 0
+----------------------------+------------+------+----------+-----+--+
|          Reserved          | Dest ID    | Rsvd | 0xFEE    | DM  |RH|
+----------------------------+------------+------+----------+-----+--+
```

| 位域    | 含义                                      |
| ------- | --------------------------------------    |
| `0xFEE` | 固定值，表示 LAPIC MSI window             |
| Dest ID | **目标 APIC ID**（或 logical ID）         |
| DM      | Destination Mode（0=physical，1=logical） |
| RH      | Redirection Hint（一般不用）              |


data 中的内容:
```txt
15        8 7          0
+----------+------------+
| Delivery |   Vector   |
+----------+------------+
```
| 位域          | 含义                                |
| ------------- | ----------------------------------- |
| Vector        | **中断向量号（IDT index）**         |
| Delivery Mode | Fixed / Lowest Priority / NMI / etc |

这个内容让 CPU 填充，然后设备触发中断的时候，就会知道写入到
到哪里，这里我感觉有点奇怪，不是写入到 APIC 的 MMIO 空间吗?
但是似乎现在又是不是这样的。

## sysfs 中是可以观察 msi 的分配的

```txt
/sys/devices/pci0000:00/0000:00:17.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1c.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1c.0/0000:04:00.0/msi_irqs
/sys/devices/pci0000:00/0000:00:01.0/msi_irqs
/sys/devices/pci0000:00/0000:00:14.3/msi_irqs
/sys/devices/pci0000:00/0000:00:16.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1f.3/msi_irqs
/sys/devices/pci0000:00/0000:00:1a.0/0000:03:00.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1a.0/msi_irqs
/sys/devices/pci0000:00/0000:00:06.0/0000:02:00.0/msi_irqs
/sys/devices/pci0000:00/0000:00:06.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1d.0/0000:06:00.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1d.0/msi_irqs
/sys/devices/pci0000:00/0000:00:02.0/msi_irqs
/sys/devices/pci0000:00/0000:00:14.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1c.2/0000:05:00.0/msi_irqs
/sys/devices/pci0000:00/0000:00:1c.2/msi_irqs
```

cd /sys/devices/pci0000:00/0000:00:1d.0/0000:06:00.0/msi_irqs 可以观察到:
```txt
139  172  173  174  175  176  177  178  179  180  181  182  183  184  185  186  187
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
