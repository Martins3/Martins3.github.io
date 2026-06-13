# 如何调试 seabios

为了看看这个问题：
```txt
	kvm-intel.emulate_invalid_guest_state=
			[KVM,Intel] Control whether to emulate invalid guest
			state. Ignored if kvm-intel.enable_unrestricted_guest=1,
			as guest state is never invalid for unrestricted
			guests. This param doesn't apply to nested guests (L2),
			as KVM never emulates invalid L2 guest state.
			Default is 1 (enabled).

	kvm-intel.unrestricted_guest=
			[KVM,Intel] Control KVM's use of unrestricted guest
			feature (virtualized real and unpaged mode). Default
			is 1 (enabled). Disable by KVM if EPT is disabled or
			hardware lacks support for it.

```

## 我忘记了如何调试 seabios 了
如何知道他的地址变换来着?

## 为什么会 Relocating ?
docs/Linking_overview.md

Relocating init from 0x000d4140 to 0x06feb1c0 (size 85408)

的确是区别的，最后无法启动的:
```txt
WARNING - Unable to allocate resource at copy_rom:235!                    Running option rom at ca00:0003
                                                                          pmm call arg1=1
                                                                          pmm call arg1=0
                                                                          pmm call arg1=1
                                                                          pmm call arg1=0
                                                                          Searching bootorder for: /pci@i0cf8/*@3

```

## 为什么 reallocate 之后，会继续使用非 allocate 的地址

```txt
br *0xed755
```

然后去 break ，的确是可以找到的:
```txt
$ bt
#0  0x00000000000ed755 in smm_setup () at ./src/fw/smm.c:268
#1  0x0000000006ff5902 in ?? ()
#2  0x0000000000000000 in ?? ()
```

似乎不是所有的地址都是 reallocate 的。

## 让我们看看 0x38000 附近有什么问题

这两段地址:
```txt
ea6b9 0
38000 30000
fd069 f0000
fec23 f0000
fec40 f0000
fec48 0
```

在 0x38000 之前
```txt
$ bt
#0  outb (port=178, value=0 '\000') at ./src/x86.h:151
#1  smm_relocate_and_restore () at ./src/fw/smm.c:170
#2  0x0000000b02000000 in ?? ()
#3  0x00006cd8000ed7db in ?? ()
#4  0x00000bb206fea220 in ?? ()
#5  0x06ff590200006cd8 in ?? ()
#6  0x0000000000000000 in ?? ()
```
所以，当时，一定在 SMM 中断中。

handle_smi cmd=0 smbase=0x00030000

## 为什么 smm 也需要 relocate ?
smm_relocate_and_restore


```c
static void
smm_relocate_and_restore(void)
{
    /* init APM status port */
    outb(0x01, PORT_SMI_STATUS);

    /* raise an SMI interrupt */
    outb(0x00, PORT_SMI_CMD);

    /* wait until SMM code executed */
    while (inb(PORT_SMI_STATUS) != 0x00)
        ;

    /* restore original memory content */
    struct smm_layout *initsmm = (void*)BUILD_SMM_INIT_ADDR;
    struct smm_layout *smm = (void*)BUILD_SMM_ADDR;
    memcpy_nowarn(&initsmm->cpu, &smm->cpu, sizeof(initsmm->cpu));
    memcpy_nowarn(&initsmm->codeentry, &smm->codeentry
                  , sizeof(initsmm->codeentry));

    // 0x0003fe00
    // 0x00038000
    // 0x000a8000
    dprintf(1, "martins3 %p\n", &initsmm->cpu);
    dprintf(1, "martins3 %p\n", &initsmm->codeentry);
    dprintf(1, "martins3 %p\n", &smm->codeentry);


    // Setup code entry point.
    smm->codeentry = SMI_INSN;
    wbinvd();
}
```

是的，这里就是 SMI 的入口:
```txt
// Entry point for QEMU smi interrupts.
        DECLFUNC entry_smi
entry_smi:
        // Transition to 32bit mode.
        movl $1f + BUILD_BIOS_ADDR, %edx
        jmp transition32_nmi_off
        .code32
1:      movl $BUILD_SMM_ADDR + 0x8000, %esp
        calll _cfunc32flat_handle_smi - BUILD_BIOS_ADDR
        rsm
        .code16
```

## 意外发现

如果在 break *0xfec48 ，在第二次 heat breakpoint 的时候，到导致 qemu 直接 crash 掉:

```txt
init smm
qemu-system-x86_64: ../hw/core/cpu-system.c:76: cpu_asidx_from_attrs: Assertion `ret < cpu->num_ases && ret >= 0' failed.
./seabios.sh: line 4: 2201722 Aborted                 (core dumped) /home/martins3/core/qemu/build/qemu-system-x86_64 -machine pc,accel=kvm,smm=on -bios /home/martins3/core/seabios/out/bios.bin -device isa-debugcon,iobase=0x402,chardev=seabios -chardev stdio,id=seabios --display none -s -S
```

## SMM 的行为可以在内核中感知到吗?

执行之后，的确有:
```txt
@[
    emulator_leave_smm+5
    em_rsm+71
    x86_emulate_insn+396
    x86_emulate_instruction+602
    handle_ud+137
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 11
```

```txt
@[
    emulator_is_smm+5
    em_rsm+44
    x86_emulate_insn+396
    x86_emulate_instruction+602
    handle_ud+137
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 11
```

vn/code/src/m/arch/x86_64/smm.c 正好如此
```txt
[  661.648895] Hello world.
[  661.648967] SMI count earlier is 11.
[  661.649079] data recieved from port 0xb2 is f1.
[  661.649338] data recieved after is 80.
[  661.649453] SMI count later is- 12.
```
正好是 11 次数。

之后继续测试，发现 SMI count 是会变化的。

使用 firecracker 作为对比，这个计数器总是不变的:
```txt
data recieved from port 0xb2 is 0.
data recieved after is 0.
```

似乎应该在这里才对 __apic_accept_irq

为什么没有捕获到
```txt
sudo bpftrace -e "kfunc:__apic_accept_irq { if (args->delivery_mode == 0x200 ) { @[kstack]=count(); } }"
```

kvm:kvm_apic_accept_irq
kvm:kvm_apicv_inhibit_changed
kvm:kvm_apicv_accept_irq

没动
```txt
echo 'dm == 0x200' > /sys/kernel/debug/tracing/events/kvm/kvm_apic_accept_irq/filter

entry="kvm:kvm_apic_accept_irq"
echo "${entry}" | sudo tee /sys/kernel/debug/tracing/set_event
echo 1 > /sys/kernel/debug/tracing/events/kvm/kvm_apic_accept_irq/enable
```

## 是直接到 SMM 的吗?

可以用 kernel module 的影响

将 firecracker 和 qemu 作为对比

奇怪的东西，为什么 SMI handler 到底是谁?

## 原来是通过 qemu
info mtree 可以看到
```txt
    00000000000000b2-00000000000000b3 (prio 0, i/o): apm-io
```

但是通过 sudo cat /proc/ioports 无法观察到

一通操作，到达这里:
```c
static void apm_ctrl_changed(uint32_t val, void *arg)
{
    PIIX4PMState *s = arg;
    PCIDevice *d = PCI_DEVICE(s);

    /* ACPI specs 3.0, 4.7.2.5 */
    acpi_pm1_cnt_update(&s->ar, val == ACPI_ENABLE, val == ACPI_DISABLE);
    if (val == ACPI_ENABLE || val == ACPI_DISABLE) {
        return;
    }

    if (d->config[0x5b] & (1 << 1)) {
        if (s->smi_irq) {
            qemu_irq_raise(s->smi_irq);
        }
    }
}
```

原来就是模拟到这些东西的:

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - apm_ctrl_changed
                              - pc_acpi_smi_interrupt : 设置 request

- kvm_arch_pre_run : 没想到，NMI 和 SMI 都是走特殊路径的
  - ret = kvm_vcpu_ioctl(cpu, KVM_NMI);
  - kvm_vcpu_ioctl(cpu, KVM_SMI)

## qemu 中的 smm=off 意味着什么?

通过 seabios : hw/i386/acpi-build.c:init_common_fadt_data 来控制 ACPI


```c
// This code is hardcoded for PIIX4 Power Management device.
static void piix4_apmc_smm_setup(int isabdf, int i440_bdf)
{
    /* check if SMM init is already done */

    u32 value = pci_config_readl(isabdf, PIIX_DEVACTB); // 从这里离开
    if (value & PIIX_DEVACTB_APMC_EN)
        return;
```

## 测试一下 qemu uefi 的 smm 模拟
windows 换了之后，直接无法启动

linux 换了之后，qemu 是相同的现象。

所以，这个和固件是没有关系的。

## windows 有 smm 计数器吗?
https://github.com/pestrela/smi_counter

## 在 latest qemu + kenrel + windows + smm=off 中测试 rweverything
的确，都是一样的效果

但是，在 windows + q35 测试 SMM=1 或者 SMM=0 都会导致问题。

在 windows + pc 测试 SMM=1 或者 SMM=0 都会导致问题。

## [ ] https://learn.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-uefi-wsmt

其实意义不大:

ShellPkg/Library/UefiShellAcpiViewCommandLib/Parsers/Wsmt/WsmtParser.c
ShellPkg/Library/UefiShellAcpiViewCommandLib/AcpiParser.h

额外的介绍，但是似乎其实和操作系统的关系不大:
- https://www.microsoft.com/en-us/security/blog/2020/11/12/system-management-mode-deep-dive-how-smm-isolation-hardens-the-platform/

## 在物理机上 trigger 一下

小米笔记本上，触发了 187a 此
```txt
[853668.914069] [martins3:greeter_init:301]
[853668.930245] action = 0 current=tee
[853668.933296] SMI count later is- 187a.
```
触发不会导致任何问题。

nixos 中发现的
```txt
[438645.793509] SMI count earlier is 121.
[438645.793522] data recieved from port 0xb2 is a0.
[438645.794078] data recieved after is 80.
[438645.794079] SMI count later is- 122.
```

## 不过还是需要解释，为什么 guest os 会触发 SMM 的

## 一个虚拟机如何知道自己是在

uefi 启动的话，其他的两个没有:
```txt
🧀  ls -la /sys/firmware/efi
.r--r--r-- 4.1k root 21 Mar 16:46  config_table
drwxr-xr-x    - root 21 Mar 16:46  efivars
.r--r--r-- 4.1k root 21 Mar 16:46  fw_platform_size
.r--r--r-- 4.1k root 21 Mar 16:46  fw_vendor
.r--r--r-- 4.1k root 21 Mar 16:46  runtime
drwxr-xr-x    - root 21 Mar 16:46  runtime-map
.r-------- 4.1k root 21 Mar 16:46  systab
```


## [ ] 为什么电源管理需要 SMM ?

## [ ] seabios SMM 提供的功能有什么?

啥功能没有，完全就是浪费精神，那么存在的意义是什么？

### [ ] 看看 smram 里面都是如何管理的

看上去 kvm 和 tcg 有不同的实现机制:
```txt
address-space: KVM-SMRAM
  0000000000000000-ffffffffffffffff (prio 0, i/o): mem-container-smram
    0000000000000000-00000000ffffffff (prio 10, i/o): smram
      00000000000a0000-00000000000bffff (prio 0, i/o): alias smram-low @pc.ram 00000000000a0000-00000000000bffff
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias mem-smram @system 0000000000000000-ffffffffffffffff
```

kvm 如何知道 smram 在哪里?

通过 MSR_IA32_SMBASE 来控制的
u64 kvm_vcpu_arch::smbase

hw/pci-host/i440fx.c 似乎 qemu 是把这个地址写死的:

```c
    /* smram, as seen by SMM CPUs */
    memory_region_init(&f->smram, OBJECT(d), "smram", 4 * GiB);
    memory_region_set_enabled(&f->smram, true);
    memory_region_init_alias(&f->low_smram, OBJECT(d), "smram-low",
                             s->ram_memory, SMRAM_C_BASE, SMRAM_C_SIZE);
    memory_region_set_enabled(&f->low_smram, true);
    memory_region_add_subregion(&f->smram, SMRAM_C_BASE, &f->low_smram);
    object_property_add_const_link(qdev_get_machine(), "smram",
                                   OBJECT(&f->smram));
```

的确有一次变化:
```txt
     0.000 :3171488/3171488 kvm:kvm_smm_transition(smbase: 196608, entering: 1) // In [1]: hex(196608) Out[1]: '0x30000'
     0.106 :3171488/3171488 kvm:kvm_smm_transition(smbase: 196608)
    15.569 :3171488/3171488 kvm:kvm_smm_transition(smbase: 655360, entering: 1) // In [2]: hex(655360) Out[2]: '0xa0000'
    15.685 :3171488/3171488 kvm:kvm_smm_transition(smbase: 655360)
    15.730 :3171488/3171488 kvm:kvm_smm_transition(smbase: 655360, entering: 1)
```

kvm_vcpu_reset 中来初始化的
```c
	if (!init_event) {
		vcpu->arch.smbase = 0x30000;

		vcpu->arch.pat = MSR_IA32_CR_PAT_DEFAULT;

		vcpu->arch.msr_misc_features_enables = 0;
		vcpu->arch.ia32_misc_enable_msr = MSR_IA32_MISC_ENABLE_PEBS_UNAVAIL |
						  MSR_IA32_MISC_ENABLE_BTS_UNAVAIL;

		__kvm_set_xcr(vcpu, 0, XFEATURE_MASK_FP);
		__kvm_set_msr(vcpu, MSR_IA32_XSS, 0, true);
	}
```
再看 seabios 的源码，可以发现:
```txt
handle_smi cmd=0 smbase=0x00030000
```
但是后来的
```txt
handle_smi cmd=b5 smbase=0x000a0000
```

所以，的确发生了一次转换，所以就有这个了。

### [ ] 看下是不是 kvm 的错误注入了

arch/x86/kvm/lapic.c

一个是 __apic_accept_irq
一个是 kvm_arch_vcpu_ioctl


## [ ]  看看 ovmf 中对于 smm 的支持写的过于复杂，完全不能理解

UefiCpuPkg/PiSmmCpuDxeSmm

的确是可以看看 OvmfPkg/PlatformPei/Platform.c 的代码

然后从 S3 的角度看看

所以，虚拟机中的 suspended 和物理机中的 syspend 的关系是什么?

```txt
在 OVMF（Open Virtual Machine Firmware，一个基于 TianoCore EDK II 的 UEFI 固件实现）中，与 SMM（System Management Mode，系统管理模式）相关的代码主要分布在以下几个关键文件中和模块中。这些文件主要位于 `OvmfPkg` 目录下，并且涉及 SMM 的初始化、驱动以及相关服务。以下是具体的分析：

### 1. **SMM 核心驱动文件**
   - **`OvmfPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf`**
     - 这是 OVMF 中用于支持 SMM 的主要 DXE（Driver Execution Environment）阶段驱动。它负责初始化和管理 SMM 的 CPU 相关功能，例如 SMI（System Management Interrupt，系统管理中断）的处理。
     - 相关源文件包括：
       - `PiSmmCpuDxeSmm.c`：SMM 主入口和初始化逻辑。
       - `SmmCpuFeaturesLib.c`：提供 CPU 特定的 SMM 功能实现。

   - **`UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf`**
     - 这个模块是 EDK II 中通用的 SMM CPU 驱动，OVMF 也会依赖它。它与 `OvmfPkg` 中的实现协同工作，提供 SMM 的基础支持。
     - 相关源文件：
       - `PiSmmCpuDxeSmm.c`：SMM 的核心逻辑。
       - `SmmInit.c`：SMM 初始化代码。

### 2. **SMM 相关协议和库**
   - **`MdePkg/Include/Protocol/SmmBase2.h`**
     - 定义了 `EFI_SMM_BASE2_PROTOCOL`，这是 SMM 基础协议，OVMF 中的 SMM 实现依赖它来注册 SMM 服务。
   - **`UefiCpuPkg/Include/Library/SmmCpuFeaturesLib.h`**
     - SMM CPU 特性库的头文件，提供了与 SMM 相关的硬件抽象接口。
   - **`OvmfPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.c`**
     - OVMF 定制的 SMM CPU 特性库实现，适配 QEMU/KVM 的虚拟化环境。

### 3. **SMM 变量存储和 LockBox**
   - **`OvmfPkg/QemuFlashFvbServicesRuntimeDxe/FvbServicesSmm.inf`**
     - 这个模块负责在 SMM 中处理固件变量存储（Firmware Volume Block，FVB），特别是与非易失性 UEFI 变量相关的逻辑。
     - 相关源文件：
       - `QemuFlash.c`：与 QEMU 的闪存模拟交互。
   - **`MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf`**
     - 提供 SMM 中的 LockBox 功能，用于安全存储数据（例如 S3 挂起到恢复的数据）。
     - 相关源文件：
       - `SmmLockBox.c`：LockBox 的 SMM 实现。

### 4. **SMM 通信和控制**
   - **`UefiCpuPkg/Include/Protocol/SmmControl2.h`**
     - 定义了 `EFI_SMM_CONTROL2_PROTOCOL`，用于触发 SMI 和控制 SMM 的进入。
   - **`OvmfPkg/SmmControl2Dxe/SmmControl2Dxe.inf`**
     - OVMF 中实现的 SMM 控制协议驱动。
     - 相关源文件：
       - `SmmControl2Dxe.c`：实现 SMI 触发和 SMM 进入逻辑。

### 5. **平台初始化相关**
   - **`OvmfPkg/PlatformPei/Platform.c`**
     - 在 PEI（Pre-EFI Initialization）阶段检测平台是否支持 SMM，并为后续阶段配置相关参数。例如，它会检查 QEMU 的 SMM 支持并设置相应的标志。
   - **`OvmfPkg/QemuRamfbDxe/QemuRamfbDxe.c`**
     - 虽然主要是显卡驱动，但与 SMM 的兼容性有关，尤其是在 Q35 机器类型下。

### 6. **构建配置文件**
   - **`OvmfPkg/OvmfPkgX64.dsc`** 和 **`OvmfPkg/OvmfPkgIa32X64.dsc`**
     - 这些 DSC（平台描述文件）中通过 `-D SMM_REQUIRE` 宏启用 SMM 支持。SMM 相关的模块（如 `PiSmmCpuDxeSmm`）会在这些文件中被包含。
   - **`OvmfPkg/OvmfPkgIa32.dsc`**
     - 类似地，针对 32 位平台的支持也在这里定义，但 SMM 在 64 位环境下的实现更为完整。

### 7. **其他辅助文件**
   - **`UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf`**
     - 处理 SMM 中的异常，例如与虚拟化相关的异常（如 VMExit）。
     - 相关源文件：
       - `SmmCpuException.c`：SMM 异常处理逻辑。
   - **`OvmfPkg/Include/OvmfPlatforms.h`**
     - 定义了与 SMM 相关的平台特性宏，例如 QEMU Q35 机器的支持。

### 总结
OVMF 中与 SMM 相关的代码主要集中在以下几个方面：
- **核心驱动**：`PiSmmCpuDxeSmm` 和相关文件。
- **协议和库**：`SmmBase2`、`SmmControl2` 和 `SmmCpuFeaturesLib`。
- **变量存储**：`FvbServicesSmm` 和 `SmmLockBox`。
- **平台支持**：`PlatformPei` 和构建配置文件。

如果你需要深入研究某部分代码，可以从 `OvmfPkg/PiSmmCpuDxeSmm` 开始，这是 SMM 功能的核心入口，然后根据需求追溯到相关的协议或库文件。
```

## SMM 在 AArch64 中的实现

deepseek 的解释:
```txt
在 x86 架构中，SMM（System Management Mode，系统管理模式）是一种特殊的处理器运行模式，用于处理低级系统任务，例如电源管理、硬件控制以及一些安全相关的功能。SMM 是一个独立于操作系统的高权限模式，通常由 BIOS 或固件触发，通过 SMI（System Management Interrupt）进入。

在 AArch64（ARM 64 位架构）中，并没有直接等价于 x86 SMM 的机制。ARM 架构的设计理念与 x86 有显著差异，它采用了一种更模块化和层次化的权限模型，主要通过 **Exception Levels（异常级别，EL0 到 EL3）** 来管理不同的执行状态和权限，而不是依赖类似 SMM 的独立模式。

### AArch64 的异常级别
AArch64 定义了以下几个异常级别：
- **EL0**：用户模式，运行普通的应用程序。
- **EL1**：内核模式，运行操作系统内核。
- **EL2**：虚拟化模式，用于支持虚拟机监控器（Hypervisor）。
- **EL3**：安全监控模式（Monitor Mode），由 TrustZone 技术支持，用于运行安全固件（如 Secure Monitor）。

在 AArch64 中，与 x86 SMM 功能最接近的机制是 **EL3 和 TrustZone 技术**。

### TrustZone 与 EL3 的作用
TrustZone 是 ARM 提供的一种硬件安全扩展，它将处理器划分为两个世界：
- **Normal World（非安全世界）**：运行普通的操作系统和应用（EL0、EL1、EL2）。
- **Secure World（安全世界）**：运行受信任的代码，通常由安全固件控制（EL3）。

当系统需要执行类似 SMM 的任务（例如安全启动、电源管理或敏感硬件控制）时，可以通过触发 **SMC（Secure Monitor Call）** 从非安全世界切换到安全世界，由运行在 EL3 的 Secure Monitor 处理这些任务。SMC 类似于 x86 中的 SMI，但它的实现更加灵活，具体功能取决于固件和系统设计。

### AArch64 如何替代 SMM 的功能
1. **电源管理**：
   - 在 x86 中，SMM 常用于处理 ACPI（高级配置与电源接口）相关的低级任务。
   - 在 AArch64 中，电源管理通常由操作系统（EL1）通过 PSCI（Power State Coordination Interface）完成。PSCI 是一种标准接口，由固件（运行在 EL3）实现，允许操作系统与低级硬件协调电源状态。

2. **硬件控制**：
   - x86 的 SMM 可以直接访问硬件资源。
   - AArch64 中，硬件控制通常由 EL3 的安全固件或 EL2 的虚拟化层管理，具体实现依赖于系统的固件设计（如 ARM Trusted Firmware）。

3. **安全功能**：
   - SMM 在 x86 中有时用于安全相关的任务。
   - AArch64 使用 TrustZone 将安全任务隔离到 Secure World，避免非安全代码的干扰。

### 总结
AArch64 没有直接对应的 SMM 机制，但通过 **TrustZone 和 EL3（安全监控模式）** 以及 **PSCI 等固件接口**，它实现了类似的功能。相比 x86 的 SMM，AArch64 的设计更加模块化，依赖于固件和操作系统的协作，而不是一个独立的硬件模式。具体实现方式因设备和厂商而异，例如在服务器、移动设备或嵌入式系统中可能会有不同的固件支持（如 UEFI 或 ARM Trusted Firmware）。
```

## aarch64 中的模拟 smm3 的方法
我们知道 aarch64 有 el0 el1 el2 el3 ，定义了 secure world 和 normal world ，
如果 aarch64 的 Hypervisor 需要模拟 secure world 吗？如果需要，指出 qemu  kvm 以及 ovmf 中对应的代码

似乎 aarch64 是不会模拟 secure world 的

## 测试下 windows 11 的安装，为什么必须 SMM 来着?

## 的确，安全启动是大量的需要 SMI 的


## smm 使用的时候，有状态问题
https://gitlab.com/qemu-project/qemu/-/issues/1198
看上去是状态没有正确保证，然后又不断触发 SMI ，所以关掉这个功能

## 似乎就是这个解决办法
https://forum.proxmox.com/threads/qemu-kvm-internal-error-smm-removal-bit-rot.94211/

> I have a Windows VM I migrated from a non-Proxmox installation and it crashes qemu about 2-3 mins after boot (see end of thread). The pxe-qemu repo disables kvm smm support but seems like it's opened the door for SMM mode calls to be made inside the guest when it hasn't been properly confirgured.
> Recompiling pxe-qemu without the following patch allows smm to be enabled and my guest no longer seems to crash repeatedly.

## [ ] smm 中断是 option rom 触发的，option rom 中如何触发的

bdf 是 0x20
vendor device : 是 1af4:1000

- init_pcirom
  - init_optionrom
    - callrom

```txt
00:04.0 Ethernet controller [0200]: Red Hat, Inc. Virtio network device [1af4:1000]
00:05.0 Ethernet controller [0200]: Red Hat, Inc. Virtio network device [1af4:1000]
00:06.0 Ethernet controller [0200]: Red Hat, Inc. Virtio network device [1af4:1000]
```

看来是在执行
```txt
0000:00:04.0/virtio-net-pci.rom    4 KiB  0x0000000800080000 0x0000000000040000 0x0000000000040000 0x00007fe3d8200000  ro
```
所以是 pc-bios/ 下的 pxe-virtio.rom 吗?

不是，似乎是 pc-bios/efi-virtio.rom

```c
static void virtio_net_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    VirtioPCIClass *vpciklass = VIRTIO_PCI_CLASS(klass);

    k->romfile = "efi-virtio.rom";
    k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    k->device_id = PCI_DEVICE_ID_VIRTIO_NET;
    k->revision = VIRTIO_PCI_ABI_VERSION;
    k->class_id = PCI_CLASS_NETWORK_ETHERNET;
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    device_class_set_props(dc, virtio_net_properties);
    vpciklass->realize = virtio_net_pci_realize;
}
```
那么 efi-virtio.rom 从哪里来的?

## [ ] 通过触发的 ip 来判断机器当时在做什么

sudo bpftrace -e 'kfunc:kvm:kvm_fast_pio { if (args->port == 0xb2) { printf("%x\n", args->port); }}'

```txt
[martins3:kvm_arch_pre_run:5452] 42373
[martins3:kvm_arch_pre_run:5452] 43182
```
不成比例啊，很多触发 NMI 都不是这个原因


### 分析 rip
似乎是 uefi 中的地址
```txt
7df5fc31
7e7c50e4
7ee55791
7ffc701c
838bf0
```

这部分似乎是 windows 的地址，这么看，这些对称就是 windows 中的地址:
```txt
 7ff6beaf3fb0
 7ff6beb1127a
 7ffc966d9d87
 7ffc966d9de8
 7ffc966eee11
 7ffc966f0dbc
 7ffc966f0ea4
 7ffc966f132c
 7ffc967c26ce
 7ffc96c4e037
 7ffca50d9db7
 7ffca6974a80
 7ffcaa577378
 7ffcaa578560
 7ffcaa580de3
 7ffcaa68350e
 7ffcab524c17
 7ffcad0cdcdb
 7ffcad15ab25
 7ffcad15abbb
 7ffcad15ad46
 7ffcad15ad68
 7ffcad15ae32
 7ffcad15ae62
 7ffcad15b0e7
 7ffcad15b301
 7ffcad15b32a
 7ffcad15b49c
 7ffcad15b65f
 7ffcad183871
 7ffcad211e3d
 7ffcad268510
 7ffcada43e7a
 7ffcae421f50
 7ffcae44b284
 7ffcaea28a46
 7ffcaea38792
 7ffcaea4bc7a
 7ffcaea60dec
 7ffcaee2adb9
 7ffcaf0e3158
 7ffcaf799f95
 7ffcaf79b423
 7ffcaf79b5d1
 7ffcaf79b6d3
 7ffcaf79b915
 7ffcaf79ba08
 7ffcaf7bf597
 7ffcaf7c1640
 7ffcaf7c21a4
 7ffcaf7d5d87
 7ffcaf7d5efd
 7ffcaf7e6035
 7ffcaf7e7f97
 7ffcaf7e8536
 7ffcaf7e8581
 7ffcaf7f6cc1
 7ffcaf7f6d8b
 7ffcaf80fc3e
 7ffcaf816c20
 7ffcaf856c56
 7ffcaf85d500
 7ffcaf8ab86a
 7ffcaf8d1f76
 7ffcaf8df9d0
 7ffcaf8dfab0
 7ffcaf8e01f0
 fffff80079bc7364
 fffff8007a7c1900
 fffff8007a7c1dd9
 fffff8007a7c2bff
 fffff8007b272bd0
 fffff8007b27635b
 fffff8007b367241
 fffff8007b432e06
 fffff8007b53c1d4
 fffff8007b54a760
 fffff8007b564215
 fffff8007b5abf96
 fffff8007b6463fb
 fffff8007bf15117
 fffff8007c3bf0e4
 fffff8007c663493
 fffff8007c80cdc0
 fffff8007d080c30
 fffff800e8208c29
 fffff800e8208f50
 fffff800e8209ffa
 fffff800e820a093
 fffff800e820a108
 fffff800e8211cfc
 fffff800e8211f11
 fffff800e8214928
 fffff800e8214ab7
 fffff800e8214acb
 fffff800e82150a2
 fffff800e82150c0
 fffff800e82150eb
 fffff800e821942d
 fffff800e8219c22
 fffff800e821fda2
 fffff800e8227202
 fffff800e822d6f8
 fffff800e822d767
 fffff800e8235355
 fffff800e8238057
 fffff800e823b76d
 fffff800e8243c49
 fffff800e824c35c
 fffff800e824c362
 fffff800e82762dd
 fffff800e8276d61
 fffff800e82771ec
 fffff800e827c96f
 fffff800e8286241
 fffff800e8287a3b
 fffff800e8288e82
 fffff800e8289110
 fffff800e828ab64
 fffff800e828ab7f
 fffff800e828c3cf
 fffff800e828ed45
 fffff800e829254f
 fffff800e82947c6
 fffff800e8294812
 fffff800e8299841
 fffff800e82aaea0
 fffff800e82ab010
 fffff800e82ab018
 fffff800e82ab01a
 fffff800e82ab01c
 fffff800e82ab01f
 fffff800e82af1d2
 fffff800e8303947
 fffff800e8314c5b
 fffff800e8314e17
 fffff800e832a9f9
 fffff800e832dac5
 fffff800e832e702
 fffff800e8334220
 fffff800e83514b0
 fffff800e83515c1
 fffff800e835c24f
 fffff800e835c2c7
 fffff800e835c3c7
 fffff800e835c3d5
 fffff800e835c3de
 fffff800e835c3eb
 fffff800e835c400
 fffff800e835c405
 fffff800e835c409
 fffff800e835c414
 fffff800e835c420
 fffff800e835c422
 fffff800e835c424
 fffff800e835c42d
 fffff800e835c44c
 fffff800e835c453
 fffff800e835c459
 fffff800e835c45c
 fffff800e835c45e
 fffff800e835c4c3
 fffff800e835c4d5
 fffff800e835c517
 fffff800e835c51a
 fffff800e835c54f
 fffff800e835facd
 fffff800e8390824
 fffff800e83a33a5
 fffff800e83bc604
 fffff800e83bc6c0
 fffff800e83cf474
 fffff800e83deb31
 fffff800e83e21a1
 fffff800e83ea064
 fffff800e83f993a
 fffff800e8402dd3
 fffff800e841cbf4
 fffff800e841d320
 fffff800e842c950
 fffff800e84561b0
 fffff800e84720cd
 fffff800e8472144
 fffff800e847215e
 fffff800e847216d
 fffff800e84721c1
 fffff800e84721f2
 fffff800e84b7a7e
 fffff800e8505ea0
 fffff800e852523c
 fffff800e85723d0
 fffff800e8673bf7
 fffff800e8673d7f
 fffff800e8674436
 fffff800e867443e
 fffff800e867ca48
 fffff800e86813f8
 fffff800e86815d0
 fffff800e86815d4
 fffff800e86815d9
 fffff800e86815e3
 fffff800e86815eb
 fffff800e86815f8
 fffff800e86815fd
 fffff800e8681ada
 fffff800e8681b2e
 fffff800e868c04c
 fffff800e868c978
 fffff800e868eb58
 fffff800e86b4bb6
 fffff800e86c7387
 fffff800e881f708
 fffff800e882505e
 fffff800e8827419
 fffff800e882774a
 fffff800e88282cd
 fffff800e8828828
 fffff800e8829a12
 fffff800e882a0c9
 fffff800e882cea6
 fffff800e882d536
 fffff800e882d5ac
 fffff800e882f519
 fffff800e883107e
 fffff800e8831095
 fffff800e884f27e
 fffff800e88515c8
 fffff800e88ce777
 fffff800e88d836a
 fffff800e88eacc2
 fffff800e8933c5e
 fffff800e898a845
 fffff800e89a2e90
 fffff800e89e8020
 fffff800e8b7b3d1
```
所以，必须拿到内核中的位置吗?



### 分析注入的方式

一部分继续是之前的注入方法:

kvm_cpu_exec 检查到 exit 的 ioport

pc_acpi_smi_interrupt

原来是这样的的，换了一个设备触发:

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - ich9_apm_ctrl_changed
                              - ich9_apm_ctrl_changed
                                - cpu_interrupt

然后另外一个 qemu thread 被唤醒:

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_wait_io_event
          - cpu_thread_is_idle
            - cpu_has_work
              - x86_cpu_has_work
                - x86_cpu_pending_interrupt

似乎这样是非常合理的，在 q35 下面有:
```txt
    0000000000000600-000000000000067f (prio 0, i/o): ich9-pm
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr
      0000000000000620-000000000000062f (prio 0, i/o): acpi-gpe0
      0000000000000630-0000000000000637 (prio 0, i/o): acpi-smi
      0000000000000660-000000000000067f (prio 0, i/o): sm-tco
```

但是 windows 隐藏了一些内容的:
```txt
  0600-067f : 0000:00:1f.0
    0600-0603 : ACPI PM1a_EVT_BLK
    0604-0605 : ACPI PM1a_CNT_BLK
    0608-060b : ACPI PM_TMR
    0620-062f : ACPI GPE0_BLK
```

所以，这个问题是，


```txt
address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io
    0000000000000000-0000000000000007 (prio 0, i/o): dma-chan
    0000000000000008-000000000000000f (prio 0, i/o): dma-cont
    0000000000000020-0000000000000021 (prio 0, i/o): kvm-pic
    0000000000000040-0000000000000043 (prio 0, i/o): kvm-pit
    0000000000000060-0000000000000060 (prio 0, i/o): i8042-data
    0000000000000061-0000000000000061 (prio 0, i/o): pcspk
    0000000000000064-0000000000000064 (prio 0, i/o): i8042-cmd
    0000000000000070-0000000000000071 (prio 0, i/o): rtc
      0000000000000070-0000000000000070 (prio 0, i/o): rtc-index
    000000000000007e-000000000000007f (prio 0, i/o): kvmvapic
    0000000000000080-0000000000000080 (prio 0, i/o): ioport80
    0000000000000081-0000000000000083 (prio 0, i/o): dma-page
    0000000000000087-0000000000000087 (prio 0, i/o): dma-page
    0000000000000089-000000000000008b (prio 0, i/o): dma-page
    000000000000008f-000000000000008f (prio 0, i/o): dma-page
    0000000000000092-0000000000000092 (prio 0, i/o): port92
    00000000000000a0-00000000000000a1 (prio 0, i/o): kvm-pic
    00000000000000b2-00000000000000b3 (prio 0, i/o): apm-io
    00000000000000c0-00000000000000cf (prio 0, i/o): dma-chan
    00000000000000d0-00000000000000df (prio 0, i/o): dma-cont
    00000000000000f0-00000000000000f0 (prio 0, i/o): ioportF0
    00000000000001ce-00000000000001d1 (prio 0, i/o): vbe
    00000000000003b4-00000000000003b5 (prio 0, i/o): vga
    00000000000003ba-00000000000003ba (prio 0, i/o): vga
    00000000000003c0-00000000000003cf (prio 0, i/o): vga
    00000000000003d4-00000000000003d5 (prio 0, i/o): vga
    00000000000003da-00000000000003da (prio 0, i/o): vga
    00000000000003f8-00000000000003ff (prio 0, i/o): serial
    0000000000000402-0000000000000402 (prio 0, i/o): isa-debugcon
    00000000000004d0-00000000000004d0 (prio 0, i/o): kvm-elcr
    00000000000004d1-00000000000004d1 (prio 0, i/o): kvm-elcr
    0000000000000510-0000000000000511 (prio 0, i/o): fwcfg
    0000000000000514-000000000000051b (prio 0, i/o): fwcfg.dma
    0000000000000600-000000000000067f (prio 0, i/o): ich9-pm
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr
      0000000000000620-000000000000062f (prio 0, i/o): acpi-gpe0
      0000000000000630-0000000000000637 (prio 0, i/o): acpi-smi
      0000000000000660-000000000000067f (prio 0, i/o): sm-tco
    0000000000000cc0-0000000000000cd7 (prio 0, i/o): acpi-pci-hotplug
    0000000000000cd8-0000000000000ce3 (prio 0, i/o): acpi-cpu-hotplug
    0000000000000cf8-0000000000000cfb (prio 0, i/o): pci-conf-idx
    0000000000000cf9-0000000000000cf9 (prio 1, i/o): lpc-reset-control
    0000000000000cfc-0000000000000cff (prio 0, i/o): pci-conf-data
    0000000000005658-0000000000005658 (prio 0, i/o): vmport
    0000000000006000-000000000000603f (prio 1, i/o): pm-smbus
    0000000000006040-000000000000607f (prio 1, i/o): virtio-pci
    0000000000006080-00000000000060bf (prio 1, i/o): virtio-pci
    00000000000060c0-00000000000060ff (prio 1, i/o): virtio-pci
    0000000000006100-000000000000611f (prio 1, i/o): ahci-idp
    0000000000006120-000000000000613f (prio 1, i/o): virtio-pci
    0000000000006140-000000000000615f (prio 1, i/o): virtio-pci
    0000000000006160-000000000000617f (prio 1, i/o): virtio-pci
    0000000000006180-000000000000619f (prio 1, i/o): virtio-pci
```

60:22 64:2247 70:198 71:331 92:2
b2:1365 b3:683 2ff:18 3f8:90 3f9:7 3fa:4 3fb:13 3fc:44 3fd:130 3fe:36 3ff:8
402:365334 510:98 511:30 514:1820 518:1820 604:2
608:1075597 630:4 cd8:39 cdc:32 cdd:2 ce0:32 cf8:159 cfc:37 cfd:3 cfe:16 mmmm

```txt
[martins3:kvm_arch_pre_run:5453] 39187
```
但是 ich9_apm_ctrl_changed 是对于每一个 CPU 调用的，所以，其实正好的

## tarce smm 触发的 ip

enter_smm_save_state_64 中添加 trace_enter(smram->rip);

```txt
           <...>-200135  [008] ..... 19677.873850: enter: enter : ea5dd
 qemu-system-x86-200135  [014] ..... 19677.890958: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.891116: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.891207: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.891327: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.891437: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.891556: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.891642: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.891761: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.893128: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.893266: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.893369: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.893487: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.894677: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.894810: enter: enter : f7e25
 qemu-system-x86-200135  [014] ..... 19677.894911: enter: enter : 7e0b
 qemu-system-x86-200135  [014] ..... 19677.895030: enter: enter : f7e25
 qemu-system-x86-200135  [012] ..... 19702.919659: enter: enter : ffffffffc09b4f68
```


```txt
Root memory region: system
  0000000000000000-00000000000bffff (prio 0, ram): pc.ram KVM
  00000000000c0000-00000000000c2fff (prio 0, rom): pc.ram @00000000000c0000 KVM
  00000000000c3000-00000000000c5fff (prio 0, ram): pc.ram @00000000000c3000 KVM
  00000000000c6000-00000000000e3fff (prio 0, rom): pc.ram @00000000000c6000 KVM
  00000000000e4000-00000000000effff (prio 0, ram): pc.ram @00000000000e4000 KVM
  00000000000f0000-00000000000fffff (prio 0, rom): pc.ram @00000000000f0000 KVM
  0000000000100000-000000007fffffff (prio 0, ram): pc.ram @0000000000100000 KVM
  00000000b0000000-00000000bfffffff (prio 0, i/o): pcie-mmcfg-mmio
  00000000fc000000-00000000fc000fff (prio 1, i/o): gpu-control-mem
  00000000fc001000-00000000fc00101f (prio 0, i/o): msix-table
  00000000fc001020-00000000fc002fff (prio 1, i/o): gpu-control-mem @0000000000001020
  00000000fc003000-00000000fc003007 (prio 0, i/o): msix-pba
  00000000fc003008-00000000fcffffff (prio 1, i/o): gpu-control-mem @0000000000003008
  00000000fd000000-00000000fdffffff (prio 1, ram): gpu-fb-mem KVM
```

## [ ] 注入 SMI 中断的时候，参数有意义吗？

## [x] pc 使用 ovmf 启动，发现无法触发 SMI 的，所以
现在看是 uefi 的bug 了

那么至少，这个就是解决办法。

### apm_ctrl_changed 这里的判断该如何理解

都是奇怪的定义
```c
static void apm_ctrl_changed(uint32_t val, void *arg)
{
    PIIX4PMState *s = arg;
    PCIDevice *d = PCI_DEVICE(s);

    /* ACPI specs 3.0, 4.7.2.5 */
    acpi_pm1_cnt_update(&s->ar, val == ACPI_ENABLE, val == ACPI_DISABLE);
    if (val == ACPI_ENABLE || val == ACPI_DISABLE) {
        return;
    }

    if (d->config[0x5b] & (1 << 1)) {
        if (s->smi_irq) {
            qemu_irq_raise(s->smi_irq);
        }
    }
}
```
如何理解这个东西?

## uefi + smm=off + windows 可以发现 windows 也是 crash 的

说明 linux 中自动做了修复了，检查出来是在哪里进行的:
```txt
 qemu-system-x86-492800  [014] ..... 106282.695151: enter: enter : fffff801cf81cd7f
```

我靠，直接 segfault 了，这是 qemu 的一个 bug 啊:
```txt
[106282.802351] qemu-system-x86[492837]: segfault at 7f58b47f9ff4 ip 00007f5b41f55661 sp 00007f58b47f9fb0 error 6 in
libc.so.6[61661,7f5b41f1c000+168000] likely on CPU 10 (core 20, socket 0)
```

## [ ] seabios 中为什么会走这里，我不知道在哪里设置的

```c
// This code is hardcoded for PIIX4 Power Management device.
static void piix4_apmc_smm_setup(int isabdf, int i440_bdf)
{
    /* check if SMM init is already done */
    u32 value = pci_config_readl(isabdf, PIIX_DEVACTB);
    if (value & PIIX_DEVACTB_APMC_EN){
	    printf("[martins3:%s:%d] %x\n", __FUNCTION__, __LINE__, isabdf);
	    dprintf(1, "[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
        return;
    }
```

## 似乎出现了异常的覆盖，但是 qemu 代码的版本似乎不对
所以，这个问题，看上去是初始化的地方不对啊
```txt
#0  pc_init1 (machine=0x55555733c750, pci_type=0x555555f52461 "i440FX") at ../hw/i386/pc_piix.c:105
#1  0x0000555555953311 in machine_run_board_init (machine=0x55555733c750, mem_path=<optimized out>, errp=<optimized out>,
    errp@entry=0x555556fe36f8 <error_fatal>) at ../hw/core/machine.c:1682
#2  0x0000555555b20758 in qemu_init_board () at ../system/vl.c:2711
#3  qmp_x_exit_preconfig (errp=0x555556fe36f8 <error_fatal>) at ../system/vl.c:2807
#4  0x0000555555b24035 in qemu_init (argc=<optimized out>, argv=<optimized out>) at ../system/vl.c:3843
#5  0x0000555555890449 in main (argc=<optimized out>, argv=<optimized out>) at ../system/main.c:68
```

```txt
#0  pci_piix_realize (dev=0x5555578bd2e0, uhci_type=0x555555f4d8b6 "piix3-usb-uhci", errp=0x7ffffffefc20)
    at ../hw/isa/piix.c:300
#1  0x0000555555a35fd8 in pci_qdev_realize (qdev=<optimized out>, errp=<optimized out>) at ../hw/pci/pci.c:2263
#2  0x0000555555d34b7b in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7ffffffefd40)
    at ../hw/core/qdev.c:494
#3  0x0000555555d37dad in property_set_bool (obj=0x5555578bd2e0, v=<optimized out>, name=<optimized out>,
    opaque=0x5555570dc6c0, errp=0x7ffffffefd40) at ../qom/object.c:2374
#4  0x0000555555d3adeb in object_property_set (obj=obj@entry=0x5555578bd2e0, name=name@entry=0x555555f5adb4 "realized",
    v=v@entry=0x5555578c6440, errp=0x7ffffffefd40, errp@entry=0x555556fe36f8 <error_fatal>) at ../qom/object.c:1449
#5  0x0000555555d3eadf in object_property_set_qobject (obj=obj@entry=0x5555578bd2e0,
    name=name@entry=0x555555f5adb4 "realized", value=value@entry=0x5555570346d0, errp=errp@entry=0x555556fe36f8 <error_fatal>)
    at ../qom/qom-qobject.c:28
#6  0x0000555555d3b444 in object_property_set_bool (obj=obj@entry=0x5555578bd2e0, name=name@entry=0x555555f5adb4 "realized",
    value=value@entry=true, errp=errp@entry=0x555556fe36f8 <error_fatal>) at ../qom/object.c:1519
#7  0x0000555555d3432c in qdev_realize (dev=dev@entry=0x5555578bd2e0, bus=<optimized out>,
    errp=errp@entry=0x555556fe36f8 <error_fatal>) at ../hw/core/qdev.c:276
#8  0x0000555555d343ce in qdev_realize_and_unref (dev=dev@entry=0x5555578bd2e0, bus=<optimized out>,
    errp=errp@entry=0x555556fe36f8 <error_fatal>) at ../hw/core/qdev.c:283
#9  0x0000555555a344a5 in pci_realize_and_unref (dev=dev@entry=0x5555578bd2e0, bus=<optimized out>,
    errp=errp@entry=0x555556fe36f8 <error_fatal>) at ../hw/pci/pci.c:2356
#10 0x0000555555bff5b8 in pc_init1 (machine=0x55555733c750, pci_type=<optimized out>) at ../hw/i386/pc_piix.c:263
#11 0x0000555555953311 in machine_run_board_init (machine=0x55555733c750, mem_path=<optimized out>, errp=<optimized out>,
    errp@entry=0x555556fe36f8 <error_fatal>) at ../hw/core/machine.c:1682
#12 0x0000555555b20758 in qemu_init_board () at ../system/vl.c:2711
#13 qmp_x_exit_preconfig (errp=0x555556fe36f8 <error_fatal>) at ../system/vl.c:2807
#14 0x0000555555b24035 in qemu_init (argc=<optimized out>, argv=<optimized out>) at ../system/vl.c:3843
#15 0x0000555555890449 in main (argc=<optimized out>, argv=<optimized out>) at ../system/main.c:68
```
### 的确很奇怪，之前似乎是没有 bug 的，是最近才有的

0a15cf0801815a359af211361fba309a2cc5c1e8

相当于这个总是 true 了:
```c
        qdev_prop_set_bit(DEVICE(&d->pm), "smm-enabled", d->smm_enabled);
```

注意，这里有好几个 smm-enabled 变量:

PIIX4PMState::smm_enabled

```txt
static const Property piix4_pm_properties[] = {
    // ...
    DEFINE_PROP_BOOL("smm-enabled", PIIX4PMState, smm_enabled, false),
    // ...
};

```

PIIXState::smm_enabled
```txt
static const Property pci_piix_props[] = {
    // ...
    DEFINE_PROP_BOOL("smm-enabled", PIIXState, smm_enabled, false),
    // ...
};
```

pci_piix_realize 中的，将 PIIXState::smm_enabled 传递给 PIIX4PMState::smm_enabled

```txt
        qdev_prop_set_bit(DEVICE(&d->pm), "smm-enabled", d->smm_enabled);
```

pc_init1 中设置的是 PIIXState 的参数，因为

所以，现在参数完全是反过来的啊

## q35 + seabios 情况特殊
这里引入了一个更加有趣的问题，是不是通过初始化 smram 之后，才可以触发 smi 中断。

```txt
static const MemoryRegionOps ich9_smi_ops = {
    .read = ich9_smi_readl,
    .write = ich9_smi_writel,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .endianness = DEVICE_LITTLE_ENDIAN,
};
```

在 pm_reset 中可以观察到 ICH9LPCPMRegs::smm_enabled 和 smm=on / off 控制

如果是 bios 启动，q35 ，无论 on / off 都有问题。
而 ovmf ，on 无问题，off 有问题

## 终极问题
- 似乎很难理解，既然是 windows 自己触发的 SMI ，那么有什么意义?
  - 同样的，既然是 uefi 的代码自己在代码中触发的，意义何在
  - 继续找找 pxe 触发的 SMI 的场景，理解一下基本问题

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
