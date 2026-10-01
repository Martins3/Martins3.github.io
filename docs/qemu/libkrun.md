# libkrun

快速体验:

```sh
sudo dnf install crun-krun
podman run --rm --runtime=krun fedora:latest uname -a # 可以发现和主机的版本不同
podman run -it --rm --runtime=krun fedora:latest
```

确认当前的确运行的虚拟机就是和虚拟机不同的:

```sh
container_id=$(podman ps --latest --quiet)
container_pid=$(podman inspect --format '{{.State.Pid}}' "$container_id")
ls -l "/proc/$container_pid/fd" | grep kvm
ps -T -p "$container_pid"
```

## 启动仅仅需要 90ms

```txt
[root@0d617644570e ~]# dmesg
[    0.000000] Linux version 6.12.76 (root@libkrunfw) (gcc (GCC) 15.2.0, GNU ld (GNU Binutils) 2.46) #1 SMP PREEMPT_DYNAMIC Tue Mar 10 13:28:56 CET 2026
[    0.000000] Command line: reboot=k panic=-1 panic_print=0 nomodule console=hvc0 rootfstype=virtiofs rw quiet no-kvmapf init=/init.krun       virtio_mmio.device=4K@0xd0000000:5 virtio_mmio.device=4K@0xd0001000:6 virtio_mmio.device=4K@0xd0002000:7 virtio_mmio.device=4K@0xd0003000:8 virtio_mmio.device=4K@0xd0004000:9 tsi_hijack  --
[    0.000000] BIOS-provided physical RAM map:
[    0.000000] BIOS-e820: [mem 0x0000000000000000-0x000000000009fbff] usable
[    0.000000] BIOS-e820: [mem 0x0000000000100000-0x0000000042430000] usable
[    0.000000] NX (Execute Disable) protection: active
[    0.000000] APIC: Static calls initialized
[    0.000000] Hypervisor detected: KVM
[    0.000000] last_pfn = 0x42430 max_arch_pfn = 0x400000000
[    0.000000] kvm-clock: Using msrs 4b564d01 and 4b564d00
[    0.000003] kvm-clock: using sched offset of 12600329 cycles
[    0.000005] clocksource: kvm-clock: mask: 0xffffffffffffffff max_cycles: 0x1cd42e4dffb, max_idle_ns: 881590591483 ns
[    0.000013] tsc: Detected 2995.200 MHz processor
[    0.000025] e820: update [mem 0x00000000-0x00000fff] usable ==> reserved
[    0.000026] e820: remove [mem 0x000a0000-0x000fffff] usable
[    0.000027] last_pfn = 0x42430 max_arch_pfn = 0x400000000
[    0.000028] x86/PAT: PAT support disabled because CONFIG_X86_PAT is disabled in the kernel.
[    0.000031] x86/PAT: Configuration [0-7]: WB  WT  UC- UC  WB  WT  UC- UC
[    0.000047] found SMP MP-table at [mem 0x0009fc00-0x0009fc0f]
[    0.000055] Using GB pages for direct mapping
[    0.000056] Incomplete global flushes, disabling PCID
[    0.000128] Intel MultiProcessor Specification v1.4
[    0.000133] MPTABLE: OEM ID: FC
[    0.000133] MPTABLE: Product ID: 000000000000
[    0.000133] MPTABLE: APIC at: 0xFEE00000
[    0.000473] Zone ranges:
[    0.000477]   DMA32    [mem 0x0000000000001000-0x000000004242ffff]
[    0.000478]   Normal   empty
[    0.000478]   Device   empty
[    0.000479] Movable zone start for each node
[    0.000480] Early memory node ranges
[    0.000480]   node   0: [mem 0x0000000000001000-0x000000000009efff]
[    0.000482]   node   0: [mem 0x0000000000100000-0x000000004242ffff]
[    0.000488] Initmem setup node 0 [mem 0x0000000000001000-0x000000004242ffff]
[    0.000647] On node 0, zone DMA32: 1 pages in unavailable ranges
[    0.005123] On node 0, zone DMA32: 97 pages in unavailable ranges
[    0.005506] On node 0, zone DMA32: 23504 pages in unavailable ranges
[    0.005509] Intel MultiProcessor Specification v1.4
[    0.005512] MPTABLE: OEM ID: FC
[    0.005512] MPTABLE: Product ID: 000000000000
[    0.005513] MPTABLE: APIC at: 0xFEE00000
[    0.005516] Processor #0 (Bootup-CPU)
[    0.005516] Processor #1
[    0.005516] Processor #2
[    0.005517] Processor #3
[    0.005517] Processor #4
[    0.005517] Processor #5
[    0.005517] Processor #6
[    0.005517] Processor #7
[    0.005518] Processor #8
[    0.005518] Processor #9
[    0.005518] Processor #10
[    0.005518] Processor #11
[    0.005518] Processor #12
[    0.005519] Processor #13
[    0.005519] Processor #14
[    0.005519] Processor #15
[    0.005535] IOAPIC[0]: apic_id 17, version 17, address 0xfec00000, GSI 0-23
[    0.005537] Processors: 16
[    0.005537] TSC deadline timer available
[    0.005542] CPU topo: Max. logical packages:  16
[    0.005543] CPU topo: Max. logical dies:      16
[    0.005544] CPU topo: Max. dies per package:   1
[    0.005546] CPU topo: Max. threads per core:   1
[    0.005548] CPU topo: Num. cores per package:     1
[    0.005548] CPU topo: Num. threads per package:   1
[    0.005548] CPU topo: Allowing 16 present CPUs plus 0 hotplug CPUs
[    0.005570] kvm-guest: APIC: eoi() replaced with kvm_guest_apic_eoi_write()
[    0.005593] kvm-guest: KVM setup pv remote TLB flush
[    0.005598] kvm-guest: setup PV sched yield
[    0.005617] [mem 0x42430001-0xffffffff] available for PCI devices
[    0.005618] Booting paravirtualized kernel on KVM
[    0.005619] clocksource: refined-jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645519600211568 ns
[    0.005626] setup_percpu: NR_CPUS:16 nr_cpumask_bits:16 nr_cpu_ids:16 nr_node_ids:1
[    0.006970] percpu: Embedded 45 pages/cpu s152728 r0 d31592 u262144
[    0.006976] pcpu-alloc: s152728 r0 d31592 u262144 alloc=1*2097152
[    0.006978] pcpu-alloc: [0] 00 01 02 03 04 05 06 07 [0] 08 09 10 11 12 13 14 15
[    0.006992] kvm-guest: PV spinlocks enabled
[    0.006994] PV qspinlock hash table entries: 256 (order: 0, 4096 bytes, linear)
[    0.006996] Kernel command line: reboot=k panic=-1 panic_print=0 nomodule console=hvc0 rootfstype=virtiofs rw quiet no-kvmapf init=/init.krun       virtio_mmio.device=4K@0xd0000000:5 virtio_mmio.device=4K@0xd0001000:6 virtio_mmio.device=4K@0xd0002000:7 virtio_mmio.device=4K@0xd0003000:8 virtio_mmio.device=4K@0xd0004000:9 tsi_hijack  --
[    0.007083] random: crng init done
[    0.007087] printk: log_buf_len individual max cpu contribution: 4096 bytes
[    0.007088] printk: log_buf_len total cpu_extra contributions: 61440 bytes
[    0.007088] printk: log_buf_len min size: 16384 bytes
[    0.007275] printk: log_buf_len: 131072 bytes
[    0.007275] printk: early log buf free: 11560(70%)
[    0.007598] Dentry cache hash table entries: 131072 (order: 8, 1048576 bytes, linear)
[    0.007772] Inode-cache hash table entries: 65536 (order: 7, 524288 bytes, linear)
[    0.007891] Built 1 zonelists, mobility grouping on.  Total pages: 271310
[    0.007893] mem auto-init: stack:off, heap alloc:off, heap free:off
[    0.009612] SLUB: HWalign=64, Order=0-3, MinObjects=0, CPUs=16, Nodes=1
[    0.010049] Dynamic Preempt: none
[    0.010093] rcu: Preemptible hierarchical RCU implementation.
[    0.010096] 	Trampoline variant of Tasks RCU enabled.
[    0.010097] 	Tracing variant of Tasks RCU enabled.
[    0.010098] rcu: RCU calculated value of scheduler-enlistment delay is 25 jiffies.
[    0.010105] RCU Tasks: Setting shift to 4 and lim to 1 rcu_task_cb_adjust=1 rcu_task_cpu_ids=16.
[    0.010106] RCU Tasks Trace: Setting shift to 4 and lim to 1 rcu_task_cb_adjust=1 rcu_task_cpu_ids=16.
[    0.010110] NR_IRQS: 4352, nr_irqs: 168, preallocated irqs: 16
[    0.010282] rcu: srcu_init: Setting srcu_struct sizes based on contention.
[    0.010340] Console: colour dummy device 80x25
[    0.010352] APIC: Switch to symmetric I/O mode setup
[    0.010357] kvm-guest: APIC: send_IPI_mask() replaced with kvm_send_ipi_mask()
[    0.010360] kvm-guest: APIC: send_IPI_mask_allbutself() replaced with kvm_send_ipi_mask_allbutself()
[    0.010361] kvm-guest: setup PV IPIs
[    0.010696] clocksource: tsc-early: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.010699] Calibrating delay loop (skipped) preset value.. 5990.40 BogoMIPS (lpj=11980800)
[    0.010746] x86/cpu: User Mode Instruction Prevention (UMIP) activated
[    0.010750] Last level iTLB entries: 4KB 0, 2MB 0, 4MB 0
[    0.010750] Last level dTLB entries: 4KB 0, 2MB 0, 4MB 0, 1GB 0
[    0.010757] Spectre V1 : Mitigation: usercopy/swapgs barriers and __user pointer sanitization
[    0.010760] Spectre V2 : WARNING: Unprivileged eBPF is enabled with eIBRS on, data leaks possible via Spectre v2 BHB attacks!
[    0.010763] Spectre V2 : Mitigation: Enhanced / Automatic IBRS
[    0.010763] Spectre V2 : Spectre v2 / PBRSB-eIBRS: Retire a single CALL on VMEXIT
[    0.010764] Spectre V2 : mitigation: Enabling conditional Indirect Branch Prediction Barrier
[    0.010766] Speculative Store Bypass: Mitigation: Speculative Store Bypass disabled via prctl
[    0.010767] Register File Data Sampling: Mitigation: Clear Register File
[    0.010780] x86/fpu: Supporting XSAVE feature 0x001: 'x87 floating point registers'
[    0.010781] x86/fpu: Supporting XSAVE feature 0x002: 'SSE registers'
[    0.010782] x86/fpu: Supporting XSAVE feature 0x004: 'AVX registers'
[    0.010782] x86/fpu: Supporting XSAVE feature 0x800: 'Control-flow User registers'
[    0.010782] x86/fpu: xstate_offset[2]:  576, xstate_sizes[2]:  256
[    0.010783] x86/fpu: xstate_offset[11]:  832, xstate_sizes[11]:   16
[    0.010783] x86/fpu: Enabled xstate features 0x807, context size is 848 bytes, using 'compacted' format.
[    0.014697] Freeing SMP alternatives memory: 36K
[    0.014697] pid_max: default: 16384 minimum: 301
[    0.014697] LSM: initializing lsm=capability,selinux
[    0.014697] SELinux:  Initializing.
[    0.014697] Mount-cache hash table entries: 2048 (order: 2, 16384 bytes, linear)
[    0.014697] Mountpoint-cache hash table entries: 2048 (order: 2, 16384 bytes, linear)
[    0.014697] smpboot: CPU0: Intel(R) Xeon(R) Processor (family: 0x6, model: 0xb7, stepping: 0x1)
[    0.014697] Performance Events: unsupported p6 CPU model 183 no PMU driver, software events only.
[    0.014697] signal: max sigframe size: 1360
[    0.014697] rcu: Hierarchical SRCU implementation.
[    0.014697] rcu: 	Max phase no-delay instances is 1000.
[    0.014697] Timer migration: 2 hierarchy levels; 8 children per group; 2 crossnode level
[    0.014697] smp: Bringing up secondary CPUs ...
[    0.014697] smpboot: x86: Booting SMP configuration:
[    0.014697] .... node  #0, CPUs:        #1  #2  #3  #4  #5  #6  #7  #8  #9 #10 #11 #12 #13 #14 #15
[    0.070811] smp: Brought up 1 node, 16 CPUs
[    0.070815] smpboot: Total of 16 processors activated (95846.40 BogoMIPS)
[    0.070975] Memory: 1031400K/1085240K available (14336K kernel code, 468K rwdata, 2652K rodata, 1608K init, 1912K bss, 46632K reserved, 0K cma-reserved)
[    0.070981] devtmpfs: initialized
[    0.070981] x86/mm: Memory block size: 128MB
[    0.070981] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645041785100000 ns
[    0.070981] futex hash table entries: 16 (order: 0, 1024 bytes, linear)
[    0.070981] NET: Registered PF_NETLINK/PF_ROUTE protocol family
[    0.070981] audit: initializing netlink subsys (disabled)
[    0.070981] audit: type=2000 audit(1790835294.445:1): state=initialized audit_enabled=0 res=1
[    0.070981] cpuidle: using governor menu
[    0.070981] HugeTLB: registered 1.00 GiB page size, pre-allocated 0 pages
[    0.070981] HugeTLB: 16380 KiB vmemmap can be freed for a 1.00 GiB page
[    0.070981] HugeTLB: registered 2.00 MiB page size, pre-allocated 0 pages
[    0.070981] HugeTLB: 28 KiB vmemmap can be freed for a 2.00 MiB page
[    0.070981] raid6: skipped pq benchmark and selected avx2x4
[    0.070981] raid6: using avx2x2 recovery algorithm
[    0.070981] pps_core: LinuxPPS API ver. 1 registered
[    0.070981] pps_core: Software ver. 5.3.6 - Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>
[    0.070981] PTP clock support registered
[    0.070986] clocksource: Switched to clocksource kvm-clock
[    0.071664] NET: Registered PF_INET protocol family
[    0.071679] IP idents hash table entries: 16384 (order: 5, 131072 bytes, linear)
[    0.071808] tcp_listen_portaddr_hash hash table entries: 512 (order: 1, 8192 bytes, linear)
[    0.071812] Table-perturb hash table entries: 65536 (order: 6, 262144 bytes, linear)
[    0.071813] TCP established hash table entries: 8192 (order: 4, 65536 bytes, linear)
[    0.071817] TCP bind hash table entries: 8192 (order: 6, 262144 bytes, linear)
[    0.071924] TCP: Hash tables configured (established 8192 bind 8192)
[    0.071979] UDP hash table entries: 512 (order: 2, 16384 bytes, linear)
[    0.071981] UDP-Lite hash table entries: 512 (order: 2, 16384 bytes, linear)
[    0.072011] NET: Registered PF_UNIX/PF_LOCAL protocol family
[    0.072029] virtio-mmio: Registering device virtio-mmio.0 at 0xd0000000-0xd0000fff, IRQ 5.
[    0.072037] virtio-mmio: Registering device virtio-mmio.1 at 0xd0001000-0xd0001fff, IRQ 6.
[    0.072042] virtio-mmio: Registering device virtio-mmio.2 at 0xd0002000-0xd0002fff, IRQ 7.
[    0.072045] virtio-mmio: Registering device virtio-mmio.3 at 0xd0003000-0xd0003fff, IRQ 8.
[    0.072050] virtio-mmio: Registering device virtio-mmio.4 at 0xd0004000-0xd0004fff, IRQ 9.
[    0.072065] clocksource: tsc: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.072106] clocksource: Switched to clocksource tsc
[    0.072110] platform rtc_cmos: registered platform RTC device (no PNP device found)
[    0.073974] workingset: timestamp_bits=46 max_order=18 bucket_order=0
[    0.074034] fuse: init (API version 7.41)
[    0.074081] SGI XFS with security attributes, no debug enabled
[    0.078388] xor: automatically using best checksumming function   avx
[    0.078883] virtiofs virtio3: Cache len: 0x20000000 @ 0x100000000
[    0.082562] Free page reporting enabled
[    0.083304] printk: legacy console [hvc0] enabled
[    0.084418] loop: module loaded
[    0.084429] tun: Universal TUN/TAP device driver, 1.6
[    0.084555] device-mapper: ioctl: 4.48.0-ioctl (2023-03-01) initialised: dm-devel@lists.linux.dev
[    0.084649] NET: Registered PF_INET6 protocol family
[    0.084796] Segment Routing with IPv6
[    0.084799] In-situ OAM (IOAM) with IPv6
[    0.084808] NET: Registered PF_PACKET protocol family
[    0.084827] NET: Registered PF_VSOCK protocol family
[    0.085632] NET: Registered PF_TSI protocol family
[    0.085634] NET: Registered PF_TSI6 protocol family
[    0.085634] NET: Registered PF_TSIU protocol family
[    0.085647] IPI shorthand broadcast: enabled
[    0.086314] sched_clock: Marking stable (84412234, 412572)->(347489064, -262664258)
[    0.086747] registered taskstats version 1
[    0.088008] Btrfs loaded, zoned=no, fsverity=no
[    0.088178] Key type encrypted registered
[    0.088227] virtio-fs: tag <> not found
[    0.088351] VFS: Mounted root (virtiofs filesystem) on device 0:19.
[    0.088390] devtmpfs: mounted
[    0.089379] Freeing unused kernel image (initmem) memory: 1608K
[    0.089384] Write protecting the kernel read-only data: 18432k
[    0.090471] Freeing unused kernel image (rodata/data gap) memory: 1444K
[    0.090475] Run /init.krun as init process
[    0.090476]   with arguments:
[    0.090476]     /init.krun
[    0.090477]   with environment:
[    0.090477]     HOME=/
[    0.090477]     TERM=linux
```

## 内存和 CPU 的扩容
podman run -it --rm --runtime=krun martins3:fedora

podman run -it --rm --runtime=krun \
    --annotation krun.cpus=4 \
    --annotation krun.ram_mib=4096 \
    martins3:fedora

podman update --cpus=4 --memory=6g  martins3:fedora

这个无法动态扩缩容，这个测试出来的。


> [!NOTE]
> 参考神奇海螺的意见，有待验证

Podman → crun 的 krun handler → libkrun → KVM

换掉最前面的 Podman，后面的 libkrun 能力不会自动增加。如果要继续使用 libkrun，就需要开发运行时控制接口、资源添加逻辑以及 guest 通知机制。

如果目标是直接使用已有的 CPU／内存热添加能力，可以考虑 Cloud Hypervisor：它提供运行时 /vm.resize 接口，但需要启动时预留最大 CPU
数和内存热插拔空间，并由 guest 支持、上线新增资源。这是更换 VM 后端及管理方式，不是简单替换 Podman 命令。Cloud Hypervisor 热插拔文档
(https://github.com/cloud-hypervisor/cloud-hypervisor/blob/main/docs/hotplug.md)



## links

想不到 libkrun 可以做这么有意思的: https://github.com/nohajc/anylinuxfs

https://github.com/containers/libkrun

那么，显然从 kata-containers 入手还是太痛苦了， 可以从 libkrun 入手来学习 rust
的

- https://github.com/boxlite-ai/boxlite
  - 基于 libkrun
  - https://github.com/boxlite-ai/boxlite/tree/main/docs/architecture

https://news.ycombinator.com/item?id=32447995

- https://github.com/containers/krunvm
- https://github.com/containers/libkrun

https://github.com/containers/libkrunfw

- https://news.ycombinator.com/item?id=32447995 : krunvm is a CLI-based utility
  for creating microVMs from OCI images https://github.com/containers/krunvm

## 附录

nix 环境本地运行的脚本参考:

```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$repo_dir"

libclang_path=$(nix build --no-link --print-out-paths nixpkgs#llvmPackages_22.libclang.lib)
libkrunfw_path=$(nix build --no-link --print-out-paths nixpkgs#libkrunfw)

LIBCLANG_PATH="$libclang_path/lib" \
	LD_LIBRARY_PATH="$libclang_path/lib" \
	make debug

PATH=/usr/bin:/bin /usr/bin/gcc \
	-O2 -g -Iinclude -Ltarget/debug \
	-Wl,-rpath,"$repo_dir/target/debug" \
	-o target/debug/libkrun-local-demo \
	demo.c -lkrun \
	-lkrun_init

LD_LIBRARY_PATH="$repo_dir/target/debug:$libkrunfw_path/lib" \
	"$repo_dir/target/debug/libkrun-local-demo" \
	/ \
	/bin/sh \
	-c \
	'echo guest-kernel; uname -r; echo guest-machine; uname -m; cat /etc/fedora-release'
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
