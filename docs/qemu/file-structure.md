# QEMU 的文件结构

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
 io
 libdecnumber
 linux-headers
 linux-user
 meson
 migration
 monitor
 nbd
 net
 pc-bios
 plugins
 po
 python
 qapi
 qga
 qobject
 qom
 replay
 roms
 scripts
 scsi
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


下面是翻译过程中需要的  helper
- helper.c
- helper.h
- bpt_helper.c
- cc_helper.c
- cc_helper_template.h
- excp_helper.c
- fpu_helper.c
- int_helper.c
- mem_helper.c
- misc_helper.c
- mpx_helper.c
- seg_helper.c
- shift_helper_template.h
- smm_helper.c
- xsave_helper.c


都可以知道显然没有关系的代码，主要是处理各种虚拟化框架, sev 是处理 memory encryption 的
- hvf
- LATX
- X86toMIPS
- btmmu.c
- hax-all.c
- hax-i386.h
- hax-interface.h
- hax-mem.c
- hax-posix.c
- hax-posix.h
- hax-windows.c
- hax-windows.h
- hyperv-proto.h
- hyperv-stub.c
- hyperv.c
- hyperv.h
- kvm-stub.c
- kvm.c
- kvm_i386.h
- svm.h
- svm_helper.c
- whp-dispatch.h
- whpx-all.c
- sev-stub.c
- sev.c
- sev_i386.h

## 问题
- [ ] virtio fs [^1] 的实现在哪里
- [ ] 为什么在 ./contrib/ 存在 vhost 的工具
- [ ] common-user/ 存在架构的 hacking 的代码
- [ ] disas 和 QEMU 是什么关系吗?
- [ ] hw 下存在

[^1]: https://virtio-fs.gitlab.io/howto-qemu.html
[^2]: https://en.wikipedia.org/wiki/Pluggable_authentication_module
