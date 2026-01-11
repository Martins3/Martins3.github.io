## interrupt remap 和 posted interupt 是什么关系?
<!-- b490f6ad-3611-4551-a7ef-b6305a7746e8 -->

关于投递 msi 中断的硬件行为，可以参考如下 qemu 的调用路线:
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - blk_aio_complete_bh
                        - blk_aio_complete
                          - blk_aio_complete
                            - virtio_blk_rw_complete
                              - address_space_stl_le
                                - address_space_stl_internal
                                  - memory_region_dispatch_write
                                    - access_with_adjusted_size
                                      - memory_region_write_accessor
                                        - kvm_apic_mem_write (写入到 LAPIC mmio 空间)
                                          - kvm_send_msi
                                            - kvm_irqchip_send_msi


设备产生的是 MSI/MSI-X 中断，本质是一次 DMA 写 LAPIC MMIO 空间的操作。

如果不加控制，设备可以：
- 向任意 CPU
- 任意 vector
- 任意 destination ID
- 写中断

所以，虽然硬件直通到虚拟机中，msix-table 不能被 vCPU 直接写，虚拟机操作直通设备的 MSI 必须被拦截，
然后根据 itre 来进行投递

在直通模式，下IRTE 决定一切
- IRTE 中包含：
- 是否允许该中断
- 目标 vCPU / pCPU
- vector
- delivery mode
- 是否是 virtual / posted interrupt
- 设备无法越权修改任何字段。


## 关键代码
- drivers/iommu/intel/irq_remapping.c

```c
struct irq_remap_ops intel_irq_remap_ops = {
	.prepare		= intel_prepare_irq_remapping,
	.enable			= intel_enable_irq_remapping,
	.disable		= disable_irq_remapping,
	.reenable		= reenable_irq_remapping,
	.enable_faulting	= enable_drhd_fault_handling,
};

static const struct irq_domain_ops intel_ir_domain_ops = {
	.select = intel_irq_remapping_select,
	.alloc = intel_irq_remapping_alloc,
	.free = intel_irq_remapping_free,
	.activate = intel_irq_remapping_activate,
	.deactivate = intel_irq_remapping_deactivate,
};

static struct irq_chip intel_ir_chip = {
	.name			= "INTEL-IR",
	.irq_ack		= apic_ack_irq,
	.irq_set_affinity	= intel_ir_set_affinity,
	.irq_compose_msi_msg	= intel_ir_compose_msi_msg,
	.irq_set_vcpu_affinity	= intel_ir_set_vcpu_affinity,
};
```

irq_remap_ops 是设备初始化相关的， intel_ir_chip 和 intel_ir_domain_ops 就是我们熟悉的东西了
用于添加一个的映射。

include/linux/dmar.h

```c
struct irte {
	union {
		struct {
			union {
				/* Shared between remapped and posted mode*/
				struct {
					__u64	present		: 1,  /*  0      */
						fpd		: 1,  /*  1      */
						__res0		: 6,  /*  2 -  6 */
						avail		: 4,  /*  8 - 11 */
						__res1		: 3,  /* 12 - 14 */
						pst		: 1,  /* 15      */
						vector		: 8,  /* 16 - 23 */
						__res2		: 40; /* 24 - 63 */
				};

				/* Remapped mode */
				struct {
					__u64	r_present	: 1,  /*  0      */
						r_fpd		: 1,  /*  1      */
						dst_mode	: 1,  /*  2      */
						redir_hint	: 1,  /*  3      */
						trigger_mode	: 1,  /*  4      */
						dlvry_mode	: 3,  /*  5 -  7 */
						r_avail		: 4,  /*  8 - 11 */
						r_res0		: 4,  /* 12 - 15 */
						r_vector	: 8,  /* 16 - 23 */
						r_res1		: 8,  /* 24 - 31 */
						dest_id		: 32; /* 32 - 63 */
				};

				/* Posted mode */
				struct {
					__u64	p_present	: 1,  /*  0      */
						p_fpd		: 1,  /*  1      */
						p_res0		: 6,  /*  2 -  7 */
						p_avail		: 4,  /*  8 - 11 */
						p_res1		: 2,  /* 12 - 13 */
						p_urgent	: 1,  /* 14      */
						p_pst		: 1,  /* 15      */
						p_vector	: 8,  /* 16 - 23 */
						p_res2		: 14, /* 24 - 37 */
						pda_l		: 26; /* 38 - 63 */
				};
				__u64 low;
			};

			union {
				/* Shared between remapped and posted mode*/
				struct {
					__u64	sid		: 16,  /* 64 - 79  */
						sq		: 2,   /* 80 - 81  */
						svt		: 2,   /* 82 - 83  */
						__res3		: 44;  /* 84 - 127 */
				};

				/* Posted mode*/
				struct {
					__u64	p_sid		: 16,  /* 64 - 79  */
						p_sq		: 2,   /* 80 - 81  */
						p_svt		: 2,   /* 82 - 83  */
						p_res3		: 12,  /* 84 - 95  */
						pda_h		: 32;  /* 96 - 127 */
				};
				__u64 high;
			};
		};
#ifdef CONFIG_IRQ_REMAP
		__u128 irte;
#endif
	};
};
```

```txt
@[
        intel_ir_set_vcpu_affinity+5
        irq_set_vcpu_affinity+206
        vmx_pi_update_irte+84
        kvm_pi_update_irte+131
        kvm_arch_irq_bypass_add_producer+143
        __connect+88
        irq_bypass_register_producer+181
        vfio_msi_set_vector_signal+427
        vfio_msi_set_block+89
        vfio_pci_core_ioctl+684
        vfio_device_fops_unl_ioctl+140
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 66

@[
    intel_ir_compose_msi_msg+5
    irq_chip_compose_msi_msg+47
    msi_domain_activate+70
    __irq_domain_activate_irq+80
    irq_domain_activate_irq+45
    __msi_domain_alloc_irqs+700
    msi_domain_alloc_irqs_all_locked+91
    __pci_enable_msix_range+980
    pci_alloc_irq_vectors_affinity+173
    vfio_pci_set_msi_trigger+157
    vfio_pci_core_ioctl+2536
    vfio_device_fops_unl_ioctl+129
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 7
```

如果是 amd ，那么类似的结果:
```txt
  b'modify_irte_ga'
  b'irq_remapping_activate'
  b'__irq_domain_activate_irq'
  b'__irq_domain_activate_irq'
  b'irq_domain_activate_irq'
  b'__setup_irq'
  b'request_threaded_irq'
  b'vfio_msi_set_vector_signal'
  b'vfio_msi_set_block'
  b'vfio_pci_set_msi_trigger'
  b'vfio_pci_ioctl'
  b'do_vfs_ioctl'
  b'ksys_ioctl'
  b'__x64_sys_ioctl'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
  b'[unknown]'
```

## intremap 几个参数的作用
<!-- d448819d-5ab2-4070-a547-00361ea1c863 -->
```txt
        intremap=       [X86-64, Intel-IOMMU]
                        on      enable Interrupt Remapping (default)
                        off     disable Interrupt Remapping
                        nosid   disable Source ID checking
                        no_x2apic_optout
                                BIOS x2APIC opt-out request will be ignored
                        nopost  disable Interrupt Posting
```

```c
static __init int setup_irqremap(char *str)
{
	if (!str)
		return -EINVAL;

	while (*str) {
		if (!strncmp(str, "on", 2)) {
			disable_irq_remap = 0;
			disable_irq_post = 0;
		} else if (!strncmp(str, "off", 3)) {
			disable_irq_remap = 1;
			disable_irq_post = 1;
		} else if (!strncmp(str, "nosid", 5))
			disable_sourceid_checking = 1;
		else if (!strncmp(str, "no_x2apic_optout", 16))
			no_x2apic_optout = 1;
		else if (!strncmp(str, "nopost", 6))
			disable_irq_post = 1;
		else if (IS_ENABLED(CONFIG_X86_POSTED_MSI) && !strncmp(str, "posted_msi", 10))
			enable_posted_msi = true;
		str += strcspn(str, ",");
		while (*str == ',')
			str++;
	}

	return 0;
}
early_param("intremap", setup_irqremap);
```
可以看到代码里面实际上多了一个 posted_msi ，这个东西我们在
docs/kernel/vfio/int-posted-msi.md
还不清楚的东西也就是
- nosid
- no_x2apic_optout
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
