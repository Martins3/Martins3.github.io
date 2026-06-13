# HyperV
[Documentation](https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/about/)
[Qemu doc for hyperv](https://github.com/qemu/qemu/blob/master/docs/hyperv.txt)


https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/nested-virtualization

- [ ] 还有自己的 layout https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/datatypes/hv_vmx_enlightened_vmcs
    - [ ] /home/maritns3/core/linux/arch/x86/include/asm/hyperv-tlfs.h:struct hv_enlightened_vmcs

- [ ] 这是 intel 的属性啊，为什么需要搭上 hyperv

## Why kvm need hyperv
https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf

Emulating hardware Interfaces can be slow
- Invent virtualization-friendly
  - (paravirtualized) interfaces!
    - Add support to guest OSes
      - ... but what about proprietary OSes?
        - We can try writing device drivers for such OSes
          - ... but some core features (interrupt handling, timekeeping,...) are not devices
            - Emulate an already supported (proprietary) hypervisor interfaces solving the exact same issues!

## 看看文档: Documentation/virt/hyperv/

## https://blog.kernel.love/hyperv-enlightenment.html
分析的很好，看一个例子就可以立刻理解.

https://www.qemu.org/docs/master/system/i386/hyperv.html

## 启动 windows 的时候会调用到这里: kvm_get_hv_cpuid

# hyperv 到底是如何影响的?

## kvm_hv_set_msr_common


# docs/kernel/cpuinfo/material/window.txt
将参数修改为，那么 CPUID 会发生修改：
arg_cpu_model="-cpu host,hv_relaxed,hv_vpindex,hv_time"
```txt
< CPUID 40000000:00 = 40000001 4b4d564b 564b4d56 0000004d | ...@KVMKVMKVM...
< CPUID 40000001:00 = 01007afb 00000000 00000000 00000000 | .z..............
---
> CPUID 40000000:00 = 40000005 7263694d 666f736f 76482074 | ...@Microsoft Hv
> CPUID 40000001:00 = 31237648 00000000 00000000 00000000 | Hv#1............
> CPUID 40000002:00 = 00003839 000a0000 00000000 00000000 | 98..............
> CPUID 40000003:00 = 00000262 00000000 00000000 00000008 | b...............
> CPUID 40000004:00 = 00000020 ffffffff 00000000 00000000 |  ...............
> CPUID 40000005:00 = ffffffff 00000040 00000000 00000000 | ....@...........
```

## 原来 cpuid leaf 40000000 是提供给 hypervisor 提供信息的

进一步参考:
- https://www.kernel.org/doc/Documentation/virtual/kvm/cpuid.txt
- https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery

## QEMU
```c
static struct {
    const char *desc;
    struct {
        uint32_t func;
        int reg;
        uint32_t bits;
    } flags[2];
    uint64_t dependencies;
} kvm_hyperv_properties[] = {
```

## 这里的内容都足够了
- https://www.qemu.org/docs/master/system/i386/hyperv.html

## eVMCS
https://www.linux-kvm.org/images/8/8e/Improving_KVM_nVMX.pdf

## 这几个参数什么含义
```txt
-cpu hv_time,hv_relaxed,hv_vapic,hv_spinlocks=0x1000
```

## hyperv 的代码什么时候会被调用
arch/x86/kvm/hyperv.c

## 惭愧，惭愧，居然这个都不知道
- https://github.com/MicrosoftDocs/Virtualization-Documentation

- https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/tlfs


https://kvm-forum.qemu.org/2021/The_Traps_of_Using_Hyper-V_Features_in_KVM_Environmen.pdf
# 如何支持 windows

## 当然，这里有 windows 的时间优化

## hv_vapic :: windows 10

### 关闭 kvm apicv
打开 hv_vapic，，运行 windows 虚拟机，可以看到很多：
```txt
@[
    kvm_hv_set_msr_common+5
    vmx_set_msr+3194
    __kvm_set_msr+145
    kvm_emulate_wrmsr+81
    vmx_handle_exit+1829
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 467
```
如果关闭，那么，这些 msr 的 write 完全消失了。

```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic+306
    kvm_apic_send_ipi+175
    kvm_lapic_reg_write+1489
    apic_mmio_write+99
    write_mmio+87
    emulator_read_write_onepage+262
    emulator_read_write+191
    x86_emulate_insn+1416
    x86_emulate_instruction+1056
    kvm_mmu_page_fault+276
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1736
```
- 这个似乎也不对吧，不知道为什么虚拟机中没有感知到 x2apic
，应该用 msr 的 exit ，也是是 windows 的原因

### 打开 kvm apciv 的话

无论是打开关闭，都是下面这个样子的:
```txt
@[
    vmx_deliver_interrupt+320
    vmx_deliver_interrupt+320
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic+306
    kvm_apic_send_ipi+175
    kvm_lapic_reg_write+1489
    handle_apic_write+133
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 3782
```
实际上完全看不到 msr 的 exit 。mmio 也是非常稀少的。

## hv_vapic :: windows 2016

如果打开 hv_vapic ，同时使用 vapic 的时候，可以看到有:
```txt
sudo bpftrace -e "tracepoint:kvm:kvm_msr { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    kvm_emulate_wrmsr+168
    kvm_emulate_wrmsr+168
    vmx_handle_exit+1829
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 2126
```
可以解释 windows 版本不同导致的问题。

此外，windows 2016 还有这个问题:
```txt
[ 4030.278593] x86/split lock detection: #AC: qemu-system-x86/31739 took a split_lock trap at address: 0x21adc3221a
[ 4033.181942] x86/split lock detection: #AC: qemu-system-x86/31738 took a split_lock trap at address: 0x21adc3220a
```

## 为什么 2016 无法 enable
```txt
  57.78%  mmio write len 4 gpa 0xfebd2008 val 0x3
   8.89%  mmio unsatisfied-read len 4 gpa 0xfebd04d0 val 0x0
   6.67%  mmio unsatisfied-read len 4 gpa 0xfebd04e0 val 0x0
   4.44%  mmio read len 4 gpa 0xfebd04d0 val 0x400e03
   4.44%  mmio unsatisfied-read len 4 gpa 0xfed000f0 val 0x0
   4.44%  mmio write len 4 gpa 0xfed00100 val 0x13c
```

这两个 backtrace 是合并的

```txt
@[
    kvm_lapic_reg_write+1
    handle_apic_write+133
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 821
```

```txt
@[
    wwrite_mmiorite_mmio+224
    write_mmio+224
    emulator_read_write_onepage+262
    emulator_read_write+191
    x86_emulate_insn+1416
    x86_emulate_instruction+1056
    kvm_mmu_page_fault+276
    vmx_handle_exit+2080
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 179
```

guest os 是可以控制 msr 的寄存器的数值的: kvm_lapic_set_base

0xfebd2008

```txt
@[
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+299
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+299
    bpf_trace_run2+134
    kvm_apic_set_eoi_accelerated+101
    handle_apic_eoi_induced+118
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 583
```

居然有一个 exit reason 是:

```txt
	[EXIT_REASON_EOI_INDUCED]             = handle_apic_eoi_induced,
```

## windows 的 exit 的原因和 linux 差别很大

```txt
  28.39%  reason EPT_VIOLATION rip 0xfffff800f7081037 info d82 0
  26.83%  reason HLT rip 0xfffff800f7080d7e info 0 0
  19.18%  reason EPT_VIOLATION rip 0xfffff800f706e9a9 info d81 0
   9.59%  reason EPT_VIOLATION rip 0xfffff800f706e9d3 info d82 0
   8.87%  reason EPT_VIOLATION rip 0xfffff800f706e9c6 info d82 0
   0.77%  reason EPT_VIOLATION rip 0xfffff801dc681350 info daa 0
   0.61%  reason EPT_VIOLATION rip 0xfffff801dc681320 info daa 0
   0.38%  reason PAUSE_INSTRUCTION rip 0xfffff800f6840dee info 0 0
   0.32%  reason EXTERNAL_INTERRUPT rip 0x7ffbe9067057 info 0 0
   0.28%  reason PENDING_INTERRUPT rip 0xfffff800f7080d7f info 0 0
   0.23%  reason EXTERNAL_INTERRUPT rip 0x7ffbe9067060 info 0 0
   0.18%  reason EXTERNAL_INTERRUPT rip 0x7ffbefd36030 info 0 0
   0.17%  reason EPT_MISCONFIG rip 0xfffff801dd301afc info 0 0
   0.16%  reason EPT_MISCONFIG rip 0xfffff801dbf58e2c info 0 0
   0.16%  reason EPT_VIOLATION rip 0xfffff800f6972490 info d82 0
   0.16%  reason EXTERNAL_INTERRUPT rip 0x7ffbe9067020 info 0 0
   0.14%  reason EXTERNAL_INTERRUPT rip 0x7ffbefd3603c info 0 0
   0.14%  reason TPR_BELOW_THRESHOLD rip 0xfffff800f69684b2 info 0 0
```

CPU 的消耗还算可以接受:
```txt
 593071 martins3  20   0   15.3g   4.9g  25332 S   5.0   7.9  14:52.83 qemu-system-x86
```

## 尝试下 QEMU 在 windows 中运行，不妙，似乎之前的 bash 都不可以用了

似乎需要开始学习 pwsh 了

## 高级啊!
- https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/user-guide/enhanced-session-mode
  - 就靠这些可以写 QEMU 对于 hyperv 的支持？ 不太可能吧!
    - 但是靠这个还可以吗? https://github.com/MicrosoftDocs/Virtualization-Documentation


## 如何理解 hv timer ?
```c
void kvm_lapic_restart_hv_timer(struct kvm_vcpu *vcpu)
{
	struct kvm_lapic *apic = vcpu->arch.apic;

	WARN_ON(!apic->lapic_timer.hv_timer_in_use);
	restart_apic_timer(apic);
}
```

## 这个也整理下吧
https://kvm-forum.qemu.org/2021/The_Traps_of_Using_Hyper-V_Features_in_KVM_Environmen.pdf

## 有办法自己安装一个虚拟机吗?
现在只要现成的 ubuntu 和 Fedora 虚拟机。

关注一下 guest 启动的 kernel 日志是做什么的?

启动的时候，这个是什么含义:
```txt
wsl: 检测到 localhost 代理配置，但未镜像到 WSL。NAT 模式下的 WSL 不支持 localhost 代理。
```

```txt
martins3@martins3:~$ ls -la /sys/block
total 0
drwxr-xr-x  2 root root 0 Aug 26 11:59 .
dr-xr-xr-x 11 root root 0 Aug 26 11:57 ..
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop0 -> ../devices/virtual/block/loop0
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop1 -> ../devices/virtual/block/loop1
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop2 -> ../devices/virtual/block/loop2
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop3 -> ../devices/virtual/block/loop3
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop4 -> ../devices/virtual/block/loop4
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop5 -> ../devices/virtual/block/loop5
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop6 -> ../devices/virtual/block/loop6
lrwxrwxrwx  1 root root 0 Aug 26 12:07 loop7 -> ../devices/virtual/block/loop7
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram0 -> ../devices/virtual/block/ram0
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram1 -> ../devices/virtual/block/ram1
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram10 -> ../devices/virtual/block/ram10
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram11 -> ../devices/virtual/block/ram11
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram12 -> ../devices/virtual/block/ram12
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram13 -> ../devices/virtual/block/ram13
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram14 -> ../devices/virtual/block/ram14
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram15 -> ../devices/virtual/block/ram15
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram2 -> ../devices/virtual/block/ram2
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram3 -> ../devices/virtual/block/ram3
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram4 -> ../devices/virtual/block/ram4
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram5 -> ../devices/virtual/block/ram5
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram6 -> ../devices/virtual/block/ram6
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram7 -> ../devices/virtual/block/ram7
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram8 -> ../devices/virtual/block/ram8
lrwxrwxrwx  1 root root 0 Aug 26 12:07 ram9 -> ../devices/virtual/block/ram9
lrwxrwxrwx  1 root root 0 Aug 26 12:07 sda -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/fd1d2cbd-ce7c-535c-966b-eb5f811c95f0/host0/target0:0:0/0:0:0:0/block/sda
lrwxrwxrwx  1 root root 0 Aug 26 12:07 sdb -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/fd1d2cbd-ce7c-535c-966b-eb5f811c95f0/host0/target0:0:0/0:0:0:1/block/sdb
lrwxrwxrwx  1 root root 0 Aug 26 12:07 sdc -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/fd1d2cbd-ce7c-535c-966b-eb5f811c95f0/host0/target0:0:0/0:0:0:2/block/sdc
lrwxrwxrwx  1 root root 0 Aug 26 12:41 sdd -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/fd1d2cbd-ce7c-535c-966b-eb5f811c95f0/host0/target0:0:0/0:0:0:3/block/sdd
```

原来网卡都是可以不用走 PCI 的:
```txt
martins3@martins3:~$ ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root 0 Aug 26 11:57 .
drwxr-xr-x 35 root root 0 Aug 26 11:57 ..
lrwxrwxrwx  1 root root 0 Aug 26 11:57 eth0 -> ../../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/136a9b88-5646-4307-86eb-b1c695ab963f/net/eth0
lrwxrwxrwx  1 root root 0 Aug 26 11:57 lo -> ../../devices/virtual/net/lo
martins3@martins3:~$ lspci
8437:00:00.0 3D controller: Microsoft Corporation Device 008e
9c49:00:00.0 System peripheral: Red Hat, Inc. Virtio file system (rev 01)
df89:00:00.0 SCSI storage controller: Red Hat, Inc. Virtio console (rev 01)
```

```txt
martins3@martins3:/dev/virtio-ports$ sudo lspci -vvv
[sudo] password for martins3:
8437:00:00.0 3D controller: Microsoft Corporation Device 008e
        Physical Slot: 2808971630
        Control: I/O+ Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx-
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Latency: 0
        Capabilities: [40] Null
        Kernel driver in use: dxgkrnl
```

如何贡献文件系统进去。

### 想不到网卡居然不是 virtio-net

## kvm 是支持的
```txt
[user@martins3 ~]$ dmesg | grep kvm
[    0.094279] kvm: no hardware support
[    0.095253] kvm: Nested Virtualization enabled
[    0.095255] SVM: kvm: Nested Paging enabled
[    0.095256] SVM: kvm: Hyper-V enlightened NPT TLB flush enabled
[    0.095256] SVM: kvm: Hyper-V Direct TLB Flush enabled
```
这里的 no hardware support 是什么意思

这里 qemu 可以展示框框到 windows 环境中，效果惊人!

## 那么这个下面的代码又是做啥的
arch/x86/hyperv/

似乎来作为 windows 的虚拟机的时候:

```txt
# CONFIG_HYPERV_VSOCKETS is not set
< # CONFIG_PCI_HYPERV is not set
< # CONFIG_PCI_HYPERV_INTERFACE is not set
< CONFIG_HYPERV_STORAGE=y
< # CONFIG_HYPERV_NET is not set
< CONFIG_HYPERV_KEYBOARD=y
< # CONFIG_DRM_HYPERV is not set
< # CONFIG_HID_HYPERV_MOUSE is not set
< CONFIG_HYPERV=y
< CONFIG_HYPERV_VTL_MODE=y
< CONFIG_HYPERV_TIMER=y
< CONFIG_HYPERV_UTILS=y
< CONFIG_HYPERV_BALLOON=y
< CONFIG_HYPERV_IOMMU=y
< # CONFIG_HYPERV_TESTING is not set
```
这里的选项可以关注下。

## 这里有一段注释
hv_apic_init
```c
	if (ms_hyperv.hints & HV_X64_APIC_ACCESS_RECOMMENDED) {
		pr_info("Hyper-V: Using enlightened APIC (%s mode)",
			x2apic_enabled() ? "x2apic" : "xapic");
		/*
		 * When in x2apic mode, don't use the Hyper-V specific APIC
		 * accessors since the field layout in the ICR register is
		 * different in x2apic mode. Furthermore, the architectural
		 * x2apic MSRs function just as well as the Hyper-V
		 * synthetic APIC MSRs, so there's no benefit in having
		 * separate Hyper-V accessors for x2apic mode. The only
		 * exception is hv_apic_eoi_write, because it benefits from
		 * lazy EOI when available, but the same accessor works for
		 * both xapic and x2apic because the field layout is the same.
		 */
		apic_update_callback(eoi, hv_apic_eoi_write);
		if (!x2apic_enabled()) {
			apic_update_callback(read, hv_apic_read);
			apic_update_callback(write, hv_apic_write);
			apic_update_callback(icr_write, hv_apic_icr_write);
			apic_update_callback(icr_read, hv_apic_icr_read);
		}
	}
```

## hyperv 的 EOI 的实现 : hv_apic_eoi_write

- https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/virtual-interrupts

中的 EOI assist 似乎正好对应:

```c
static void hv_apic_eoi_write(void)
{
	struct hv_vp_assist_page *hvp = hv_vp_assist_page[smp_processor_id()];

	if (hvp && (xchg(&hvp->apic_assist, 0) & 0x1))
		return;

	wrmsr(HV_X64_MSR_EOI, APIC_EOI_ACK, 0);
}
```
似乎真的可以

## 这个是做什么的?
- include/hyperv/hvgdk.h                                                           |  308 +++


## 这个看过没?
linux/Documentation/virt/hyperv/

## 也许有用，但是有点老了
http://events17.linuxfoundation.org/sites/events/files/slides/VMBus%20%28Hyper-V%29%20devices%20in%20QEMU%252FKVM_0.pdf


## 果然，windows 还是提供的 API
https://learn.microsoft.com/en-us/virtualization/api/hypervisor-platform/funcs/whvcancelrunvirtualprocessor

## qemu 中 whpx 的支持
https://qemu.weilnetz.de/w64/

具体支持的代码在:
target/i386/whpx/

## 可以测试下 hyper-v 中磁盘和网络的性能多少

## 为什么 hyperv 基本存在哪些东西

```txt
obj-$(CONFIG_HYPERV_VMBUS)	+= hv_vmbus.o
obj-$(CONFIG_HYPERV_UTILS)	+= hv_utils.o
obj-$(CONFIG_HYPERV_BALLOON)	+= hv_balloon.o
obj-$(CONFIG_MSHV_ROOT)		+= mshv_root.o
obj-$(CONFIG_MSHV_VTL)          += mshv_vtl.o
```

drivers/hv/Kconfig

```txt
config MSHV_ROOT
	tristate "Microsoft Hyper-V root partition support"
	depends on HYPERV && (X86_64 || ARM64)
	depends on !HYPERV_VTL_MODE
	# The hypervisor interface operates on 4k pages. Enforcing it here
	# simplifies many assumptions in the root partition code.
	# e.g. When withdrawing memory, the hypervisor gives back 4k pages in
	# no particular order, making it impossible to reassemble larger pages
	depends on PAGE_SIZE_4KB
	select EVENTFD
	select VIRT_XFER_TO_GUEST_WORK
	select HMM_MIRROR
	select MMU_NOTIFIER
	default n
	help
	  Select this option to enable support for booting and running as root
	  partition on Microsoft Hyper-V.

	  If unsure, say N.
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

让 Linux 能够作为 Hyper-V 的 root partition（根分区）启动和运行。
关键背景
在 Hyper-V 架构中，root partition 是具有特权的管理分区，类似于 Xen 中的 Dom0。它直接与 hypervisor 交互，负责：
• 创建和管理其他 guest partition（虚拟机）
• 分配和回收硬件资源（内存、I/O 设备等）
• 处理设备仿真和 I/O 转发

典型用途
该功能主要用于让 Linux 直接运行在 Hyper-V hypervisor 的底层管理分区中，例如某些 Azure 主机节点或特定嵌套虚拟化场景，而不是
仅仅作为一个普通 guest（此时只需要 CONFIG_HYPERV 即可）。如果你只是想在 Hyper-V 里跑 Linux 虚拟机，通常不需要开启这个选项

(好高级，还是没搞懂啊)

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
