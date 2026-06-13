# posted interrupt

## posted interrupt 基本测试

### enable_device_posted_irqs
新内核可以通过 enable_device_posted_irqs 来直接观察:
```txt
🤒  cat /sys/module/kvm_intel/parameters/enable_device_posted_irqs
N
```
也可以手动关闭掉
```sh
sudo modprobe kvm_intel enable_device_posted_irqs=0
```

在 map_iommu 中读取，在
```c
	iommu->cap = dmar_readq(iommu->reg + DMAR_CAP_REG);
```

在 set_irq_posting_cap 中将 posted interrupt 清理掉

```c
bool kvm_arch_has_irq_bypass(void)
{
	return enable_apicv && irq_remapping_cap(IRQ_POSTING_CAP);
}
```
### 性能对比
测试环境为 13900k + ASUS 主板，这个主板的 iommu 有

```txt
 lspci -s 0000:02:00.0 -vv
02:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000 (rev 01) (prog-if 02 [NVM Express])
        Subsystem: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000
        Control: I/O- Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx+
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Latency: 0, Cache Line Size: 64 bytes
        Interrupt: pin A routed to IRQ 16
```

虚拟机中:
```txt
jobs: 1 (f=1): [r(1)][0.5%][r=971MiB/s][r=249k IOPS][eta 16m:36s]
```

物理机中 360k

所以，这个性能开销还是蛮大的

### SR-IOV 也支持 posted interrupts 吗?
<!-- b85b810a-49cd-4205-b326-f31708c77d87 -->

没有任何问题，看上去 SR-IOV 完全就是一个解耦的功能，就像是多了一个 vfio 设备一样

```txt
 663:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:17:00.2    0-edge      vfio-msix[0](0000:17:00.2)
```

## 关键代码和路径
- arch/x86/kvm/vmx/posted_intr.c : 代码量很少的
- arch/x86/include/asm/posted_intr.h

当启用 posted interrupt 的时候:
- pi_update_irte
  - irq_set_vcpu_affinity : kernel/irq/manage.c
    - `chip->irq_set_vcpu_affinity`
      - intel_ir_set_vcpu_affinity
        - modify_irte
      - amd_ir_set_vcpu_affinity


## 虚拟设备例如 virtio-blk 如何使用 posted interrupt
<!-- f79ffc2e-ee09-430a-bf48-f5f9592c54f2 -->

vmx_deliver_interrupt -> kvm_vcpu_trigger_posted_interrupt

```c
static inline void kvm_vcpu_trigger_posted_interrupt(struct kvm_vcpu *vcpu,
						     int pi_vec)
{
#ifdef CONFIG_SMP
	if (vcpu->mode == IN_GUEST_MODE) {
		/*
		 * The vector of the virtual has already been set in the PIR.
		 * Send a notification event to deliver the virtual interrupt
		 * unless the vCPU is the currently running vCPU, i.e. the
		 * event is being sent from a fastpath VM-Exit handler, in
		 * which case the PIR will be synced to the vIRR before
		 * re-entering the guest.
		 *
		 * When the target is not the running vCPU, the following
		 * possibilities emerge:
		 *
		 * Case 1: vCPU stays in non-root mode. Sending a notification
		 * event posts the interrupt to the vCPU.
		 *
		 * Case 2: vCPU exits to root mode and is still runnable. The
		 * PIR will be synced to the vIRR before re-entering the guest.
		 * Sending a notification event is ok as the host IRQ handler
		 * will ignore the spurious event.
		 *
		 * Case 3: vCPU exits to root mode and is blocked. vcpu_block()
		 * has already synced PIR to vIRR and never blocks the vCPU if
		 * the vIRR is not empty. Therefore, a blocked vCPU here does
		 * not wait for any requested interrupts in PIR, and sending a
		 * notification event also results in a benign, spurious event.
		 */

		if (vcpu != kvm_get_running_vcpu())
			__apic_send_IPI_mask(get_cpu_mask(vcpu->cpu), pi_vec);
		return;
	}
#endif
	/*
	 * The vCPU isn't in the guest; wake the vCPU in case it is blocking,
	 * otherwise do nothing as KVM will grab the highest priority pending
	 * IRQ via ->sync_pir_to_irr() in vcpu_enter_guest().
	 */
	kvm_vcpu_wake_up(vcpu);
}
```
所以，这里分析的三种情况就是:
1. vcpu->mode == IN_GUEST_MODE && vcpu != kvm_get_running_vcpu()
	- 需要注入的中断在其他的 pCPU 上运行，那么就需要 posted interrupt ipi 通知了，但是如果 vCPU 实际上不是，
	那么就可以在 /proc/interrupts 中 PIN : Posted-interrupt notification event 来观察到 kvm_posted_intr_ipi() 被调用
	kvm_posted_intr_ipi() 不需要做太多事情，因为前面已经设置了 pi_test_and_set_pir ，vCPU 开始执行的时候，会检查到这个 bit
2. vcpu->mode == IN_GUEST_MODE && vcpu == kvm_get_running_vcpu()
	- IN_GUEST_MODE 不是说这个 vCPU 正在 guest mode 中，也许在 vmexit 处理的快速路径中
	- kvm_get_running_vcpu() 获取当前 CPU 关联的 vCPU
	- 所以 vcpu == kvm_get_running_vcpu() 的含义就是，vCPU 正在 VM-exit 的 fastpath 中 ，马上会重新进入 pCPU 执行中，所以什么都不需要做
3. vcpu->mode != IN_GUEST_MODE
	- 需要唤醒 vCPU ，走的普通的 process wake up 机制

所以，可以想到 iommu 来注入中断其实非常类似的过程，iommu 注入中断的时候，
也是写入 pi_desc 的 bitmap ，然后让 iommu 来触发 ipi POSTED_INTR_VECTOR ，然后在 vCPU 接受 POSTED_INTR_VECTOR
那么就会检查 pi_desc。

这里在强调一次，posted interrupts 是 iommu 和 APICv 两个功能一起合作才有的。

## pi_desc 只是普通内存吗?
<!-- bacd0f02-461a-44f2-9c86-0f37fe09ee36 -->

是的
```c
/* Posted-Interrupt Descriptor */
struct pi_desc {
	unsigned long pir[NR_PIR_WORDS];     /* Posted interrupt requested */
	union {
		struct {
			u16	notifications; /* Suppress and outstanding bits */
			u8	nv;
			u8	rsvd_2;
			u32	ndst;
		};
		u64 control;
	};
	u32 rsvd[6];
} __aligned(64);
```

vmx_pi_update_irte 中:
```c
		struct intel_iommu_pi_data pi_data = {
			.pi_desc_addr = __pa(vcpu_to_pi_desc(vcpu)),
			.vector = vector,
		};
```

```c
static struct pi_desc *vcpu_to_pi_desc(struct kvm_vcpu *vcpu)
{
	return &(to_vt(vcpu)->pi_desc);
}
```

```c
struct vcpu_vt {
	/* Posted interrupt descriptor */
	struct pi_desc pi_desc;

	/* Used if this vCPU is waiting for PI notification wakeup. */
	struct list_head pi_wakeup_list;

	union vmx_exit_reason exit_reason;

	unsigned long	exit_qualification;
	u32		exit_intr_info;

	/*
	 * If true, guest state has been loaded into hardware, and host state
	 * saved into vcpu_{vt,vmx,tdx}.  If false, host state is loaded into
	 * hardware.
	 */
	bool		guest_state_loaded;
	bool		emulation_required;

#ifdef CONFIG_X86_64
	u64		msr_host_kernel_gs_base;
#endif
};
```

每一个 vCPU 都会有一个 pi_desc ，需要写入到两个位置中

1. arch/x86/kvm/vmx/vmx.c:init_vmcs() 中，提供地址给 vmcs 就可以了:
```c
		vmcs_write64(POSTED_INTR_DESC_ADDR, __pa((&vmx->vt.pi_desc)));
```
2. drivers/iommu/intel/irq_remapping.c 中的
```txt
		irte_pi.pda_l = (pi_data->pi_desc_addr >>
				(32 - PDA_LOW_BIT)) & ~(-1UL << PDA_LOW_BIT);
		irte_pi.pda_h = (pi_data->pi_desc_addr >> 32) &
				~(-1UL << PDA_HIGH_BIT);
```
中断注入的整个流程大致如下:

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
Device MSI
  ↓
VT-d Interrupt Remapping
  ↓  (查 IRTE)
IRTE.p == 1 ?
  ↓ yes
IRTE.PDA --> PID (内存)
  ↓
原子置位 PID.PIR
  ↓
（若 SN=0）发 posted notification vector
  ↓
目标 pCPU LAPIC
  ↓
VMCS.POSTED_INTR_DESC_ADDR 指向同一个 PID
```

SN 是 Suppress Notification

所以，注意 pi_desc 是一个 vCPU 关联的，itre 指向的记事本，
无论 vCPU 在那个 pCPU 上执行，itre 都是指向到是一个固定的 pi_desc ，
将中断记录到其中。

每一个 vCPU 都有自己的 vmcs ，在初始化的时候，vmcs 关联上自己的 pi_desc 就可以了


## posted interrupt 如何 vCPU 迁移和虚拟机中的中断绑定
<!-- 7b8e2160-8d52-4466-b4ad-29c454834430 -->

### vCPU 迁移

先思考一个简单问题:

vmx_vcpu_pi_put() 在 vCPU 即将被挂起（put / block / schedule out）时，决定：
- 是否允许 Posted Interrupt 通过 notification IRQ 唤醒该 vCPU ，也就是 pi_enable_wakeup_handler
- 还是直接 Suppress Notification，只更新 PIR 位图，不打断 host CPU ，也就是 pi_set_sn

vmx_vcpu_pi_put 的判断是，只有一个 vCPU 为:
- 没被 host 抢占 (vCPU 的时间片用完了)
- 正在 guest 中睡眠 (虚拟机中执行 mwait / halt)
- 并且 guest 当前能接收中断

```txt
@[
        vmx_vcpu_pi_put+5
        vmx_vcpu_put+18
        kvm_arch_vcpu_put+297
        vcpu_put+25
        kvm_arch_vcpu_ioctl_run+525
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 53060
```
接下来，这个 vCPU 被放到 pCPU 的 wakeup_vcpus_on_cpu 上，
当接受到中断之后，就会执行来做唤醒操作也就是执行 sysvec_kvm_posted_intr_wakeup_ipi
这个 pCPU 就是当前 vCPU 使用 CPU
```txt
	list_add_tail(&vt->pi_wakeup_list,
		      &per_cpu(wakeup_vcpus_on_cpu, vcpu->cpu));
```

所以再回到这个问题，vCPU 切换 pCPU 咋办?

vmx_vcpu_pi_load 中会配置 ndst 的:
```c
	do {
		new.control = old.control;

		/*
		 * Clear SN (as above) and refresh the destination APIC ID to
		 * handle task migration (@cpu != vcpu->cpu).
		 */
		new.ndst = dest;
		__pi_clear_sn(&new);

		/*
		 * Restore the notification vector; in the blocking case, the
		 * descriptor was modified on "put" to use the wakeup vector.
		 */
		new.nv = POSTED_INTR_VECTOR;
	} while (pi_try_set_control(pi_desc, &old.control, new.control));
```

### 直通设备中断亲和性绑定
<!-- 3fd71561-51af-46c3-ab18-5fc4787e0389 -->

完全都是相同的机制，让虚拟机去写 msix-table ，qemu 监听到之后
，例如知道了从 vCPU 1 切换到 vCPU 2 ，那么就会调用 ioctl VFIO_DEVICE_SET_IRQS
，最后来修改 itre ，让 itre 指向到不同的 pi_desc 上。

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - vfio_msix_vector_release
                            - vfio_device_irq_set_signaling
                              - vfio_device_io_set_irqs
```txt
@[
        intel_ir_set_vcpu_affinity+5
        irq_set_vcpu_affinity+206
        kvm_pi_update_irte+131
        kvm_arch_irq_bypass_del_producer+95
        __disconnect+59
        irq_bypass_unregister_producer+51
        vfio_msi_set_vector_signal+104
        vfio_msi_set_block+89
        vfio_pci_core_ioctl+684
        vfio_device_fops_unl_ioctl+140
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 1
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
]: 1
```
也就是直接重置 irte 了，这是很合理的。

## 仅仅直通设备 (vfio_msihandler) 如何 vCPU 迁移和虚拟机中的中断绑定
<!-- 6df74471-8bc2-493f-ab51-b4cf11f52095 -->

irq 的参数是 eventfd_ctx 还是走 gsi 机制的
```txt
@[
        vmx_deliver_interrupt+5
        __apic_accept_irq+251
        kvm_irq_delivery_to_apic_fast+336
        kvm_arch_set_irq_inatomic+217
        irqfd_wakeup+275
        __wake_up_common+120
        eventfd_signal_mask+112
        vfio_msihandler+19
        __handle_irq_event_percpu+85
        handle_irq_event+56
        handle_edge_irq+199
        __common_interrupt+76
        common_interrupt+128
        asm_common_interrupt+38
        cpuidle_enter_state+211
        cpuidle_enter+45
        cpuidle_idle_call+241
        do_idle+119
        cpu_startup_entry+41
        start_secondary+296
        common_startup_64+318
]: 393269
```
那么，vCPU 迁移，显然没有任何影响，因为注入的对象就是 vCPU ，vCPU 在那个 pCPU 上，那是 vmx_deliver_interrupt 的工作。

如果虚拟机中修改被直通设备的中断亲和性，那么这个结果完全符合我们的预期


- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - msix_handle_mask_update
                            - msix_fire_vector_notifier
                              - vfio_msix_vector_use
                                - vfio_msix_vector_do_use
                                  - vfio_update_kvm_msi_virq
                                    - kvm_irqchip_commit_routes  : 调用 KVM_SET_GSI_ROUTING

内核中观察到:
```txt
@[
        irqfd_update+1
        kvm_irq_routing_update+167
        kvm_set_irq_routing+494
        kvm_vm_ioctl+1543
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 20
```

## itre 是如何用于 posted interrupt 和 interrupt remmaping 的
<!-- e2c02481-ec21-4e4e-83bd-43ab4b963918 -->

如果修改中断亲和性:
```sh
echo 10 | sudo tee /proc/irq/368/smp_affinity_list
```
```txt
@[
        __modify_irte_ga.isra.0+1
        irte_ga_set_affinity+72
        amd_ir_set_affinity+122
        msi_domain_set_affinity+79
        irq_do_set_affinity+207
        irq_set_affinity_locked+235
        __irq_set_affinity+72
        write_irq_affinity.isra.0+229
        proc_reg_write+89
        vfs_write+207
        ksys_write+99
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 2
```
如果是 posted interrupt 的中断修改，也就是上面提到了

intel_ir_set_affinity 和 intel_ir_set_vcpu_affinity 什么关系?

## 文档
- Intel SDM Vol 3, Section 29.6 “Posted-Interrupt Processing”


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
