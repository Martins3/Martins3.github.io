## 基本启动流程
- start_kernel : 进入通用启动流程
  - setup_arch : 架构相关启动
    - setup_memory_region
    - parse_early_param
    - e820_register_active_region
      - add_active_range
    - init_memory_mapping : 构建内核地址空间的直接映射
    - paging_init
      - free_area_init_nodes
  - setup_per_cpu_areas
  - build_all_zonelists
  - setup_per_cpu_pageset

## TODO
- [ ] 可以将 Loongson 的启动整理过来吗 ?
- [ ] 将陈华才的整理过来一下



| File               | blank | comment | code |desc|
|--------------------|-------|---------|------||
| main.c             | 218   | 245     | 1004 ||
| initramfs.c        | 79    | 17      | 592  ||
| do_mounts.c        | 92    | 92      | 533  ||
| do_mounts_rd.c     | 44    | 40      | 261  ||
| do_mounts_md.c     | 33    | 42      | 229  ||
| calibrate.c        | 35    | 77      | 204  ||
| init_task.c        | 5     | 9       | 179  ||
| do_mounts_initrd.c | 19    | 15      | 109  ||
| do_mounts.h        | 18    | 1       | 44   ||
| version.c          | 7     | 9       | 39   ||
| noinitramfs.c      | 6     | 10      | 24   ||
| Makefile           | 7     | 9       | 22   ||


# init/do_mount.c

1. init_mount_tree

```c
struct file_system_type rootfs_fs_type = {
	.name		= "rootfs",
	.init_fs_context = rootfs_init_fs_context,
	.kill_sb	= kill_litter_super,
};
```

## 多核启动[^1]
QEMU 中 cpu_is_bsp 和 do_cpu_sipi

在 x86-64 多核系统的初始化中，需要有一个核作为 bootstrap processor (BSP)，由这个 BSP 来执行操作系统初始化代码。

下面就是加冕仪式了，新晋的 BSP 需要带上王冠（设置自己 IA32_APIC_BASE 寄存器的 BSP 标志位为 1）来表明自己的身份。

那其他的 APs 可不可以也这样做呢，当然不行，否则岂不是要谋反么。此时 APs 进入 wait for SIPI 的状态，等待着 BSP 的发号施令。

- [ ] 理解一下: secondary_startup_64

## 火焰图中的 swapper 到底是什么

实际上：
```txt
sudo bpftrace -e 'kfunc:do_idle { @[comm] = count(); }'

Attaching 1 probe...
^C

@[swapper/15]: 3
@[swapper/13]: 3
@[swapper/31]: 3
@[swapper/28]: 4
@[swapper/23]: 5
@[swapper/27]: 5
@[swapper/5]: 6
@[swapper/26]: 6
@[swapper/1]: 7
@[swapper/22]: 10
@[swapper/7]: 10
@[swapper/9]: 12
@[swapper/11]: 15
@[swapper/3]: 17
@[swapper/20]: 24
@[swapper/18]: 27
@[swapper/25]: 30
@[swapper/24]: 54
@[swapper/17]: 70
@[swapper/21]: 74
@[swapper/30]: 84
@[swapper/16]: 172
@[swapper/12]: 213
@[swapper/29]: 226
@[swapper/19]: 248
@[swapper/14]: 259
@[swapper/0]: 287
@[swapper/8]: 303
@[swapper/10]: 305
@[swapper/4]: 433
@[swapper/2]: 436
@[swapper/6]: 480
```

```txt
sudo bpftrace -e 'kfunc:do_idle { @[pid] = count(); }'

Attaching 1 probe...
^C

@[0]: 3804
```
也就是说，他们的 pid 都相同，但是他们

cpu_startup_entry 中打断点
```txt
$ p &$lx_current()
$8 = (struct task_struct *) 0xffff8881002f10c0
```

[^1]: https://zhuanlan.zhihu.com/p/67989330
