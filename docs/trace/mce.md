# mce 的工作原理

- https://unix.stackexchange.com/questions/451655/running-mcelog-on-an-amd-processor
- https://www.cnblogs.com/dataart/p/10374028.html

- f9781bb18ed828e7b83b7bac4a4ad7cd497ee7d7

- [ ] 在老内核上可以正确运行吗 ?

## 文档
- https://docs.kernel.org/driver-api/edac.html : edac 的文档
- https://lwn.net/Articles/480575/ : edac 的更新 patch


## 能不能注册 ecc 的错误给 guest

ecc 是 mce 的一种才对

## mce

- 发现只要是替换内核，那么 /dev/ 下没有 mcelog 的

- mcelog 操作需要/dev/mcelog 设备，这个设备通常自动由 udev 创建，也可以通过手工命令创建 mknod /dev/mcelog c 10 227。设备创建后剋通过 ls -lh /dev/mcelog 检查：
  - [ ] 似乎 centos 8 没有办法自动创建

> 默认没有配置 /sys/devices/system/machinecheck/machinecheck0/trigger，这时这个内容是空的。当将/usr/sbin/mcelog 添加到这个 proc 文件中
，就会在内核错误发生时触发运行/usr/sbin/mcelog 来处理解码错误日志，方便排查故障。

/etc/mcelog/mcelog.conf 是 mcelog 配置文件


这一步似乎是必须的:
- modprobe mce-inject
- cd /sys/devices/system/machinecheck/machinecheck0 && echo 3 > tolerant # 为了防止出现 hardware 错误的时候，不要将机器 panic

注入的方法: https://www.cnblogs.com/augusite/p/15561662.html

## 参考资料
- https://huataihuang.gitbooks.io/cloud-atlas/content/os/linux/log/mcelog.html
- https://www.cnblogs.com/muahao/p/6003910.html
- https://stackoverflow.com/questions/38496643/how-can-we-generate-mcemachine-check-errors : 如何使用 memory inject
- https://mcelog.org/ : 官方文档

## mce 和 edac 的关系
- https://cloud-atlas.readthedocs.io/zh_CN/latest/linux/server/hardware/edac.html

- https://mcelog.org/faq.html#13 : mcelog 认为 edac 可以在 AMD 上工作

- [ ] 为什么 AMD 无法正确的支持 mcelog 的哇 ?

- https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=890301
  - 从 debian 中也是将这个关闭了

## [ ] mcedev 已经被内核抛弃了

```c
config X86_MCELOG_LEGACY
    bool "Support for deprecated /dev/mcelog character device"
    depends on X86_MCE
    help
      Enable support for /dev/mcelog which is needed by the old mcelog
      userspace logging daemon. Consider switching to the new generation
      rasdaemon solution.
```

内核中在 2017 的时候将这个移除了:
```diff
History:        #0
Commit:         5de97c9f6d85fd83af76e09e338b18e7adb1ae60
Author:         Tony Luck <tony.luck@intel.com>
Committer:      Ingo Molnar <mingo@kernel.org>
Author Date:    Mon 27 Mar 2017 05:33:03 PM CST
Committer Date: Tue 28 Mar 2017 02:55:01 PM CST

x86/mce: Factor out and deprecate the /dev/mcelog driver

Move all code relating to /dev/mcelog to a separate source file.
/dev/mcelog driver can now operate from the machine check notifier with
lowest prio.

Signed-off-by: Tony Luck <tony.luck@intel.com>
[ Move the mce_helper and trigger functionality behind CONFIG_X86_MCELOG_LEGACY. ]
Signed-off-by: Borislav Petkov <bp@suse.de>
Cc: Linus Torvalds <torvalds@linux-foundation.org>
Cc: Peter Zijlstra <peterz@infradead.org>
Cc: Thomas Gleixner <tglx@linutronix.de>
Cc: linux-edac <linux-edac@vger.kernel.org>
Link: http://lkml.kernel.org/r/20170327093304.10683-6-bp@alien8.de
[ Renamed CONFIG_X86_MCELOG to CONFIG_X86_MCELOG_LEGACY. ]
Signed-off-by: Ingo Molnar <mingo@kernel.org>

Signed-off-by: Ingo Molnar <mingo@kernel.org>
```

- 相关的讨论:
  - https://lore.kernel.org/all/20170327093304.10683-6-bp@alien8.de/T/#u

## [ ] 确认一下，在 ARM 中也是使用 rasdaemon 的

- [ ] 是不是 ARM 上没有 mce 而已?

## x86 mce 处理流程

```c
/*
 * The default IDT entries which are set up in trap_init() before
 * cpu_init() is invoked. Interrupt stacks cannot be used at that point and
 * the traps which use them are reinitialized with IST after cpu_init() has
 * set up TSS.
 */
static const __initconst struct idt_data def_idts[] = {
    INTG(X86_TRAP_DE,       asm_exc_divide_error),
    INTG(X86_TRAP_NMI,      asm_exc_nmi),
    INTG(X86_TRAP_BR,       asm_exc_bounds),
    INTG(X86_TRAP_UD,       asm_exc_invalid_op),
    INTG(X86_TRAP_NM,       asm_exc_device_not_available),
    INTG(X86_TRAP_OLD_MF,       asm_exc_coproc_segment_overrun),
    INTG(X86_TRAP_TS,       asm_exc_invalid_tss),
    INTG(X86_TRAP_NP,       asm_exc_segment_not_present),
    INTG(X86_TRAP_SS,       asm_exc_stack_segment),
    INTG(X86_TRAP_GP,       asm_exc_general_protection),
    INTG(X86_TRAP_SPURIOUS,     asm_exc_spurious_interrupt_bug),
    INTG(X86_TRAP_MF,       asm_exc_coprocessor_error),
    INTG(X86_TRAP_AC,       asm_exc_alignment_check),
    INTG(X86_TRAP_XF,       asm_exc_simd_coprocessor_error),

#ifdef CONFIG_X86_32
    TSKG(X86_TRAP_DF,       GDT_ENTRY_DOUBLEFAULT_TSS),
#else
    INTG(X86_TRAP_DF,       asm_exc_double_fault),
#endif
    INTG(X86_TRAP_DB,       asm_exc_debug),

#ifdef CONFIG_X86_MCE
    INTG(X86_TRAP_MC,       asm_exc_machine_check),
#endif

    SYSG(X86_TRAP_OF,       asm_exc_overflow),
#if defined(CONFIG_IA32_EMULATION)
    SYSG(IA32_SYSCALL_VECTOR,   entry_INT80_compat),
#elif defined(CONFIG_X86_32)
    SYSG(IA32_SYSCALL_VECTOR,   entry_INT80_32),
#endif
};


/*
 * Fields are zero when not available. Also, this struct is shared with
 * userspace mcelog and thus must keep existing fields at current offsets.
 * Only add new fields to the end of the structure
 */
struct mce {
    __u64 status;       /* Bank's MCi_STATUS MSR */
    __u64 misc;     /* Bank's MCi_MISC MSR */
    __u64 addr;     /* Bank's MCi_ADDR MSR */
    __u64 mcgstatus;    /* Machine Check Global Status MSR */
    __u64 ip;       /* Instruction Pointer when the error happened */
    __u64 tsc;      /* CPU time stamp counter */
    __u64 time;     /* Wall time_t when error was detected */
    __u8  cpuvendor;    /* Kernel's X86_VENDOR enum */
    __u8  inject_flags; /* Software inject flags */
    __u8  severity;     /* Error severity */
    __u8  pad;
    __u32 cpuid;        /* CPUID 1 EAX */
    __u8  cs;       /* Code segment */
    __u8  bank;     /* Machine check bank reporting the error */
    __u8  cpu;      /* CPU number; obsoleted by extcpu */
    __u8  finished;     /* Entry is valid */
    __u32 extcpu;       /* Linux CPU number that detected the error */
    __u32 socketid;     /* CPU socket ID */
    __u32 apicid;       /* CPU initial APIC ID */
    __u64 mcgcap;       /* MCGCAP MSR: machine check capabilities of CPU */
    __u64 synd;     /* MCA_SYND MSR: only valid on SMCA systems */
    __u64 ipid;     /* MCA_IPID MSR: only valid on SMCA systems */
    __u64 ppin;     /* Protected Processor Inventory Number */
    __u32 microcode;    /* Microcode revision */
    __u64 kflags;       /* Internal kernel use */
};

```
- do_machine_check
  - `__mc_scan_banks`
    - mce_read_aux ：初始化 struct mce ，通过 mce_rdmsrl
    - mce_log


## 错误注入的方法
MCE error handling can use the MCE inject:
    https://git.kernel.org/pub/scm/utils/cpu/mce/mce-inject.git
For it to work, Kernel mce-inject module should be compiled and loaded.

> mce-inject 似乎很老了，似乎是依赖 /dev/mcelog 的
> mce 只是 x86 专用的

APEI error injection can use this tool:
    https://git.kernel.org/pub/scm/linux/kernel/git/gong.chen/mce-test.git/

AER error injection can use this tool:
    https://git.kernel.org/pub/scm/linux/kernel/git/gong.chen/aer-inject.git/


从现在的内核上来说，mce-inject 是通过 debugfs 来进行的

- https://linux.die.net/man/8/mce-inject
- https://stackoverflow.com/questions/38496643/how-can-we-generate-mcemachine-check-errors ：

注入错误之后，如何检查:
- https://unix.stackexchange.com/questions/533196/where-does-rasdaemon-record-its-logs

## APEI 错误
似乎是 ACPI 有关的:
- https://blog.csdn.net/qq_21186033/article/details/116977474

## [ ] AER 是在这个体系下的吗

## edac 和 mce 是什么关系，可以从内核中找到吗

虽然 /dev/mcelog 被移除了，但是 mce 文件夹还是存在很多内容哇

## 这三个错误的注入都是依靠什么的
简单的检测一下，发现其中的 : arch/x86/kernel/cpu/mce/inject.c

## mcelog 的基本工作原理
很多错误并不是致命的，mcelog 可以将周期性的错误汇总一下:

基本的传输过程是: mce_log 将 mce queue 到 mcedev 上，最后通过 /dev/mcedev 提供取出。

## [ ] 了解 rasdaemon 的工作原理
https://github.com/mchehab/rasdaemon

- 为什么感觉似乎是只能收集内存错误
  - [ ] 内存错误不是 mce 中的一种

> Its long term goal is to be the userspace tool that will collect all
hardware error events reported by the Linux Kernel from several sources
(EDAC, MCE, PCI, ...) into one common framework.

> 难道 EDAC 和 MCE 不是一个东西 ?

--enable-aer            enable PCIe AER events (currently experimental)
--enable-mce            enable MCE events (currently experimental)

- [ ] 为什么 mce 只是实验的功能 ?
- [ ] 是如何支持 mce 功能的 ?

分析函数 read_ras_event_all_cpus，应该是通过 ftrace 导出的。

## edac 中，intel 也是支持的
https://lkml.iu.edu/hypermail/linux/kernel/1903.0/02742.html

## [ ] 和虚拟化有关系吗

同时，在 QEMU 中有 mce_init 来初始化的

- mce_init : machine check exception, 初始化之后，那些 helper 就可以正确工作了

## kvm 如何支持 mce

- kvm_vcpu_ioctl_x86_setup_mce
- kvm_vcpu_ioctl_x86_set_mce 向 guest 注入 mce
  - 初始化 vcpu->arch.mce_banks; @todo 暂时不知道如何这个 bank 是如何传递给 Guest 的
  - kvm_queue_exception(vcpu, MC_VECTOR);

应该是设置一下 msr 寄存器之类的操作吧

```txt
4.105 KVM_X86_SETUP_MCE

Capability: KVM_CAP_MCE
Architectures: x86
Type: vcpu ioctl
Parameters: u64 mcg_cap (in)
Returns: 0 on success,
         -EFAULT if u64 mcg_cap cannot be read,
         -EINVAL if the requested number of banks is invalid,
         -EINVAL if requested MCE capability is not supported.

Initializes MCE support for use. The u64 mcg_cap parameter
has the same format as the MSR_IA32_MCG_CAP register and
specifies which capabilities should be enabled. The maximum
supported number of error-reporting banks can be retrieved when
checking for KVM_CAP_MCE. The supported capabilities can be
retrieved with KVM_X86_GET_MCE_CAP_SUPPORTED.

4.106 KVM_X86_SET_MCE

Capability: KVM_CAP_MCE
Architectures: x86
Type: vcpu ioctl
Parameters: struct kvm_x86_mce (in)
Returns: 0 on success,
         -EFAULT if struct kvm_x86_mce cannot be read,
         -EINVAL if the bank number is invalid,
         -EINVAL if VAL bit is not set in status field.

Inject a machine check error (MCE) into the guest. The input
parameter is:

struct kvm_x86_mce {
    __u64 status;
    __u64 addr;
    __u64 misc;
    __u64 mcg_status;
    __u8 bank;
    __u8 pad1[7];
    __u64 pad2[3];
};

If the MCE being reported is an uncorrected error, KVM will
inject it as an MCE exception into the guest. If the guest
MCG_STATUS register reports that an MCE is in progress, KVM
causes an KVM_EXIT_SHUTDOWN vmexit.

Otherwise, if the MCE is a corrected error, KVM will just
store it in the corresponding bank (provided this bank is
not holding a previously reported uncorrected error).
```

## rasdaemon 的一段 log

- aer_event
```txt
systemd[1]: Starting RAS daemon to log the RAS events...
rasdaemon[644709]: rasdaemon: ras:mc_event event enabled
rasdaemon[644709]: rasdaemon: Enabled event ras:mc_event
rasdaemon[644709]: rasdaemon: ras:aer_event event enabled
rasdaemon[644709]: rasdaemon: Enabled event ras:aer_event
rasdaemon[644709]: rasdaemon: mce:mce_record event enabled
rasdaemon[644709]: rasdaemon: Enabled event mce:mce_record
rasdaemon[644709]: rasdaemon: ras:extlog_mem_event event enabled
rasdaemon[644709]: rasdaemon: Enabled event ras:extlog_mem_event
rasdaemon[644709]: ras:mc_event event enabled
rasdaemon[644709]: Enabled event ras:mc_event
rasdaemon[644709]: ras:aer_event event enabled
rasdaemon[644709]: Enabled event ras:aer_event
rasdaemon[644709]: mce:mce_record event enabled
rasdaemon[644709]: Enabled event mce:mce_record
rasdaemon[644709]: ras:extlog_mem_event event enabled
rasdaemon[644709]: Enabled event ras:extlog_mem_event
systemd[1]: Started RAS daemon to log the RAS events.
rasdaemon[644709]: wait_access() failed, /sys/kernel/debug/tracing/instances/rasdaemon/events>
rasdaemon[644709]: rasdaemon: wait_access() failed, /sys/kernel/debug/tracing/instances/rasda>
rasdaemon[644709]: rasdaemon: Can't get net:net_dev_xmit_timeout traces. Perhaps this feature>
rasdaemon[644709]: wait_access() failed, /sys/kernel/debug/tracing/instances/rasdaemon/events>
rasdaemon[644709]: rasdaemon: wait_access() failed, /sys/kernel/debug/tracing/instances/rasda>
rasdaemon[644709]: rasdaemon: Can't get devlink:devlink_health_report traces. Perhaps this fe>
rasdaemon[644709]: rasdaemon: Can't get traces from devlink:devlink_health_report
rasdaemon[644709]: rasdaemon: block:block_rq_complete event enabled
rasdaemon[644709]: rasdaemon: Enabled event block:block_rq_complete
rasdaemon[644709]: rasdaemon: Listening to events for cpus 0 to 0
rasdaemon[644709]: overriding event (1153) ras:mc_event with new print handler
rasdaemon[644709]: overriding event (1150) ras:aer_event with new print handler
rasdaemon[644709]: overriding event (113) mce:mce_record with new print handler
rasdaemon[644709]: overriding event (1154) ras:extlog_mem_event with new print handler
rasdaemon[644709]: overriding event (978) block:block_rq_c
```

## manual
Intel 64 and IA32 Architectures Software Developer's manual, Volume 3, System programming guide Parts 1 and 2. Machine checks are described in Chapter 14 in Part1 and in Append ix E in Part2.
