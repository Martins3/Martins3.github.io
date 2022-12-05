# 使用 QEMU 阅读内核


- 将类似的构成一个系列:
  - 使用 kgdb
  - 使用 ktest kunit 等

## kvm 或者 tcg 调试的基本原理

## 基本的使用方法

## 调试专题

### 网络的整个流程

### block
https://blogs.oracle.com/linux/post/how-to-emulate-block-devices-with-qemu

## [ ] 将 alpine.sh 脚本可以运行 3.10

默认 3.10 内核将 ext4 设置为模块模式，这会导致如下的错误:
```txt
[    2.113383] rtc_cmos 00:05: setting system clock to 2022-10-17 05:51:57 UTC (1665985917)
[    2.128502] input: VirtualPS/2 VMware VMMouse as /devices/platform/i8042/serio1/input/input2
[    2.130439] input: VirtualPS/2 VMware VMMouse as /devices/platform/i8042/serio1/input/input3
[    2.248555] md: Waiting for all devices to be available before autodetect
[    2.257265] md: If you don't use raid, use raid=noautodetect
[    2.267496] md: Autodetecting RAID arrays.
[    2.272993] md: autorun ...
[    2.276214] md: ... autorun DONE.
[    2.280246] List of all partitions:
[    2.284788] No filesystem could mount root, tried:
[    2.291461] Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
[    2.292321] CPU: 3 PID: 1 Comm: swapper/0 Not tainted 3.10.0+ #1
[    2.292321] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    2.292321] Call Trace:
[    2.292321]  [<ffffffff817792c2>] dump_stack+0x19/0x1b
[    2.292321]  [<ffffffff81772941>] panic+0xe8/0x21f
[    2.292321]  [<ffffffff81d88790>] mount_block_root+0x291/0x2a0
[    2.292321]  [<ffffffff81d887f2>] mount_root+0x53/0x56
[    2.292321]  [<ffffffff81d88931>] prepare_namespace+0x13c/0x174
[    2.292321]  [<ffffffff81d8840e>] kernel_init_freeable+0x222/0x249
[    2.292321]  [<ffffffff81d87b1f>] ? initcall_blacklist+0xb0/0xb0
[    2.292321]  [<ffffffff81767b60>] ? rest_init+0x80/0x80
[    2.292321]  [<ffffffff81767b6e>] kernel_init+0xe/0x100
[    2.292321]  [<ffffffff8178cd1d>] ret_from_fork_nospec_begin+0x7/0x21
[    2.292321]  [<ffffffff81767b60>] ? rest_init+0x80/0x80
[    2.292321] Kernel Offset: disabled
```
