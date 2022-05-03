# QEMU 概述

我个人的计算机学习习惯是，首先知道一个东西的大致是个什么东西，可以做什么，不可以做什么，以后有问题知道在哪里查询。
对于 QEMU 也是如此，本文首先简单地分析一下 QEMU 里面大概有什么。

QEMU 是虚拟化的集大成者，有的项目可以利用 kvm 构建虚拟机，例如 [firecracker](https://github.com/firecracker-microvm/firecracker)，但是
其只能使用 virtio IO 模拟设备[^4]，但是 QEMU 还可以使用设备模拟和直通的方法，有的项目，例如 Apple M1 Mac 上的 [Rosetta](https://en.wikipedia.org/wiki/Rosetta_(software))，但是其只能进行
x86 到 arm 上的二进制翻译器，而 QEMU 可以支持多达 20 个架构的互相翻译.

![](./img/qemu.svg)

## CPU
图上只是画了两个，是这两个我比较熟悉 :-) ，但是实际上还支持其他的一些技术，例如 xen 。

kvm 利用硬件加速，总体代码量很小，而且大多数代码都在 Linux 内核中。tcg 技术是将一个架构，例如 x86，的指令翻译为 tcg IR (Tiny Code Generator) (Intermediate Representation)，然后 tcg ir
翻译为另一个架构的指令，例如 arm 上。

kvm 不能跨架构，而利用 tcg 可以在 arm 上运行 x86 的操作系统，一般来说，kvm 要比 tcg 快 10 ~ 20 倍。

除了运行一个操作系统，tcg 还可以翻译一个用户态程序，例如在 x86 版本的微信运行在 arm 的操作系统上，这个功能 Rosetta 也可以做到，但是 Rosetta 也仅仅只能实现这个功能。

最后看一下，其管理的文件的位置:
| cpu | source code location               |
|-----|------------------------------------|
| tcg | `accel` `tcg` `target/$(arch)/tcg` |
| kvm | `accel` `target/$(arch)/kvm`       |

accel 中处理的是 CPU 模拟的通用结构，因为 tcg 下是各个架构指令翻译为 tcg IR，`target/$(arch)/tcg` 中是 tcg IR 翻译为各个架构指令的代码。
总体来说，内核在各个架构上提供的 kvm 接口比较统一，但是还是存在一定的差异，其差异的地方定义在 `target/$(arch)/kvm` 中。

## Device

| device | source code location                           |
|--------|------------------------------------------------|
| block  | `./block` 后端模拟，`hw/block` 前端模拟        |
| char   | `./char`  后端模拟, `hw/char` 前端模拟         |
| net    | ./slirp 和 `./net` 后端模拟，`hw/net` 前端模拟 |
| video  | `hw/display`                                   |
| sound  | `./sound`                                      |
| arch   | `hw/$(arch)/` `hw/intc/`                       |

## backend
qemu-system-x86_64 -chardev help
qemu-system-x86_64 -netdev help

QEMU 可以模拟的设备:
qemu-system-x86_64 -device help

获取 nic 支持的模式:
qemu-system-x86_64 -net nic,model=help


 accel
 audio
 authz
 backends
-  block : 后端
-  chardev : 后端
 bsd-user
 common-user
 configs
-  contrib : 各种工具
  - ivshmem: http://just4coding.com/2021/09/12/qemu-ivshmem/
 crypto
 disas
 docs
-  dump : dump 将 Guest 的 memory
-  ebpf : 网络
- submodules
  -  dtc : device tree compiler
  -  capstone
-  fpu
-  fsdev : 9p
 gdb-xml
 hw
 include
-  io
 libdecnumber
 linux-headers
 linux-user
 meson
 migration
 monitor
-  nbd
-  net : TAP TUN
 pc-bios
 plugins
 po
 python
-  qapi : 使用各种工具交互
-  qga : qemu guest agent
-  qobject
-  qom
 replay
 roms
 scripts
-  scsi : 似乎只是一个工具
 semihosting
 slirp
 softmmu
 storage-daemon
 stubs
 subprojects
 target
 tcg
 tests
 tools
 trace
 ui
 util
 block.c
 blockdev-nbd.c
 blockdev.c
 blockjob.c
 cpu.c
 cpus-common.c
 disas.c
 gdbstub.c
gitdm.config
hmp-commands-info.hx
hmp-commands.hx
qemu-img-cmds.hx
qemu-options.hx
qemu.nsi
qemu.sasl
 iothread.c
 job-qmp.c
 job.c
Kconfig
Kconfig.host
 LICENSE
MAINTAINERS
Makefile
memory_ldst.c.inc
meson.build
 meson_options.txt
 module-common.c
 os-posix.c
 os-win32.c
 page-vary-common.c
 page-vary.c
 qemu-bridge-helper.c
 qemu-edid.c
 qemu-img.c
 qemu-io-cmds.c
 qemu-io.c
 qemu-keymap.c
 qemu-nbd.c
 replication.c
trace-events

## target/i386

| Filename         | desc                                                                                                                     | line |
|------------------|--------------------------------------------------------------------------------------------------------------------------|------|
| cpu-param.h      |                                                                                                                          |      |
| cpu-qom.h        |                                                                                                                          |      |
| cpu.h            | 定义 x86 各种寄存器, CPUX86State, X86CPU                                                                                 |      |
| cpu.c            | 处理 cpuid，处理 x86_cpu_common_class_init 提供的各种函数定义                                                            | 7000 |
| machine.c        | VMStateDescription 的定义                                                                                                | 1450 |
| translate.c      | 中间存在一个被 accel/tcg/translate-all.c 引用的函数，但是 gen_intermediate_code 部分用不上的，应该只有很少的部分才被需要 | 8000 |
| ops_sse.h        | 似乎只是原来的 tcg 翻译 sse 需要的工作                                                                                   |      |
| ops_sse_header.h | 同上                                                                                                                         |      |

用于调试的:
- monitor.c : hqm 之类的接口
- arch_dump.c : 给 coredump 之类提供对应的接口函数
- arch_memory_mapping.c : 对外提供 x86_cpu_get_memory_mapping, 从而实现各种内容
- gdbstub.c

## 问题
- [ ] virtio fs [^1] 的实现在哪里
- [ ] 为什么在 ./contrib/ 存在 vhost 的工具
- [ ] common-user/ 存在架构的 hacking 的代码
- [ ] disas 和 QEMU 是什么关系吗?
- [ ] qemu 中的 trace 使用一下
- [ ] scsi

## NBD

- NBD 是做什么的，和 NFS 有什么关系?
- sshfs 是基于 nfs 开发的吗?


[^1]: https://virtio-fs.gitlab.io/howto-qemu.html
[^2]: https://en.wikipedia.org/wiki/Pluggable_authentication_module
[^3]: 通过数其源码中 ./target/ 下的目录数确定的
[^4]: https://github.com/firecracker-microvm/firecracker/issues/776
