# preempt rt
## 最近 RT 更新
我按你本地这棵较新的内核仓库扫了一下，最近关于 RT 的更新大致不是“一个大功能落地”，而是持续做两类事情：一类是把更多子系统适配到 PREEMPT_RT 语义；另一类是继续清理 scheduler / softirq / irq 里不适合 RT 的旧假设。
按时间线看，比较值得记的有这些：

```txt
2026-02-16 f8e6343b7a89 Drivers: hv: vmbus: Use kthread for vmbus interrupts on PREEMPT_RT

这是很典型的 RT 方向：把原本不适合 RT 的中断处理继续往线程化上下文迁。
2026-02-13 26f29b149169 net: fix backlog_unlock_irq_restore() vs CONFIG_PREEMPT_RT

说明网络栈里仍在持续修 RT 语义下的锁/irq 假设问题。
2026-01-22 94894c9c477e sched/rt: Skip currently executing CPU in rto_next_cpu()

这是直接落在 sched/rt 上的修正，属于 RT 调度类本身的小步改进。
2025-11-27 22abd832776b sched/rt: Remove a preempt-disable section in rt_mutex_setprio()

这个很有代表性：继续减少不必要的 preempt-disable 区域，让 RT 路径更干净，也更符合“尽量缩短不可抢占区”的总方向。
2025-09-04 3253cb49cbad softirq: Allow to drop the softirq-BKL lock on PREEMPT_RT

这是 softirq/底半部方向的 RT 改进，和我们前面说的“RT 要把更多执行时间重新纳入 scheduler 控制”是一致的。
2025-09 一串驱动和子系统修复

ALSA: pcm: Disable bottom softirqs as part of spin_lock_irq() on PREEMPT_RT
usbip: Fix locking bug in RT-enabled kernels
bpf: Reject bpf_timer for PREEMPT_RT
```

这些说明近一阶段 RT 的重点之一，是把各子系统在 RT 语义下的边界条件一个个补齐。
所以如果你问“最近 RT 的更新方向是什么”，我会总结成 4 点：

继续做 irq / softirq 线程化与上下文整理
继续减少 preempt_disable() 和错误的 atomic-context 假设
继续把普通子系统的锁语义修到兼容 RT
sched/rt 本身也有小步修正，但最近主旋律不是“重写 RT 调度算法”，而是“让整个内核更像一个真正可实时化的系统”
一句话说，最近的 RT 更新更像“系统性收尾和扩面”，不是再来一次像当年 CONFIG_PREEMPT_RT 那样的大里程碑。


## 和 preemption 的区别

> [!NOTE]
> 参考神奇海螺的意见，有待验证

PREEMPT_DYNAMIC：只是让同一份内核在启动时选择抢占模型，比如 none/voluntary/full/lazy。
PREEMPT_LAZY：是普通抢占模型的一种，调度器没那么激进地打断 SCHED_NORMAL 任务，目的是减少 lock holder preemption，拿回一点吞吐。
PREEMPT_RT：不是“再调一个抢占档位”，而是把锁、中断、softirq 这些底层机制都改了。
2）scheduler 看到的世界不一样

在 PREEMPT_LAZY + DYNAMIC 下，scheduler 仍然运行在普通主线语义上：很多 spinlock_t 临界区还是不可睡、会阻挡调度器介入。
在 PREEMPT_RT 下，很多原本挡住 scheduler 的路径被改成可抢占/可睡眠/可线程化，调度器能更频繁、更早接管 CPU。
3）锁和中断语义差别最大

PREEMPT_LAZY + DYNAMIC：spinlock_t 还是普通 spinlock 语义；softirq、中断整体也还是传统主线模型。
PREEMPT_RT：spinlock_t/rwlock_t 大量走 rtmutex 语义，带优先级继承；中断线程化；softirq 更可抢占。这才是 RT 延迟下降的根子。
所以如果你问“它们和 scheduler 的关系差在哪”：

PREEMPT_LAZY + PREEMPT_DYNAMIC 只是调整 scheduler 何时更积极地抢占；
PREEMPT_RT 是把更多内核执行时间真正纳入 scheduler 控制。
再落到你当前环境：

你现在这台机器是 CONFIG_PREEMPT_DYNAMIC=y、CONFIG_PREEMPT_LAZY=y、CONFIG_PREEMPT_RT 没开。
这说明你当前看到的是“主线可调抢占 + lazy 策略”，不是 RT 语义。
一句话总结：lazy/dynamic 是“同一套主线调度规则下，调节抢占积极程度”；RT 是“连锁、中断、softirq 的运行语义都变了”，所以它不是更激进的 full，而是另一类内核。

## 到底涉及什么代码?

```txt
📁 核心代码分布

 目录               主要内容
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 kernel/locking/    RT-Mutex、RT-Spinlock、读写锁 RT 实现
 kernel/sched/      实时调度、RT 锁等待调度
 kernel/time/       高精度定时器软中断化
 kernel/softirq.c   可抢占软中断
 kernel/rcu/        RCU 线程化回调

🔑 关键文件

• kernel/locking/rtmutex.c - RT-Mutex 核心（优先级继承机制）
• kernel/locking/spinlock_rt.c - Spinlock 的 RT 替代实现
• kernel/locking/rwbase_rt.c - 读写锁基础 RT 实现
• include/linux/spinlock_rt.h - RT Spinlock 头文件
• include/linux/local_lock_internal.h - 本地锁 RT 实现

⚡ 核心机制

1. 睡眠锁替代自旋锁 - spinlock_t 基于 rt_mutex_base 实现，可睡眠而非忙等待
2. 中断线程化 - 所有中断默认在线程上下文执行
3. 软中断可抢占 - softirq 在进程上下文运行
4. 优先级继承 - 防止优先级反转问题

📊 PREEMPT_RT vs 普通内核对比

 特性            普通内核          PREEMPT_RT
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 spinlock        忙等待+禁用抢占   睡眠等待+禁用迁移
 软中断          不可抢占          可抢占
 hrtimer 默认    硬中断            软中断(ktimersd)
 RCU callbacks   softirq           线程上下文
```

## 虚拟机开机日志说明

```txt
[    3.087572] sd 1:0:1:0: [sdb] Attached SCSI disk
[    3.122583] kvm: RT requires X86_FEATURE_CONSTANT_TSC
[    4.435458] scsi 1:0:20:1: CD-ROM            QEMU     QEMU CD-ROM      2.5+ PQ: 0 ANSI: 5
[    4.438642] BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
[    4.438652] in_atomic(): 0, irqs_disabled(): 0, non_block: 0, pid: 16, name: pr/legacy
[    4.438657] preempt_count: 0, expected: 0
[    4.438660] RCU nest depth: 1, expected: 0
[    4.438664] 4 locks held by pr/legacy/16:
[    4.438668]  #0: ffffffff82963500 (console_lock){+.+.}-{0:0}, at: legacy_kthread_func+0x6d/0x150
[    4.438695]  #1: ffffffff82963558 (console_srcu){....}-{0:0}, at: console_flush_one_record+0x85/0x4f0
[    4.438712]  #2: ffffffff829fad40 (printing_lock){+.+.}-{3:3}, at: vt_console_print+0x5d/0x490
[    4.438732]  #3: ffffffff82966380 (rcu_read_lock){....}-{1:3}, at: rt_spin_trylock+0x62/0x130
[    4.438753] CPU: 18 UID: 0 PID: 16 Comm: pr/legacy Not tainted 6.19.6-00001-ge27d5805c1f5 #2 PREEMPT_{RT,(full)}
[    4.438762] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[    4.438773] Call Trace:
[    4.438780]  <TASK>
[    4.438789]  dump_stack_lvl+0x75/0xb0
[    4.438810]  __might_resched.cold+0xcc/0xe1
[    4.438832]  console_conditional_schedule+0x2f/0x40
[    4.438849]  fbcon_redraw+0x99/0x230 [fb]
[    4.438914]  fbcon_scroll+0x17f/0x1e0 [fb]
[    4.438951]  con_scroll+0x111/0x240
[    4.438982]  lf+0xa8/0xb0
[    4.439003]  vt_console_print+0x324/0x490
[    4.439044]  console_flush_one_record+0x2a2/0x4f0
[    4.439081]  ? __pfx_legacy_kthread_func+0x10/0x10
[    4.439094]  legacy_kthread_func+0x82/0x150
[    4.439109]  ? __pfx_autoremove_wake_function+0x10/0x10
[    4.439135]  kthread+0x123/0x240
[    4.439159]  ? __pfx_kthread+0x10/0x10
[    4.439177]  ret_from_fork+0x288/0x330
[    4.439185]  ? __pfx_kthread+0x10/0x10
[    4.439198]  ret_from_fork_asm+0x1a/0x30
[    4.439265]  </TASK>
[    4.440823] sr 1:0:20:1: Power-on or device reset occurred
[    4.441402] sr 1:0:20:1: [sr0] scsi3-mmc drive: 16x/50x cd/rw xa/form2 cdda tray
[    4.441616] cdrom: Uniform CD-ROM driver Revision: 3.20
[    4.476979] sr 1:0:20:1: Attached scsi CD-ROM sr0
[   13.258709] SGI XFS with ACLs, security attributes, realtime, scrub, repair, quota, no debug enabled
[   13.259133] BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
[   13.259134] in_atomic(): 0, irqs_disabled(): 0, non_block: 0, pid: 16, name: pr/legacy
[   13.259135] preempt_count: 0, expected: 0
[   13.259136] RCU nest depth: 1, expected: 0
[   13.259136] 4 locks held by pr/legacy/16:
[   13.259137]  #0: ffffffff82963500 (console_lock){+.+.}-{0:0}, at: legacy_kthread_func+0x6d/0x150
[   13.259143]  #1: ffffffff82963558 (console_srcu){....}-{0:0}, at: console_flush_one_record+0x85/0x4f0
[   13.259146]  #2: ffffffff829fad40 (printing_lock){+.+.}-{3:3}, at: vt_console_print+0x5d/0x490
[   13.259150]  #3: ffffffff82966380 (rcu_read_lock){....}-{1:3}, at: rt_spin_trylock+0x62/0x130
[   13.259155] CPU: 18 UID: 0 PID: 16 Comm: pr/legacy Tainted: G        W           6.19.6-00001-ge27d5805c1f5 #2 PREEMPT_{RT,(full)}
[   13.259157] Tainted: [W]=WARN
[   13.259158] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[   13.259158] Call Trace:
[   13.259161]  <TASK>
[   13.259163]  dump_stack_lvl+0x75/0xb0
[   13.259167]  __might_resched.cold+0xcc/0xe1
[   13.259172]  console_conditional_schedule+0x2f/0x40
[   13.259176]  fbcon_redraw+0x99/0x230 [fb]
[   13.259189]  fbcon_scroll+0x17f/0x1e0 [fb]
[   13.259197]  con_scroll+0x111/0x240
[   13.259203]  lf+0xa8/0xb0
[   13.259207]  vt_console_print+0x324/0x490
[   13.259215]  console_flush_one_record+0x2a2/0x4f0
[   13.259222]  ? __pfx_legacy_kthread_func+0x10/0x10
[   13.259224]  legacy_kthread_func+0x82/0x150
[   13.259227]  ? __pfx_autoremove_wake_function+0x10/0x10
[   13.259232]  kthread+0x123/0x240
[   13.259237]  ? __pfx_kthread+0x10/0x10
[   13.259240]  ret_from_fork+0x288/0x330
[   13.259242]  ? __pfx_kthread+0x10/0x10
[   13.259244]  ret_from_fork_asm+0x1a/0x30
[   13.259257]  </TASK>
[   13.270289] XFS (dm-0): Mounting V5 Filesystem bb99c5b8-43c5-42ef-be3b-51ebff93f03a
[   13.298831] XFS (dm-0): Starting recovery (logdev: internal)
[   13.388408] XFS (dm-0): Ending recovery (logdev: internal)
[   14.087429] systemd-journald[400]: Received SIGTERM from PID 1 (systemd).
[   14.341831] systemd[1]: systemd 257.10-1.fc42 running in system mode (+PAM +AUDIT +SELINUX -APPARMOR +IMA +IPE +SMACK +SECCOMP -GCRYPT +GNUTLS +OPENSSL +ACL +BLKID +CURL +ELFUTILS +FIDO2 +IDN2 -IDN -IPTC +KMOD +LIBCRYPTSETUP +LIBCRYPTSETUP_PLUGINS +LIBFDISK +PCRE2 +PWQUALITY +P11KIT +QRENCODE +TPM2 +BZIP2 +LZ4 +XZ +ZLIB +ZSTD +BPF_FRAMEWORK +BTF +XKBCOMMON +UTMP +SYSVINIT +LIBARCHIVE)
[   14.341836] systemd[1]: Detected virtualization kvm.
[   14.341843] systemd[1]: Detected architecture x86-64.
[   14.344361] BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
[   14.344363] in_atomic(): 0, irqs_disabled(): 0, non_block: 0, pid: 16, name: pr/legacy
[   14.344363] preempt_count: 0, expected: 0
[   14.344364] RCU nest depth: 1, expected: 0
[   14.344364] 4 locks held by pr/legacy/16:
[   14.344365]  #0: ffffffff82963500 (console_lock){+.+.}-{0:0}, at: legacy_kthread_func+0x6d/0x150
[   14.344371]  #1: ffffffff82963558 (console_srcu){....}-{0:0}, at: console_flush_one_record+0x85/0x4f0
[   14.344373]  #2: ffffffff829fad40 (printing_lock){+.+.}-{3:3}, at: vt_console_print+0x5d/0x490
[   14.344375]  #3: ffffffff82966380 (rcu_read_lock){....}-{1:3}, at: rt_spin_trylock+0x62/0x130
[   14.344379] CPU: 18 UID: 0 PID: 16 Comm: pr/legacy Tainted: G        W           6.19.6-00001-ge27d5805c1f5 #2 PREEMPT_{RT,(full)}
[   14.344381] Tainted: [W]=WARN
[   14.344382] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[   14.344382] Call Trace:
[   14.344384]  <TASK>
[   14.344385]  dump_stack_lvl+0x75/0xb0
[   14.344388]  __might_resched.cold+0xcc/0xe1
[   14.344392]  console_conditional_schedule+0x2f/0x40
[   14.344394]  fbcon_redraw+0x99/0x230 [fb]
[   14.344403]  fbcon_scroll+0x17f/0x1e0 [fb]
[   14.344408]  con_scroll+0x111/0x240
[   14.344412]  lf+0xa8/0xb0
[   14.344415]  vt_console_print+0x324/0x490
[   14.344420]  console_flush_one_record+0x2a2/0x4f0
[   14.344425]  ? __pfx_legacy_kthread_func+0x10/0x10
[   14.344426]  legacy_kthread_func+0x82/0x150
[   14.344428]  ? __pfx_autoremove_wake_function+0x10/0x10
[   14.344433]  kthread+0x123/0x240
[   14.344436]  ? __pfx_kthread+0x10/0x10
[   14.344438]  ret_from_fork+0x288/0x330
[   14.344440]  ? __pfx_kthread+0x10/0x10
[   14.344441]  ret_from_fork_asm+0x1a/0x30
[   14.344449]  </TASK>
[   14.383023] systemd[1]: bpf-restrict-fs: BPF LSM hook not enabled in the kernel, BPF LSM not supported.
[   14.852995] systemd[1]: initrd-switch-root.service: Deactivated successfully.
[   14.856643] systemd[1]: Stopped initrd-switch-root.service - Switch Root.
[   14.879347] systemd[1]: systemd-journald.service: Scheduled restart job, restart counter is at 1.
[   14.918740] systemd[1]: Created slice system-getty.slice - Slice /system/getty.
[   14.923626] systemd[1]: Created slice system-serial\x2dgetty.slice - Slice /system/serial-getty.
[   14.930083] systemd[1]: Created slice system-sshd\x2dkeygen.slice - Slice /system/sshd-keygen.
[   14.935000] systemd[1]: Created slice system-systemd\x2dzram\x2dsetup.slice - Slice /system/systemd-zram-setup.
[   14.941325] systemd[1]: Created slice user.slice - User and Session Slice.
[   14.941604] systemd[1]: systemd-ask-password-console.path - Dispatch Password Requests to Console Directory Watch was skipped because of an unmet condition check (ConditionPathExists=!/run/plymouth/pid).
[   14.942378] systemd[1]: Started systemd-ask-password-wall.path - Forward Password Requests to Wall Directory Watch.
[   14.944739] systemd[1]: proc-sys-fs-binfmt_misc.automount - Arbitrary Executable File Formats File System Automount Point was skipped because of an unmet condition check (ConditionPathExists=/proc/sys/fs/binfmt_misc).
[   14.944892] systemd[1]: Expecting device dev-disk-by\x2duuid-71d81e4b\x2d5e84\x2d4679\x2da30b\x2deadb22a3f3f9.device - /dev/disk/by-uuid/71d81e4b-5e84-4679-a30b-eadb22a3f3f9...
[   14.948969] systemd[1]: Expecting device dev-hvc0.device - /dev/hvc0...
[   14.951077] systemd[1]: Expecting device dev-ttyS0.device - /dev/ttyS0...
[   14.958968] systemd[1]: Expecting device dev-zram0.device - /dev/zram0...
[   14.962286] systemd[1]: Reached target cryptsetup.target - Local Encrypted Volumes.
[   14.963204] systemd[1]: Stopped target initrd-switch-root.target - Switch Root.
[   14.965114] systemd[1]: Stopped target initrd-fs.target - Initrd File Systems.
[   14.965950] systemd[1]: Stopped target initrd-root-fs.target - Initrd Root File System.
[   14.967882] systemd[1]: Reached target integritysetup.target - Local Integrity Protected Volumes.
[   14.969034] systemd[1]: Reached target paths.target - Path Units.
[   14.970024] systemd[1]: Reached target slices.target - Slice Units.
[   14.971302] systemd[1]: Reached target veritysetup.target - Local Verity Protected Volumes.
[   14.974023] systemd[1]: Listening on dm-event.socket - Device-mapper event daemon FIFOs.
[   14.975146] systemd[1]: Listening on lvm2-lvmpolld.socket - LVM2 poll daemon socket.
[   14.978544] systemd[1]: Listening on systemd-coredump.socket - Process Core Dump Socket.
[   14.981269] systemd[1]: Listening on systemd-creds.socket - Credential Encryption/Decryption.
[   14.981673] systemd[1]: Listening on systemd-initctl.socket - initctl Compatibility Named Pipe.
[   14.984875] systemd[1]: systemd-journald-audit.socket - Journal Audit Socket was skipped because of an unmet condition check (ConditionSecurity=audit).
[   14.985169] systemd[1]: Listening on systemd-oomd.socket - Userspace Out-Of-Memory (OOM) Killer Socket.
[   14.985251] systemd[1]: systemd-pcrextend.socket - TPM PCR Measurements was skipped because of an unmet condition check (ConditionSecurity=measured-uki).
[   14.985259] systemd[1]: systemd-pcrlock.socket - Make TPM PCR Policy was skipped because of an unmet condition check (ConditionSecurity=measured-uki).
[   14.985398] systemd[1]: Listening on systemd-udevd-control.socket - udev Control Socket.
[   14.985845] systemd[1]: Listening on systemd-udevd-kernel.socket - udev Kernel Socket.
[   14.987344] systemd[1]: Listening on systemd-userdbd.socket - User Database Manager Socket.
[   14.992056] systemd[1]: Mounting dev-hugepages.mount - Huge Pages File System...
[   14.995801] systemd[1]: Mounting dev-mqueue.mount - POSIX Message Queue File System...
[   14.998535] systemd[1]: Mounting sys-kernel-debug.mount - Kernel Debug File System...
[   15.017991] systemd[1]: Mounting sys-kernel-tracing.mount - Kernel Trace File System...
[   15.018531] systemd[1]: auth-rpcgss-module.service - Kernel Module supporting RPCSEC_GSS was skipped because of an unmet condition check (ConditionPathExists=/etc/krb5.keytab).
[   15.018605] systemd[1]: fips-crypto-policy-overlay.service - Bind-mount FIPS crypto-policy in FIPS mode was skipped because of an unmet condition check (ConditionKernelCommandLine=fips=1).
[   15.018701] systemd[1]: iscsi-starter.service was skipped because of an unmet condition check (ConditionDirectoryNotEmpty=/var/lib/iscsi/nodes).
[   15.018735] systemd[1]: kmod-static-nodes.service - Create List of Static Device Nodes was skipped because of an unmet condition check (ConditionFileNotEmpty=/lib/modules/6.19.6-00001-ge27d5805c1f5/modules.devname).
[   15.022006] systemd[1]: Starting lvm2-monitor.service - Monitoring of LVM2 mirrors, snapshots etc. using dmeventd or progress polling...
[   15.025324] systemd[1]: Starting modprobe@configfs.service - Load Kernel Module configfs...
[   15.028465] systemd[1]: Starting modprobe@dm_mod.service - Load Kernel Module dm_mod...
[   15.031209] systemd[1]: Starting modprobe@dm_multipath.service - Load Kernel Module dm_multipath...
[   15.033306] systemd[1]: Starting modprobe@drm.service - Load Kernel Module drm...
[   15.036242] systemd[1]: Starting modprobe@efi_pstore.service - Load Kernel Module efi_pstore...
[   15.055660] systemd[1]: Starting modprobe@fuse.service - Load Kernel Module fuse...
[   15.058782] systemd[1]: Starting modprobe@loop.service - Load Kernel Module loop...
[   15.059228] systemd[1]: plymouth-switch-root.service: Deactivated successfully.
[   15.059407] systemd[1]: Stopped plymouth-switch-root.service - Plymouth switch root service.
[   15.059715] systemd[1]: systemd-fsck-root.service: Deactivated successfully.
[   15.059944] systemd[1]: Stopped systemd-fsck-root.service - File System Check on Root Device.
[   15.061168] systemd[1]: systemd-hibernate-clear.service - Clear Stale Hibernate Storage Info was skipped because of an unmet condition check (ConditionPathExists=/sys/firmware/efi/efivars/HibernateLocation-8cf2644b-4b0b-428f-9387-6d876050dc67).
[   15.061448] systemd[1]: systemd-journald.service: unit configures an IP firewall, but the local system does not support BPF/cgroup firewalling.
[   15.061453] systemd[1]: systemd-journald.service: (This warning is only shown for the first unit using IP firewalling.)
[   15.064279] systemd[1]: Starting systemd-journald.service - Journal Service...
[   15.067903] systemd[1]: Starting systemd-modules-load.service - Load Kernel Modules...
[   15.070209] systemd[1]: systemd-pcrmachine.service - TPM PCR Machine ID Measurement was skipped because of an unmet condition check (ConditionSecurity=measured-uki).
[   15.073738] systemd[1]: Starting systemd-remount-fs.service - Remount Root and Kernel File Systems...
[   15.077637] systemd[1]: Starting systemd-tmpfiles-setup-dev-early.service - Create Static Device Nodes in /dev gracefully...
[   15.077937] systemd[1]: systemd-tpm2-setup-early.service - Early TPM SRK Setup was skipped because of an unmet condition check (ConditionSecurity=measured-uki).
[   15.080652] systemd[1]: Starting systemd-udev-load-credentials.service - Load udev Rules from Credentials...
[   15.084023] systemd[1]: Starting systemd-udev-trigger.service - Coldplug All udev Devices...
[   15.091047] systemd[1]: Mounted dev-hugepages.mount - Huge Pages File System.
[   15.092321] systemd[1]: Mounted dev-mqueue.mount - POSIX Message Queue File System.
[   15.093545] systemd[1]: Mounted sys-kernel-debug.mount - Kernel Debug File System.
[   15.094711] systemd[1]: Mounted sys-kernel-tracing.mount - Kernel Trace File System.
[   15.096950] systemd[1]: modprobe@configfs.service: Deactivated successfully.
[   15.097731] systemd[1]: Finished modprobe@configfs.service - Load Kernel Module configfs.
[   15.100700] systemd[1]: Finished lvm2-monitor.service - Monitoring of LVM2 mirrors, snapshots etc. using dmeventd or progress polling.
[   15.102344] systemd[1]: modprobe@dm_mod.service: Deactivated successfully.
[   15.103552] systemd[1]: Finished modprobe@dm_mod.service - Load Kernel Module dm_mod.
[   15.104992] systemd[1]: modprobe@dm_multipath.service: Deactivated successfully.
[   15.106163] systemd[1]: Finished modprobe@dm_multipath.service - Load Kernel Module dm_multipath.
[   15.108514] systemd[1]: modprobe@drm.service: Deactivated successfully.
[   15.110127] systemd[1]: Finished modprobe@drm.service - Load Kernel Module drm.
[   15.111870] systemd[1]: modprobe@efi_pstore.service: Deactivated successfully.
[   15.114161] systemd[1]: Finished modprobe@efi_pstore.service - Load Kernel Module efi_pstore.
[   15.115565] systemd[1]: modprobe@fuse.service: Deactivated successfully.
[   15.117231] systemd[1]: Finished modprobe@fuse.service - Load Kernel Module fuse.
[   15.120383] systemd[1]: modprobe@loop.service: Deactivated successfully.
[   15.122376] systemd[1]: Finished modprobe@loop.service - Load Kernel Module loop.
[   15.124034] systemd[1]: Finished systemd-modules-load.service - Load Kernel Modules.
[   15.125778] systemd[1]: Finished systemd-remount-fs.service - Remount Root and Kernel File Systems.
[   15.127108] systemd-journald[946]: Collecting audit messages is disabled.
[   15.129813] systemd[1]: Mounting sys-fs-fuse-connections.mount - FUSE Control File System...
[   15.130144] systemd[1]: multipathd.service - Device-Mapper Multipath Device Controller was skipped because of an unmet condition check (ConditionPathExists=/etc/multipath.conf).
[   15.130177] systemd[1]: iscsi-onboot.service - Special handling of early boot iSCSI sessions was skipped because of an unmet condition check (ConditionDirectoryNotEmpty=/sys/class/iscsi_session).
[   15.130373] systemd[1]: systemd-hwdb-update.service - Rebuild Hardware Database was skipped because of an unmet condition check (ConditionNeedsUpdate=/etc).
[   15.130400] systemd[1]: systemd-pstore.service - Platform Persistent Storage Archival was skipped because of an unmet condition check (ConditionDirectoryNotEmpty=/sys/fs/pstore).
[   15.133472] systemd[1]: Starting systemd-random-seed.service - Load/Save OS Random Seed...
[   15.134847] systemd[1]: systemd-repart.service - Repartition Root Disk was skipped because no trigger condition checks were met.
[   15.163634] systemd[1]: Starting systemd-sysctl.service - Apply Kernel Variables...
[   15.164065] systemd[1]: systemd-tpm2-setup.service - TPM SRK Setup was skipped because of an unmet condition check (ConditionSecurity=measured-uki).
[   15.164554] systemd[1]: Started systemd-journald.service - Journal Service.
[   15.254594] systemd-journald[946]: Received client request to flush runtime journal.
[   15.265242] systemd-journald[946]: File /var/log/journal/8aea6f2de1694bf2a6bb6f5172a4846c/system.journal corrupted or uncleanly shut down, renaming and replacing.
[   15.882643] virtio_net virtio4 ens6: renamed from eth1
[   15.883146] BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
[   15.883148] in_atomic(): 0, irqs_disabled(): 0, non_block: 0, pid: 16, name: pr/legacy
[   15.883149] preempt_count: 0, expected: 0
[   15.883149] RCU nest depth: 1, expected: 0
[   15.883150] 4 locks held by pr/legacy/16:
[   15.883151]  #0: ffffffff82963500 (console_lock){+.+.}-{0:0}, at: legacy_kthread_func+0x6d/0x150
[   15.883157]  #1: ffffffff82963558 (console_srcu){....}-{0:0}, at: console_flush_one_record+0x85/0x4f0
[   15.883160]  #2: ffffffff829fad40 (printing_lock){+.+.}-{3:3}, at: vt_console_print+0x5d/0x490
[   15.883164]  #3: ffffffff82966380 (rcu_read_lock){....}-{1:3}, at: rt_spin_trylock+0x62/0x130
[   15.883169] CPU: 2 UID: 0 PID: 16 Comm: pr/legacy Tainted: G        W           6.19.6-00001-ge27d5805c1f5 #2 PREEMPT_{RT,(full)}
[   15.883171] Tainted: [W]=WARN
[   15.883171] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[   15.883172] Call Trace:
[   15.883174]  <TASK>
[   15.883176]  dump_stack_lvl+0x75/0xb0
[   15.883180]  __might_resched.cold+0xcc/0xe1
[   15.883185]  console_conditional_schedule+0x2f/0x40
[   15.883189]  fbcon_redraw+0x99/0x230 [fb]
[   15.883201]  fbcon_scroll+0x17f/0x1e0 [fb]
[   15.883208]  con_scroll+0x111/0x240
[   15.883213]  lf+0xa8/0xb0
[   15.883217]  vt_console_print+0x324/0x490 (<--- 这个地方持有 rcu
[   15.883224]  console_flush_one_record+0x2a2/0x4f0
[   15.883231]  ? __pfx_legacy_kthread_func+0x10/0x10
[   15.883233]  legacy_kthread_func+0x82/0x150
[   15.883236]  ? __pfx_autoremove_wake_function+0x10/0x10
[   15.883241]  kthread+0x123/0x240
[   15.883245]  ? __pfx_kthread+0x10/0x10
[   15.883249]  ret_from_fork+0x288/0x330
[   15.883251]  ? __pfx_kthread+0x10/0x10
[   15.883253]  ret_from_fork_asm+0x1a/0x30
[   15.883265]  </TASK>
[   15.894621] virtio_net virtio3 ens5: renamed from eth0
[   16.304415] XFS (vdb2): Mounting V5 Filesystem 71d81e4b-5e84-4679-a30b-eadb22a3f3f9
[   16.312722] XFS (vdb2): Starting recovery (logdev: internal)
[   16.316075] XFS (vdb2): Ending recovery (logdev: internal)
[   16.430156] zram0: detected capacity change from 0 to 15087616
[   16.773490] Adding 7543804k swap on /dev/zram0.  Priority:100 extents:1 across:7543804k SSDsc
[   59.922944] hrtimer: interrupt took 64880 ns
[   59.923925] BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
[   59.923932] in_atomic(): 0, irqs_disabled(): 0, non_block: 0, pid: 16, name: pr/legacy
[   59.923936] preempt_count: 0, expected: 0
[   59.923938] RCU nest depth: 1, expected: 0
[   59.923940] 4 locks held by pr/legacy/16:
[   59.923943]  #0: ffffffff82963500 (console_lock){+.+.}-{0:0}, at: legacy_kthread_func+0x6d/0x150
[   59.923963]  #1: ffffffff82963558 (console_srcu){....}-{0:0}, at: console_flush_one_record+0x85/0x4f0
[   59.923974]  #2: ffffffff829fad40 (printing_lock){+.+.}-{3:3}, at: vt_console_print+0x5d/0x490
[   59.923986]  #3: ffffffff82966380 (rcu_read_lock){....}-{1:3}, at: rt_spin_trylock+0x62/0x130
[   59.924002] CPU: 2 UID: 0 PID: 16 Comm: pr/legacy Tainted: G        W           6.19.6-00001-ge27d5805c1f5 #2 PREEMPT_{RT,(full)}
[   59.924009] Tainted: [W]=WARN
[   59.924011] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[   59.924014] Call Trace:
[   59.924020]  <TASK>
[   59.924027]  dump_stack_lvl+0x75/0xb0
[   59.924041]  __might_resched.cold+0xcc/0xe1
[   59.924055]  console_conditional_schedule+0x2f/0x40
[   59.924069]  fbcon_redraw+0x99/0x230 [fb]
[   59.924111]  fbcon_scroll+0x17f/0x1e0 [fb]
[   59.924134]  con_scroll+0x111/0x240
[   59.924154]  lf+0xa8/0xb0
[   59.924168]  vt_console_print+0x324/0x490
[   59.924193]  console_flush_one_record+0x2a2/0x4f0
[   59.924222]  ? __pfx_legacy_kthread_func+0x10/0x10
[   59.924231]  legacy_kthread_func+0x82/0x150
[   59.924242]  ? __pfx_autoremove_wake_function+0x10/0x10
[   59.924261]  kthread+0x123/0x240
[   59.924277]  ? __pfx_kthread+0x10/0x10
[   59.924290]  ret_from_fork+0x288/0x330
[   59.924296]  ? __pfx_kthread+0x10/0x10
[   59.924305]  ret_from_fork_asm+0x1a/0x30
[   59.924351]  </TASK>
```

整个的分析结果是:

1. RCU 配置 -- RT 专属

```txt
rcu: Preemptible hierarchical RCU implementation.
rcu: RCU priority boosting: priority 1 delay 500 ms.
rcu: RCU_SOFTIRQ processing moved to rcuc kthreads.
```

- Preemptible RCU: RT 内核用可抢占 RCU，普通内核通常用不可抢占版本
- RCU priority boosting: RT 专属功能。当 RCU 读者持有临界区太久（500ms），会被提升到优先级 1（SCHED_FIFO），防止 RCU grace period 被低优先级任务无限阻塞
- RCU_SOFTIRQ -> rcuc kthreads: RT 把 RCU softirq 回调搬到专门的内核线程（rcuc/N）中执行，这样 softirq 处理变成可调度、可抢占的，符合 RT "一切都在 scheduler 控制下"的原则

2. KVM 与 RT 的兼容性提示

```txt
kvm: RT requires X86_FEATURE_CONSTANT_TSC
```

这是 KVM 在 RT 内核中的检查。RT 内核对时间精度要求极高，需要恒定频率的 TSC。这台 QEMU 虚拟机 CPU 模型（Raptor Lake family 6 model 183）没有暴露 constant_tsc 特性给 guest，所以 KVM 打了这条提示。意味着 guest 里嵌套虚拟化可能有问题，但作为 guest 本身运行不受影响。

3. 反复出现的 BUG: sleeping in invalid context（共 5 次）

这是本次日志中最值得关注的问题。每次 stack trace 完全一致：

BUG: sleeping function called from invalid context at kernel/printk/printk.c:3377
RCU nest depth: 1, expected: 0
4 locks held by pr/legacy/16:
  #0: console_lock           at legacy_kthread_func
  #1: console_srcu           at console_flush_one_record
  #2: printing_lock          at vt_console_print
  #3: rcu_read_lock          at rt_spin_trylock

调用链：
legacy_kthread_func -> console_flush_one_record -> vt_console_print
  -> lf -> con_scroll -> fbcon_scroll -> fbcon_redraw
    -> console_conditional_schedule -> __might_resched  [BUG!]

根因分析:

pr/legacy 是 printk 的 legacy console 输出内核线程。在 RT 内核下：
- printing_lock 是一个 rt_spinlock（基于 rtmutex），获取时会进入 RCU 读临界区（rt_spin_trylock 中）
- 在持有这个 rt_spinlock + RCU read lock 的情况下，fbcon_redraw 调用了 console_conditional_schedule()，它内部调了 __might_resched
- RT 内核的 __might_resched 检测到当前 RCU nest depth = 1（即在 RCU 读临界区内），不允许调度，于是报 BUG

本质: 这是 fbcon（framebuffer console）驱动在 RT 内核下的已知兼容性问题。fbcon 的滚屏路径（scroll -> redraw）试图在持锁的上下文中做
conditional schedule，和 RT 的 rt_spinlock 语义冲突。在普通内核中 spinlock 本来就禁止调度所以这个路径不会触发，但 RT 下 spinlock
变成可睡眠锁，lockdep/might_resched 检查变得更严格。

影响: 这个 WARNING 本身不致命（只是检测到"不应该在这里调度"），但你的 cmdline 里有 oops=panic，所以如果 panic_on_warn
也被设置了就会导致重启。目前看到5次 WARNING 都没 panic，说明 panic_on_warn 没有开启。

(好的，现在人来分析一下)

```c
static int legacy_kthread_func(void *unused)
{
	bool try_again;

wait_for_event:
	wait_event_interruptible(legacy_wait, legacy_kthread_should_wakeup());

	do {
		bool handover = false;
		u64 next_seq = 0;

		if (kthread_should_stop())
			return 0;

		console_lock();
		console_flush_one_record(true, &next_seq, &handover, &try_again);
		if (!handover)
			__console_unlock();

	} while (try_again);

	goto wait_for_event;
}
```

所以，现在就是在这里的了就打开了 rcu ，然后最后又使用了:
```txt
$ l *(vt_console_print+0x5d)
0xffffffff8197983d is in vt_console_print (drivers/tty/vt/vt.c:3364).
3359            int kmsg_console;
3360
3361            WARN_CONSOLE_UNLOCKED();
3362
3363            /* this protects against concurrent oops only */
3364            if (!spin_trylock(&printing_lock))
3365                    return;
3366
3367            kmsg_console = vt_get_kmsg_redirect();
3368            if (kmsg_console && vc_cons_allocated(kmsg_console - 1))
```

所以，感觉是 VT 的问题哦

## preempt_rt 的分析解释还不错

```txt
config PREEMPT_RT
	bool "Fully Preemptible Kernel (Real-Time)"
	depends on EXPERT && ARCH_SUPPORTS_RT
	select PREEMPTION
	help
	  This option turns the kernel into a real-time kernel by replacing
	  various locking primitives (spinlocks, rwlocks, etc.) with
	  preemptible priority-inheritance aware variants, enforcing
	  interrupt threading and introducing mechanisms to break up long
	  non-preemptible sections. This makes the kernel, except for very
	  low level and critical code paths (entry code, scheduler, low
	  level interrupt handling) fully preemptible and brings most
	  execution contexts under scheduler control.

	  Select this if you are building a kernel for systems which
	  require real-time guarantees.
```

打开这个东西并不容易，简单看了一圈，没怎么找到。

- https://news.ycombinator.com/item?id=38290145

其实才知道，Linux 的实时性一直做的不好，一直都是在努力。


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
