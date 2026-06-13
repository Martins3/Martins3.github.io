## VECTOR 总是从 32 开始的

因为这是 idt

```txt
➜ /sys/kernel/debug/irq/domains cat VECTOR
name:   VECTOR
 size:   0
 mapped: 83
 flags:  0x00000103
Online bitmaps:        8
Global available:   1545
Global reserved:      12
Total allocated:      71
System: 39: 0-19,21,50,128,236,240-244,246-255
 | CPU | avl | man | mac | act | vectors
     0   193     6     6    9  32-35,37-38,40-41,48
     1   193     6     6    9  32-36,38-41
     2   193     6     6    9  32-40
     3   191     6     6   11  32-36,38-43
     4   195     6     6    7  32-38
     5   193     6     6    9  32-38,40-41
     6   192     6     6   10  32-33,35-38,40-43
     7   195     6     6    7  32-34,36-38,40
```

## 中断亲和性绑定需要触发一次中断才可以

设置 flags 的地方:
```txt
^C
  b'irq_set_affinity_locked'
  b'write_irq_affinity.isra.6'
  b'irq_affinity_list_proc_write'
  b'proc_reg_write'
  b'__vfs_write'
  b'vfs_write'
  b'ksys_write'
  b'__x64_sys_write'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
  b'[unknown]'
  b'[unknown]'
    1
```

```txt
  b'irq_matrix_alloc'
  b'irq_matrix_alloc'
  b'assign_vector_locked'
  b'apic_set_affinity'
  b'msi_set_affinity'
  b'irq_do_set_affinity'
  b'irq_move_masked_irq'
  b'__irq_move_irq' //
  b'apic_ack_irq'   // 在这里检查 flags
  b'apic_ack_edge'
  b'irq_chip_ack_parent'
  b'handle_edge_irq'
  b'do_IRQ'
  b'ret_from_intr'
  b'__softirqentry_text_start'
  b'irq_exit'
  b'smp_apic_timer_interrupt'
  b'apic_timer_interrupt'
  b'default_idle'
  b'arch_cpu_idle'
  b'default_idle_call'
  b'do_idle'
  b'cpu_startup_entry'
  b'start_secondary'
  b'secondary_startup_64'
    2
```

没有触发中断的时候，

```txt
[root@hygon-tencentos-18-42 15:11:15 irqs]$cat 133
handler:  handle_edge_irq
device:   0000:32:00.0
status:   0x00000000
istate:   0x00000000
ddepth:   0
wdepth:   0
dstate:   0x25401300
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_SET
            IRQD_SETAFFINITY_PENDING
            IRQD_CAN_RESERVE
node:     3
affinity: 46
effectiv: 46
pending:  44
domain:  PCI-MSI-3
 hwirq:   0x1900003
 chip:    PCI-MSI
  flags:   0x30
             IRQCHIP_SKIP_SET_WAKE
             IRQCHIP_ONESHOT_SAFE
 parent:
    domain:  VECTOR
     hwirq:   0x85
     chip:    APIC
      flags:   0x0
     Vector:    41
     Target:    46
     Previous vector:    49
     Previous target:    44
     move_in_progress: 1
     is_managed:       0
     can_reserve:      1
     has_reserved:     0
     cleanup_pending:  0
```
所以，其必须触发一次中断，中断才可以移动过去的。


## struct irqaction 是注册驱动的

```c
/**
 * struct irqaction - per interrupt action descriptor
 * @handler:	interrupt handler function
 * @name:	name of the device
 * @dev_id:	cookie to identify the device
 * @percpu_dev_id:	cookie to identify the device
 * @next:	pointer to the next irqaction for shared interrupts
 * @irq:	interrupt number
 * @flags:	flags (see IRQF_* above)
 * @thread_fn:	interrupt handler function for threaded interrupts
 * @thread:	thread pointer for threaded interrupts
 * @secondary:	pointer to secondary irqaction (force threading)
 * @thread_flags:	flags related to @thread
 * @thread_mask:	bitmask for keeping track of @thread activity
 * @dir:	pointer to the proc/irq/NN/name entry
 */
struct irqaction {
	irq_handler_t		handler;
	void			*dev_id;
	void __percpu		*percpu_dev_id;
	struct irqaction	*next;
	irq_handler_t		thread_fn;
	struct task_struct	*thread;
	struct irqaction	*secondary;
	unsigned int		irq;
	unsigned int		flags;
	unsigned long		thread_flags;
	unsigned long		thread_mask;
	const char		*name;
	struct proc_dir_entry	*dir;
} ____cacheline_internodealigned_in_smp;
```

- irqaction::next 来形成一个环


## request_threaded_irq 中参数 irq 是 linux irq

验证方法

直接把这个打印出来，打印结果和 cat /proc/interrupts 对比即可。

既然需要在 request_threaded_irq 的时候，提供 linux irq ，那么这些 linux irq 都是如何提前获取到的。

- pci 设备 : pci_request_irq 中调用 pci_irq_vector(dev, nr)
  - 如果不打开 msi ，可以参考 e1000_request_irq
- i8042_check_aux 中直接写死了
- serial_link_irq_chain : 有好几种获取方法，在 qemu 中测试，应该是 serial_pnp_probe 来注册的
  - platform_get_irq
  - pnp_irq
  - fwnode_irq_get
- floopy : fd_request_irq : 提前定义好使用 6 号
- apci : acpi_os_install_interrupt_handler 应该是通过 acpi 定义好的

### msi
让人容易误解的地方在于，如果 pci 没有 msi ，那么其
irq 可以从 pci 中读取到。这个时候会走到 ioapic 上。
这里的 irq 应该指的是 ioapic 的编号，全局一共有的
irq number 就只有 32 个。
```c
/*
 * Read interrupt line and base address registers.
 * The architecture-dependent code can tweak these, of course.
 */
static void pci_read_irq(struct pci_dev *dev)
{
	unsigned char irq;

	/* VFs are not allowed to use INTx, so skip the config reads */
	if (dev->is_virtfn) {
		dev->pin = 0;
		dev->irq = 0;
		return;
	}

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq);
	dev->pin = irq;
	if (irq)
		pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq);
	dev->irq = irq;
}
```

```txt
🧀  cat /proc/interrupts
           CPU0       CPU1
  1:          9          0  IO-APIC   1-edge      i8042
  4:          0        727  IO-APIC   4-edge      ttyS0
  9:          0          0  IO-APIC   9-fasteoi   acpi
 10:       8061          0  IO-APIC  10-fasteoi   virtio5, nvme0q0, nvme0q1, virtio3, virtio0
 11:          0        772  IO-APIC  11-fasteoi   virtio1, virtio2, nvme1q0, nvme1q1
 12:          0        125  IO-APIC  12-edge      i8042
```

### iommu 中断配置

```txt
@[
    irte_ga_set_affinity+5
    amd_ir_set_affinity+119
    msi_domain_set_affinity+77
    irq_do_set_affinity+465
    irq_set_affinity_locked+337
    irq_set_affinity+63
    write_irq_affinity.constprop.0.isra.0+257
    proc_reg_write+89
    vfs_write+239
    ksys_write+111
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
```

## irq_data 记录 irq 在每一个 domain 中基本信息
```c
/**
 * struct irq_data - per irq chip data passed down to chip functions
 * @mask:		precomputed bitmask for accessing the chip registers
 * @irq:		interrupt number
 * @hwirq:		hardware interrupt number, local to the interrupt domain
 * @common:		point to data shared by all irqchips
 * @chip:		low level interrupt hardware access
 * @domain:		Interrupt translation domain; responsible for mapping
 *			between hwirq number and linux irq number.
 * @parent_data:	pointer to parent struct irq_data to support hierarchy
 *			irq_domain
 * @chip_data:		platform-specific per-chip private data for the chip
 *			methods, to allow shared chip implementations
 */
struct irq_data {
	u32			mask;
	unsigned int		irq;
	unsigned long		hwirq;
	struct irq_common_data	*common;
	struct irq_chip		*chip;
	struct irq_domain	*domain;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_data		*parent_data;
#endif
	void			*chip_data;
};
```

分配的地方:
```c
static struct irq_data *irq_domain_insert_irq_data(struct irq_domain *domain,
						   struct irq_data *child)
{
	struct irq_data *irq_data;

	irq_data = kzalloc_node(sizeof(*irq_data), GFP_KERNEL,
				irq_data_get_node(child));
	if (irq_data) {
		child->parent_data = irq_data;
		irq_data->irq = child->irq;
		irq_data->common = child->common;
		irq_data->domain = domain;
	}

	return irq_data;
}
```

## 调用 action handler 的时候，irq 参数就是 linux irq
```c
irqreturn_t __handle_irq_event_percpu(struct irq_desc *desc)
{
	irqreturn_t retval = IRQ_NONE;
	unsigned int irq = desc->irq_data.irq;

	for_each_action_of_desc(desc, action) {
		res = action->handler(irq, action->dev_id);  // 进入到 nvme irq ，这个 irq 是 hwirq
}
```


| Language       | Lines | Code | Comments | Blanks |
|----------------|-------|------|----------|--------|
| manage.c       | 2944  | 1632 | 899      | 413    |
| irqdomain.c    | 2135  | 1351 | 470      | 314    |
| msi.c          | 1722  | 1027 | 458      | 237    |
| chip.c         | 1619  | 921  | 460      | 238    |
| irqdesc.c      | 1053  | 728  | 149      | 176    |
| generic-chip.c | 737   | 476  | 160      | 101    |
| proc.c         | 535   | 382  | 63       | 90     |
| matrix.c       | 519   | 321  | 128      | 70     |
| spurious.c     | 478   | 258  | 161      | 59     |
| ipi.c          | 345   | 187  | 112      | 46     |
| devres.c       | 325   | 175  | 105      | 45     |
| irq_sim.c      | 297   | 213  | 33       | 51     |
| pm.c           | 260   | 149  | 70       | 41     |
| debugfs.c      | 257   | 211  | 2        | 44     |
| cpuhotplug.c   | 254   | 124  | 97       | 33     |
| handle.c       | 242   | 111  | 93       | 38     |
| ipi-mux.c      | 206   | 120  | 52       | 34     |
| resend.c       | 202   | 107  | 67       | 28     |
| autoprobe.c    | 184   | 92   | 71       | 21     |
| affinity.c     | 128   | 71   | 37       | 20     |
| migration.c    | 119   | 56   | 45       | 18     |
| dummychip.c    | 64    | 36   | 21       | 7      |

## 概述
![img](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111554895-528341955.png)
- `struct irq_chip`结构，描述的是中断控制器的底层操作函数集，这些函数集最终完成对控制器硬件的操作；
- `struct irq_domain`结构，用于硬件中断号和 Linux IRQ 中断号（virq，虚拟中断号）之间的映射；
- `struct irq_chip` 结构体中的每个函数指针，都会携带一个指向 `struct irq_data` 的指针作为参数
- `struct irq_desc` 中直接包含了一个 `struct irq_data`


![img](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111647851-1005315068.png)

- 每个中断控制器都对应一个 IRQ Domain；
- 中断控制器驱动通过`irq_domain_add_*()`接口来创建 IRQ Domain；
- IRQ Domain 支持三种映射方式：linear map（线性映射），tree map（树映射），no map（不映射）；
  - linear map：维护固定大小的表，索引是硬件中断号，如果硬件中断最大数量固定，并且数值不大，可以选择线性映射；
  - tree map：硬件中断号可能很大，可以选择树映射；
  - no map：硬件中断号直接就是 Linux 的中断号；

![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111718514-879227841.png)

![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111755704-1231972965.png)

## 关键问题
3. 如何理解 /sys/devices/pci0000:00/*/irq 是什么

- irq_desc 是唯一的，那么一个 irq_desc 在任何 irq_domain 中都是唯一的

## 组合起来


```txt
@[
    apic_ack_edge+5
    handle_edge_irq+168
    __common_interrupt+61
    common_interrupt+134
    asm_common_interrupt+38
    default_idle+19
    default_idle_call+71
    do_idle+244
    cpu_startup_entry+42
    start_secondary+158
    common_startup_64+318
]: 2
```

中断触发过程，从中断如何找到 irq_desc 的

common_interrupt  -> call_irq_handler -> 	`desc = __this_cpu_read(vector_irq[vector]);`

如果通过 /proc/irq 修改中断亲和性的时候:
```txt
@[
    apic_update_vector+1
    assign_vector_locked+176
    apic_set_affinity+82
    msi_set_affinity+120
    irq_do_set_affinity+227
    irq_move_masked_irq+120
    __irq_move_irq+65
    apic_ack_edge+99
    handle_edge_irq+168
    __common_interrupt+61
    common_interrupt+134
    asm_common_interrupt+38
    default_idle+19
    default_idle_call+71
    do_idle+244
    cpu_startup_entry+42
    start_secondary+158
    common_startup_64+318
]: 1
```

一个 `irq_desc` 只能发送给一个 CPU 吗，应该是的，看 effective_affinity_list 是如此

irq_desc 在每一层 domain 中有一个 irq_data ，从而可以知道一个 irq_desc 如果触发了，
那么可以一路知道是哪个中断控制器发出来的。

然后，通过 domain 和 hwirq ，可以实现从最下一层的 irq 一直走到 irq_desc 。


## 记录两个 backtrace 一下
- i8042 键盘中断流程
```txt
Call Trace:
 <IRQ>
 dump_stack+0x64/0x7c
 pollwake+0x2a/0x90
 ? check_preempt_curr+0x3a/0x70
 ? ttwu_do_wakeup.isra.0+0xd/0xd0
 __wake_up_common+0x75/0x140
 __wake_up_common_lock+0x77/0xb0
 evdev_events+0x7c/0xa0
 input_to_handler+0x90/0xf0
 input_pass_values.part.0+0x119/0x140
 input_handle_event+0x20e/0x5f0
 input_event+0x4a/0x70
 atkbd_interrupt+0x47f/0x640
 serio_interrupt+0x42/0x90
 i8042_interrupt+0x146/0x250
 __handle_irq_event_percpu+0x38/0x150
 handle_irq_event_percpu+0x2c/0x80
 handle_irq_event+0x23/0x50
 handle_edge_irq+0x79/0x190
 __common_interrupt+0x39/0x90
 common_interrupt+0x76/0xa0
 </IRQ>
 asm_common_interrupt+0x1e/0x40
```

- usb 键盘中断流程
```txt
show_stack+0x2c/0x100
dump_stack+0x90/0xc0
input_event+0x30/0xc8
hidinput_report_event+0x44/0x68
hid_report_raw_event+0x230/0x470
hid_input_report+0x134/0x1b0
hid_irq_in+0x9c/0x280
__usb_hcd_giveback_urb+0xa0/0x120
finish_urb+0xac/0x1c0
ohci_work.part.8+0x218/0x550
ohci_irq+0x108/0x320
usb_hcd_irq+0x28/0x40
__handle_irq_event_percpu+0x70/0x1b8
handle_irq_event_percpu+0x20/0x88
handle_irq_event+0x44/0xa8
handle_level_irq+0xdc/0x188
generic_handle_irq+0x24/0x40
extioi_irq_dispatch+0x178/0x210
generic_handle_irq+0x24/0x40
do_IRQ+0x18/0x28
except_vec_vi_end+0x94/0xb8
__cpu_wait+0x20/0x24
calculate_cpu_foreign_map+0x148/0x180
```

## 当有 iommu 的时候
```txt
12)               |  msi_domain_set_affinity() {
12)               |    amd_ir_set_affinity() {
12)   0.151 us    |      irqd_cfg();
12)               |      apic_set_affinity() {
12)   0.161 us    |        _raw_spin_lock();
12)               |        assign_vector_locked() {
12)               |          irq_matrix_alloc() {
12)   0.340 us    |            matrix_alloc_area.constprop.0();
12)   0.712 us    |          }
12)   0.221 us    |          apic_update_vector();
12)               |          apic_update_irq_cfg() {
12)   0.121 us    |            apic_default_calc_apicid();
12)   0.621 us    |          }
12)   2.735 us    |        }
12)   0.141 us    |        _raw_spin_unlock();
12)   3.487 us    |      }
12)               |      irte_ga_set_affinity() {
12)               |        __modify_irte_ga.isra.0() {
12)   0.120 us    |          get_irq_table.isra.0();
12)   0.120 us    |          _raw_spin_lock_irqsave();
12)   0.321 us    |          _raw_spin_unlock_irqrestore();
12)   1.533 us    |        }
12)               |        iommu_flush_irt_and_complete() {
12)   0.100 us    |          build_inv_irt();
12)   0.120 us    |          build_completion_wait.isra.0();
12)   0.110 us    |          _raw_spin_lock_irqsave();
12)   0.130 us    |          __iommu_queue_command_sync();
12)   0.130 us    |          __iommu_queue_command_sync();
12)               |          __const_udelay() {
12)               |            delay_halt() {
12)   0.942 us    |              delay_halt_mwaitx();
12)   1.282 us    |            }
12)   1.593 us    |          }
12)   0.121 us    |          _raw_spin_unlock_irqrestore();
12)   4.088 us    |        }
12)   6.112 us    |      }
12)               |      vector_schedule_cleanup() {
12)               |        __vector_schedule_cleanup() {
12)   0.111 us    |          _raw_spin_lock();
12)               |          add_timer_on() {
12)               |            lock_timer_base() {
12)   0.111 us    |              _raw_spin_lock_irqsave();
12)   0.311 us    |            }
12)   0.150 us    |            _raw_spin_unlock();
12)   0.111 us    |            _raw_spin_lock();
12)   0.110 us    |            calc_wheel_index();
12)               |            enqueue_timer() {
12)               |              wake_up_nohz_cpu() {
12)               |                native_smp_send_reschedule() {
12)   0.140 us    |                  default_send_IPI_single_phys();
12)   0.511 us    |                }
12)   0.741 us    |              }
12)   1.032 us    |            }
12)   0.501 us    |            _raw_spin_unlock_irqrestore();
12)   2.935 us    |          }
12)   0.190 us    |          _raw_spin_unlock();
12)   3.677 us    |        }
12)   3.877 us    |      }
12) + 14.687 us   |    }
12) + 16.812 us   |  }
```

## irq_desc

当分配 irq_desc 的时候，需要逐级

- pci_alloc_irq_vectors_affinity
  - __pci_enable_msix_range
    - msix_capability_init
      - msix_setup_interrupts
        - pci_msi_setup_msi_irqs
          - msi_domain_alloc_irqs_all_locked
            - msi_domain_alloc_locked
              - __msi_domain_alloc_irqs
                - __irq_domain_alloc_irqs
                  - irq_domain_alloc_irqs_locked
                    - irq_domain_alloc_descs
                      - irq_domain_alloc_descs
                        - __irq_alloc_descs
                          - alloc_descs


early_irq_init 是初始化的分配给那些 legacy 的。

在# kernel/irq/irqdesc.c 管理 linux irq 到 irq_dec 的管理

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
