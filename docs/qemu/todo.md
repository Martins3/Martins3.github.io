# QEMU 还有的挑战
- [ ] docs/devel/qapi-code-gen.txt 和 qmp 如何工作的，是如何生成的。

## qmp

- [ ] `qmp_block_commit` 的唯一调用者是如何被生成的。
- [ ] `OBJECT_DECLARE_SIMPLE_TYPE` 是什么意思，和类似的 macro 有什么区别




## 为什么在 QEMU 中执行 stress-ng --vm-bytes 1000M --vm-keep -m 1 ，但是再 host 中的 htop 中可以观测到两个 CPU 非常繁忙

## 还可以这样啊
https://www.qemu.org/docs/master/system/secrets.html

-object secret,id=key1,format=raw,file=key.aes

## qemu - nvme
https://github.com/manishrma/nvme-qemu

## 其他的东西
https://github.com/changeofpace/VivienneVMM

https://chromium.googlesource.com/chromiumos/docs/+/deee88d6c4abac0e871a542a1f2a595ab7c131cc/cros_vm.md

https://github.com/bao-project/bao-hypervisor

https://people.redhat.com/~cohuck/2022/01/05/qemu-machine-types.html

https://people.redhat.com/~cohuck/2022/01/21/qemu-machine-types-part2.html

## 太有趣了
https://github.com/containers/krunvm

## 为什么 qemu 中用这个测试，只有 1438k ，而 host 上有 1700k

每秒 5000 次 kvm_exit ，不知道发生了什么
```txt
[global]
time_based
runtime=1000
# ioengine=io_uring
ioengine=io_uring
# ioengine=sync
iodepth=128
direct=1
bs=4k

[trash]
rw=randread
# filename=/home/martins3/hack/iso/tmp.iso
# filename=/home/martins3/hack/vm/windows8.img
# filename=/dev/nvme0n1p1
filename=/dev/nullb0
# size=10G
# filename=/dev/nullb0

# [trash]
# ioengine=sync
# iodepth=1
# direct=0
# size=30G
# rw=randwrite
# # filename=/home/martins3/hack/qemu.ram
# filename=/home/martins3//hack/tmp/a
numjobs=1
```

kvmexit 可以统计到
```txt
912941   912965   EXTERNAL_INTERRUPT                  10155
912941   912965   MSR_WRITE                           10453
912941   912965   PREEMPTION_TIMER                    10155
```
不明白这几个都是在做啥

## qemu 的演化

- device drive 模式到 blockdev
```txt
   Block device options
       The  QEMU  block device handling options have a long history and have gone through several iterations as the feature
       set and complexity of the block layer have grown. Many online guides to QEMU often reference  older  and  deprecated
       options, which can lead to confusion.

       The  most  explicit  way  to  describe  disks  is to use a combination of -device to specify the hardware device and
       -blockdev to describe the backend. The device defines what the guest sees and the backend describes how QEMU handles
       the data. It is the only guaranteed stable interface for describing block devices and as  such  is  recommended  for
       management tools and scripting.

       The  -drive option combines the device and backend into a single command line option which is a more human friendly.
       There is however no interface stability guarantee although some older board models still need updating to work  with
       the modern blockdev forms.

       Older  options  like  -hda are essentially macros which expand into -drive options for various drive interfaces. The
       original forms bake in a lot of assumptions from the days when QEMU was emulating a legacy PC, they are  not  recom‐
       mended for modern configurations.
```

- -nic netdev 的切换
https://www.qemu.org/2018/05/31/nic-parameter/


## 这个是什么意思

```txt
QEMU 8.2.50 monitor - type 'help' for more information
(qemu) qemu-system-x86_64: Slirp: external icmpv6 not supported yet
qemu-system-x86_64: Slirp: external icmpv6 not supported yet
qemu-system-x86_64: Slirp: external icmpv6 not supported yet
qemu-system-x86_64: Slirp: external icmpv6 not supported yet
```

https://www.bilibili.com/video/BV1pA411A7rZ

https://mergeboard.com/blog/2-qemu-microvm-docker/

## 热升级观察一波
- https://lore.kernel.org/qemu-devel/917a64ea-c161-b612-4266-343368d6b3f9@oracle.com/

## 这个看看
- https://docs.redhat.com/en/documentation/Red_Hat_Enterprise_Linux/7/html/Virtualization_Tuning_and_Optimization_Guide/chap-Virtualization_Tuning_Optimization_Guide-Introduction.html#sect-Virtualization_Tuning_Optimization_Guide-Why_Optimization_Matters

总结的挺好的。

## 这个文件是做什么的

- hw/arm/smmuv3.c

## qemu 可以对于 guest 来进行统计吗?

## qemu 的 common-user/ 存在架构的 hacking 的代码，信号和 syscall


## 有趣的项目
https://github.com/leaningtech/webvm


## 有趣的讨论
https://news.ycombinator.com/item?id=19736309

## dirty log 可以借助 热迁移的 dirty log 的实现吗?
看看 qemu 中的 vga_dirty_log_start

- vfio_legacy_dma_unmap 和 vfio_legacy_dma_map 启动一次，为什么需要调用那么多次?


## 看看
https://developer.apple.com/documentation/virtualization

https://mp.weixin.qq.com/s/CWTgMg3v5-AX2CcVdskf8w

https://lore.kernel.org/all/87pllyezmh.fsf@suse.de/

## 整理下 pc 和 q35 区别，现在看差别越来越大了
iommu sriov 有区别

## 项目
https://github.com/winapps-org/winapps

## virtualization
- [LC-3 虚拟机](https://justinmeiners.github.io/) : 只有几百行
- [dockerpi](https://github.com/lukechilds/dockerpi) : 其实是一百行左右的 Dockerfile，在其中运行 qemu 模拟 raspberrypi 的硬件环境法
- [OSX-KVM](https://github.com/kholia/OSX-KVM) : 利用 kvm 实现运行 OSX 的虚拟机
- [Docker-OSX](https://github.com/sickcodes/Docker-OSX) : 类似 dockerpi, 提供安装 [OSX-KVM](https://github.com/kholia/OSX-KVM) 的自动安装
- [macos virtualbox](https://github.com/myspaghetti/macos-virtualbox) : 提供一个脚本，在 virtualbox 中间运行 macos
- [v86](https://github.com/copy/v86/) : 使用 js 写的 x86 硬件虚拟化，可以在网页上运行机器 [windows95 in electron](https://github.com/felixrieseberg/windows95) : 利用 v86 实现运行 windows95 在 electron 中间
  - https://news.ycombinator.com/item?id=40940225 : 第一个 commnet 整理了特别多

## 项目
- [ ] https://github.com/intel/haxm : 对于 Windows 和 Mac 存在良好的支持，但是 Linux 上根本无法编译的，感觉类似于 KVM 的东西
  - https://www.qemu.org/2017/11/22/haxm-usage-windows/ : 在 qemu 上利用这个东西实现加速

https://github.com/steren/awesome-cloudrun : Google 的产品介绍，不知道能不能白嫖

https://github.com/rootsongjc/awesome-cloud-native#api-gateway
> 完全不知道云原生在干什么 !

dockerpi : docker 作为一个容器，为什么可以实现跨架构, 因为里面还安装了一个 qemu !
  - https://github.com/dhruvvyas90/qemu-rpi-kernel
https://www.vagrantup.com/intro/getting-started/up : 基于虚拟化的环境开发，那么，所以和 docker 的关系是什么 ?
https://github.com/weaveworks/footloose : 让 docker 类似虚拟机，看来虚拟机和 containers 的区别不仅仅如此啊
https://github.com/kholia/OSX-KVM

https://github.com/fireeye/speakeasy : Speakeasy is a portable, modular, binary emulator designed to emulate Windows kernel and user mode malware.
https://github.com/Kelvinhack/kHypervisor : window 的 ept hypervisor

https://github.com/Friz-zy/awesome-linux-containers


## TODO
- [ ] https://github.com/Martins3/Martins3.github.io/issues/22

- [ ] [An Introduction to Clear Containers](https://lwn.net/Articles/644675/)
  - https://github.com/clearcontainers 已经死掉了，迁移到 kata containers 了

- https://mergeboard.com/blog/2-qemu-microvm-docker/

- https://mp.weixin.qq.com/s/v7UmToZ-323K4axfW91apA

## 里里外外都看看
https://news.ycombinator.com/item?id=42438449

https://github.com/arceos-hypervisor/2024-virtualization-campus

## 真的可以实现这种物理网卡直通到虚拟机网卡的切换?
https://qemu.readthedocs.io/en/v9.1.0/system/virtio-net-failover.html

## 这个参数如何理解 -snapshot

## 指向的 barrier 项目听都没有听过
- https://qemu.readthedocs.io/en/v9.1.0/system/barrier.html

## 从工程上，这个是有必要看看的，其实 qemu 的 ci 按道理是很麻烦的
而且还依赖环境，但是他们还是有办法搭建一个出来环境来测试，其中的各种基础技术都是我们所需要的:

https://qemu.readthedocs.io/en/v9.1.0/devel/index.html

## 有趣的总结
https://www.ubicloud.com/blog/cloud-virtualization-red-hat-aws-firecracker-and-ubicloud-internals

## qemu 最基本的核心
qmp hmp cmdline qobj

最有趣的是 qemu 也是 bus + device ，可以和 kernel 的 sysfs 来对比

https://xz.aliyun.com/t/8320

## 使用基于 qemu 的双系统

- 忽然想到，其实可以这样，首先启动 Linux，使用 windows 所在的盘为一个盘，然后采用双系统的方法启动 windows 系统？
  - 似乎不行，因为这个盘里面没有 windows

- root 分区中到底有什么？
  - /boot 中内容

- 使用直通或者 /dev/nvme0n1 之类的方法，root 分区就在该 disk 上，
双系统无法启动哇。

- 可以用 ventory 在 qemu 给一个盘安装系统吗?
  - 从 usb 启动，该 usb 中提前安装 ventory
  - ventory 启动之后，利用一个盘启动

## 有趣的
https://github.com/ading2210/linuxpdf

## mac 中使用 ubuntu 中的一点解决方案
https://blog.disintegrator.dev/posts/dev-virtual-machine

## hmp 的这个命令做什么的
```txt
(qemu) help cpu
cpu index -- set the default CPU
```

## qmp 命令中的 help 和 info 命令都看看

info qom-tree

## 这种报错如何解决
添加一个参数
```txt
-device kvm-apic
```
但是结果为:

```txt
qemu-system-x86_64: -device kvm-apic: Parameter 'driver' expects a pluggable device type
```

## 理解一下这个东西
man qemu(1)
```txt
              kernel-irqchip=on|off|split
                     Controls  KVM  in-kernel   irqchip
                     support.  The  default is full ac‐
                     celeration of the  interrupt  con‐
                     trollers.  On  x86,  split irqchip
                     reduces the kernel attack surface,
                     at a performance cost for  non-MSI
                     interrupts.  Disabling the in-ker‐
                     nel irqchip completely is not rec‐
                     ommended except for debugging pur‐
                     poses.
```

## 实在有趣
https://blog.vmsplice.net/2024/01/key-value-stores-foundation-of-file.html


## 类似的项目应该很多吧
https://zhuanlan.zhihu.com/p/676339639

## 有趣的项目
https://github.com/panda-re/panda

## mac 上的 docker 都是通过 qemu 来实现的吗?
https://orbstack.dev/

## 可以通过 microvm 来看看 ACPI 的基本原理
acpi_setup_microvm

## 调查下，现在好可以做出来什么操作
https://github.com/varnish/tinykvm


## 理解一下这个问题
```txt
              Copy-on-read avoids accessing the same backing file sectors repeatedly and is useful when the backing file is over a slow network. By
              default copy-on-read is off.

              Instead of -cdrom you can use:

                 qemu-system-x86_64 -drive file=file,index=2,media=cdrom

              Instead of -hda, -hdb, -hdc, -hdd, you can use:

                 qemu-system-x86_64 -drive file=file,index=0,media=disk
                 qemu-system-x86_64 -drive file=file,index=1,media=disk
                 qemu-system-x86_64 -drive file=file,index=2,media=disk
                 qemu-system-x86_64 -drive file=file,index=3,media=disk

```
## 为什么 info roms 没有包含 virtio-net-pci.rom
```txt
(qemu) info roms
fw=genroms/kvmvapic.bin size=0x002400 name="kvmvapic.bin"
fw=genroms/linuxboot_dma.bin size=0x000600 name="linuxboot_dma.bin"
addr=00000000fffc0000 size=0x040000 mem=rom name="/home/martins3/core//seabios/out/bios.bin"
/rom@etc/acpi/tables size=0x200000 name="etc/acpi/tables"
/rom@etc/table-loader size=0x010000 name="etc/table-loader"
/rom@etc/acpi/rsdp size=0x010000 name="etc/acpi/rsdp"
(qemu) info r
ramblock              registers             replay
rocker                rocker-of-dpa-flows   rocker-of-dpa-groups
rocker-ports          roms
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                  pc.ram    4 KiB  0x0000000000000000 0x0000000800000000 0x0000000800000000 0x00007fdbcfe00000  rw
 0000:00:0d.0/gpu-fb-mem    4 KiB  0x0000000800140000 0x0000000001000000 0x0000000001000000 0x00007fdbbe000000  rw
    /rom@etc/acpi/tables    4 KiB  0x0000000801140000 0x0000000000020000 0x0000000000200000 0x00007fdbbdc00000  ro
                 pc.bios    4 KiB  0x0000000800000000 0x0000000000040000 0x0000000000040000 0x00007fe3d8600000  ro
0000:00:04.0/virtio-net-pci.rom    4 KiB  0x0000000800080000 0x0000000000040000 0x0000000000040000 0x00007fe3d8200000  ro
0000:00:05.0/virtio-net-pci.rom    4 KiB  0x00000008000c0000 0x0000000000040000 0x0000000000040000 0x00007fdbbfe00000  ro
0000:00:06.0/virtio-net-pci.rom    4 KiB  0x0000000800100000 0x0000000000040000 0x0000000000040000 0x00007fdbbfc00000  ro
                  pc.rom    4 KiB  0x0000000800040000 0x0000000000020000 0x0000000000020000 0x00007fe3d8400000  ro
   /rom@etc/table-loader    4 KiB  0x0000000801340000 0x0000000000001000 0x0000000000010000 0x00007fdbbf200000  ro
      /rom@etc/acpi/rsdp    4 KiB  0x0000000801380000 0x0000000000001000 0x0000000000010000 0x00007fdbbda00000  ro
```


## 这个就是现在主流的虚拟化技术了
nixos/modules/virtualisation/

## 看看 qemu 的 ./configure -h 看看吧

## qemu 中有 hyperv_enabled 的函数啊，所以 qemu 支持在 windows 运行才对的

## 原来一个 qemu 一个 numa 支持的有线
```txt
qemu-system-x86_64: cannot set up guest memory 'mem0': Cannot allocate memory
```
打开 hack_memory_cpu ，ram=2048G ，如果 numa_node = 2 ，那么出现这个报错，但是如果
numa node = 16 就可以正常启动。


## 这个做啥的来着

```txt
-object secret,id=masterKey0,format=raw,file=/var/lib/libvirt/qemu/domain-2-d131351d-21d5-4635-8/master-key.aes
```

chardev 参数还可以这样写?
```txt
-mon chardev=charmonitor,id=monitor,mode=control
```

做啥的？
```txt
-add-fd set=9,fd=49
```

## 想不到这个东西也是默认打开的?

```txt
              mem-merge=on|off
                     Enables  or disables memory merge support. This feature, when
                     supported by the host, de-duplicates identical  memory  pages
                     among VMs instances (enabled by default).
```

## qemu 和 ebpf 在搞啥呢？
  bpf             eBPF support

ebpf/ 目录


## 有趣的
https://wiki.qemu.org/Features/

## 这个研究一下
https://github.com/community-scripts/ProxmoxVE

## 有趣的工作
https://github.com/ChefKissInc/QEMUAppleSilicon/wiki

## dump 相关的整理一下

- clone
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - x86_cpu_dump_state
            - cpu_memory_rw_debug
              - cpu_asidx_from_attrs
                - __GI___assert_fail
                  - __assert_fail_base
                    - __GI_abort
                      - __GI_raise

internal error 的时候也是 dump 这里的

## -object qtest 是做什么的

## gdb 中 thread apply all bt 可以看到这个 thread

看上去是内核中的一个 thread ，但是挂到 qemu 下面了

```txt
Thread 2 (Thread 0x7fac197fb6c0 (LWP 1000568) "kvm-nx-lpage-re"):
#0  0x0000000000000000 in ?? ()
Backtrace stopped: Cannot access memory at address 0x0
```

## 勇哥的博客，整理一下
https://blog.csdn.net/huang987246510/article/details/128403256

## 看看 QEMU_MADV_DONTFORK 都是啥意思?

## qemu 有一个网络后端是 xdp ，真的不可以理解啊

## 有趣的东西
- https://github.com/multiarch/qemu-user-static
- https://github.com/qemus/qemu


这不就是 ceph 的工作吗?
https://github.com/sheepdog/sheepdog

## 这个 anti detection 比想象的简单
https://github.com/zhaodice/qemu-anti-detection


## 这个实现是在搞什么?
https://github.com/qemu/qemu/commit/a6f02277595136832c9e9bcaf447ab574f7b1128


## qemu 测试框架
https://www.bilibili.com/video/BV1pA411A7rZ

## qxl 是什么?

## 有什么办法可以实现 list 所有的 drive 和所有的 device ?


## 这个是做什么的
net/passt.c

## 很好啊
https://github.com/luchina-gabriel/OSX-PROXMOX

## qemu 的线程模型

find_merge_commit 6327540d92e4ef4039dc812d

```txt
The following changes since commit 711a1ddf899bef577907a10db77475c8834da52f:

  Merge tag 'pull-10.2-maintainer-171125-2' of https://gitlab.com/stsquad/qemu into staging (2025-11-18 09:18:23 +0100)

are available in the Git repository at:

  https://repo.or.cz/qemu/kevin.git tags/for-upstream

for you to fetch changes up to 837c04e9fc798cddafe721e2abbbd0d932571793:

  win32-aio: Run CB in original context (2025-11-18 18:01:57 +0100)

----------------------------------------------------------------
Block layer patches

- Multi-threading fixes in several block drivers

----------------------------------------------------------------
Hanna Czenczek (19):
      block: Note on aio_co_wake use if not yet yielding
      rbd: Run co BH CB in the coroutine’s AioContext
      iscsi: Run co BH CB in the coroutine’s AioContext
      nfs: Run co BH CB in the coroutine’s AioContext
      curl: Fix coroutine waking
      nvme: Kick and check completions in BDS context
      nvme: Fix coroutine waking
      nvme: Note in which AioContext some functions run
      block/io: Take reqs_lock for tracked_requests
      qcow2: Re-initialize lock in invalidate_cache
      qcow2: Fix cache_clean_timer
      qcow2: Schedule cache-clean-timer in realtime
      ssh: Run restart_coroutine in current AioContext
      blkreplay: Run BH in coroutine’s AioContext
      block: Note in which AioContext AIO CBs are called
      iscsi: Create AIO BH in original AioContext
      null-aio: Run CB in original AioContext
      win32-aio: Run CB in original context

 block/qcow2.h                    |   5 +-
 include/block/aio.h              |  15 ++++
 include/block/block_int-common.h |   7 +-
 block/blkreplay.c                |   3 +-
 block/curl.c                     |  45 ++++++++----
 block/io.c                       |   3 +
 block/iscsi.c                    |  63 +++++++----------
 block/nfs.c                      |  41 +++++------
 block/null.c                     |   7 +-
 block/nvme.c                     | 113 +++++++++++++++++++++----------
 block/qcow2.c                    | 143 +++++++++++++++++++++++++++++++--------
 block/rbd.c                      |  12 ++--
 block/ssh.c                      |  22 +++---
 block/win32-aio.c                |  31 +++++++--
 15 files changed, 346 insertions(+), 181 deletions(-)
```


## virtio-win.iso 中的目录还是需要继续仔细看看了

## 这里的文档也需要看看
https://www.linux-kvm.org/page/Documents

## qemu 现在三座大山

- thread model
- block
- migration (马上搞完)

## 我是真的没有想到，原来还可以这样

从物理机中直接 ssh 到虚拟机中去:
```txt
ssh root@10.0.2.15
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
