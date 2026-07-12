## 设计文档
- https://assets.amazon.science/96/c6/302e527240a3b1f86c86c3e8fc3d/firecracker-lightweight-virtualization-for-serverless-applications.pdf
- https://www.usenix.org/system/files/nsdi20-paper-agache.pdf : 2020 的

之后再去看看，算是对于计算机的一个总结了
docs/prod-host-setup.md

所有的代码都是在 src/vmm/ 下
- src/vmm/src/dumbo/

## 得靠 collei.sh 来安装，而且必须是一个 raw 格式才可以

## 为什么不可以 Passthrough ，非要搞这种东西
src/vmm/src/cpu_config/x86_64/cpuid/

而且让我在 hygon 下无法运行

## 细节
- [ ] mmds / Dumbo : guest 和 host 之间通信，为什么不是 vsock 就可以了，结果还需要自己的网络栈
- [ ] Firecracker also provides a *metadata service* that securely shares configuration information between the host and guest operating system.

## 关联项目
- [ ] https://github.com/rust-vmm/vm-device : 只有一千行，也许就是用于学习  firecracker 的基础了, 其实这是一个大的 project 的小部分，其他的可以好好分析一下
  - 但是 https://github.com/firecracker-microvm/firecracker-containerd 不就是用于封装 microvm 的吗?
- [ ] The *jailer* provides a second line of defense in case the virtualization barrier is ever compromised.
- [ ] https://github.com/weaveworks/ignite : firecracker 的管理工具
  - 已经凉了

## some simple code flow
- main
  - run_with_api
    - bind_and_run
      - handle_request
        - try_from_request
          - parse_put_actions

## vsock
csm : VsockConnection
- new_peer_init
- new_local_init

- local_port
- peer_port

VSock ==> VsockChannel ==> VsockConnection

VSock<VsockBackend>::process ==> VSock<VsockBackend>::handle_txq_event ==> VsockChannel::recv_pkt ==> apply_conn_mutation + VsockConnection::recv_pkt

vmm/src/vmm_config/vsock.rs::VsockBuilder::create_unixsock_vsock will initlize related data.

### unix domain socket

> This module implements the Unix Domain Sockets backend for vsock - a mediator between
> guest-side AF_VSOCK sockets and host-side AF_UNIX sockets. The heavy lifting is performed by
> `muxer::VsockMuxer`, a connection multiplexer that uses `super::csm::VsockConnection` for
> handling vsock connection states.
> Check out `muxer.rs` for a more detailed explanation of the inner workings of this backend.

## Rust language
- [ ] https://stackoverflow.com/questions/30938499/why-is-the-sized-bound-necessary-in-this-trait
- https://doc.rust-lang.org/nomicon/hrtb.html

### lib
- [ ] https://doc.rust-lang.org/beta/core/num/struct.Wrapping.html

## 阅读
https://news.ycombinator.com/item?id=36666782
https://news.ycombinator.com/item?id=38651805
https://aws.amazon.com/cn/blogs/china/deep-analysis-aws-firecracker-principle-virtualization-container-runtime-technology/?nc1=b_rp
https://www.talhoffman.com/2021/07/18/firecracker-internals/
- https://news.ycombinator.com/item?id=34964197
  - 文章本身不错，comment 里面资料也不少


## 调试一下
```txt
Mar 15 06:17:15 localhost mount[333]: mount: /sysroot: wrong fs type, bad option, bad superblock on /dev/vda, missing codepage or helper program, or other error.
Mar 15 06:17:15 localhost mount[333]:        dmesg(1) may have more information after failed mount system call.
Mar 15 06:17:15 localhost systemd[1]: sysroot.mount: Mount process exited, code=exited, status=32/n/a
Mar 15 06:17:15 localhost systemd[1]: sysroot.mount: Failed with result 'exit-code'.
Mar 15 06:17:15 localhost systemd[1]: Failed to mount /sysroot.
```

## 常用但是缺少的功能
- VFIO 的功能
- scsi 功能
  - 支持 virtio scsi
  -  CDROM
- 用户态网络
- 显示
- 热迁移
  - 所以也不需要定义 machine 的概念
- 二进制翻译
- virtio fs 和 plan 9p 都是不支持的

其他的功能也是缺失的:
- 似乎 qemu 是支持 xen 的

似乎必须指定 kernel 才可以?


## 搞清楚他的这个 systemd 都是如何启用的

当使用 code/qemu/fire-rootfs.sh 的材料的时候:
```txt
[    0.366512] systemd[1]: systemd 255.4-1ubuntu8.4 running in system mode (+PAM +AUDIT +SELINUX +APPARMOR +IMA
+SMACK +SECCOMP +GCRYPT -GNUTLS +OPENSSL +ACL +BLKID +CURL +ELFUTILS +FIDO2 +IDN2 -IDN +IPTC +KMOD +LIBCRYPTSETU
P +LIBFDISK +PCRE2 -PWQUALITY +P11KIT +QRENCODE +TPM2 +BZIP2 +LZ4 +XZ +ZLIB +ZSTD -BPF_FRAMEWORK -XKBCOMMON +UTM
P +SYSVINIT default-hierarchy=unified)
[    0.368582] systemd[1]: Detected virtualization kvm.
[    0.368907] systemd[1]: Detected architecture x86-64.
[    0.370037] systemd[1]: Hostname set to <ubuntu-fc-uvm>.
[    0.402459] systemd[1]: Queued start job for default target graphical.target.
[    0.403346] systemd[1]: Created slice system-getty.slice - Slice /system/getty.
[    0.404359] systemd[1]: Created slice system-modprobe.slice - Slice /system/modprobe.
[    0.405434] systemd[1]: Created slice system-serial\x2dgetty.slice - Slice /system/serial-getty.
[    0.406530] systemd[1]: Created slice user.slice - User and Session Slice.
[    0.407379] systemd[1]: Started systemd-ask-password-console.path - Dispatch Password Requests to Console Dir
ectory Watch.
[    0.408579] systemd[1]: Started systemd-ask-password-wall.path - Forward Password Requests to Wall Directory
Watch.
[    0.410076] systemd[1]: Set up automount proc-sys-fs-binfmt_misc.automount - Arbitrary Executable File Format
s File System Automount Point.
[    0.411332] systemd[1]: Expecting device dev-ttyS0.device - /dev/ttyS0...
[    0.412071] systemd[1]: Reached target cryptsetup.target - Local Encrypted Volumes.
[    0.412926] systemd[1]: Reached target integritysetup.target - Local Integrity Protected Volumes.
[    0.413901] systemd[1]: Reached target paths.target - Path Units.
[    0.414588] systemd[1]: Reached target remote-fs.target - Remote File Systems.
[    0.415390] systemd[1]: Reached target slices.target - Slice Units.
[    0.416088] systemd[1]: Reached target swap.target - Swaps.
[    0.416723] systemd[1]: Reached target veritysetup.target - Local Verity Protected Volumes.
[    0.417697] systemd[1]: Listening on systemd-initctl.socket - initctl Compatibility Named Pipe.
[    0.418695] systemd[1]: Listening on systemd-journald-dev-log.socket - Journal Socket (/dev/log).
[    0.419696] systemd[1]: Listening on systemd-journald.socket - Journal Socket.
[    0.420503] systemd[1]: systemd-pcrextend.socket - TPM2 PCR Extension (Varlink) was skipped because of an unm
et condition check (ConditionSecurity=measured-uki).
[    0.421849] systemd[1]: Listening on systemd-udevd-control.socket - udev Control Socket.
[    0.422777] systemd[1]: Listening on systemd-udevd-kernel.socket - udev Kernel Socket.
[    0.424216] systemd[1]: Mounting dev-hugepages.mount - Huge Pages File System...
[    0.425247] systemd[1]: Mounting dev-mqueue.mount - POSIX Message Queue File System...
[    0.426333] systemd[1]: Mounting sys-kernel-debug.mount - Kernel Debug File System...
[    0.427217] systemd[1]: sys-kernel-tracing.mount - Kernel Trace File System was skipped because of an unmet c
ondition check (ConditionPathExists=/sys/kernel/tracing).
[    0.428362] systemd[1]: Mounting tmp.mount - Temporary Directory /tmp...
[    0.429416] systemd[1]: Mounting var-lib-systemd.mount - /var/lib/systemd...
[    0.430860] systemd[1]: Starting systemd-journald.service - Journal Service...
[    0.431676] systemd[1]: kmod-static-nodes.service - Create List of Static Device Nodes was skipped because of
 an unmet condition check (ConditionFileNotEmpty=/lib/modules/5.10.225/modules.devname).
[    0.433045] systemd[1]: Starting modprobe@configfs.service - Load Kernel Module configfs...
[    0.434237] systemd[1]: Starting modprobe@dm_mod.service - Load Kernel Module dm_mod...
[    0.436451] systemd[1]: Starting modprobe@drm.service - Load Kernel Module drm...
[    0.437555] systemd[1]: Starting modprobe@efi_pstore.service - Load Kernel Module efi_pstore...
[    0.438758] systemd[1]: Starting modprobe@fuse.service - Load Kernel Module fuse...
[    0.439877] systemd[1]: Starting modprobe@loop.service - Load Kernel Module loop...
[    0.440022] systemd-journald[609]: Collecting audit messages is disabled.
[    0.441648] systemd[1]: Starting systemd-modules-load.service - Load Kernel Modules...
[    0.442553] systemd[1]: systemd-pcrmachine.service - TPM2 PCR Machine ID Measurement was skipped because of a
n unmet condition check (ConditionSecurity=measured-uki).
[    0.443728] systemd[1]: Starting systemd-remount-fs.service - Remount Root and Kernel File Systems...
[    0.444990] systemd[1]: Starting systemd-tmpfiles-setup-dev-early.service - Create Static Device Nodes in /de
v gracefully...
[    0.446138] systemd[1]: systemd-tpm2-setup-early.service - TPM2 SRK Setup (Early) was skipped because of an u
nmet condition check (ConditionSecurity=measured-uki).
[    0.447413] systemd[1]: Starting systemd-udev-trigger.service - Coldplug All udev Devices...
[    0.448767] systemd[1]: Started systemd-journald.service - Journal Service.
[    0.464094] systemd-journald[609]: Received client request to flush runtime journal.
```

发现了三个奇怪的 mount ，整个体系完全懵逼的:
```txt
root@ubuntu-fc-uvm:~# cat /proc/mounts
/dev/root / squashfs ro,relatime 0 0
systemd-1 /proc/sys/fs/binfmt_misc autofs rw,relatime,fd=31,pgrp=1,timeout=0,minproto=5,maxproto=5,direct 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,nosuid,nodev,noexec,relatime 0 0
```

## docs/hugepages.md
```txt
## Huge Pages and Snapshotting

Restoring a Firecracker snapshot of a microVM backed by huge pages will also use
huge pages to back the restored guest. There is no option to flip between
regular, 4K, pages and huge pages at restore time. Furthermore, snapshots of
microVMs backed with huge pages can only be restored via UFFD. Lastly, note that
even for guests backed by huge pages, differential snapshots will always track
write accesses to guest memory at 4K granularity.

When restoring snapshots via UFFD, Firecracker will send the configured page
size (in KiB) for each memory region as part of the initial handshake, as
described in our documentation on
[UFFD-assisted snapshot-restore](snapshotting/handling-page-faults-on-snapshot-resume.md).


### Why does Firecracker not offer a transparent huge pages (THP) setting?

Firecracker's guest memory is memfd based. Linux (as of 6.1) does not offer a
way to dynamically enable THP for such memory regions. Additionally, UFFD does
not integrate with THP (no transparent huge pages will be allocated during
userfaulting). Please refer to the [Linux Documentation][thp_docs] for more
information.
```
所以 memory 的 backend 都是唯一啊！

## rootfs 制作
1. docs/rootfs-and-kernel-setup.md 中介绍的
tools/devtool:cmd_build_ci_artifacts ->
resources/rebuild.sh:prepare_and_build_rootfs

2. tools/test-popular-containers/build_rootfs.sh
和 resources/ 中如何制作 initramfs

不过，还需要掌握 ctr ，就有点烦了

总之，通过 docker 来制作 rootfs 而且还可以携带 systemd

3. tools/devctr 开发的环境
似乎都会借用 debootstrap 之类的工具

4. https://github.com/firecracker-microvm/firecracker-containerd/tree/main/tools/image-builder : 其实限制很大


## 为什么还需要特殊支持一下 i8042

## 重点关注一下
docs/network-setup.md ，介绍了有趣的东西

## 简单看下，
1. 如何通过 stdio 来 serial 的，可不可以在 serial 的界面实现类似 qemu ctrl A 的功能
  - 似乎实现的 serial 比 QEMU 的好用
2. 如何处理信号的，有什么特殊之处的吗?

## 为什么没有办法正常关机啊
```txt
reboot: Power off not available: System halted instead
```

## 如果用了相同的资源，现在配置的 qemu 的确启动慢很多
的确有点问题，启动内核大致为 0.6s 的时间。

## 把 gdb firecracker 和 gdb kernel 配置上，基本上可以正常使用了

## 看看 firecracker 如何和 balloon 协助，尤其是现在没有配置 balloon 的情况

## 但是，既然，firecracker 不去热迁移，他搞这个 cpu tempalte 的意义是什么?

## 看看对于各种 bios 的支持
1. 似乎 virtio net 没有 rom 的支持了
2. 没有 seabios
3. 中需要 edk2 的支持吗?
4. SMBIOS 也是没有的

qemu 下的这个，firecracker 没有 dmi
 acpi   dmi   memmap


普通的机器的:
```txt
[ 2145.243775] BUG: unable to handle kernel NULL pointer dereference at 0000000000000000
[ 2145.244189] PGD 109b9f067 P4D 109b9f067 PUD 10ab5a067 PMD 0
[ 2145.244490] Oops: 0002 [#1] SMP NOPTI
[ 2145.245343] Hardware name: QEMU Standard PC (Q35 + ICH9, 2009), BIOS rel-1.16.2-14-g1e1da7a96300-dirty-20240907_171524-nixos 04/01/2014
[ 2145.246012] RIP: 0010:sysrq_handle_crash+0x12/0x20
[ 2145.246271] Code: 5c 41 5d 41 5e 41 5f eb d3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 0f 1f 44 00 00 c7 05 9d a2 cc 00 01 00 00 00 0f ae f8 <c6> 04 25 00 00 00 00 01 c3 0f 1f 44 00 00 0f 1f
```

firecracker ，直接跳过了 Hardware name 了:
```txt
sysrq: Trigger a crash
Kernel panic - not syncing: sysrq triggered crash
CPU: 2 UID: 0 PID: 1479 Comm: tee Not tainted 6.13.2-00001-g934999804fb6-dirty #11
```

## 似乎不支持重启吧

## arm 环境中还是需要解决如何安装的，然后在 arm 环境中测试一些

## src/clippy-tracing/

## 网卡支持 virtio 吗？

## docs/formal-verification.md
所以，这个 mmoc lock 的问题，是不是也可以使用这种方法来记录

## 这个 config 是在有趣的
```json
  "machine-config": {
    "vcpu_count": 32,
    "mem_size_mib": 4098,
    "smt": true,
    "track_dirty_pages": true,
    "huge_pages": "None"
  },
```
## virtio-blk 支持 trim 吗?
如果在 xfs 也是支持 trim ，那么似乎 raw 格式的内存就可以配置的很大
不支持的

## 可以思考一下，如何将两个都嫁接到一起的方法
让虚拟机有两个启动方式。

action 有很多都是可以共用的

可以添加一个 action 对于 fire 的判断

top 之类都是可以支持的

支持 pidfile 功能能否支持是关键

## SIG34 什么作用?

## 用这个做对比分析下
systemd-analyze

## 没有办法 kdump 来分析内核

## 需要的功能
1. 打开 qcow2 文件的互斥


```txt
#0  vmm::devices::legacy::serial::{impl#3}::write (self=0x555555d5ea50,
    buf=...) at src/vmm/src/devices/legacy/serial.rs:133
#1  0x00005555556ad07d in std::io::Write::write_all<vmm::devices::legacy::serial::SerialOut> (self=0x555555d5ea50, buf=...)
    at /build/rustc-1.82.0-src/library/std/src/io/mod.rs:1689
#2  0x000055555567e58b in vm_superio::serial::Serial<vmm::devices::legacy::EventFdTrigger, vmm::devices::legacy::serial::SerialEventsWrapper, vmm::devices::legacy::serial::SerialOut>::write<vmm::devices::legacy::EventFdTrigger, vmm::devices::legacy::serial::SerialEventsWrapper, vmm::devices::legacy::serial::SerialOut> (self=0x555555d5ea30, offset=0, value=76)
    at /home/martins3/.cargo/registry/src/mirrors.tuna.tsinghua.edu.cn-df7c3c540f42cdbd/vm-superio-0.8.0/src/serial.rs:593
#3  0x0000555555885036 in vmm::devices::legacy::serial::SerialWrapper<vmm::devices::legacy::EventFdTrigger, vmm::devices::legacy::serial::SerialEventsWrapper, std::io::stdio::Stdin>::bus_write<std::io::stdio::Stdin> (
    self=0x555555d5ea30, offset=0, data=...)
    at src/vmm/src/devices/legacy/serial.rs:353
#4  0x00005555558819a8 in vmm::devices::bus::BusDevice::write (
    self=0x555555d5ea28, offset=0, data=...)
    at src/vmm/src/devices/bus.rs:191
#5  0x0000555555882065 in vmm::devices::bus::Bus::write (
    self=0x7ffff7b53360, addr=1016, data=...)
    at src/vmm/src/devices/bus.rs:300
#6  0x00005555558f9f34 in vmm::vstate::vcpu::x86_64::Peripherals::run_arch_emulation (self=0x7ffff7b53358, exit=...)
    at src/vmm/src/vstate/vcpu/x86_64.rs:646
#7  0x00005555558fc3c1 in vmm::vstate::vcpu::handle_kvm_exit (
    peripherals=0x7ffff7b53358, emulation_result=...)
    at src/vmm/src/vstate/vcpu/mod.rs:609
#8  0x00005555558fc209 in vmm::vstate::vcpu::Vcpu::run_emulation (
    self=0x7ffff7b53358) at src/vmm/src/vstate/vcpu/mod.rs:528
#9  0x00005555558fb074 in vmm::vstate::vcpu::Vcpu::running (
    self=0x7ffff7b53358) at src/vmm/src/vstate/vcpu/mod.rs:309
#10 0x00005555558ef873 in vmm::utils::sm::StateMachine<vmm::vstate::vcpu::Vcpu>::run<vmm::vstate::vcpu::Vcpu> (machine=0x7ffff7b53358,
    starting_state_fn=0x5555558fb7a0 <vmm::vstate::vcpu::Vcpu::paused>)
    at src/vmm/src/utils/sm.rs:72
#11 0x00005555558fb03e in vmm::vstate::vcpu::Vcpu::run (
    self=0x7ffff7b53358, seccomp_filter=...)
    at src/vmm/src/vstate/vcpu/mod.rs:301
#12 0x00005555558faecf in vmm::vstate::vcpu::{impl#0}::start_threaded::{closure#0} () at src/vmm/src/vstate/vcpu/mod.rs:274
#13 0x00005555556afe36 in std::sys::backtrace::__rust_begin_short_backtrace<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()> (f=...)
    at /build/rustc-1.82.0-src/library/std/src/sys/backtrace.rs:154
#14 0x00005555556d7fa4 in std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()> () at /build/rustc-1.82.0-src/library/std/src/thread/mod.rs:522
#15 0x0000555555684a94 in core::panic::unwind_safe::{impl#23}::call_once<(), std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()>> (self=...)
    at /build/rustc-1.82.0-src/library/core/src/panic/unwind_safe.rs:272
#16 0x00005555556d8280 in std::panicking::try::do_call<core::panic::unwind_safe::AssertUnwindSafe<std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()>>, ()> (data=0x7ffff7b53b80)
    at /build/rustc-1.82.0-src/library/std/src/panicking.rs:554
#17 0x00005555556d781f in std::panicking::try<(), core::panic::unwind_safe::AssertUnwindSafe<std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()>>> (f=...) at /build/rustc-1.82.0-src/library/std/src/panicking.rs:518
#18 std::panic::catch_unwind<core::panic::unwind_safe::AssertUnwindSafe<std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()>>, ()> (f=...)
    at /build/rustc-1.82.0-src/library/std/src/panic.rs:345
#19 std::thread::{impl#0}::spawn_unchecked_::{closure#1}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()> ()
    at /build/rustc-1.82.0-src/library/std/src/thread/mod.rs:521
#20 0x00005555556e0b8f in core::ops::function::FnOnce::call_once<std::thread::{impl#0}::spawn_unchecked_::{closure_env#1}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0}, ()>, ()> ()
    at /build/rustc-1.82.0-src/library/core/src/ops/function.rs:250
#21 0x0000555555b66fcb in std::sys::pal::unix::thread::Thread::new::thread_start ()
#22 0x00007ffff7e30d02 in start_thread ()
   from /nix/store/nqb2ns2d1lahnd5ncwmn6k84qfd7vx2k-glibc-2.40-36/lib/libc.so.6
#23 0x00007ffff7eb03ac in __clone3 ()
   from /nix/store/nqb2ns2d1lahnd5ncwmn6k84qfd7vx2k-glibc-2.40-36/lib/libc.so.6
```

```txt
- __clone3
  - start_thread
    - std::sys::pal::unix::thread::Thread::new::thread_start
      - core::ops::function::FnOnce::call_once<std::thread::{impl#0}::spawn_unchecked_::{closure_env#1}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
        - std::thread::{impl#0}::spawn_unchecked_::{closure#1}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
          - std::panic::catch_unwind<core::panic::unwind_safe::AssertUnwindSafe<std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
            - std::panicking::try<(),
              - std::panicking::try::do_call<core::panic::unwind_safe::AssertUnwindSafe<std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure_env#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
                - core::panic::unwind_safe::{impl#23}::call_once<(),
                  - std::thread::{impl#0}::spawn_unchecked_::{closure#1}::{closure#0}<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
                    - std::sys::backtrace::__rust_begin_short_backtrace<vmm::vstate::vcpu::{impl#0}::start_threaded::{closure_env#0},
                      - vmm::vstate::vcpu::{impl#0}::start_threaded::{closure#0}
                        - vmm::vstate::vcpu::Vcpu::run
                          - vmm::utils::sm::StateMachine<vmm::vstate::vcpu::Vcpu>::run<vmm::vstate::vcpu::Vcpu>
                            - vmm::vstate::vcpu::Vcpu::running
                              - vmm::vstate::vcpu::Vcpu::run_emulation
                                - vmm::vstate::vcpu::handle_kvm_exit
                                  - vmm::vstate::vcpu::x86_64::Peripherals::run_arch_emulation
                                    - vmm::devices::bus::Bus::write
                                      - vmm::devices::bus::BusDevice::write
                                        - vmm::devices::legacy::serial::SerialWrapper<vmm::devices::legacy::EventFdTrigger,
                                          - vm_superio::serial::Serial<vmm::devices::legacy::EventFdTrigger,
                                            - std::io::Write::write_all<vmm::devices::legacy::serial::SerialOut>
                                              - vmm::devices::legacy::serial::{impl#3}::write
```

可以很容易就找到 firecracker/src/vmm/src/devices/bus.rs
```rs
    pub fn write(&mut self, offset: u64, data: &[u8]) {
        match self {
            Self::I8042Device(x) => x.bus_write(offset, data),
            #[cfg(target_arch = "aarch64")]
            Self::RTCDevice(x) => x.bus_write(offset, data),
            Self::BootTimer(x) => x.bus_write(offset, data),
            Self::MmioTransport(x) => x.bus_write(offset, data),
            Self::Serial(x) => x.bus_write(offset, data),
            #[cfg(test)]
            Self::Dummy(x) => x.bus_write(offset, data),
            #[cfg(test)]
            Self::Constant(x) => x.bus_write(offset, data),
        }
    }
```
好家伙，一共支持

## 有趣
- https://news.ycombinator.com/item?id=32683834
- https://news.ycombinator.com/item?id=32767784

- https://www.usenix.org/publications/loginonline/freebsd-firecracker

## 有暂停虚拟机的方法吗?

## fire.json 参考 tests/framework/vm_config.json

## 无论 shutdown 和 reboot 都无法结束 firecracker ，这个有点烦
参考这里，按道理 reboot 是可以的，但是实际上还是有问题
https://github.com/firecracker-microvm/firecracker/blob/main/FAQ.md

reboot 也是不行

```txt
kvm: exiting hardware virtualization
reboot: Restarting system
reboot: machine restart
```

但是 echo c | sudo tee /proc/sysrq-trigger 可以，不知道哪里有问题。

## qemu 果然 nb ，apple 也可以办到
https://www.reddit.com/r/qemu_kvm/comments/1jrenes/we_emulated_ios_14_in_qemu/

## 可以让 firecracker 用 kvm-clock 吗?


## 当使用 firecracker ping 物理机中的 br-in ，

```txt
***************** 277f0a00 ***************
[2025-4-24 11:02:20.030462] [277f0a00][__netif_receive_skb_core][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030467] [277f0a00][packet_rcv          ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030469] [277f0a00][ovs_vport_receive   ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030471] [277f0a00][ovs_dp_process_packet][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030477] [277f0a00][enqueue_to_backlog  ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030480] [277f0a00][__netif_receive_skb_core][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030481] [277f0a00][packet_rcv          ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030482] [277f0a00][ip_rcv              ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030483] [277f0a00][ip_rcv_core         ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030485] [277f0a00][nf_hook_slow        ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517 *ipv4 in chain: PRE_ROUTING*
[2025-4-24 11:02:20.030514] [277f0a00][nft_do_chain        ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517 *iptables table:mangle, chain:PREROUT* *packet is accepted*
[2025-4-24 11:02:20.030518] [277f0a00][ip_route_input_slow ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030521] [277f0a00][fib_validate_source ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030524] [277f0a00][ip_local_deliver    ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030525] [277f0a00][nf_hook_slow        ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517 *ipv4 in chain: INPUT*
[2025-4-24 11:02:20.030525] [277f0a00][nft_do_chain        ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517 *iptables table:filter, chain:INPUT* *packet is accepted*
[2025-4-24 11:02:20.030529] [277f0a00][ip_local_deliver_finish][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030530] [277f0a00][icmp_rcv            ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030532] [277f0a00][icmp_echo           ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030534] [277f0a00][icmp_reply          ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517
[2025-4-24 11:02:20.030567] [277f0a00][consume_skb         ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.101.10 -> 10.0.0.2 ping request, seq: 85, id: 1517 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!

***************** 277f0400 ***************
[2025-4-24 11:02:20.030539] [277f0400][__ip_local_out      ][cpu:9  ][     ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030540] [277f0400][nf_hook_slow        ][cpu:9  ][     ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517 *ipv4 in chain: OUTPUT*
[2025-4-24 11:02:20.030545] [277f0400][ip_output           ][cpu:9  ][     ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030546] [277f0400][nf_hook_slow        ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517 *ipv4 in chain: POST_ROUTING*
[2025-4-24 11:02:20.030548] [277f0400][ip_finish_output    ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030549] [277f0400][ip_finish_output2   ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030551] [277f0400][__dev_queue_xmit    ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030553] [277f0400][dev_hard_start_xmit ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517 *skb is successfully sent to the NIC driver*
[2025-4-24 11:02:20.030554] [277f0400][ovs_vport_receive   ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030555] [277f0400][ovs_dp_process_packet][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030556] [277f0400][__dev_queue_xmit    ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517
[2025-4-24 11:02:20.030558] [277f0400][dev_hard_start_xmit ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517 *skb is successfully sent to the NIC driver*
[2025-4-24 11:02:20.030589] [277f0400][consume_skb         ][cpu:9  ][   ][pid:208837 ][firecracker ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.10 ping reply, seq: 85, id: 1517 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!
```

作为对比，qemu 虚拟机 ping 的结果为:
```txt
***************** 8ff03300 ***************
[2025-4-24 11:06:06.147588] [8ff03300][__netif_receive_skb_core][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147593] [8ff03300][packet_rcv          ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147596] [8ff03300][ovs_vport_receive   ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147597] [8ff03300][ovs_dp_process_packet][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147599] [8ff03300][enqueue_to_backlog  ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147603] [8ff03300][__netif_receive_skb_core][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147604] [8ff03300][packet_rcv          ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147605] [8ff03300][ip_rcv              ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147607] [8ff03300][ip_rcv_core         ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147608] [8ff03300][nf_hook_slow        ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203 *ipv4 in chain: PRE_ROUTING*
[2025-4-24 11:06:06.147611] [8ff03300][nft_do_chain        ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203 *iptables table:mangle, chain:PREROUT* *packet is accepted*
[2025-4-24 11:06:06.147642] [8ff03300][ip_route_input_slow ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147644] [8ff03300][fib_validate_source ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147646] [8ff03300][ip_local_deliver    ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147647] [8ff03300][nf_hook_slow        ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203 *ipv4 in chain: INPUT*
[2025-4-24 11:06:06.147648] [8ff03300][nft_do_chain        ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203 *iptables table:filter, chain:INPUT* *packet is accepted*
[2025-4-24 11:06:06.147651] [8ff03300][ip_local_deliver_finish][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147653] [8ff03300][icmp_rcv            ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147655] [8ff03300][icmp_echo           ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147657] [8ff03300][icmp_reply          ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203
[2025-4-24 11:06:06.147694] [8ff03300][consume_skb         ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping request, seq: 13, id: 4203 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!

***************** 8ff02a00 ***************
[2025-4-24 11:06:06.147662] [8ff02a00][__ip_local_out      ][cpu:0  ][     ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147663] [8ff02a00][nf_hook_slow        ][cpu:0  ][     ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203 *ipv4 in chain: OUTPUT*
[2025-4-24 11:06:06.147666] [8ff02a00][ip_output           ][cpu:0  ][     ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147667] [8ff02a00][nf_hook_slow        ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203 *ipv4 in chain: POST_ROUTING*
[2025-4-24 11:06:06.147669] [8ff02a00][ip_finish_output    ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147670] [8ff02a00][ip_finish_output2   ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147675] [8ff02a00][__dev_queue_xmit    ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147676] [8ff02a00][dev_hard_start_xmit ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203 *skb is successfully sent to the NIC driver*
[2025-4-24 11:06:06.147677] [8ff02a00][ovs_vport_receive   ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147678] [8ff02a00][ovs_dp_process_packet][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147680] [8ff02a00][__dev_queue_xmit    ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203
[2025-4-24 11:06:06.147682] [8ff02a00][dev_hard_start_xmit ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203 *skb is successfully sent to the NIC driver*
[2025-4-24 11:06:06.147703] [8ff02a00][consume_skb         ][cpu:0  ][   ][pid:206249 ][vhost-206245][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping reply, seq: 13, id: 4203 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!
```

如果是物理机的 ping ，结果为，也就是发生在中断的
```txt
***************** ea605900 ***************
[2025-4-24 11:08:12.599356] [ea605900][napi_gro_receive_entry][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599361] [ea605900][dev_gro_receive     ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599364] [ea605900][__netif_receive_skb_core][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599365] [ea605900][packet_rcv          ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599367] [ea605900][ovs_vport_receive   ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599368] [ea605900][ovs_dp_process_packet][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599371] [ea605900][enqueue_to_backlog  ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599374] [ea605900][__netif_receive_skb_core][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599375] [ea605900][packet_rcv          ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599376] [ea605900][ip_rcv              ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599377] [ea605900][ip_rcv_core         ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599380] [ea605900][nf_hook_slow        ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1 *ipv4 in chain: PRE_ROUTING*
[2025-4-24 11:08:12.599382] [ea605900][nft_do_chain        ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1 *iptables table:mangle, chain:PREROUT* *packet is accepted*
[2025-4-24 11:08:12.599387] [ea605900][ip_route_input_slow ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599389] [ea605900][fib_validate_source ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599392] [ea605900][ip_local_deliver    ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599393] [ea605900][nf_hook_slow        ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1 *ipv4 in chain: INPUT*
[2025-4-24 11:08:12.599395] [ea605900][nft_do_chain        ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1 *iptables table:filter, chain:INPUT* *packet is accepted*
[2025-4-24 11:08:12.599398] [ea605900][ip_local_deliver_finish][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599400] [ea605900][icmp_rcv            ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599402] [ea605900][icmp_echo           ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599405] [ea605900][icmp_reply          ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1
[2025-4-24 11:08:12.599440] [ea605900][consume_skb         ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.5 -> 10.0.0.2 ping request, seq: 39, id: 1 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!

***************** ea605100 ***************
[2025-4-24 11:08:12.599410] [ea605100][__ip_local_out      ][cpu:15 ][     ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599411] [ea605100][nf_hook_slow        ][cpu:15 ][     ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1 *ipv4 in chain: OUTPUT*
[2025-4-24 11:08:12.599416] [ea605100][ip_output           ][cpu:15 ][     ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599418] [ea605100][nf_hook_slow        ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1 *ipv4 in chain: POST_ROUTING*
[2025-4-24 11:08:12.599419] [ea605100][ip_finish_output    ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599421] [ea605100][ip_finish_output2   ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599423] [ea605100][__dev_queue_xmit    ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599425] [ea605100][dev_hard_start_xmit ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1 *skb is successfully sent to the NIC driver*
[2025-4-24 11:08:12.599427] [ea605100][ovs_dp_process_packet][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599428] [ea605100][__dev_queue_xmit    ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1
[2025-4-24 11:08:12.599430] [ea605100][dev_hard_start_xmit ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1 *skb is successfully sent to the NIC driver*
[2025-4-24 11:08:12.599446] [ea605100][consume_skb         ][cpu:15 ][   ][pid:0      ][swapper/15  ][ns:0] ICMP: 10.0.0.2 -> 10.0.0.5 ping reply, seq: 39, id: 1 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!
]
```
## 这个好
https://github.com/firecracker-microvm/firecracker-containerd

https://github.com/firecracker-microvm/firecracker

## sanbox 在 ai 时代
https://zdyxry.github.io/2026/02/08/Weekly-Issue-%20%E5%9B%9E%E5%AE%B6%E8%BF%87%E5%B9%B4/

https://github.com/arcboxlabs/arcbox
https://github.com/deeplethe/forkd

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
